"""Resume the disposable NetBSD development image through its serial console."""
import os
import re
from pathlib import Path
import socket
import subprocess
import time

root = Path(__file__).resolve().parents[1]
smol = root / "smolBSD"
command = ["qemu-system-x86_64", "-machine", "microvm,rtc=on,acpi=off,pic=off",
           "-accel", "kvm", "-cpu", "host", "-m", "2048", "-smp", "2",
           "-kernel", str(smol / "kernels/netbsd-SMOL"),
           "-append", "console=com root=NAME=felix-buildroot -s",
           "-global", "virtio-mmio.force-legacy=false",
           "-drive", f"file={smol}/images/felix-build-amd64.img,format=raw,if=none,id=root",
           "-device", "virtio-blk-device,drive=root",
           "-netdev", "user,id=net", "-device", "virtio-net-device,netdev=net",
           "-fsdev", f"local,path={root},security_model=none,id=share",
           "-device", "virtio-9p-device,fsdev=share,mount_tag=felix",
           "-display", "none", "-monitor", "none",
           "-serial", "tcp:127.0.0.1:2301,server=on,wait=on", "-no-reboot"]
with (root / "build/native-console.log").open("wb", buffering=0) as log:
    proc = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=log)
    conn = None
    try:
        for _ in range(100):
            try:
                conn = socket.create_connection(("127.0.0.1", 2301), timeout=1)
                break
            except OSError:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU exited; see native-console.log")
                time.sleep(.1)
        if conn is None:
            raise RuntimeError("Serial listener timeout")
        conn.settimeout(1)
        data = bytearray()
        shell_entered = sent = False
        deadline = time.monotonic() + 3600
        while proc.poll() is None and time.monotonic() < deadline:
            try:
                chunk = conn.recv(65536)
            except socket.timeout:
                continue
            if not chunk:
                break
            log.write(chunk)
            data.extend(chunk)
            data = data[-10000:]
            if not shell_entered and b"Enter pathname" in data:
                conn.sendall(b"\n")
                shell_entered = True
                data.clear()
            if shell_entered and not sent and b"# " in data:
                # Execute shared source directly, so fixes do not require
                # reconstructing the 2 GB development filesystem.
                script = (
                    "mount -u -o rw /; mkdir -p /mnt; "
                    "cd /dev; sh MAKEDEV vio9p0; cd /; mount_9p -cu /dev/vio9p0 /mnt; "
                    "ifconfig vioif0 10.0.2.15/24 up; route add default 10.0.2.2; "
                    "echo nameserver 10.0.2.3 >/etc/resolv.conf; "
                    "cp /mnt/userland/build-*.sh /mnt/userland/pack-runtime.sh /usr/src/felix/; "
                    "cp /mnt/userland/tinyx/dix/dixfonts.c /usr/src/felix/tinyx/dix/; "
                    "cp /mnt/userland/flwm/Frame.C /usr/src/felix/flwm/; "
                    "cp -R /mnt/userland/apps /usr/src/felix/; "
                    "cp /mnt/userland/tinyx/kdrive/remote/*.[ch] /usr/src/felix/tinyx/kdrive/remote/; "
                    "sh /usr/src/felix/build-netbsd.sh > /mnt/build/native-build.log 2>&1; "
                    "result=$?; echo FELIX_BUILD_RESULT=$result; "
                    "if [ $result = 0 ]; then cp /usr/src/felix/felix-runtime.tar /mnt/build/; fi; "
                    "sync; poweroff\n"
                )
                conn.sendall(script.encode())
                sent = True
                data.clear()
            result = re.search(rb"(?:^|\n)FELIX_BUILD_RESULT=(\d+)\r?\n", data)
            if result:
                print(data.decode(errors="replace"), flush=True)
                break
        else:
            raise RuntimeError("Native build timeout")
        if not sent:
            raise RuntimeError("Guest never reached its single-user shell")
        proc.wait(timeout=30)
        if b"FELIX_BUILD_RESULT=0" not in data:
            raise RuntimeError("Native build failed; see build/native-build.log")
    finally:
        if conn:
            conn.close()
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
