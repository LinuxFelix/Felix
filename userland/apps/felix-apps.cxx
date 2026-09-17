// Felix desktop applications. SPDX-License-Identifier: MIT
// One executable shares the statically linked FLTK toolkit across all apps.
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <dirent.h>
#include "aqua.h"

static int execute(const std::vector<std::string>& args) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (!pid) {
        std::vector<char*> av;
        for (const auto& arg: args) av.push_back(const_cast<char*>(arg.c_str()));
        av.push_back(nullptr); execvp(av[0], av.data()); _exit(127);
    }
    int status;
    while (waitpid(pid,&status,0) < 0) if (errno != EINTR) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

struct Notepad {
    Fl_Window win{760,540,"Felix Notepad"};
    Fl_Text_Buffer text;
    Fl_Text_Editor editor{10,50,740,450};
    Fl_Box status{10,505,740,25};
    std::string path,search_term;
    bool dirty=false, loading=false;
    std::string documents() const {
        const char *home=getenv("HOME");
        return std::string(home?home:"/root")+"/Documents/";
    }
    void find(bool again=false) {
        if(!again || search_term.empty()) {
            const char *answer=fl_input("Find text (search wraps to the beginning):",search_term.c_str());
            if(!answer || !*answer)return;
            search_term=answer;
        }
        const std::string& needle=search_term;int position=0;
        if(text.search_forward(editor.insert_position(),needle.c_str(),&position)
           || text.search_forward(0,needle.c_str(),&position)) {
            text.select(position,position+(int)needle.size());
            editor.insert_position(position+(int)needle.size());
            editor.show_insert_position();editor.take_focus();
        } else fl_message("Text not found.");
    }
    static void modified(int,int inserted,int deleted,int,const char*,void *p) {
        auto *self=static_cast<Notepad*>(p);
        if (!self->loading && (inserted || deleted)) { self->dirty=true; self->update(); }
    }
    void update() {
        std::string label=(dirty ? "* " : "")+(path.empty() ? std::string("Untitled") : path)+" - Felix Notepad";
        win.copy_label(label.c_str());
        std::string info=(dirty?"Unsaved changes":path.empty()?"New document":"Saved")+std::string("  |  ")+std::to_string(text.length())+" bytes  |  "+(path.empty()?"Untitled":path);
        status.copy_label(info.c_str());
    }
    bool save(bool choose=false) {
        std::string target=path;
        if (choose || target.empty()) {
            const char *selected=fl_file_chooser("Save text document","*.txt",target.empty()?(documents()+"Untitled.txt").c_str():target.c_str());
            if (!selected) return false;
            target=selected;
            if (target != path && access(target.c_str(),F_OK)==0
                && fl_choice("Replace %s?","Cancel","Replace",nullptr,target.c_str())!=1) return false;
        }
        // Save beside the destination and rename atomically; failed writes
        // never truncate the user's existing document.
        std::string temp=target+".felix-XXXXXX";
        std::vector<char> name(temp.begin(),temp.end()); name.push_back(0);
        int fd=mkstemp(name.data());
        if(fd<0) { fl_alert("Cannot save: %s",strerror(errno)); return false; }
        char *value=text.text(); size_t left=strlen(value), offset=0;
        bool ok=true;
        while(left) {
            ssize_t n=write(fd,value+offset,left);
            if(n<0 && errno==EINTR) continue;
            if(n<=0) {ok=false;break;}
            offset+=n;left-=n;
        }
        free(value);
        if(fsync(fd)) ok=false;
        if(close(fd)) ok=false;
        if(ok && rename(name.data(),target.c_str())) ok=false;
        if(!ok) { unlink(name.data());fl_alert("Save failed: %s",strerror(errno));return false; }
        path=target;dirty=false;update();return true;
    }
    bool discard() {
        if(!dirty) return true;
        int answer=fl_choice("Save changes to this document?","Cancel","Save","Discard");
        return answer==2 || (answer==1 && save());
    }
    void open() {
        if(!discard()) return;
        const char *file=fl_file_chooser("Open text document","*",documents().c_str());
        if(!file) return;
        std::string selected=file;
        struct stat st;
        if(stat(selected.c_str(),&st) || !S_ISREG(st.st_mode) || st.st_size>16*1024*1024) {
            fl_alert("Choose a regular text file no larger than 16 MB.");return;
        }
        Fl_Text_Buffer incoming;
        if(incoming.loadfile(selected.c_str())) {fl_alert("Could not read this file.");return;}
        char *value=incoming.text();loading=true;text.text(value);loading=false;free(value);
        path=selected;dirty=false;update();
    }
    Notepad() {
        win.begin();
        auto *fresh=new Fl_Button(10,10,80,30,"New");
        auto *load=new Fl_Button(100,10,80,30,"Open");
        auto *savebutton=new Fl_Button(190,10,80,30,"Save");
        auto *saveas=new Fl_Button(280,10,90,30,"Save As");
        auto *findbutton=new Fl_Button(380,10,80,30,"Find");
        findbutton->shortcut(FL_CTRL+'f');
        findbutton->callback([](Fl_Widget*,void*p){((Notepad*)p)->find();},this);
        auto *wrap=new Fl_Check_Button(480,10,150,30,"Word wrap");
        auto *next=new Fl_Button(640,10,100,30,"Find next");next->shortcut(FL_F+3);next->tooltip("Find the next match (F3)");
        next->callback([](Fl_Widget*,void*p){((Notepad*)p)->find(true);},this);
        wrap->value(1);
        wrap->callback([](Fl_Widget*w,void*p){((Notepad*)p)->editor.wrap_mode(((Fl_Check_Button*)w)->value()?Fl_Text_Display::WRAP_AT_BOUNDS:Fl_Text_Display::WRAP_NONE,0);},this);
        fresh->callback([](Fl_Widget*,void *p){auto*s=(Notepad*)p;if(s->discard()){s->loading=true;s->text.text("");s->loading=false;s->path.clear();s->dirty=false;s->update();}},this);
        load->callback([](Fl_Widget*,void*p){((Notepad*)p)->open();},this);
        savebutton->callback([](Fl_Widget*,void*p){((Notepad*)p)->save();},this);
        saveas->callback([](Fl_Widget*,void*p){((Notepad*)p)->save(true);},this);
        saveas->shortcut(FL_CTRL|FL_SHIFT|'s');
        savebutton->tooltip("Save document (Ctrl+S)");load->tooltip("Open document (Ctrl+O)");
        savebutton->shortcut(FL_CTRL+'s');load->shortcut(FL_CTRL+'o');fresh->shortcut(FL_CTRL+'n');
        editor.buffer(&text);editor.textfont(FL_COURIER);editor.textsize(14);
        editor.wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS,0);
        win.size_range(760,360);
        text.add_modify_callback(modified,this);status.align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        win.resizable(editor);win.end();
        win.callback([](Fl_Widget*,void*p){auto*s=(Notepad*)p;if(s->discard())s->win.hide();},this);
        update();win.show();
    }
};

static std::string settings_path() {
    const char *home=getenv("HOME");return std::string(home?home:"/root")+"/.felix/settings";
}
static std::map<std::string,std::string> read_settings() {
    std::map<std::string,std::string> result;
    FILE *file=fopen(settings_path().c_str(),"r");
    if(!file)return result;
    char line[4096];
    while(fgets(line,sizeof(line),file)) {
        char *eq=strchr(line,'=');if(!eq)continue;*eq++=0;
        eq[strcspn(eq,"\r\n")]=0;result[line]=eq;
    }
    fclose(file);
    for(const char *name:{"wallpaper","midnight","silver"}) {
        std::string old=std::string("/usr/share/felix/theme/")+name+".ppm";
        auto selected=result.find("wallpaper");
        if(selected!=result.end() && selected->second==old)selected->second=std::string("/usr/share/felix/theme/")+name+".png";
    }
    return result;
}
static bool valid_ipv4(const char *value) {struct in_addr addr;return inet_pton(AF_INET,value,&addr)==1;}
static bool valid_interface(const char *value) {
    if(!*value || strlen(value)>15) return false;
    return strspn(value,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == strlen(value);
}
static bool valid_color(const std::string& value) {
    return value.size()==7 && value[0]=='#' && strspn(value.c_str()+1,"0123456789abcdefABCDEF")==6;
}
static bool apply_x(const std::map<std::string,std::string>& values) {
    Display *d=XOpenDisplay(nullptr);if(!d)return false;
    auto get=[&](const char *key,int fallback,int min,int max) {
        auto it=values.find(key);int n=it==values.end()?fallback:atoi(it->second.c_str());
        return n<min?min:n>max?max:n;
    };
    XChangePointerControl(d,True,True,get("mouse",10,2,30),10,0);
    unsigned long reduced=get("reduced_motion",0,0,1);
    XChangeProperty(d,DefaultRootWindow(d),XInternAtom(d,"_FELIX_REDUCED_MOTION",False),XA_CARDINAL,32,PropModeReplace,(unsigned char*)&reduced,1);
    XSetScreenSaver(d,get("saver",0,0,120)*60,60,PreferBlanking,AllowExposures);
    XKeyboardControl k{};k.auto_repeat_mode=get("repeat",1,0,1)?AutoRepeatModeOn:AutoRepeatModeOff;
    k.bell_percent=get("bell",0,0,100);
    XChangeKeyboardControl(d,KBAutoRepeatMode|KBBellPercent,&k);XSync(d,False);XCloseDisplay(d);
    auto color=values.find("color");
    std::string bg=color==values.end()?"#66A9CE":color->second;
    if(!valid_color(bg))bg="#66A9CE";
    auto wallpaper=values.find("wallpaper");
    if(wallpaper==values.end())return execute({"/usr/bin/felix-root","--image","/usr/share/felix/theme/wallpaper.png"})==0;
    if(wallpaper!=values.end() && !wallpaper->second.empty())
        return execute({"/usr/bin/felix-root","--image",wallpaper->second})==0;
    return execute({"/usr/bin/felix-root",bg})==0;
}
struct Settings {
    Fl_Window win{700,500,"Felix Settings"};
    Fl_Value_Slider *mouse;
    Fl_Input *color,*wallpaper,*iface,*address,*mask,*gateway,*dns,*datetime;
    Fl_Input *saver,*bell;
    Fl_Check_Button *repeat,*reduced_motion;
    Fl_Choice *wired;
    Fl_Box *network_state;
    void refresh_network() {
        wired->clear();DIR *dir=opendir("/sys/class/net");int selected=-1,count=0;
        if(dir){while(auto *entry=readdir(dir)) {
            std::string name=entry->d_name,base="/sys/class/net/"+name;
            if(!valid_interface(name.c_str())||name=="lo"||!access((base+"/wireless").c_str(),F_OK)||!access((base+"/phy80211").c_str(),F_OK))continue;
            wired->add(name.c_str());if(name==iface->value())selected=count;count++;
        }closedir(dir);}
        if(selected<0 && count){selected=0;iface->value(wired->text(0));}
        wired->value(selected);
        std::string name=iface->value(),base="/sys/class/net/"+name;
        if(!valid_interface(name.c_str())||access(base.c_str(),F_OK)){network_state->copy_label("No wired adapter found. Connect an Ethernet adapter and refresh.");return;}
        std::ifstream carrier(base+"/carrier");int link=0;carrier>>link;
        std::string ip;struct ifaddrs *all=nullptr;
        if(!getifaddrs(&all)){for(auto*p=all;p;p=p->ifa_next)if(p->ifa_addr && name==p->ifa_name && p->ifa_addr->sa_family==AF_INET){char text[INET_ADDRSTRLEN];if(inet_ntop(AF_INET,&((sockaddr_in*)p->ifa_addr)->sin_addr,text,sizeof(text)))ip=text;}freeifaddrs(all);}
        std::string info=name+"  |  "+(link?"Cable connected":"Cable unplugged")+"  |  "+(ip.empty()?"No IP address yet":ip);
        network_state->copy_label(info.c_str());
    }
    std::map<std::string,std::string> values=read_settings();
    static Fl_Input *input(int y,const char *label,const char *value) {
        auto *i=new Fl_Input(170,y,470,30,label);i->value(value);return i;
    }
    std::string get(const char*k,const char*fallback){auto it=values.find(k);return it==values.end()?fallback:it->second;}
    bool save() {
        auto valid_number=[](const char *s,long max) {
            if(!*s || strspn(s,"0123456789")!=strlen(s))return false;
            char *end;errno=0;long value=strtol(s,&end,10);
            return !errno && !*end && value>=0 && value<=max;
        };
        if(!valid_number(saver->value(),120)||!valid_number(bell->value(),100)) {
            fl_alert("Enter a screen saver delay from 0 to 120 minutes and a bell volume from 0 to 100.");return false;
        }
        if(!valid_color(color->value())){fl_alert("Enter a color such as #AEE1FF.");return false;}
        if(strpbrk(wallpaper->value(),"\r\n")){fl_alert("Invalid wallpaper filename.");return false;}
        values["mouse"]=std::to_string((int)(mouse->value()*10));
        values["color"]=color->value();values["wallpaper"]=wallpaper->value();
        values["saver"]=saver->value();values["bell"]=bell->value();values["repeat"]=repeat->value()?"1":"0";
        values["reduced_motion"]=reduced_motion->value()?"1":"0";
        std::string path=settings_path(),dir=path.substr(0,path.rfind('/'));
        mkdir(dir.c_str(),0700);
        FILE *f=fopen((path+".tmp").c_str(),"w");if(!f){fl_alert("Cannot save settings: %s",strerror(errno));return false;}
        bool ok=true;
        for(const auto& v:values)if(fprintf(f,"%s=%s\n",v.first.c_str(),v.second.c_str())<0)ok=false;
        if(fclose(f))ok=false;
        if(!ok || rename((path+".tmp").c_str(),path.c_str())){fl_alert("Could not save settings.");return false;}
        if(geteuid()==0 && getenv("SUDO_UID") && getenv("SUDO_GID")) {
            uid_t uid=(uid_t)strtoul(getenv("SUDO_UID"),nullptr,10);gid_t gid=(gid_t)strtoul(getenv("SUDO_GID"),nullptr,10);
            if(uid>=1000){chown(dir.c_str(),uid,gid);chown(path.c_str(),uid,gid);}
        }
        return true;
    }
    void network() {
        if(!valid_interface(iface->value()) || !valid_ipv4(address->value()) || !valid_ipv4(mask->value())
           || !valid_ipv4(gateway->value()) || !valid_ipv4(dns->value())) {fl_alert("Enter an interface name and valid IPv4 addresses.");return;}
        if(fl_choice("Apply this network configuration to the running session?","Cancel","Apply",nullptr)!=1)return;
        std::string marker=std::string("/run/felix-static-")+iface->value();
        FILE *flag=fopen(marker.c_str(),"w");if(!flag){fl_alert("Cannot update network mode.");return;}fclose(flag);
        std::string pidpath=std::string("/run/felix-dhcp-")+iface->value()+".pid";
        FILE *pidfile=fopen(pidpath.c_str(),"r");long dhcp_pid=0;
        if(pidfile){
            if(fscanf(pidfile,"%ld",&dhcp_pid)==1 && dhcp_pid>1 && dhcp_pid<=2147483647) {
                std::ifstream command("/proc/"+std::to_string(dhcp_pid)+"/cmdline");
                std::vector<std::string> args;std::string arg;
                while(std::getline(command,arg,'\0'))args.push_back(arg);
                if(!args.empty() && (args[0]=="udhcpc" || args[0]=="/sbin/udhcpc"))
                    for(size_t i=1;i+1<args.size();++i)
                        if(args[i]=="-i" && args[i+1]==iface->value()){kill((pid_t)dhcp_pid,SIGTERM);break;}
            }
            fclose(pidfile);
        }
        int code=execute({"/sbin/ifconfig",iface->value(),address->value(),"netmask",mask->value(),"up"});
        if(code){fl_alert("ifconfig failed (%d). Check the interface name.",code);return;}
        code=execute({"/sbin/ip","route","replace","default","via",gateway->value(),"dev",iface->value()});

        if(code){fl_alert("Could not change the default route.");return;}
        FILE *f=fopen("/etc/resolv.conf","w");
        if(!f){fl_alert("Cannot save DNS: %s",strerror(errno));return;}
        fprintf(f,"nameserver %s\n",dns->value());fclose(f);
        fl_message("Network and DNS applied to this running session.");
    }
    void clock() {
        struct tm tm{};char *end=strptime(datetime->value(),"%Y-%m-%d %H:%M:%S",&tm);
        if(!end || *end || tm.tm_year<100 || tm.tm_year>199){fl_alert("Use UTC time YYYY-MM-DD HH:MM:SS (2000-2099).");return;}
        struct timeval tv{timegm(&tm),0};
        if(settimeofday(&tv,nullptr))fl_alert("Cannot set the clock: %s",strerror(errno));
        else fl_message("Guest system clock updated (UTC).");
    }
    Settings() {
        win.begin();auto *tabs=new Fl_Tabs(10,10,680,425);
        auto *desktop=new Fl_Group(10,40,680,395,"Desktop");
        mouse=new Fl_Value_Slider(170,70,470,30,"Mouse speed");mouse->type(FL_HOR_NICE_SLIDER);mouse->bounds(.2,3);mouse->step(.1);mouse->value(atoi(get("mouse","10").c_str())/10.0);
        color=input(120,"Background",get("color","#66A9CE").c_str());
        wallpaper=input(170,"Wallpaper",get("wallpaper","/usr/share/felix/theme/wallpaper.png").c_str());
        auto *browse=new Fl_Button(170,210,180,30,"Choose image");
        browse->callback([](Fl_Widget*,void*p){auto*s=(Settings*)p;const char*f=fl_file_chooser("Wallpaper (PNG or PPM)","*.{png,ppm}",nullptr);if(f)s->wallpaper->value(f);},this);
        auto *design=new Fl_Choice(460,210,180,30,"Design");
        design->add("Ocean glass|Midnight glass|Silver mist");
        std::string current=wallpaper->value();
        design->value(current.find("midnight.png")!=std::string::npos?1:current.find("silver.png")!=std::string::npos?2:0);
        design->callback([](Fl_Widget*w,void*p){
            static const char *files[]={"wallpaper.png","midnight.png","silver.png"};
            int index=((Fl_Choice*)w)->value();if(index<0||index>2)return;
            std::string file=std::string("/usr/share/felix/theme/")+files[index];
            ((Settings*)p)->wallpaper->value(file.c_str());
        },this);
        auto *note=new Fl_Box(40,270,610,90,"Leave wallpaper empty to use the solid color.\nMouse speed changes guest pointer acceleration.\nDesktop settings are saved in ~/.felix/settings.");note->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);desktop->end();
        desktop->begin();reduced_motion=new Fl_Check_Button(170,370,420,28,"Reduce motion (disable window animations)");reduced_motion->value(get("reduced_motion","0")=="1");desktop->end();
        auto *networktab=new Fl_Group(10,40,680,395,"Network");
        iface=input(65,"Interface","eth0");iface->resize(170,65,230,30);
        wired=new Fl_Choice(420,65,220,30);wired->tooltip("Detected wired adapters");
        wired->callback([](Fl_Widget*,void*p){auto*s=(Settings*)p;if(s->wired->text()){s->iface->value(s->wired->text());s->refresh_network();}},this);
        address=input(110,"IPv4 address","");mask=input(155,"Netmask","255.255.255.0");gateway=input(200,"Gateway","");dns=input(245,"DNS server","");
        auto *net=new Fl_Button(170,300,200,32,"Apply network");net->callback([](Fl_Widget*,void*p){((Settings*)p)->network();},this);
        auto *wifi_button=new Fl_Button(390,300,200,32,"WiFi Configuration");wifi_button->callback([](Fl_Widget*,void*){if(fork()==0){execlp("wificonfig","wificonfig",(char*)nullptr);_exit(127);}});
        auto *dhcp=new Fl_Button(170,350,240,32,"Use automatic Ethernet (DHCP)");dhcp->callback([](Fl_Widget*,void*p){auto*s=(Settings*)p;if(!valid_interface(s->iface->value())){fl_alert("Enter a valid interface name.");return;}std::string marker=std::string("/run/felix-static-")+s->iface->value();unlink(marker.c_str());fl_message("Automatic addressing enabled. Connect the Ethernet cable; allow a few seconds.");},this);
        auto *refresh_link=new Fl_Button(430,350,210,32,"Refresh link status");refresh_link->callback([](Fl_Widget*,void*p){((Settings*)p)->refresh_network();},this);
        network_state=new Fl_Box(40,390,610,35);network_state->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);network_state->labelsize(12);refresh_network();
        networktab->end();
        auto *xtab=new Fl_Group(10,40,680,395,"X settings");
        saver=input(80,"Screen saver (min)",get("saver","0").c_str());bell=input(130,"Bell volume (0-100)",get("bell","0").c_str());
        repeat=new Fl_Check_Button(170,190,300,30,"Keyboard repeat");repeat->value(get("repeat","1")=="1");
        new Fl_Box(70,250,540,100,"Choose when the screen goes blank while idle.\nSet the delay to 0 to keep the screen on.\nKeyboard repeat applies while a key is held down.");xtab->end();
        auto *clocktab=new Fl_Group(10,40,680,395,"Date and time");
        time_t now=time(nullptr);char stamp[32];strftime(stamp,sizeof(stamp),"%Y-%m-%d %H:%M:%S",gmtime(&now));
        datetime=input(85,"UTC date and time",stamp);
        auto *timebutton=new Fl_Button(170,140,200,32,"Set date and time");timebutton->callback([](Fl_Widget*,void*p){((Settings*)p)->clock();},this);
        new Fl_Box(50,210,590,90,"Enter the date and time in UTC.\nThis adjustment applies to the current session.");clocktab->end();
        auto *system=new Fl_Group(10,40,680,395,"System");
        struct statvfs fs{};char info[256];
        const char *storage=access("/etc/felix-live",F_OK)==0
            ? "Live session: save important files to a persistent drive."
            : "Installed system: files and applications are saved on your drive.";
        if(!statvfs("/",&fs))snprintf(info,sizeof(info),"Felix 1.1\nAvailable space: %.1f MB\n%s",fs.f_bavail*(double)fs.f_frsize/1000000,storage);
        else strcpy(info,"Unable to read filesystem capacity.");
        auto *disk=new Fl_Box(30,80,630,130,info);disk->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
        auto *shutdown=new Fl_Button(170,250,200,32,"Shut down Felix");shutdown->callback([](Fl_Widget*,void*){if(fl_choice("Shut down Felix? Save your work before continuing.","Cancel","Shut down",nullptr)==1){sync();execute({"/sbin/poweroff","-f"});}});
        system->end();tabs->end();
        auto *apply=new Fl_Button(440,450,120,32,"Save and apply");apply->callback([](Fl_Widget*,void*p){auto*s=(Settings*)p;if(s->save()){if(!apply_x(s->values))fl_alert("Saved, but the wallpaper or X settings could not be applied.");}},this);
        auto *close=new Fl_Button(575,450,100,32,"Close");close->callback([](Fl_Widget*,void*p){((Settings*)p)->win.hide();},this);
        win.end();win.show();
    }
};

struct Packages {
    Fl_Window win{820,620,"Applications"};
    Fl_Input *query,*package;
    Fl_Text_Buffer output;
    Fl_Text_Display *log;
    Fl_Box *state;
    pid_t child=-1;
    int fd=-1;
    bool eof=false;
    bool reaped=false;
    int exit_status=0;
    std::string result;
    std::vector<Fl_Button*> buttons;
    void run(std::vector<std::string> command) {
        if(child>0)return;
        int pipefd[2];if(pipe(pipefd)){fl_alert("pipe: %s",strerror(errno));return;}
        output.text("");result.clear();eof=false;reaped=false;
        child=fork();
        if(child<0){close(pipefd[0]);close(pipefd[1]);fl_alert("fork: %s",strerror(errno));return;}
        if(!child){
            close(pipefd[0]);dup2(pipefd[1],1);dup2(pipefd[1],2);close(pipefd[1]);
            int nullfd=open("/dev/null",O_RDONLY);if(nullfd>=0){dup2(nullfd,0);close(nullfd);}
            setenv("TERM","dumb",1);
            std::vector<char*> args;for(auto& arg:command)args.push_back(const_cast<char*>(arg.c_str()));args.push_back(nullptr);
            execvp(args[0],args.data());perror("apk");_exit(127);
        }
        close(pipefd[1]);fd=pipefd[0];fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
        for(auto*b:buttons)b->deactivate();query->deactivate();package->deactivate();state->copy_label("Working... Results will appear above.");
        Fl::add_timeout(.1,tick,this);
    }
    static void tick(void *p) {
        auto*s=(Packages*)p;char buffer[4097];
        for(int i=0;i<16 && !s->eof;i++) {
            ssize_t n=read(s->fd,buffer,sizeof(buffer)-1);
            if(n>0){buffer[n]=0;s->output.append(buffer);if(s->output.length()>1024*1024)s->output.remove(0,s->output.length()-1024*1024);}
            else if(!n){s->eof=true;close(s->fd);s->fd=-1;}
            else if(errno!=EINTR && errno!=EAGAIN && errno!=EWOULDBLOCK){s->eof=true;close(s->fd);s->fd=-1;}
            else break;
        }
        if(!s->reaped) {
            pid_t done=waitpid(s->child,&s->exit_status,WNOHANG);
            if(done==s->child)s->reaped=true;
        }
        if(s->reaped && s->eof){
            s->child=-1;if(s->fd>=0){close(s->fd);s->fd=-1;}
            for(auto*b:s->buttons)b->activate();
            s->query->activate();s->package->activate();
            s->state->copy_label(WIFEXITED(s->exit_status)&&WEXITSTATUS(s->exit_status)==0?"Finished.":"Couldn't finish. See the details above.");
        } else Fl::repeat_timeout(.1,tick,s);
    }
    std::string selected() {
        std::string name=package->value();
        if(name.empty() || name[0]=='-' || strspn(name.c_str(),"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+_.-")!=name.size()) {
            fl_alert("Enter the exact package name shown in the search results.");return "";
        }
        return name;
    }
    Packages() {
        win.begin();
        auto *heading=new Fl_Box(24,18,500,28,"Applications");heading->labelfont(FL_HELVETICA_BOLD);heading->labelsize(20);heading->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        auto *subtitle=new Fl_Box(24,50,570,22,"Find software for your Felix desktop.");subtitle->labelcolor(fl_rgb_color(88,111,131));subtitle->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        query=new Fl_Input(24,104,630,32,"Find software");query->align(FL_ALIGN_TOP_LEFT);query->textsize(14);
        auto *search=new Fl_Button(666,104,130,32,"Search");buttons.push_back(search);
        search->callback([](Fl_Widget*,void*p){auto*s=(Packages*)p;const char*q=s->query->value();if(!*q||q[0]=='-'){fl_alert("Enter a search term.");return;}s->run({"/sbin/apk","search","-v",q});},this);
        query->when(FL_WHEN_ENTER_KEY_ALWAYS);query->callback(search->callback(),this);
        auto *refresh=new Fl_Button(646,24,150,30,"Refresh catalog");buttons.push_back(refresh);refresh->callback([](Fl_Widget*,void*p){((Packages*)p)->run({"/sbin/apk","update"});},this);
        package=new Fl_Input(24,500,412,32,"Package name");package->align(FL_ALIGN_TOP_LEFT);package->textsize(14);
        auto *preview=new Fl_Button(448,500,106,32,"Preview");buttons.push_back(preview);preview->callback([](Fl_Widget*,void*p){auto*s=(Packages*)p;auto name=s->selected();if(!name.empty())s->run({"/sbin/apk","add","--simulate",name});},this);
        auto *install=new Fl_Button(566,500,106,32,"Install");install->color(fl_rgb_color(80,164,226));install->labelfont(FL_HELVETICA_BOLD);buttons.push_back(install);install->callback([](Fl_Widget*,void*p){auto*s=(Packages*)p;auto name=s->selected();if(!name.empty()&&fl_choice("Install %s and its dependencies from the configured Alpine x86 repository?","Cancel","Install",nullptr,name.c_str())==1)s->run({"/sbin/apk","add",name});},this);
        auto *list=new Fl_Button(684,500,112,32,"Installed");buttons.push_back(list);list->callback([](Fl_Widget*,void*p){((Packages*)p)->run({"/sbin/apk","info"});},this);
        log=new Fl_Text_Display(24,176,772,284,"Results");log->align(FL_ALIGN_TOP_LEFT);log->box(FL_THIN_DOWN_BOX);log->color(FL_WHITE);log->textcolor(FL_FOREGROUND_COLOR);log->buffer(&output);log->textfont(FL_COURIER);log->textsize(13);
        output.text("Welcome to Applications\n\nRefresh the catalog to get the latest package list.\nSearch for a program, then enter its package name below.\n\nPreview shows dependencies before you install.\nChoose Installed to see software already on this system.\n");
        state=new Fl_Box(24,554,772,42,"Ready. Connect to the internet to refresh or install software.");state->labelsize(12);state->labelcolor(fl_rgb_color(88,111,131));state->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
        win.size_range(820,620);
        win.resizable(log);win.end();
        win.callback([](Fl_Widget*,void*p){auto*s=(Packages*)p;if(s->child>0)fl_alert("Wait for apk to finish before closing this window.");else s->win.hide();},this);
        win.show();
    }
};

#include "wificonfig.h"
#include "installer.h"
#include "clock.h"

int main(int argc,char **argv) {
    if(argc>=3 && !strcmp(argv[1],"wifi")) {
        if(!strcmp(argv[2],"--self-test"))return wifi::self_test();
        if(!strcmp(argv[2],"--autoconnect"))return wifi::autoconnect();
        if(!strcmp(argv[2],"--list")){for(auto& name:wifi::interfaces())puts(name.c_str());return 0;}
        if(argc==5 && !strcmp(argv[2],"--control-test")){auto result=wifi::request(argv[3],argv[4]);puts(result.c_str());return result.empty()?1:0;}
        if(argc==4 && !strcmp(argv[2],"--profile-test")){std::string mode,name,pass;std::getline(std::cin,mode);std::getline(std::cin,name);std::getline(std::cin,pass);auto id=wifi::add_network(argv[3],name,pass,atoi(mode.c_str()),true);puts(id.c_str());return id.empty()?1:0;}
    }
    if(argc==3 && !strcmp(argv[1],"settings") && !strcmp(argv[2],"--apply"))return apply_x(read_settings())?0:1;
    // Font selection can open X with Xft: keep it after the headless Wi-Fi modes.
    Fl::set_font(FL_HELVETICA,"DejaVu Sans");
    Fl::set_font(FL_HELVETICA_BOLD,"BDejaVu Sans");
    Fl::set_font(FL_HELVETICA_ITALIC,"IDejaVu Sans");
    Fl::set_font(FL_HELVETICA_BOLD_ITALIC,"PDejaVu Sans");
    Fl::set_font(FL_COURIER,"DejaVu Sans Mono");
    Fl::set_font(FL_COURIER_BOLD,"BDejaVu Sans Mono");
    Fl::set_font(FL_COURIER_ITALIC,"IDejaVu Sans Mono");
    Fl::set_font(FL_COURIER_BOLD_ITALIC,"PDejaVu Sans Mono");
    apply_aqua_theme();
    const char *app=argc>1?argv[1]:"notepad";
    if(!strcmp(app,"clock")){DesktopClock ui;return Fl::run();}
    if(!strcmp(app,"askpass")){const char*p=fl_password("Administrator password:");if(!p)return 1;puts(p);return 0;}
    if(geteuid()!=0 && (!strcmp(app,"packages")||!strcmp(app,"settings")||!strcmp(app,"wifi"))) {
        setenv("SUDO_ASKPASS","/usr/bin/felix-askpass",1);
        std::vector<std::string> args={"sudo","-A","/usr/bin/env",std::string("HOME=")+(getenv("HOME")?getenv("HOME"):"/root"),std::string("DISPLAY=")+(getenv("DISPLAY")?getenv("DISPLAY"):":0"),std::string("XAUTHORITY=")+(getenv("XAUTHORITY")?getenv("XAUTHORITY"):""),"/usr/bin/felix-apps",app};
        if(!strcmp(app,"wifi") && argc==3 && !strcmp(argv[2],"--hardware"))args.push_back("--hardware");
        std::vector<char*> av;for(auto&a:args)av.push_back(const_cast<char*>(a.c_str()));av.push_back(nullptr);execvp(av[0],av.data());fl_alert("Cannot start sudo.");return 1;
    }
    if(!strcmp(app,"installer")){if(geteuid()!=0||access("/etc/felix-live",F_OK)){fl_alert("Boot the live Felix ISO to install.");return 1;}Installer ui;return Fl::run();}
    if(!strcmp(app,"run")) {
        const char *command=fl_input("Run a program or shell command:","");
        if(!command || !*command)return 0;
        pid_t child=fork();
        if(child<0){fl_alert("Could not launch: %s",strerror(errno));return 1;}
        if(!child) {
            setsid();int fd=open("/tmp/felix-launch.log",O_WRONLY|O_CREAT|O_TRUNC,0600);
            if(fd>=0){dup2(fd,1);dup2(fd,2);close(fd);}
            execl("/bin/sh","sh","-c",command,(char*)nullptr);_exit(127);
        }
        return 0;
    }
    if(!strcmp(app,"notepad")){Notepad ui;return Fl::run();}
    if(!strcmp(app,"settings")){Settings ui;return Fl::run();}
    if(!strcmp(app,"packages")){Packages ui;return Fl::run();}
    if(!strcmp(app,"wifi")){
        if(argc==3 && !strcmp(argv[2],"--hardware")){WifiHardware ui;return Fl::run();}
        WifiConfig ui;return Fl::run();
    }
    fprintf(stderr,"usage: felix-apps notepad|settings|packages|wifi|run\n");return 1;
}
