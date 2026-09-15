#!/usr/bin/env python3
"""Test the real fbBlt byte-copy path without requiring a running X server.

Extract only that path so this regression also runs with the Windows compiler.
All cases use byte-aligned GXcopy and a full plane mask; the unreachable general
ROP path is replaced with an assertion. Expected results use an immutable image.
"""
import os, pathlib, shutil, subprocess, sys
root=pathlib.Path(__file__).resolve().parents[1]
source=(root/'userland/tinyx/fb/fbblt.c').read_text()
start=source.index('void\nfbBlt(')
end=source.index('    FbInitializeMergeRop(alu, pm);',start)
code='''#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t FbBits;
typedef unsigned char CARD8;
typedef int FbStride;
typedef int Bool;
#define GXcopy 3
#define FB_ALLONES UINT32_MAX
#define FbCheck24Pix(pm) 1
#define FbDeclareMergeRop()
#define fbBlt24(...) abort()
'''+source[start:end]+'''    abort(); /* A test accidentally left the byte-copy path. */
}
int main(void) {
    enum { STRIDE=512, ROWS=48, SIZE=STRIDE*ROWS };
    uint32_t actual[SIZE/4],snapshot[SIZE/4],expected[SIZE/4];
    int count=0;
    for(int bpp=8;bpp<=32;bpp+=8)for(int dx=-7;dx<=7;dx++)for(int dy=-4;dy<=4;dy++)
    for(int preoffset=0;preoffset<2;preoffset++) {
        int sx=32,sy=8,tx=sx+dx,ty=sy+dy,width=64*bpp/8,height=24;
        for(int i=0;i<SIZE/4;i++)actual[i]=0x13579bdfu+(uint32_t)i*2654435761u;
        memcpy(snapshot,actual,SIZE);memcpy(expected,actual,SIZE);
        for(int y=0;y<height;y++)
            memcpy((char*)expected+(ty+y)*STRIDE+tx*4,(char*)snapshot+(sy+y)*STRIDE+sx*4,width);
        fbBlt(actual+sy*(STRIDE/4)+(preoffset?sx:0),STRIDE/4,preoffset?0:sx*32,
              actual+ty*(STRIDE/4)+(preoffset?tx:0),STRIDE/4,preoffset?0:tx*32,
              width*8,height,GXcopy,FB_ALLONES,bpp,dx>0,dy>0);
        if(memcmp(actual,expected,SIZE)) {fprintf(stderr,"copy mismatch bpp=%d dx=%d dy=%d preoffset=%d\\n",bpp,dx,dy,preoffset);return 1;}
        count++;
    }
    printf("FB_BLIT_TEST_OK: %d overlapping rectangle cases\\n",count);
    return 0;
}
'''
out=root/'build/tests';out.mkdir(parents=True,exist_ok=True)
c=out/'fbblt-regression.c';c.write_text(code)
exe=out/('fbblt-regression.exe' if os.name=='nt' else 'fbblt-regression')
compiler=os.environ.get('CC') or shutil.which('gcc') or shutil.which('clang')
if not compiler:raise SystemExit('A C compiler is required')
subprocess.run([compiler,'-std=c99','-O2',str(c),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
