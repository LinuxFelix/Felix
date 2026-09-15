#include <FL/fl_draw.H>
#include <FL/x.H>
class DesktopClock : public Fl_Window {
    pid_t settings_pid=-1;
    static void tick(void *p) {
        auto *c=static_cast<DesktopClock*>(p);
        if(c->settings_pid>0 && waitpid(c->settings_pid,nullptr,WNOHANG)==c->settings_pid)c->settings_pid=-1;
        int x=Fl::w()-c->w()-12;
        if(c->x()!=x || c->y()!=12)c->position(x,12);
        c->redraw();
        Fl::repeat_timeout(1,tick,p);
    }
    void draw() override {
        fl_color(FL_BACKGROUND_COLOR);fl_rectf(0,0,w(),h());
        aqua_button(0,0,w(),h(),fl_rgb_color(189,221,242));
        time_t now=time(nullptr);struct tm t;localtime_r(&now,&t);
        char digits[16],date[64];strftime(digits,sizeof(digits),"%H:%M:%S",&t);
        strftime(date,sizeof(date),"%a %d %b  %Z",&t);
        fl_font(FL_COURIER,23);fl_color(255,255,255);fl_draw(digits,14,30);
        fl_color(32,63,90);fl_draw(digits,13,29);
        fl_font(FL_HELVETICA,10);fl_color(62,92,117);fl_draw(date,9,49);
        // Expose the displayed digits for nonvisual ticking/format checks.
        XChangeProperty(fl_display,fl_xid(this),XInternAtom(fl_display,"_FELIX_CLOCK_TEXT",False),
            XA_STRING,8,PropModeReplace,(const unsigned char*)digits,strlen(digits));
    }
public:
    int handle(int event) override {
        if(event==FL_PUSH && Fl::event_button()==FL_LEFT_MOUSE) {
            if(settings_pid>0)return 1;
            settings_pid=fork();
            if(settings_pid==0){execlp("felix-apps","felix-apps","settings",(char*)nullptr);_exit(127);}
            return 1;
        }
        return Fl_Window::handle(event);
    }
    DesktopClock():Fl_Window(Fl::w()-170,12,158,58,"Felix Clock") {
        tooltip("Open Settings to adjust date and time");
        set_override();end();show();
        XStoreName(fl_display,fl_xid(this),"Felix Clock");
        Fl::add_timeout(1,tick,this);
    }
    ~DesktopClock(){Fl::remove_timeout(tick,this);}
};
