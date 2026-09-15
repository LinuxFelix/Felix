#!/usr/bin/env python3
"""Capture unedited screenshots from the released Felix 1.0 ISO."""
from pathlib import Path
import hashlib, json, re, secrets, socket, subprocess, time
from rfb_client import Viewer

root=Path(__file__).resolve().parents[1]
out=root/'build/release'
iso=out/'felix-1.0-x86.iso'
expected=(out/(iso.name+'.sha256')).read_text().split()[0]
assert hashlib.sha256(iso.read_bytes()).hexdigest()==expected
serial=None;viewer=None;transcript=bytearray();shots=[]
with open(out/'screenshots-qemu.log','wb') as log:
    proc=subprocess.Popen(['qemu-system-i386','-accel','kvm','-cpu','host','-m','512',
        '-cdrom',str(iso),'-boot','d','-vga','std','-nic','user,model=e1000',
        '-display','none','-vnc','127.0.0.1:19','-serial',
        'tcp:127.0.0.1:2311,server=on,wait=off','-no-reboot'],stdout=log,stderr=log)
    try:
        for _ in range(100):
            if proc.poll() is not None:raise RuntimeError('QEMU exited')
            try:serial=socket.create_connection(('127.0.0.1',2311),.5);break
            except OSError:time.sleep(.2)
        if serial is None:raise RuntimeError('Serial connection unavailable')
        serial.settimeout(.3)
        time.sleep(25)
        def command(line):
            marker='CAPTURE_'+secrets.token_hex(6)
            serial.sendall((line+f'; printf "\\n{marker}:%s\\n" "$?"\n').encode())
            data=bytearray();deadline=time.monotonic()+30
            while time.monotonic()<deadline:
                try:data.extend(serial.recv(65536))
                except socket.timeout:continue
                match=re.search(rb'[\r\n]'+marker.encode()+rb':(\d+)[\r\n]',data)
                if match:
                    transcript.extend(data)
                    if match[1]!=b'0':raise RuntimeError(data.decode(errors='replace'))
                    return
            raise RuntimeError('Guest command timed out')
        command('stty -echo; . /etc/os-release; test "$VERSION_ID" = 1.0 && pidof flwm && pidof wbar')
        viewer=Viewer('127.0.0.1',5919)
        def capture(name):
            time.sleep(2)
            viewer.screenshot(out/name)
            shots.append(name)
            print('CAPTURED',name,flush=True)
        capture('felix-1.0-desktop.png')
        for app in ('notepad','settings','packages'):
            command(f'DISPLAY=:0 felix-apps {app} >/tmp/capture-app.log 2>&1 & shotpid=$!; sleep 2; kill -0 $shotpid')
            capture(f'felix-1.0-{app}.png')
            command('kill "$shotpid"; sleep 1')
        command('DISPLAY=:0 felix-terminal >/tmp/capture-terminal.log 2>&1 & shotpid=$!; sleep 2; kill -0 $shotpid')
        capture('felix-1.0-terminal.png')
        (out/'screenshots-1.0.json').write_text(json.dumps({
            'release':'1.0','iso':iso.name,'iso_sha256':expected,
            'method':'Unedited QEMU framebuffer captures from the release ISO',
            'screenshots':shots},indent=2)+'\n')
    finally:
        (out/'screenshots-serial.log').write_bytes(transcript)
        if viewer:viewer.close()
        if serial:serial.close()
        proc.terminate()
        try:proc.wait(timeout=10)
        except subprocess.TimeoutExpired:proc.kill();proc.wait()
