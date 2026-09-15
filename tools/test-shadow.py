#!/usr/bin/env python3
"""Exercise TinyX's actual packed shadow updater with banked mock framebuffers."""
from pathlib import Path
import os, subprocess
root=Path(__file__).resolve().parents[1]
source=(root/'userland/tinyx/miext/shadow/shpacked.c').read_text()
body=source[source.index('void\nshadowUpdatePacked('):source.index('\nshadowUpdateProc')]
prefix=r'''
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t FbBits, CARD32;
typedef int FbStride;
typedef void *ScreenPtr;
typedef struct {int x1,y1,x2,y2;} BoxRec, *BoxPtr;
typedef struct {int count; BoxRec boxes[2];} RegionRec, *RegionPtr;
typedef struct {int drawable;} PixmapRec, *PixmapPtr;
typedef struct shadowBuf {
    RegionRec damage; PixmapPtr pPixmap;
    void *(*window)(ScreenPtr,int,int,int,CARD32*,void*);
    void *closure;
} *shadowBufPtr;
#define shadowDamage(p) (&(p)->damage)
#define REGION_NUM_RECTS(r) ((r)->count)
#define REGION_RECTS(r) ((r)->boxes)
#define _X_UNUSED
#define FB_SHIFT 5
#define FB_MASK 31
#define SHADOW_WINDOW_WRITE 1
static FbBits ram[32*12],screen[32*12],expected[32*12];
static int bpp,bank,empty;
#define fbGetDrawable(d,base,stride,bits,x,y) do {base=ram;stride=32;bits=bpp;} while(0)
static void *window(ScreenPtr s,int row,int offset,int mode,CARD32 *size,void *data) {
    assert(row>=0 && row<12 && offset>=0 && offset<128);
    int length=bank-offset%bank;
    if(length>128-offset)length=128-offset;
    *size=empty?0:length;
    return (char*)screen+row*128+offset;
}
int main(void);
'''
suffix=r'''
int main(void) {
    PixmapRec pix={0};
    struct shadowBuf buf={.pPixmap=&pix,.window=window};
    int cases=0;
    for(bpp=8;bpp<=32;bpp+=8)for(bank=4;bank<=128;bank*=2)
    for(int left=0;left<7;left++)for(int width=1;width<12;width++) {
        for(int i=0;i<384;i++){ram[i]=0xabc00000u+i;screen[i]=expected[i]=0xdeadbeef;}
        buf.damage.count=2;
        buf.damage.boxes[0]=(BoxRec){left,1,left+width,5};
        buf.damage.boxes[1]=(BoxRec){2,7,5,10};
        for(int n=0;n<2;n++) {
            BoxRec box=buf.damage.boxes[n];
            int start=box.x1*bpp/32,end=(box.x2*bpp+31)/32;
            for(int y=box.y1;y<box.y2;y++)for(int x=start;x<end;x++)expected[y*32+x]=ram[y*32+x];
        }
        shadowUpdatePacked(NULL,&buf);
        assert(!memcmp(screen,expected,sizeof screen));cases++;
    }
    empty=1;shadowUpdatePacked(NULL,&buf); /* must return, not spin */
    buf.damage.count=0;shadowUpdatePacked(NULL,&buf);
    printf("SHADOW_COPY_OK: %d banked/unaligned/multi-rectangle cases; empty bank and damage\n",cases);
}
'''
out=root/'build/tests';out.mkdir(parents=True,exist_ok=True)
c=out/'shadow-regression.c';c.write_text(prefix+body+suffix)
exe=out/'shadow-regression'
subprocess.run([os.environ.get('CC','cc'),'-std=c99','-O2',str(c),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True,timeout=10)
