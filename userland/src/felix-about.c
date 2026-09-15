/* A small real X client makes titlebars and the menu testable without xterm. */
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/keysym.h>
#include <string.h>
int main(void) {
    Display *d = XOpenDisplay(0);
    XEvent e;
    const char *lines[] = {"Felix 1.0", "Small system. Your space.", "Right-click the desktop to open applications.", "Press Escape to close."};
    int i, s;
    Window w;
    XftDraw *draw;
    XftFont *font;
    XftColor color;
    Atom close;
    if (!d) return 1;
    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d,s), 100, 100, 360, 150, 0,
                           BlackPixel(d,s), WhitePixel(d,s));
    XStoreName(d,w,"About Felix");
    close = XInternAtom(d,"WM_DELETE_WINDOW",False);
    XSetWMProtocols(d,w,&close,1);
    XSelectInput(d,w,ExposureMask|KeyPressMask|StructureNotifyMask);
    font = XftFontOpenName(d,s,"DejaVu Sans:size=10");
    if (!font) return 1;
    draw = XftDrawCreate(d,w,DefaultVisual(d,s),DefaultColormap(d,s));
    XftColorAllocName(d,DefaultVisual(d,s),DefaultColormap(d,s),"black",&color);
    XMapWindow(d,w);
    for (;;) {
        XNextEvent(d,&e);
        if (e.type == Expose) for (i=0;i<4;i++) XftDrawStringUtf8(draw,&color,font,18,30+i*28,(const FcChar8*)lines[i],strlen(lines[i]));
        if ((e.type == ClientMessage && (Atom)e.xclient.data.l[0] == close)
            || (e.type == KeyPress && XLookupKeysym(&e.xkey,0) == XK_Escape)) break;
    }
    XftDrawDestroy(draw); XftFontClose(d,font);
    XftColorFree(d,DefaultVisual(d,s),DefaultColormap(d,s),&color);
    XDestroyWindow(d,w); XCloseDisplay(d); return 0;
}
