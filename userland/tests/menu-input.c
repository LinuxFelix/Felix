/* Direct XTEST events, without xdotool's newer XKB/XInput assumptions. */
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static Window popup(Display*d,Window root) {
    Window r,p,*children=0,result=0;unsigned n=0;
    if(XQueryTree(d,root,&r,&p,&children,&n))for(unsigned i=0;i<n;i++){
        XWindowAttributes a;
        if(XGetWindowAttributes(d,children[i],&a)&&a.override_redirect&&a.map_state==IsViewable&&a.width>100&&a.height>60){
            Window cr,cp,*cc=0;unsigned count=0;
            if(XQueryTree(d,children[i],&cr,&cp,&cc,&count)&&!count)result=children[i];
            if(cc)XFree(cc);
        }
    }
    if(children)XFree(children);return result;
}
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    Display*d=XOpenDisplay(0);if(!d)return 3;
    Window root=DefaultRootWindow(d);XEvent e;memset(&e,0,sizeof(e));
    if(!strcmp(argv[1],"open")) {
        XTestFakeMotionEvent(d,-1,850,650,CurrentTime);XSync(d,False);
        e.xbutton.display=d;e.xbutton.window=root;e.xbutton.root=root;
        e.xbutton.x=e.xbutton.x_root=850;e.xbutton.y=e.xbutton.y_root=650;
        e.xbutton.same_screen=True;e.xbutton.button=3;e.type=ButtonPress;
        XTestFakeButtonEvent(d,3,True,CurrentTime);XSync(d,False);
        usleep(100000);
        e.type=ButtonRelease;e.xbutton.state=Button3Mask;
        XTestFakeButtonEvent(d,3,False,CurrentTime);XSync(d,False);
        usleep(100000);
        if(!popup(d,root)){fprintf(stderr,"No popup was created\n");return 4;}
    }else if(!strcmp(argv[1],"exit")){
        Window menu=popup(d,root);if(!menu)return 5;
        XWindowAttributes a;XGetWindowAttributes(d,menu,&a);
        XTestFakeMotionEvent(d,-1,a.x+a.width/2,a.y+a.height-12,CurrentTime);XSync(d,False);
        usleep(100000);
        e.xbutton.display=d;e.xbutton.window=menu;e.xbutton.root=root;
        e.xbutton.x=a.width/2;e.xbutton.y=a.height-12;
        e.xbutton.x_root=a.x+e.xbutton.x;e.xbutton.y_root=a.y+e.xbutton.y;
        e.xbutton.same_screen=True;e.xbutton.button=1;e.type=ButtonPress;
        XTestFakeButtonEvent(d,1,True,CurrentTime);XSync(d,False);
        e.type=ButtonRelease;e.xbutton.state=Button1Mask;
        XTestFakeButtonEvent(d,1,False,CurrentTime);XSync(d,False);
    }else{
        Window menu=popup(d,root);if(!menu)return 5;
        e.xkey.display=d;e.xkey.window=menu;e.xkey.root=root;e.xkey.same_screen=True;
        e.xkey.keycode=XKeysymToKeycode(d,XStringToKeysym(argv[1]));
        XTestFakeKeyEvent(d,e.xkey.keycode,True,CurrentTime);
        XTestFakeKeyEvent(d,e.xkey.keycode,False,10);
        XSync(d,False);
    }
    XCloseDisplay(d);return 0;
}
