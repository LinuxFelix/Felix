/* Integration driver for TinyX's core input API; does not require XKB. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xlibint.h>
#include <X11/extensions/xtestproto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static Display *d;
static Window find(Window w,const char *name) {
    char *title=NULL;Window root,parent,*children=NULL,found=0;unsigned n=0,i;
    if(XFetchName(d,w,&title) && title) {int match=strstr(title,name)!=NULL;XFree(title);if(match)return w;}
    if(XQueryTree(d,w,&root,&parent,&children,&n)) {
        for(i=0;i<n && !found;i++)found=find(children[i],name);
        if(children)XFree(children);
    }
    return found;
}
static void fake(int type,int detail) {
    int op,event,error;xXTestFakeInputReq *req;
    if(!XQueryExtension(d,"XTEST",&op,&event,&error))exit(3);
    req=(xXTestFakeInputReq*)_XGetRequest(d,op,sizeof(*req));
    memset((char*)req+4,0,sizeof(*req)-4);
    req->xtReqType=X_XTestFakeInput;req->type=type;req->detail=detail;
    req->time=CurrentTime;req->root=None;req->rootX=0;req->rootY=0;
    XSync(d,False);usleep(15000);
}
static void key(const char *name,int ctrl) {
    KeySym sym=XStringToKeysym(name);KeyCode code=XKeysymToKeycode(d,sym);
    if(ctrl)fake(KeyPress,XKeysymToKeycode(d,XK_Control_L));
    fake(KeyPress,code);fake(KeyRelease,code);
    if(ctrl)fake(KeyRelease,XKeysymToKeycode(d,XK_Control_L));
}
int main(int argc,char **argv) {
    Window w;int i;
    d=XOpenDisplay(NULL);if(!d || argc<3)return 1;
    w=find(DefaultRootWindow(d),argv[2]);if(!w){fputs("window not found\n",stderr);return 2;}
    if(!strcmp(argv[1],"settings")) {
        int num,den,threshold,timeout,interval,blanking,exposures;XKeyboardState k;
        XGetPointerControl(d,&num,&den,&threshold);XGetScreenSaver(d,&timeout,&interval,&blanking,&exposures);XGetKeyboardControl(d,&k);
        printf("mouse=%d/%d saver=%d bell=%d repeat=%d\n",num,den,timeout,k.bell_percent,k.global_auto_repeat);
    } else if(!strcmp(argv[1],"geometry")) {
        XWindowAttributes a;Window child;int x,y;XGetWindowAttributes(d,w,&a);XTranslateCoordinates(d,w,DefaultRootWindow(d),0,0,&x,&y,&child);
        printf("%lu %d %d %d %d\n",w,x,y,a.width,a.height);
    } else if(!strcmp(argv[1],"move"))XMoveWindow(d,w,atoi(argv[3]),atoi(argv[4]));
    else if(!strcmp(argv[1],"focus"))XSetInputFocus(d,w,RevertToParent,CurrentTime);
    else if(!strcmp(argv[1],"click")) {
        XWarpPointer(d,None,w,0,0,0,0,atoi(argv[3]),atoi(argv[4]));XSync(d,False);usleep(100000);fake(ButtonPress,1);fake(ButtonRelease,1);
    } else if(!strcmp(argv[1],"key"))key(argv[3],argc>4 && !strcmp(argv[4],"ctrl"));
    else if(!strcmp(argv[1],"type")) {
        for(i=0;argv[3][i];i++) {
            unsigned char c=argv[3][i];KeySym sym=c=='\n'?XK_Return:c;KeyCode code=XKeysymToKeycode(d,sym);
            int shift=XKeycodeToKeysym(d,code,0)!=sym;
            if(shift)fake(KeyPress,XKeysymToKeycode(d,XK_Shift_L));
            fake(KeyPress,code);fake(KeyRelease,code);
            if(shift)fake(KeyRelease,XKeysymToKeycode(d,XK_Shift_L));
        }
    }
    XSync(d,False);XCloseDisplay(d);return 0;
}
