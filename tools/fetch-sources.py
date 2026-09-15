#!/usr/bin/env python3
"""Fetch pinned upstream trees; retain only Felix patches in the repository."""
from pathlib import Path
import argparse, hashlib, subprocess, tempfile, urllib.request
parser=argparse.ArgumentParser()
parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
root=parser.parse_args().root.resolve()
(root/'userland').mkdir(parents=True,exist_ok=True)
sources={
 'tinyx':('https://github.com/tinycorelinux/tinyx.git','feab72ca891bc04b18763763e15ee4e532369cdf'),
 'flwm':('https://github.com/tinycorelinux/flwm.git','196f61c036ef65d65d5ec4095667f9a2d21822e2'),
 'wbar':('https://github.com/SpartanJ/wbar.git','f29900c82bf9184a29ba7c608a95cd0b714df8ec')}
def git(directory,*args):
    return subprocess.check_output(['git','-C',str(directory),*args],stderr=subprocess.STDOUT)
for name,(url,revision) in sources.items():
    target=root/'userland'/name;patch=root/'patches'/(name+'.patch')
    if target.exists():
        if git(target,'rev-parse','HEAD').decode().strip()!=revision:
            raise SystemExit(f'{target}: unexpected upstream revision; preserve local work and resolve manually.')
        # Never reset a developer tree. A matching reverse check proves that
        # every required Felix patch is present while preserving other edits.
        git(target,'apply','--reverse','--check',str(patch))
        print(name,'already prepared',flush=True)
        continue
    staging=Path(tempfile.mkdtemp(prefix=f'.{name}-',dir=target.parent))
    subprocess.run(['git','clone','--no-checkout',url,str(staging)],check=True)
    git(staging,'checkout','--detach',revision)
    git(staging,'apply','--check',str(patch))
    git(staging,'apply',str(patch))
    staging.rename(target)
    print(name,revision,flush=True)
archive=root/'userland/fltk-1.4.5-source.tar.gz'
digest='eede1fb2b8e9c2e581e77082e15252145855c79aad30070ee3b24aabe2f926f1'
if not archive.exists():
    temp=archive.with_suffix('.download')
    urllib.request.urlretrieve('https://github.com/fltk/fltk/releases/download/release-1.4.5/fltk-1.4.5-source.tar.gz',temp)
    if hashlib.sha256(temp.read_bytes()).hexdigest()!=digest:raise SystemExit('FLTK checksum mismatch')
    temp.rename(archive)
if hashlib.sha256(archive.read_bytes()).hexdigest()!=digest:raise SystemExit('FLTK checksum mismatch')
print('SOURCES_READY')
