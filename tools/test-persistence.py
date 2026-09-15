#!/usr/bin/env python3
import importlib.util, pathlib, tempfile, subprocess, os
spec=importlib.util.spec_from_file_location('apps_test',pathlib.Path(__file__).with_name('test-apps.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
out=module.OUT
fd,path=tempfile.mkstemp(prefix='felix-data-test-',suffix='.img',dir=out)
try:
    os.ftruncate(fd,32*1024*1024);os.close(fd)
    subprocess.run(['mkfs.ext4','-q','-F','-L','FELIXDATA',path],check=True)
    logs=[]
    for run in range(2):
        g=module.Guest(path)
        try:
            mounts=g.command('mount; cat /proc/partitions; blkid')
            assert 'on /root type ext4' in mounts, 'Persistent home was not mounted'
            if not run:
                written=g.command("printf 'persistent Felix document' >/root/Documents/persist.txt && printf 'color=#AEE1FF\\nmouse=20\\n' >/root/.felix/settings && sync && echo PERSIST_WRITE_OK")
                assert 'PERSIST_WRITE_OK' in written, 'Could not save to persistent home'
            else:
                data=g.command('cat /root/Documents/persist.txt; cat /root/.felix/settings')
                assert 'persistent Felix document' in data and 'mouse=20' in data
                actual=g.command("/tmp/xdrive settings 'About Felix'")
                assert 'mouse=20/10' in actual, 'Saved settings were not restored'
            g.command('sync')
            g.s.sendall(b'poweroff -f\n');g.p.wait(timeout=15)
        finally:
            logs.append(bytes(g.transcript));g.close('persistence-last-boot.log')
    (out/'persistence-test.log').write_bytes(b'\nBOOT AGAIN\n'.join(logs))
    print('ISO_PERSISTENCE_TEST_OK')
finally:
    pathlib.Path(path).unlink(missing_ok=True)
