/* Observe real flwm positions and reduced-motion behavior, without screenshots. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
static Window parent(Display*d,Window w){Window root,p,*children=0;unsigned n;if(!XQueryTree(d,w,&root,&p,&children,&n))return None;if(children)XFree(children);return p;}
int main(void){
 Display*d=XOpenDisplay(0);if(!d)return 1;Window root=DefaultRootWindow(d);
 Atom reduced=XInternAtom(d,"_FELIX_REDUCED_MOTION",False);
 for(unsigned long off=0;off<2;off++){
  XChangeProperty(d,root,reduced,XA_CARDINAL,32,PropModeReplace,(unsigned char*)&off,1);
  Window w=XCreateSimpleWindow(d,root,120,200,280,120,0,0,0xeeeeee);
  XStoreName(d,w,"Felix animation test");XMapWindow(d,w);XFlush(d);
  int first=0,last=0,seen=0,changes=0;
  for(int i=0;i<60;i++){
   Window frame=parent(d,w);XWindowAttributes a;
   if(frame && frame!=root && XGetWindowAttributes(d,frame,&a) && a.map_state==IsViewable){
    if(!seen){first=last=a.y;seen=1;}else if(a.y!=last){changes++;last=a.y;}
   }usleep(5000);
  }
  if(!seen || (!off && (changes<1 || first<=last)) || (off && changes)){fprintf(stderr,"motion mismatch reduced=%lu first=%d last=%d changes=%d\n",off,first,last,changes);return 2;}
  XDestroyWindow(d,w);XSync(d,False);usleep(250000);
 }
 unsigned long off=0;XChangeProperty(d,root,reduced,XA_CARDINAL,32,PropModeReplace,(unsigned char*)&off,1);
 /* Destroy during opening: a dangling timeout must never reach the client. */
 for(int i=0;i<25;i++){Window w=XCreateSimpleWindow(d,root,10,10,100,80,0,0,0);XMapWindow(d,w);XFlush(d);usleep(20000);XDestroyWindow(d,w);XFlush(d);}
 usleep(250000);XCloseDisplay(d);puts("MOTION_OK: upward opening, reduced motion, rapid destruction");return 0;
}
