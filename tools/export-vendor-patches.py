#!/usr/bin/env python3
"""Maintainer helper: export Felix changes, excluding generated build files."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
out=root/'patches';out.mkdir(exist_ok=True)
for name in ('tinyx','flwm','wbar'):
    directory=root/'userland'/name
    patch=subprocess.check_output(['git','diff','--binary','HEAD'],cwd=directory)
    new=subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=directory).decode().splitlines()
    for path in new:
        if path=='Glass.H' or (path.startswith('kdrive/remote/') and not path.endswith('Makefile.in')):
            result=subprocess.run(['git','diff','--no-index','--binary','--','/dev/null',path],cwd=directory,stdout=subprocess.PIPE)
            if result.returncode not in (0,1):raise RuntimeError(path)
            patch+=result.stdout
    (out/(name+'.patch')).write_bytes(patch)
    print(name,len(patch),'patch bytes')
