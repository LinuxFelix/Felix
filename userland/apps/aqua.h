#ifndef FELIX_AQUA_H
#define FELIX_AQUA_H
#include <FL/fl_draw.H>
// Reflective controls drawn with horizontal spans; no compositor or animation.
static void aqua_button(int x,int y,int w,int h,Fl_Color c) {
    if(w<4 || h<4)return;
    unsigned char r,g,b;Fl::get_color(c,r,g,b);
    for(int row=0;row<h;row++) {
        int light=row<h/2 ? 218-70*row/h : 42+48*row/h;
        fl_color((uchar)(r+(255-r)*light/255),(uchar)(g+(255-g)*light/255),(uchar)(b+(255-b)*light/255));
        int inset=(row==0||row==h-1)?5:(row==1||row==h-2)?2:1;
        if(w>2*inset)fl_xyline(x+inset,y+row,x+w-inset-1);
    }
    fl_color(139,163,183);fl_rounded_rect(x,y,w,h,6);
    fl_color(247,253,255);fl_xyline(x+5,y+1,x+w-6);
    fl_color(191,213,228);fl_xyline(x+5,y+h-2,x+w-6);
}
static void aqua_inset(int x,int y,int w,int h,Fl_Color c) {
    fl_color(c);fl_rectf(x,y,w,h);
    fl_color(164,184,201);fl_rect(x,y,w,h);
    fl_color(227,236,243);fl_xyline(x+1,y+1,x+w-2);
}
static void aqua_down(int x,int y,int w,int h,Fl_Color c) {
    if(c==FL_BACKGROUND2_COLOR||c==FL_WHITE)aqua_inset(x,y,w,h,c);
    else aqua_button(x,y,w,h,fl_darker(c));
}
static void apply_aqua_theme() {
    Fl::scheme("none");
    Fl::background(230,239,246);Fl::background2(255,255,255);
    Fl::foreground(34,52,69);Fl::set_color(FL_SELECTION_COLOR,72,153,218);
    Fl::set_boxtype(FL_UP_BOX,aqua_button,3,3,6,6);
    Fl::set_boxtype(FL_DOWN_BOX,aqua_down,3,3,6,6);
    Fl::set_boxtype(FL_THIN_DOWN_BOX,aqua_inset,2,2,4,4);
}
#endif
