/* Solid color and P6 PPM wallpaper without an image library. MIT license. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static int number(FILE *f) {
    int c,n=0,count=0;
    do { c=fgetc(f);if(c=='#'){while(c!='\n' && c!=EOF)c=fgetc(f);} } while(c!=EOF && isspace(c));
    while(c!=EOF && isdigit(c)) {if(++count>6)return -1;n=n*10+c-'0';c=fgetc(f);}
    return count && c!=EOF && isspace(c) ? n : -1;
}
static unsigned long channel(unsigned char value,unsigned long mask) {
    unsigned shift=0;if(!mask)return 0;
    while(!(mask&1)){mask>>=1;shift++;}
    return (((unsigned long)value*mask+127)/255)<<shift;
}
static void publish(Display *d,Pixmap pix) {
    Window root=DefaultRootWindow(d);
    Atom own=XInternAtom(d,"_FELIX_ROOT_PIXMAP",False),type;
    int format;unsigned long count,after;unsigned char *data=NULL;
    if(XGetWindowProperty(d,root,own,0,1,False,XA_PIXMAP,&type,&format,&count,&after,&data)==Success && data) {
        if(type==XA_PIXMAP && format==32 && count==1)XKillClient(d,*(Pixmap*)data);
        XFree(data);
    }
    const char *names[]={"_FELIX_ROOT_PIXMAP","_XROOTPMAP_ID","ESETROOT_PMAP_ID"};
    for(int i=0;i<3;i++)XChangeProperty(d,root,XInternAtom(d,names[i],False),XA_PIXMAP,32,PropModeReplace,(unsigned char*)&pix,1);
    XSetWindowBackgroundPixmap(d,root,pix);
    // Keep the pixmap valid for wbar and urxvt after this utility exits.
    XSetCloseDownMode(d,RetainPermanent);
}
static int wallpaper(Display *d,const char *path) {
    FILE *f=fopen(path,"rb");int w,h,max,s=DefaultScreen(d),x,y;
    unsigned char *pixels;XImage *img;Pixmap pix;GC gc;
    if(!f)return 1;
    if(fgetc(f)!='P' || fgetc(f)!='6' || !isspace(fgetc(f))) {fclose(f);return 1;}
    w=number(f);h=number(f);max=number(f);
    if(w<1 || h<1 || w>8192 || h>8192 || max!=255) {fclose(f);return 1;}
    pixels=malloc((size_t)w*h*3);
    if(!pixels){fclose(f);return 1;}
    if(fread(pixels,3,(size_t)w*h,f)!=(size_t)w*h){free(pixels);fclose(f);return 1;}
    fclose(f);
    img=XCreateImage(d,DefaultVisual(d,s),DefaultDepth(d,s),ZPixmap,0,NULL,DisplayWidth(d,s),DisplayHeight(d,s),32,0);
    if(!img){free(pixels);return 1;}
    img->data=calloc(img->bytes_per_line,img->height);
    if(!img->data){XDestroyImage(img);free(pixels);return 1;}
    for(y=0;y<img->height;y++)for(x=0;x<img->width;x++) {
        const unsigned char *p=pixels+((size_t)(y*h/img->height)*w+x*w/img->width)*3;
        XPutPixel(img,x,y,channel(p[0],img->red_mask)|channel(p[1],img->green_mask)|channel(p[2],img->blue_mask));
    }
    free(pixels);
    pix=XCreatePixmap(d,DefaultRootWindow(d),img->width,img->height,DefaultDepth(d,s));gc=XCreateGC(d,pix,0,NULL);
    XPutImage(d,pix,gc,img,0,0,0,0,img->width,img->height);
    publish(d,pix);
    XFreeGC(d,gc);XDestroyImage(img);return 0;
}
int main(int argc,char **argv) {
    Display *d=XOpenDisplay(NULL);XColor color;int result=1;
    if(!d)return 1;
    if(argc==3 && !strcmp(argv[1],"--window-exists")) {
        Window root,parent,*children=NULL;unsigned count=0,i;
        if(XQueryTree(d,DefaultRootWindow(d),&root,&parent,&children,&count)) {
            for(i=0;i<count;i++) {
                char *name=NULL;
                if(XFetchName(d,children[i],&name) && name) {
                    if(!strcmp(name,argv[2]))result=0;
                    XFree(name);
                }
            }
            if(children)XFree(children);
        }
        XCloseDisplay(d);return result;
    }
    if(argc==3 && !strcmp(argv[1],"--ppm"))result=wallpaper(d,argv[2]);
    else if(argc==2 && XParseColor(d,DefaultColormap(d,DefaultScreen(d)),argv[1],&color)
            && XAllocColor(d,DefaultColormap(d,DefaultScreen(d)),&color)) {
        int s=DefaultScreen(d);
        Pixmap pix=XCreatePixmap(d,DefaultRootWindow(d),DisplayWidth(d,s),DisplayHeight(d,s),DefaultDepth(d,s));
        GC gc=XCreateGC(d,pix,0,NULL);XSetForeground(d,gc,color.pixel);
        XFillRectangle(d,pix,gc,0,0,DisplayWidth(d,s),DisplayHeight(d,s));XFreeGC(d,gc);
        publish(d,pix);result=0;
    }
    if(!result)XClearWindow(d,DefaultRootWindow(d));
    else fputs("usage: felix-root '#RRGGBB' | --ppm image.ppm (P6, maxval 255)\n",stderr);
    XSync(d,False);XCloseDisplay(d);return result;
}
