#!/usr/bin/env python3
"""Select Syslinux Recovery through serial and verify no desktop is started."""
import pathlib,socket,subprocess,time
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'build/release'
p=subprocess.Popen(['qemu-system-i386','-accel','kvm','-cpu','host','-m','512','-cdrom',str(out/'felix-1.0-x86.iso'),'-boot','d','-vga','std','-nic','user,model=e1000','-display','none','-serial','tcp:127.0.0.1:2314,server=on,wait=off','-no-reboot'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
s=None;data=bytearray()
try:
    for _ in range(50):
        try:s=socket.create_connection(('127.0.0.1',2314),.2);break
        except OSError:time.sleep(.1)
    if s is None:raise RuntimeError('Serial connection failed')
    s.settimeout(.2)
    end=time.monotonic()+8
    while time.monotonic()<end and b'Recovery shell (text mode)' not in data:
        try:data.extend(s.recv(65536))
        except socket.timeout:pass
    assert b'Recovery shell (text mode)' in data,'Syslinux menu missing'
    s.sendall(b'\x1b[B\x1b[B\r')
    end=time.monotonic()+20
    while time.monotonic()<end:
        try:data.extend(s.recv(65536))
        except socket.timeout:pass
        if b'/ #' in data:break
    s.sendall(b"stty -echo; grep -q felix.rescue=1 /proc/cmdline && ! pidof Xfbdev; echo RECOVERY_RESULT:$?\n")
    end=time.monotonic()+5
    while time.monotonic()<end:
        try:data.extend(s.recv(65536))
        except socket.timeout:pass
        if b'\r\nRECOVERY_RESULT:0\r\n' in data:break
    (out/'recovery-check.log').write_bytes(data)
    assert b'\r\nRECOVERY_RESULT:0\r\n' in data,'Recovery entry did not boot into text mode'
    print('SYSLINUX_RECOVERY_OK')
finally:
    if s:s.close()
    p.terminate()
    try:p.wait(timeout=5)
    except subprocess.TimeoutExpired:p.kill();p.wait()
