#!/usr/bin/env python3
"""Exercise real TinyX menus with Xlib events; no display or screenshots."""
import pathlib, re, secrets, socket, subprocess, time, json, sys, base64, shutil
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'build/release'
cycles=int(sys.argv[1]) if len(sys.argv)>1 else 60
builder=pathlib.Path('/var/tmp/felix-linux/alpine')
shutil.copy2(root/'userland/tests/menu-input.c',builder/'work/menu-input.c')
subprocess.run(['chroot',str(builder),'sh','-c','cc -Os /work/menu-input.c -lX11 -lXtst -o /work/menu-input && strip /work/menu-input'],check=True)
helper=base64.b64encode((builder/'work/menu-input').read_bytes()).decode()
data=bytearray()
serial=qmp=None
def connect(port):
    for _ in range(60):
        try:return socket.create_connection(('127.0.0.1',port),.3)
        except OSError:time.sleep(.1)
    raise RuntimeError('QEMU connection timed out')
def command(line):
    marker='MENU_'+secrets.token_hex(4)
    serial.sendall((line+f'; printf "\\n{marker}:%s\\n" "$?"\n').encode())
    buf=bytearray();end=time.monotonic()+15
    while time.monotonic()<end:
        try:chunk=serial.recv(65536)
        except socket.timeout:continue
        if not chunk:raise RuntimeError('Guest stopped')
        buf.extend(chunk);data.extend(chunk)
        m=re.search(rb'[\r\n]'+marker.encode()+rb':(\d+)[\r\n]',buf)
        if m:
            assert m[1]==b'0',buf.decode(errors='replace')
            return
    raise RuntimeError('Guest command timed out')
def request(name,args=None):
    qmp.sendall((json.dumps({'execute':name,'arguments':args or {}})+'\n').encode())
    while True:
        item=json.loads(qfile.readline())
        if 'error' in item:raise RuntimeError(item)
        if 'return' in item:
            if name=='human-monitor-command' and item['return']:print(item['return'],flush=True)
            return item['return']
def mouse_menu():
    command('DISPLAY=:0 /tmp/menu-input open')
def key(k):
    names={'esc':'Escape','ret':'Return','home':'Home','down':'Down','up':'Up'}
    # Return activates logout, so the input client can lose X during XSync.
    command('DISPLAY=:0 /tmp/menu-input '+names.get(k,k)+(' || :' if k=='ret' else ''))
with open(out/'menu-qemu.log','wb') as log:
    p=subprocess.Popen(['qemu-system-i386','-accel','kvm','-cpu','host','-m','512',
        '-cdrom',str(out/'felix-1.1-x86.iso'),'-boot','d','-vga','std',
        '-nic','user,model=e1000','-display','none',
        '-serial','tcp:127.0.0.1:2316,server=on,wait=off',
        '-qmp','tcp:127.0.0.1:2317,server=on,wait=off','-no-reboot'],stdout=log,stderr=log)
    try:
        serial=connect(2316);serial.settimeout(.2)
        qmp=connect(2317);qmp.settimeout(5);qfile=qmp.makefile('rb')
        qfile.readline();request('qmp_capabilities')
        time.sleep(25)
        command('stty -echo; wm=$(pidof flwm); test -n "$wm"')
        command('apk add --no-cache libxtst >/tmp/menu-tools.log 2>&1')
        command(': >/tmp/menu-input.b64')
        for offset in range(0,len(helper),1800):command("printf '%s' '"+helper[offset:offset+1800]+"' >>/tmp/menu-input.b64")
        command('base64 -d /tmp/menu-input.b64 >/tmp/menu-input; chmod +x /tmp/menu-input')
        command('DISPLAY=:0 felix-apps notepad >/tmp/menu-notepad.log 2>&1 & app=$!; sleep 1; kill -0 $app')
        for i in range(cycles):
            mouse_menu()
            if i==20:
                command('kill $app; sleep 1')
                key('home');key('down');key('up')
            key('esc')
            if i%10==0:command('test "$(pidof flwm)" = "$wm"')
        command('test "$(pidof flwm)" = "$wm" && pidof Xfbdev && pidof wbar; cat /tmp/menu-notepad.log')
        mouse_menu()
        for _ in range(32):key('down')
        key('ret')
        command('sleep 3; ps; ! pidof flwm && ! pidof Xfbdev && ! pidof wbar')
        print(f'MENU_ISO_OK: {cycles} verified popup openings; original flwm PID retained until logout')
        print('EXIT_TO_CONSOLE_OK: menu action stops the desktop')
    finally:
        (out/'menu-serial.log').write_bytes(data)
        if serial:serial.close()
        if qmp:qmp.close()
        p.terminate()
        try:p.wait(timeout=5)
        except subprocess.TimeoutExpired:p.kill();p.wait()
