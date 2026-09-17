#!/usr/bin/env python3
"""Integration checks using real FLTK windows in the booted ISO."""
import pathlib, socket, subprocess, time, re, base64
from rfb_client import Viewer
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=ROOT/'build/release'
class Guest:
    def __init__(self,data_disk=None):
        self.log=open(OUT/'apps-qemu.log','wb')
        args=['qemu-system-i386','-accel','kvm','-cpu','host','-m','512','-cdrom',str(OUT/'felix-1.1-x86.iso'),'-boot','d','-vga','std','-nic','user,model=e1000','-display','none','-vnc','127.0.0.1:19','-serial','tcp:127.0.0.1:2311,server=on,wait=off','-no-reboot']
        if data_disk:args+=['-drive',f'file={data_disk},format=raw,if=ide']
        self.p=subprocess.Popen(args,stdout=self.log,stderr=self.log)
        for _ in range(100):
            try:self.s=socket.create_connection(('127.0.0.1',2311),.5);break
            except OSError:time.sleep(.2)
        self.s.settimeout(.3);self.transcript=bytearray();self.seq=0
        self.read_until(b'starting TinyX',60);time.sleep(5)
        self.command('export DISPLAY=:0 TERM=dumb; stty -echo')
        encoded=base64.encodebytes((OUT/'xdrive-test').read_bytes()).decode()
        self.command("cat >/tmp/xdrive.b64 <<'END_DRIVER'\n"+encoded+"END_DRIVER\nbase64 -d /tmp/xdrive.b64 >/tmp/xdrive; chmod +x /tmp/xdrive",30)
    def read_until(self,marker,timeout=15):
        data=bytearray();end=time.monotonic()+timeout
        while time.monotonic()<end:
            try:
                part=self.s.recv(65536)
                if not part:raise EOFError('guest disconnected')
                data.extend(part);self.transcript.extend(part)
            except socket.timeout:pass
            if marker in data:return data.decode(errors='replace')
        raise TimeoutError(data.decode(errors='replace')[-3000:])
    def command(self,cmd,timeout=15):
        self.seq+=1;marker=f'COMMAND_DONE_{self.seq}'
        separator=' ' if cmd.rstrip().endswith('&') else '; '
        self.s.sendall((cmd+separator+f"printf '\\n%s\\n' {marker}\n").encode())
        result=self.read_until(('\r\n'+marker+'\r\n').encode(),timeout)
        print(result,flush=True);return result
    def screenshot(self,name):
        time.sleep(.5);v=Viewer(port=5919);v.screenshot(OUT/name);v.close()
    def close(self,log_name='apps-test.log'):
        (OUT/log_name).write_bytes(self.transcript)
        self.s.close();self.p.terminate()
        try:self.p.wait(timeout=10)
        except subprocess.TimeoutExpired:self.p.kill();self.p.wait()
        self.log.close()

def main():
    g=Guest()
    try:
        g.command('felix-apps notepad >/tmp/notepad.log 2>&1 &')
        time.sleep(2)
        g.command("/tmp/xdrive move 'Felix Notepad' 20 40; /tmp/xdrive focus 'Felix Notepad'; /tmp/xdrive click 'Felix Notepad' 100 100; /tmp/xdrive type 'Felix Notepad' 'Felix ISO text save test'; /tmp/xdrive key 'Felix Notepad' s ctrl")
        g.screenshot('notepad-save-dialog.png')
        g.command("/tmp/xdrive type 'Save text document' '/root/Documents/felix-test.txt'; /tmp/xdrive key 'Save text document' Return")
        time.sleep(1)
        saved=g.command('cat /root/Documents/felix-test.txt')
        assert 'Felix ISO text save test' in saved, 'Notepad did not save typed text'
        g.command("/tmp/xdrive key 'Felix Notepad' n ctrl; /tmp/xdrive key 'Felix Notepad' o ctrl")
        time.sleep(.5)
        g.command("/tmp/xdrive type 'Open text document' '/root/Documents/felix-test.txt'; /tmp/xdrive key 'Open text document' Return")
        time.sleep(.5)
        g.command("/tmp/xdrive click 'Felix Notepad' 100 100; /tmp/xdrive key 'Felix Notepad' End ctrl; /tmp/xdrive type 'Felix Notepad' ' and reopened'; /tmp/xdrive key 'Felix Notepad' s ctrl")
        saved=g.command('cat /root/Documents/felix-test.txt')
        assert 'Felix ISO text save test and reopened' in saved, 'Notepad reopen/save failed'
        g.screenshot('felix-notepad.png')
        g.command('felix-apps settings >/tmp/settings.log 2>&1 &')
        time.sleep(1)
        g.command("/tmp/xdrive move 'Felix Settings' 60 80; /tmp/xdrive focus 'Felix Settings'; /tmp/xdrive click 'Felix Settings' 300 135; /tmp/xdrive key 'Felix Settings' a ctrl; /tmp/xdrive type 'Felix Settings' '#8899AA'; /tmp/xdrive click 'Felix Settings' 500 465")
        assert 'color=#8899AA' in g.command('cat /root/.felix/settings')
        g.screenshot('felix-settings.png')
        g.command("printf 'P6\\n1 1\\n255\\n\\020\\040\\100' >/root/Documents/test.ppm; /tmp/xdrive click 'Felix Settings' 300 185; /tmp/xdrive type 'Felix Settings' '/root/Documents/test.ppm'; /tmp/xdrive click 'Felix Settings' 500 465")
        assert 'wallpaper=/root/Documents/test.ppm' in g.command('cat /root/.felix/settings')
        v=Viewer(port=5919);pixels=v.frame();v.close()
        assert pixels[-4:-1]==bytes([64,32,16]), 'Wallpaper was not rendered'
        g.command("/tmp/xdrive click 'Felix Settings' 625 85; /tmp/xdrive click 'Felix Settings' 500 465; /tmp/xdrive click 'Felix Settings' 155 25; /tmp/xdrive click 'Felix Settings' 300 95; /tmp/xdrive key 'Felix Settings' a ctrl; /tmp/xdrive type 'Felix Settings' '5'; /tmp/xdrive click 'Felix Settings' 300 145; /tmp/xdrive key 'Felix Settings' a ctrl; /tmp/xdrive type 'Felix Settings' '25'; /tmp/xdrive click 'Felix Settings' 180 205; /tmp/xdrive click 'Felix Settings' 500 465")
        actual=g.command("/tmp/xdrive settings 'Felix Settings'")
        assert 'saver=300 bell=25 repeat=0' in actual, 'X settings did not apply'
        assert 'mouse=10/10' not in actual, 'Mouse speed did not change'
        g.command("/tmp/xdrive click 'Felix Settings' 95 25; /tmp/xdrive click 'Felix Settings' 270 315; /tmp/xdrive key 'Felix Settings' Return")
        time.sleep(.5)
        g.command("/tmp/xdrive key 'Felix Settings' Return; ip addr show eth0; ip route; cat /etc/resolv.conf")
        g.command("/tmp/xdrive click 'Felix Settings' 235 25; /tmp/xdrive click 'Felix Settings' 270 155; /tmp/xdrive key 'Felix Settings' Return; date -u")
        g.command('felix-apps packages >/tmp/packages.log 2>&1 &')
        time.sleep(1)
        g.command("/tmp/xdrive move 'Felix Applications' 100 100; /tmp/xdrive focus 'Felix Applications'; /tmp/xdrive click 'Felix Applications' 750 30")
        time.sleep(8)
        g.command("/tmp/xdrive click 'Felix Applications' 250 30; /tmp/xdrive type 'Felix Applications' 'nano'; /tmp/xdrive click 'Felix Applications' 650 30")
        time.sleep(2)
        g.command("/tmp/xdrive click 'Felix Applications' 250 75; /tmp/xdrive type 'Felix Applications' 'nano'; /tmp/xdrive click 'Felix Applications' 540 75")
        time.sleep(2)
        g.screenshot('felix-applications.png')
        before=g.command('apk info -e nano; echo PREVIEW_INSTALLED:$?')
        assert 'PREVIEW_INSTALLED:1' in before, 'Preview unexpectedly installed the package'
        g.command("/tmp/xdrive click 'Felix Applications' 655 75")
        g.screenshot('package-install-dialog.png')
        g.command("/tmp/xdrive key 'Felix Applications' Return")
        time.sleep(6)
        installed=g.command('apk info -e nano; echo PACKAGE_INSTALLED:$?; nano --version')
        assert 'PACKAGE_INSTALLED:0' in installed and 'GNU nano' in installed, 'GUI package installation failed'
        g.screenshot('felix-applications-installed.png')
        print('ISO_APPS_TEST_OK',flush=True)
    finally:g.close()
if __name__=='__main__':main()
