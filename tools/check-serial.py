#!/usr/bin/env python3
"""Read boot diagnostics without opening or capturing the graphical display."""
import pathlib,socket,subprocess,time
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'build/release'
with open(out/'serial-qemu.log','wb') as log:
    p=subprocess.Popen(['qemu-system-i386','-accel','kvm','-cpu','host','-m','512',
        '-cdrom',str(out/'felix-1.0-x86.iso'),'-boot','d','-vga','std',
        '-nic','user,model=e1000','-display','none',
        '-serial','tcp:127.0.0.1:2313,server=on,wait=off','-no-reboot'],stdout=log,stderr=log)
    s=None;data=bytearray()
    try:
        for _ in range(50):
            try:s=socket.create_connection(('127.0.0.1',2313),.5);break
            except OSError:time.sleep(.2)
        if s is None:raise RuntimeError('Serial port did not open')
        s.settimeout(.3)
        end=time.monotonic()+25
        while time.monotonic()<end:
            try:data.extend(s.recv(65536))
            except socket.timeout:pass
        s.sendall(b"stty -echo; ps; cat /tmp/tinyx.log /tmp/wbar.log /tmp/network.log; pidof Xfbdev >/dev/null && pidof wbar >/dev/null && pidof flwm >/dev/null && test ! -e /usr/bin/pmdock; echo DESKTOP_RESULT:$?; wificonfig --self-test && wpa_supplicant -v && iw dev && test -z \"$(wificonfig --list)\" && test -e /sys/class/net/eth0 && test -s /lib/firmware/rt2870.bin.zst && test -s /lib/firmware/mt7601u.bin.zst && sha256sum -c /usr/share/felix/wifi-firmware.sha256 >/tmp/firmware-check.log; echo WIFI_RESULT:$?; DISPLAY=:0 felix-terminal -e sh -c 'echo TERMINAL_PTY_OK >/tmp/terminal-pty; sleep 20' >/tmp/terminal-test.log 2>&1 & sleep 3; DISPLAY=:0 /usr/share/felix/check-theme && cat /tmp/terminal-pty; echo THEME_RESULT:$?; cat /tmp/terminal-test.log; printf '\\nSERIAL_CHECK_DONE\\n'\n")
        s.sendall(b"DISPLAY=:0 /usr/share/felix/check-fonts; echo FONT_RESULT:$?; DISPLAY=:0 felix-apps notepad >/tmp/notepad-font.log 2>&1 & app=$!; DISPLAY=:0 felix-about >/tmp/about-font.log 2>&1 & about=$!; sleep 2; kill -0 $app && kill -0 $about; echo FONT_APPS_RESULT:$?; cat /tmp/notepad-font.log /tmp/about-font.log; kill $app $about; printf '\\nFONT_CHECK_DONE\\n'\n")
        end=time.monotonic()+15
        while time.monotonic()<end:
            try:data.extend(s.recv(65536))
            except socket.timeout:pass
            if b'\r\nFONT_CHECK_DONE\r\n' in data:break
        (out/'serial-check.log').write_bytes(data)
        print(data.decode(errors='replace')[-16000:])
        assert b'\r\nDESKTOP_RESULT:0\r\n' in data, 'Desktop processes are missing'
        assert b'\r\nFONT_RESULT:0\r\n' in data and b'\r\nFONT_APPS_RESULT:0\r\n' in data, 'DejaVu rendering or app startup failed'
        assert b'Start Felix desktop' in data and b'Recovery shell (text mode)' in data, 'Syslinux menu missing'
        assert b'\r\nWIFI_RESULT:0\r\n' in data, 'Wi-Fi tools, firmware or Ethernet detection failed'
        assert b'\r\nTHEME_RESULT:0\r\n' in data and b'TERMINAL_PTY_OK' in data, 'Dock layout, root pixmap or terminal PTY failed'
        assert b'unknown or malformed option' not in data and b'malformed option.' not in data, 'Terminal launch options were rejected'
        assert b'Desktop stopped' not in data, 'Session restarted during boot'
        assert b"Can't lookup blockdev" not in data, 'Missing-disk warning returned'
        print('SERIAL_STARTUP_CHECK_OK')
    finally:
        if s:s.close()
        p.terminate()
        try:p.wait(timeout=10)
        except subprocess.TimeoutExpired:p.kill();p.wait()
