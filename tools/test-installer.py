#!/usr/bin/env python3
"""Destructive installer test, exclusively inside QEMU on a new disposable disk.
No host block device is passed to QEMU. No graphical display is opened/captured.
"""
import os,pathlib,re,secrets,socket,subprocess,tempfile,time
root=pathlib.Path(__file__).resolve().parents[1]
release=root/'build/release'
work=pathlib.Path(tempfile.mkdtemp(prefix='felix-install-test-',dir='/var/tmp'))
disk=work/'disk.qcow2';password=secrets.token_hex(10);user='felixuser'
subprocess.run(['qemu-img','create','-f','qcow2',str(disk),'3G'],check=True,stdout=subprocess.DEVNULL)
transcript=bytearray()
class Guest:
    def __init__(self,live):
        self.log=open(work/('live-qemu.log' if live else 'disk-qemu.log'),'ab')
        args=['qemu-system-i386','-accel','kvm','-cpu','host','-m','768','-vga','std','-nic','user,model=e1000','-display','none','-serial','tcp:127.0.0.1:2315,server=on,wait=off','-no-reboot','-drive',f'file={disk},format=qcow2,if=ide']
        if live:args+=['-cdrom',str(release/'felix-1.1-x86.iso'),'-boot','d']
        else:args+=['-boot','c']
        self.p=subprocess.Popen(args,stdout=self.log,stderr=self.log)
        for _ in range(60):
            try:self.s=socket.create_connection(('127.0.0.1',2315),.2);break
            except OSError:time.sleep(.1)
        else:raise RuntimeError('QEMU serial failed')
        self.s.settimeout(.2);self.buf=bytearray()
    def until(self,pattern,timeout=40):
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            m=re.search(pattern,self.buf)
            if m:return m
            try:chunk=self.s.recv(65536)
            except socket.timeout:continue
            if not chunk:raise RuntimeError('QEMU stopped before expected output')
            self.buf.extend(chunk);transcript.extend(chunk)
        raise RuntimeError('Timed out: '+str(pattern)+'\n'+self.buf.decode(errors='replace')[-4000:])
    def send(self,s):self.s.sendall(s.encode())
    def run(self,command,timeout=60,expected=0):
        marker='CHECK_'+secrets.token_hex(4)
        self.buf.clear();self.send(command+f'; code=$?; printf "\\n{marker}:%s\\n" "$code"\n')
        result=self.until(rb'[\r\n]'+marker.encode()+rb':([0-9]+)[\r\n]',timeout)
        assert int(result[1])==expected,self.buf.decode(errors='replace')[-6000:]
        return self.buf.decode(errors='replace')
    def login(self):
        self.until(rb'login:');self.buf.clear();self.send(user+'\n')
        self.until(rb'Password:');self.buf.clear();self.send(password+'\n')
        self.until(rb'\$ ');self.run('stty -echo')
    def close(self):
        self.s.close();self.p.terminate()
        try:self.p.wait(timeout=5)
        except subprocess.TimeoutExpired:self.p.kill();self.p.wait()
        self.log.close()
g=None
try:
    g=Guest(True);g.until(rb'/ # ');g.run('stty -echo')
    import hashlib
    expected=hashlib.sha256((root/'userland/bin/felix-install').read_bytes()).hexdigest()
    assert expected in g.run('sha256sum /usr/bin/felix-install'), 'ISO contains a stale installer'
    listed=g.run('/usr/bin/felix-install --list');assert '/dev/sda\t' in listed
    print('DISPOSABLE_TARGET_FOUND',flush=True)
    g.run("token=$(/usr/bin/felix-install --list | awk '$1==\"/dev/sda\"{print $3}'); "+f"printf '%s\\n' '{password}' | /usr/bin/felix-install --erase-and-install /dev/sda \"$token\" {user}",timeout=240)
    print('INSTALLATION_COMPLETE',flush=True)
    g.run("mkdir -p /mnt/guard; mount -t ext4 /dev/sda1 /mnt/guard; test -z \"$(/usr/bin/felix-install --list)\"")
    g.run(f"printf '%s\\n' '{password}' | /usr/bin/felix-install --erase-and-install /dev/sda \"$token\" {user}",expected=1)
    g.run('umount /mnt/guard')
    print('MOUNTED_DISK_REFUSED',flush=True)
    g.run('sync');g.close();g=None
    for boot in range(2):
        g=Guest(False);g.login()
        g.run("test $(id -u) -ne 0 && awk '$2==\"/\" && $3==\"ext4\" && $4 ~ /rw/ {ok=1} END{exit !ok}' /proc/mounts && ! grep -q initrd= /proc/cmdline")
        if boot==1:
            g.run('grep -q felix.text=1 /proc/cmdline && ! pidof Xfbdev')
            g.run(f"printf '%s\\n' '{password}' | sudo -S startx >/tmp/installed-startx.log 2>&1 & sleep 8; pidof Xfbdev")
        g.run("tries=0; while ! pidof wbar >/dev/null && test $tries -lt 10; do sleep 1; tries=$((tries+1)); done; ps; id; cat /tmp/tinyx.log /tmp/wbar.log; test $(stat -c %u /proc/$(pidof flwm)) -eq $(id -u) && pidof wbar >/dev/null")
        g.run(f"printf '%s\\n' '{password}' | sudo -S -k sh -c 'test $(id -u) -eq 0'")
        if boot==0:
            g.run('echo DISK_PERSISTENCE_OK > ~/Documents/installed-proof.txt; sync')
            g.run(f"printf '%s\\n' '{password}' | sudo -S sed -i 's/^DEFAULT felix$/DEFAULT text/' /boot/syslinux/extlinux.conf")
        else:g.run('grep -q DISK_PERSISTENCE_OK ~/Documents/installed-proof.txt')
        g.run(f"printf '%s\\n' '{password}' | sudo -S -k sync")
        g.send(f"printf '%s\\n' '{password}' | sudo -S poweroff\n")
        try:g.p.wait(timeout=15)
        except subprocess.TimeoutExpired:pass
        g.close();g=None
        print(f'DISK_ONLY_BOOT_{boot+1}_OK',flush=True)
    print('INSTALLER_E2E_OK: disk root, non-root desktop, sudo, mounted-disk refusal and persistent document across reboot',flush=True)
finally:
    if g:g.close()
    (release/'installer-test.log').write_text(transcript.decode(errors='replace').replace(password,'[test password redacted]'))
    (release/'installer-test-disk.txt').write_text(str(disk)+'\n')
