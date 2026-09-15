#!/usr/bin/env python3
"""Test the FLTK app's backend against real wpa_supplicant, without a display.

Run under `unshare --net`: a dummy Ethernet device exercises configuration and
control transport only. It cannot verify radio scanning or authentication.
"""
import os, pathlib, subprocess, tempfile, time

root=pathlib.Path('/var/tmp/felix-linux/alpine')
subprocess.run(['ip','link','add','felix-test','type','dummy'],check=True)
subprocess.run(['ip','link','set','felix-test','up'],check=True)
base=root/'run/felix-wifi'
base.mkdir(mode=0o700,parents=True,exist_ok=True)
(base/'control').mkdir(mode=0o700,exist_ok=True)
app=['chroot',str(root),'/out/felix-apps','wifi']

def control(command):
    return subprocess.check_output(app+['--control-test','felix-test',command],text=True).strip()

with tempfile.TemporaryDirectory(prefix='test-',dir=base) as work:
    conf=pathlib.Path(work)/'wpa.conf'
    conf.write_text('ctrl_interface=/run/felix-wifi/control\nupdate_config=1\n')
    conf.chmod(0o600)
    with open(pathlib.Path(work)/'daemon.log','wb') as log:
        daemon=subprocess.Popen(['chroot',str(root),'/sbin/wpa_supplicant','-D','wired','-i','felix-test','-c','/'+str(conf.relative_to(root))],stdout=log,stderr=log,umask=0o077)
        try:
            for _ in range(50):
                if (base/'control/felix-test').exists():break
                if daemon.poll() is not None:raise RuntimeError('Test daemon failed: '+(pathlib.Path(work)/'daemon.log').read_text())
                time.sleep(.1)
            assert control('PING')=='PONG'
            subprocess.run(app+['--self-test'],check=True)
            # Exercise the same configuration function used by Save & connect.
            fixtures=[(0,'Felix open',''),(1,'Felix "quoted" \\ ssid','test"pass\\word'),(2,'Felix SAE','sae"pass\\word')]
            ids=[]
            for mode,ssid,password in fixtures:
                ident=subprocess.check_output(app+['--profile-test','felix-test'],input=f'{mode}\n{ssid}\n{password}\n',text=True).strip()
                assert ident.isdigit(),ident
                ids.append(ident)
                assert control(f'GET_NETWORK {ident} scan_ssid')=='1'
                assert control(f'GET_NETWORK {ident} key_mgmt')==['NONE','WPA-PSK','SAE'][mode]
            assert control('SAVE_CONFIG')=='OK'
            saved=conf.read_text()
            assert 'psk="test"pass\\word"' in saved, 'PSK bytes changed during save'
            assert 'sae_password=' in saved
            assert conf.stat().st_mode & 0o777 == 0o600, 'Configuration permissions are not private'
            assert control('RECONFIGURE')=='OK'
            assert len(control('LIST_NETWORKS').splitlines())==4
            assert control('SAVE_CONFIG')=='OK'
            assert conf.read_text()==saved, 'Profile bytes changed after reloading'
            assert control('REMOVE_NETWORK '+ids[1])=='OK'
            assert control('SAVE_CONFIG')=='OK'
            assert control('RECONFIGURE')=='OK'
            assert len(control('LIST_NETWORKS').splitlines())==3
            assert 'test"pass\\word' not in conf.read_text()
            print('WIFI_REAL_DAEMON_CONTROL_OK: open/WPA2/WPA3 profiles, quoted credentials, save/reload, private permissions and forgetting')
        finally:
            daemon.terminate()
            try:daemon.wait(timeout=5)
            except subprocess.TimeoutExpired:daemon.kill();daemon.wait()
