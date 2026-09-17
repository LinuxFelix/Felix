#!/usr/bin/env python3
"""Select the live text boot entry, start/stop/restart X through the serial shell."""
import pathlib,socket,subprocess,time,re,secrets
root=pathlib.Path(__file__).resolve().parents[1];out=root/'build/release'
p=subprocess.Popen(['qemu-system-i386','-accel','kvm','-cpu','host','-m','512','-cdrom',str(out/'felix-1.1-x86.iso'),'-boot','d','-vga','std','-nic','user,model=e1000','-display','none','-serial','tcp:127.0.0.1:2318,server=on,wait=off','-no-reboot'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
s=None;data=bytearray()
def until(pattern,timeout=20):
    buf=bytearray();end=time.monotonic()+timeout
    while time.monotonic()<end:
        try:chunk=s.recv(65536)
        except socket.timeout:continue
        if not chunk:raise RuntimeError('Guest stopped')
        data.extend(chunk);buf.extend(chunk)
        match=re.search(pattern,buf)
        if match:return match
    raise RuntimeError(buf.decode(errors='replace')[-4000:])
def run(cmd):
    marker='TEXT_'+secrets.token_hex(4)
    s.sendall((cmd+f'; printf "\\n{marker}:%s\\n" "$?"\n').encode())
    m=until(rb'[\r\n]'+marker.encode()+rb':(\d+)[\r\n]')
    assert m[1]==b'0',data.decode(errors='replace')[-5000:]
try:
    for _ in range(50):
        try:s=socket.create_connection(('127.0.0.1',2318),.2);break
        except OSError:time.sleep(.1)
    if not s:raise RuntimeError('Serial unavailable')
    s.settimeout(.2)
    until(rb'Felix text mode \(startx\)',8)
    s.sendall(b'\x1b[B\x1b[B\x1b[B\r')
    until(rb'/ #')
    run('stty -echo; grep -q felix.text=1 /proc/cmdline && test -e /dev/fb0 && ! pidof Xfbdev')
    for attempt in range(2):
        run('startx >/tmp/startx-test.log 2>&1 & session=$!; sleep 5; pidof Xfbdev && pidof flwm && pidof wbar')
        run('! startx >/tmp/startx-duplicate.log 2>&1 && grep -q "already running" /tmp/startx-duplicate.log')
        run("DISPLAY=:0 felix-terminal -e sleep 12 >/tmp/text-terminal.log 2>&1 & sleep 2; DISPLAY=:0 /usr/share/felix/check-theme")
        run('kill -TERM $(pidof flwm); sleep 3; ! pidof Xfbdev && ! pidof wbar && test ! -d /run/felix-x.lock')
    print('TEXT_BOOT_STARTX_OK: no automatic X, two start/stop cycles, duplicate refused, clock ticks')
finally:
    (out/'textboot-check.log').write_bytes(data)
    if s:s.close()
    p.terminate()
    try:p.wait(timeout=5)
    except subprocess.TimeoutExpired:p.kill();p.wait()
