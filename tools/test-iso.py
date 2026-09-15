#!/usr/bin/env python3
"""Boot the actual BIOS ISO and drive its serial console and QEMU display."""
import pathlib, socket, subprocess, time, sys
from rfb_client import Viewer
ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/release'

def connect(port, proc):
    for _ in range(100):
        if proc.poll() is not None:
            raise RuntimeError('QEMU exited before becoming ready')
        try:
            s=socket.create_connection(('127.0.0.1', port), .5)
            s.settimeout(.5)
            return s
        except OSError: time.sleep(.2)
    raise TimeoutError('QEMU listener')

def main():
    with open(OUT/'qemu-test.log','wb') as log:
        p=subprocess.Popen(['qemu-system-i386','-accel','kvm','-cpu','host','-m','512',
            '-smp','1','-cdrom',str(OUT/'felix-1.0-x86.iso'),'-boot','d','-vga','std',
            '-nic','user,model=e1000','-display','none','-vnc','127.0.0.1:19',
            '-serial','tcp:127.0.0.1:2311,server=on,wait=off','-no-reboot'],stdout=log,stderr=log)
        s=connect(2311,p)
        received=bytearray()
        try:
            deadline=time.monotonic()+90
            while time.monotonic()<deadline:
                try: received.extend(s.recv(65536))
                except socket.timeout: pass
                if b'starting TinyX' in received: break
                if p.poll() is not None: break
            time.sleep(5)
            command="uname -a; cat /tmp/tinyx.log; ps; cat /tmp/network.log; DISPLAY=:0 /usr/bin/felix-root '#AEE1FF'; echo FELIX_X_RESULT:$?; apk update; echo FELIX_APK_RESULT:$?; echo FELIX_TEST_DONE"
            s.sendall(command.encode()+b'\n')
            deadline=time.monotonic()+45
            while time.monotonic()<deadline:
                try: received.extend(s.recv(65536))
                except socket.timeout: pass
                if b'\r\nFELIX_TEST_DONE\r\n' in received: break
            (OUT/'boot-test.log').write_bytes(received)
            print(received.decode(errors='replace')[-15000:])
            r=Viewer('127.0.0.1',5919)
            r.screenshot(str(OUT/'felix-linux-desktop.png'))
            r.close()
            assert b'\r\nFELIX_X_RESULT:0\r\n' in received, 'TinyX readiness failed'
            assert b'6.18.50-felix' in received and b'i686 Linux' in received, 'Wrong kernel or architecture'
            assert b'\r\nFELIX_APK_RESULT:0\r\n' in received, 'Repository refresh failed'
            print('ISO_BOOT_TEST_OK')
        finally:
            s.close();p.terminate()
            try:p.wait(timeout=10)
            except subprocess.TimeoutExpired:p.kill();p.wait()
if __name__=='__main__': main()
