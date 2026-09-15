/* Inspect window geometry and root-pixmap metadata only; no pixel capture. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static Window named(Display*d,Window w,const char*target) {
    char *name=NULL;
    if(XFetchName(d,w,&name)&&name){int match=!strcmp(name,target);XFree(name);if(match)return w;}
    Window root,parent,*children=NULL,found=0;unsigned n=0;
    if(XQueryTree(d,w,&root,&parent,&children,&n))for(unsigned i=0;i<n&&!found;i++)found=named(d,children[i],target);
    if(children)XFree(children);return found;
}
int main(void) {
    Display*d=XOpenDisplay(NULL);if(!d)return 1;
    Window root=DefaultRootWindow(d),dock=named(d,root,"wbar"),terminal=named(d,root,"Terminal");
    if(!dock||!terminal){fprintf(stderr,"Missing wbar or terminal window\n");return 2;}
    XWindowAttributes a;XGetWindowAttributes(d,dock,&a);int x,y;Window child;
    XTranslateCoordinates(d,dock,root,0,0,&x,&y,&child);
    int sw=DisplayWidth(d,DefaultScreen(d)),sh=DisplayHeight(d,DefaultScreen(d));
    printf("Wbar geometry: %dx%d%+d%+d on %dx%d\n",a.width,a.height,x,y,sw,sh);
    if(x<0||x>100||abs(y+a.height/2-sh/2)>20||a.height<=a.width)return 3;
    Atom type;int format;unsigned long n,after;unsigned char *data=NULL;
    if(XGetWindowProperty(d,root,XInternAtom(d,"_XROOTPMAP_ID",False),0,1,False,XA_PIXMAP,&type,&format,&n,&after,&data)!=Success||!data||type!=XA_PIXMAP||format!=32||n!=1)return 4;
    Pixmap pix=*(Pixmap*)data;XFree(data);unsigned w,h,border,depth;
    if(!XGetGeometry(d,pix,&root,&x,&y,&w,&h,&border,&depth)||w!=(unsigned)sw||h!=(unsigned)sh)return 5;
    printf("ROOT_PIXMAP_OK: %ux%u depth %u\n",w,h,depth);
    Window clock=named(d,root,"Felix Clock");
    if(!clock)return 6;
    XGetWindowAttributes(d,clock,&a);
    if(a.x!=sw-a.width-12||a.y!=12||!a.override_redirect)return 7;
    char previous[16]={0};
    for(int i=0;i<2;i++) {
        if(XGetWindowProperty(d,clock,XInternAtom(d,"_FELIX_CLOCK_TEXT",False),0,16,False,XA_STRING,&type,&format,&n,&after,&data)!=Success||!data||n!=8)return 8;
        if(data[2]!=':'||data[5]!=':')return 9;
        if(i && !memcmp(previous,data,8))return 10;
        memcpy(previous,data,8);XFree(data);
        if(!i)sleep(2);
    }
    puts("CLOCK_GEOMETRY_AND_TICK_OK");
    XMoveWindow(d,terminal,220,160);XSync(d,False);
    XCloseDisplay(d);puts("THEME_METADATA_OK");return 0;
}
