#!/usr/bin/env python3
"""Boot the 1.1 ISO with several Ethernet models and verify a DHCP route."""
from pathlib import Path
import re, secrets, socket, subprocess, time
root=Path(__file__).resolve().parents[1]
iso=root/'build/release/felix-1.1-x86.iso'
for model in ('e1000','e1000e','rtl8139','pcnet','virtio-net-pci','vmxnet3'):
    data=bytearray();connection=None
    with open(root/f'build/1.1-ethernet-{model}-qemu.log','wb') as log:
        guest=subprocess.Popen(['qemu-system-i386','-accel','kvm','-cpu','host','-m','512',
            '-cdrom',str(iso),'-boot','d','-vga','std','-display','none',
            '-nic',f'user,model={model}','-serial','tcp:127.0.0.1:2325,server=on,wait=off','-no-reboot'],stdout=log,stderr=log)
        try:
            for _ in range(60):
                if guest.poll() is not None:raise RuntimeError(f'{model}: QEMU exited')
                try:connection=socket.create_connection(('127.0.0.1',2325),.3);break
                except OSError:time.sleep(.2)
            if connection is None:raise RuntimeError('Serial timeout')
            connection.settimeout(.3);time.sleep(25)
            marker='ETH_'+secrets.token_hex(5)
            command='stty -echo; ip -4 address; ip route; cat /tmp/network.log; test -n "$(pidof felix-ethernet)"; ip -4 addr show eth0 | grep -q "inet 10.0.2." && ip route | grep -q "default via 10.0.2.2"'
            if model=='e1000':
                # Reused PIDs must neither suppress DHCP nor kill another process.
                command += '; old=$(cat /run/felix-dhcp-eth0.pid); kill "$old"; sleep 1; sleep 60 & unrelated=$!; echo "$unrelated" > /run/felix-dhcp-eth0.pid; sleep 8; current=$(cat /run/felix-dhcp-eth0.pid); test "$current" != "$unrelated" && kill -0 "$unrelated" && tr "\\000" " " < /proc/$current/cmdline | grep -q "udhcpc -f -i eth0" && ip -4 addr show eth0 | grep -q "inet 10.0.2."'
            connection.sendall((command+f'; printf "\\n{marker}:%s\\n" "$?"\n').encode())
            deadline=time.monotonic()+15
            while time.monotonic()<deadline:
                try:data.extend(connection.recv(65536))
                except socket.timeout:continue
                match=re.search(rb'[\r\n]'+marker.encode()+rb':(\d+)',data)
                if match:
                    assert match[1]==b'0',data.decode(errors='replace')
                    print('ETHERNET_DHCP_OK',model,flush=True);break
            else:raise RuntimeError(f'{model}: DHCP check timed out')
        finally:
            (root/f'build/1.1-ethernet-{model}.log').write_bytes(data)
            if connection:connection.close()
            guest.terminate()
            try:guest.wait(timeout=5)
            except subprocess.TimeoutExpired:guest.kill();guest.wait()
