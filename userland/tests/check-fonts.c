/* Nonvisual check of the actual font files and TinyX's Xft rendering path. */
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    const char *families[]={"DejaVu Sans","DejaVu Sans Mono"};
    Display *d=XOpenDisplay(NULL);
    if(!d)return 1;
    int screen=DefaultScreen(d);
    Pixmap pix=XCreatePixmap(d,RootWindow(d,screen),320,48,DefaultDepth(d,screen));
    XftDraw *draw=XftDrawCreate(d,pix,DefaultVisual(d,screen),DefaultColormap(d,screen));
    XftColor color;
    if(!draw || !XftColorAllocName(d,DefaultVisual(d,screen),DefaultColormap(d,screen),"white",&color))return 2;
    for(int i=0;i<2;i++){
        XftFont *font=XftFontOpenName(d,screen,families[i]);
        FcChar8 *family=NULL,*file=NULL;
        if(!font || FcPatternGetString(font->pattern,FC_FAMILY,0,&family)!=FcResultMatch || strcmp((char*)family,families[i]))return 3;
        if(FcPatternGetString(font->pattern,FC_FILE,0,&file)!=FcResultMatch || !strstr((char*)file,"DejaVu"))return 4;
        if(!XftCharExists(d,font,0x105) || !XftCharExists(d,font,0x17e))return 5;
        XftDrawStringUtf8(draw,&color,font,2,20,(const FcChar8*)"Felix abc 123",13);
        XSync(d,False);
        printf("FONT_OK: %s (%s)\n",family,file);
        XftFontClose(d,font);
    }
    XftColorFree(d,DefaultVisual(d,screen),DefaultColormap(d,screen),&color);
    XftDrawDestroy(draw);XFreePixmap(d,pix);XCloseDisplay(d);
    return 0;
}
