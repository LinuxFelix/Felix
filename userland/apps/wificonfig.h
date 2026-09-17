// WiFConfig: FLTK frontend to wpa_supplicant's local control socket. MIT.
#include <FL/Fl_Choice.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Hold_Browser.H>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <dirent.h>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace wifi {
static const char *control_dir="/run/felix-wifi/control";
static bool interface_name(const std::string& s) {
    return !s.empty() && s.size()<16 && s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-")==std::string::npos;
}
static std::vector<std::string> interfaces() {
    std::vector<std::string> names;DIR *dir=opendir("/sys/class/net");if(!dir)return names;
    while(auto *entry=readdir(dir)) {
        std::string name=entry->d_name,base="/sys/class/net/"+name;
        if(interface_name(name) && (!access((base+"/wireless").c_str(),F_OK) || !access((base+"/phy80211").c_str(),F_OK)))names.push_back(name);
    }
    closedir(dir);std::sort(names.begin(),names.end());return names;
}
static std::string hex(const std::string& s) {
    static const char chars[]="0123456789abcdef";std::string result;
    for(unsigned char c:s){result+=chars[c>>4];result+=chars[c&15];}return result;
}
static std::string quote(const std::string& s) {
    // The PSK parser takes bytes between the first and last quote literally.
    // Shell-style escaping would silently change passwords containing slashes.
    return "\""+s+"\"";
}
static std::string unescape(const std::string& s) {
    std::string result;
    for(size_t i=0;i<s.size();i++) {
        if(s[i]=='\\' && i+3<s.size() && s[i+1]=='x') {
            auto nibble=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
            int a=nibble(s[i+2]),b=nibble(s[i+3]);if(a>=0&&b>=0){result+=char(a*16+b);i+=3;continue;}
        }
        if(s[i]=='\\' && i+1<s.size()) {
            char c=s[i+1];
            if(c=='\\'||c=='"'){result+=c;i++;continue;}
            if(c=='n'||c=='r'||c=='t'||c=='e'){result+=c=='n'?'\n':c=='r'?'\r':c=='t'?'\t':char(27);i++;continue;}
        }
        result+=s[i];
    }return result;
}
static std::string display_text(std::string s) {
    for(char &c:s)if((unsigned char)c<32 || c==127)c='?';return s;
}
static std::vector<std::string> split(const std::string& s,char delimiter) {
    std::vector<std::string> out;size_t start=0,end;
    while((end=s.find(delimiter,start))!=std::string::npos){out.push_back(s.substr(start,end-start));start=end+1;}
    out.push_back(s.substr(start));return out;
}
static std::map<std::string,std::string> status_values(const std::string& s) {
    std::map<std::string,std::string> out;
    for(auto& line:split(s,'\n')){auto p=line.find('=');if(p!=std::string::npos)out[line.substr(0,p)]=line.substr(p+1);}return out;
}
static std::string request(const std::string& iface,const std::string& command) {
    if(!interface_name(iface))return "";
    mkdir("/run/felix-wifi",0700);
    char temp[]="/run/felix-wifi/client-XXXXXX";
    if(!mkdtemp(temp))return "";
    std::string local=std::string(temp)+"/sock";
    int fd=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC,0);std::string reply;
    if(fd>=0) {
        sockaddr_un from{},to{};from.sun_family=to.sun_family=AF_UNIX;
        snprintf(from.sun_path,sizeof(from.sun_path),"%s",local.c_str());
        snprintf(to.sun_path,sizeof(to.sun_path),"%s/%s",control_dir,iface.c_str());
        if(!bind(fd,(sockaddr*)&from,sizeof(from)) && !connect(fd,(sockaddr*)&to,sizeof(to))
           && send(fd,command.data(),command.size(),0)==(ssize_t)command.size()) {
            pollfd wait{fd,POLLIN,0};
            if(poll(&wait,1,500)>0) {char buffer[65536];ssize_t n=recv(fd,buffer,sizeof(buffer),0);if(n>0)reply.assign(buffer,n);}
        }
        close(fd);
    }
    unlink(local.c_str());rmdir(temp);
    while(!reply.empty() && (reply.back()=='\n'||reply.back()=='\r'))reply.pop_back();
    return reply;
}
static std::string profile_path(const std::string& iface) {
    const char *home=getenv("HOME");return std::string(home?home:"/root")+"/.felix/wifi/"+iface+".conf";
}
static std::string add_network(const std::string& iface,const std::string& name,const std::string& pass,int mode,bool hidden) {
    if(name.empty() || name.size()>32 || mode<0 || mode>2 || pass.find_first_of("\r\n")!=std::string::npos)return "";
    if((mode==1 && (pass.size()<8 || pass.size()>63)) || (mode==2 && (pass.empty() || pass.size()>63)))return "";
    std::string id=request(iface,"ADD_NETWORK");
    if(id.empty() || id.find_first_not_of("0123456789")!=std::string::npos)return "";
    auto set=[&](const char *key,const std::string& value){return request(iface,"SET_NETWORK "+id+" "+key+" "+value)=="OK";};
    bool ok=set("ssid",hex(name)) && set("scan_ssid",hidden?"1":"0");
    if(mode==0)ok=ok && set("key_mgmt","NONE");
    else if(mode==1)ok=ok && set("key_mgmt","WPA-PSK") && set("psk",quote(pass));
    else ok=ok && set("key_mgmt","SAE") && set("sae_password",hex(pass)) && set("ieee80211w","2");
    if(!ok){request(iface,"REMOVE_NETWORK "+id);return "";}
    return id;
}
static bool prepare(const std::string& iface) {
    if(!interface_name(iface))return false;
    const char *home=getenv("HOME");std::string dir=std::string(home?home:"/root")+"/.felix";
    mkdir(dir.c_str(),0700);dir+="/wifi";mkdir(dir.c_str(),0700);chmod(dir.c_str(),0700);
    mkdir("/run/felix-wifi",0700);mkdir(control_dir,0700);
    std::string path=profile_path(iface);
    int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
    if(fd>=0) {
        std::string data=std::string("ctrl_interface=")+control_dir+"\nupdate_config=1\n";
        bool ok=write(fd,data.data(),data.size())==(ssize_t)data.size();close(fd);return ok;
    }
    struct stat st{};
    return errno==EEXIST && !lstat(path.c_str(),&st) && S_ISREG(st.st_mode) && st.st_uid==geteuid() && !chmod(path.c_str(),0600);
}
static pid_t spawn(const std::vector<std::string>& args,const std::string& log) {
    pid_t pid=fork();if(pid)return pid;
    // SAVE_CONFIG replaces its file using the daemon's inherited umask.
    umask(0077);
    int fd=open(log.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd>=0){dup2(fd,1);dup2(fd,2);close(fd);}
    int input=open("/dev/null",O_RDONLY);if(input>=0){dup2(input,0);close(input);}
    std::vector<char*> av;for(const auto& arg:args)av.push_back(const_cast<char*>(arg.c_str()));av.push_back(nullptr);
    execvp(av[0],av.data());_exit(127);
}
static pid_t start(const std::string& iface) {
    return spawn({"/sbin/wpa_supplicant","-B","-D","nl80211","-i",iface,"-c",profile_path(iface),"-P","/run/felix-wifi/"+iface+".pid","-f","/run/felix-wifi/"+iface+".log"},"/run/felix-wifi/start.log");
}
static void stop_dhcp(const std::string& iface) {
    std::string path="/run/felix-wifi/"+iface+"-dhcp.pid";
    FILE *f=fopen(path.c_str(),"r");int pid=0;if(!f)return;
    int count=fscanf(f,"%d",&pid);fclose(f);if(count!=1 || pid<=1)return;
    f=fopen(("/proc/"+std::to_string(pid)+"/cmdline").c_str(),"r");if(!f)return;
    char data[2048];size_t n=fread(data,1,sizeof(data),f);fclose(f);
    auto args=split(std::string(data,n),'\0');
    if(args.empty() || (args[0]!="/sbin/udhcpc" && args[0]!="udhcpc"))return;
    for(size_t i=1;i+1<args.size();i++)if(args[i]=="-i" && args[i+1]==iface){kill(pid,SIGUSR2);kill(pid,SIGTERM);unlink(path.c_str());break;}
}
static pid_t dhcp(const std::string& iface) {
    stop_dhcp(iface);
    // After the first lease, udhcpc backgrounds itself and keeps renewing it.
    return spawn({"/sbin/udhcpc","-i",iface,"-n","-t","5","-T","3","-p","/run/felix-wifi/"+iface+"-dhcp.pid"},"/run/felix-wifi/dhcp.log");
}
static int autoconnect() {
    for(const auto& iface:interfaces()) {
        FILE *f=fopen(profile_path(iface).c_str(),"r");if(!f)continue;
        char line[512];bool saved=false;while(fgets(line,sizeof(line),f))if(strstr(line,"network={"))saved=true;fclose(f);if(!saved)continue;
        if(!prepare(iface))continue;
        if(request(iface,"PING")!="PONG") {pid_t pid=start(iface);if(pid>0)waitpid(pid,nullptr,0);}
        request(iface,"RECONNECT");
        for(int tries=0;tries<30;tries++) {
            auto values=status_values(request(iface,"STATUS"));
            if(values["wpa_state"]=="COMPLETED"){pid_t pid=dhcp(iface);if(pid>0)waitpid(pid,nullptr,0);break;}
            sleep(1);
        }
    }return 0;
}
static int self_test() {
    if(hex("a\"\\")!="61225c" || quote("a\"\\")!=std::string("\"")+"a\"\\"+"\"" || unescape("Cafe\\x20WiFi")!="Cafe WiFi")return 1;
    if(interface_name("../x") || !interface_name("wlan0") || interface_name(""))return 2;
    if(status_values("wpa_state=COMPLETED\nssid=Test")["wpa_state"]!="COMPLETED")return 3;
    if(unescape("a\\\"b\\\\c\\t")!="a\"b\\c\t")return 4;
    puts("WIFI_CODEC_TEST_OK");return 0;
}
}

#include "wifi-hardware.h"
struct WifiConfig {
    WifiHardware *hardware=nullptr;
    Fl_Window win{800,610,"Felix WiFConfig"};
    Fl_Choice *adapter,*security;
    Fl_Hold_Browser *networks;
    Fl_Input *ssid,*country;
    Fl_Secret_Input *password;
    Fl_Check_Button *hidden;
    Fl_Box *state;
    struct Network {std::string ssid,flags,id;};
    std::vector<std::string> adapters;
    std::vector<Network> rows;
    std::string iface;
    pid_t child=-1;
    enum Job {NONE,START,DHCP} job=NONE;
    int scan_ticks=0,connecting=0;
    bool saved_view=false;
    bool save_failed=false;
    void message(const std::string& value){state->copy_label(value.c_str());}
    bool ready() {
        if(iface.empty()){message("No Wi-Fi adapter found. Connect a supported adapter, then refresh.");return false;}
        if(child>0){message("Please wait for the current operation.");return false;}
        if(wifi::request(iface,"PING")!="PONG"){message("Click Enable adapter to start wpa_supplicant.");return false;}
        return true;
    }
    void refresh() {
        if(child>0 || connecting){message("Wait for the current operation before refreshing adapters.");return;}
        saved_view=false;scan_ticks=0;
        adapters=wifi::interfaces();adapter->clear();rows.clear();networks->clear();iface.clear();
        for(const auto& name:adapters)adapter->add(name.c_str());
        if(!adapters.empty()){adapter->value(0);iface=adapters[0];message("Select Enable adapter, then Scan. Saved networks are available under Saved.");}
        else message("No usable Wi-Fi interface found. Open Hardware to identify your card and check firmware.");
    }
    void enable() {
        if(iface.empty() || child>0){ready();return;}
        if(wifi::request(iface,"PING")=="PONG"){message("Adapter ready. Click Scan or Saved.");return;}
        if(!wifi::prepare(iface)){message("Cannot create a private Wi-Fi configuration file.");return;}
        // This affects Wi-Fi radio blocks only; hardware switches remain authoritative.
        execute({"rfkill","unblock","wifi"});
        child=wifi::start(iface);job=START;
        message(child>0?"Starting wpa_supplicant...":"Could not start wpa_supplicant.");
    }
    void scan() {
        if(!ready())return;
        if(wifi::request(iface,"SCAN")!="OK"){message("Scan failed. Check the radio switch, driver and firmware.");return;}
        scan_ticks=4;message("Scanning for Wi-Fi networks...");
    }
    void results(bool saved) {
        if(!ready())return;
        auto reply=wifi::request(iface,saved?"LIST_NETWORKS":"SCAN_RESULTS");
        if(reply.empty() || reply.find("FAIL")==0){message("Could not read network list.");return;}
        rows.clear();networks->clear();saved_view=saved;
        auto lines=wifi::split(reply,'\n');
        for(size_t i=1;i<lines.size();i++) {
            auto fields=wifi::split(lines[i],'\t');if(fields.size()<(saved?4u:5u))continue;
            Network row{wifi::unescape(fields[saved?1:4]),fields[saved?3:3],saved?fields[0]:""};
            rows.push_back(row);
            std::string label=wifi::display_text(row.ssid.empty()?"(hidden network)":row.ssid)+"    "+(saved?row.flags:fields[2]+" dBm  "+row.flags);
            networks->add(label.c_str());
        }
        message(saved?"Select a saved network and Connect, or Forget to remove it.":"Select a network, enter its password and Connect. Hidden networks can be entered manually.");
    }
    void selected() {
        int index=networks->value()-1;if(index<0 || index>=(int)rows.size())return;
        auto& row=rows[index];ssid->value(row.ssid.c_str());password->value("");
        if(!saved_view)security->value(row.flags.find("SAE")!=std::string::npos?2:row.flags.find("WPA")!=std::string::npos?1:0);
        if(!saved_view && (row.flags.find("EAP")!=std::string::npos || row.flags.find("WEP")!=std::string::npos))message("This network uses enterprise authentication or WEP, which this app does not configure.");
    }
    void connect_network() {
        if(!ready())return;
        if(connecting){message("Disconnect or wait before starting another connection.");return;}
        scan_ticks=0;save_failed=false;
        if(saved_view) {
            int i=networks->value()-1;if(i<0 || i>=(int)rows.size()){message("Select a saved network first.");return;}
            if(wifi::request(iface,"SELECT_NETWORK "+rows[i].id)!="OK"){message("Could not select saved network.");return;}
        } else {
            std::string name=ssid->value(),pass=password->value(),region=country->value();
            int selected=networks->value()-1;
            if(selected>=0 && selected<(int)rows.size() && rows[selected].ssid==name && (rows[selected].flags.find("EAP")!=std::string::npos || rows[selected].flags.find("WEP")!=std::string::npos)){message("Enterprise authentication and WEP are not supported by this app.");return;}
            if(name.empty() || name.size()>32){message("SSID must contain 1 to 32 bytes.");return;}
            if(name.find_first_of("\r\n")!=std::string::npos || pass.find_first_of("\r\n")!=std::string::npos){message("Network name and password cannot contain line breaks.");return;}
            int mode=security->value();
            if(mode==1 && (pass.size()<8 || pass.size()>63)){message("WPA/WPA2 Personal needs an 8 to 63 character passphrase.");return;}
            if(mode==2 && (pass.empty() || pass.size()>63)){message("WPA3 Personal needs a 1 to 63 byte password.");return;}
            if(!region.empty()) {
                if(region.size()!=2 || region.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ")!=std::string::npos){message("Country must be two uppercase letters, such as LT.");return;}
                if(wifi::request(iface,"SET country "+region)!="OK"){message("Could not apply the selected country.");return;}
            }
            std::string id=wifi::add_network(iface,name,pass,mode,hidden->value());
            password->value("");
            if(id.empty()){message("wpa_supplicant rejected the network configuration.");return;}
            if(wifi::request(iface,"SELECT_NETWORK "+id)!="OK") {
                wifi::request(iface,"REMOVE_NETWORK "+id);message("wpa_supplicant rejected the network configuration.");return;
            }
            save_failed=wifi::request(iface,"SAVE_CONFIG")!="OK";
        }
        wifi::request(iface,"RECONNECT");connecting=45;message("Connecting... authentication can take several seconds.");
    }
    void forget() {
        if(connecting){message("Disconnect or wait before forgetting a profile.");return;}
        if(!ready() || !saved_view){message("Open Saved and select the profile to forget.");return;}
        int i=networks->value()-1;if(i<0 || i>=(int)rows.size())return;
        if(wifi::request(iface,"REMOVE_NETWORK "+rows[i].id)!="OK" || wifi::request(iface,"SAVE_CONFIG")!="OK"){message("Could not remove the saved profile.");return;}
        results(true);
    }
    static void tick(void *p) {
        auto*s=(WifiConfig*)p;
        if(s->child>0) {
            int status=0;pid_t done=waitpid(s->child,&status,WNOHANG);
            if(done==s->child) {
                s->child=-1;
                if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)s->message(s->job==START?"Adapter startup failed. Check /run/felix-wifi/start.log and the radio switch.":"Wi-Fi associated, but DHCP failed. Check /run/felix-wifi/dhcp.log.");
                else s->message(s->job==START?"Adapter enabled. Click Scan or Saved.":s->save_failed?"Connected, but saving the profile failed. It will not survive a restart.":"Connected: authentication and DHCP completed.");
                s->job=NONE;
            }
        }
        if(s->scan_ticks>0 && --s->scan_ticks==0)s->results(false);
        if(s->connecting>0) {
            auto values=wifi::status_values(wifi::request(s->iface,"STATUS"));
            if(values["wpa_state"]=="COMPLETED") {
                s->connecting=0;s->child=wifi::dhcp(s->iface);s->job=DHCP;s->message("Authenticated. Requesting an IP address...");
            } else if(--s->connecting==0){wifi::request(s->iface,"DISCONNECT");s->message("Connection timed out. Check password, security type and signal, then retry.");}
            else s->message("Connecting: "+values["wpa_state"]);
        }
        Fl::repeat_timeout(1,tick,s);
    }
    WifiConfig() {
        win.begin();adapter=new Fl_Choice(90,15,200,30,"Adapter");
        auto *refresh_btn=new Fl_Button(300,15,100,30,"Refresh");refresh_btn->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->refresh();},this);
        auto *enable_btn=new Fl_Button(410,15,140,30,"Enable adapter");enable_btn->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->enable();},this);
        auto *scan_btn=new Fl_Button(560,15,100,30,"Scan");scan_btn->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->scan();},this);
        auto *saved=new Fl_Button(670,15,110,30,"Saved");saved->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->results(true);},this);
        adapter->callback([](Fl_Widget*,void*p){auto*s=(WifiConfig*)p;if(s->child>0||s->connecting){s->adapter->value(std::find(s->adapters.begin(),s->adapters.end(),s->iface)-s->adapters.begin());s->message("Finish or disconnect the current operation before changing adapters.");return;}int i=s->adapter->value();if(i>=0&&i<(int)s->adapters.size()){s->iface=s->adapters[i];s->rows.clear();s->networks->clear();s->saved_view=false;s->scan_ticks=0;}},this);
        networks=new Fl_Hold_Browser(20,60,760,205);networks->format_char(0);networks->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->selected();},this);
        ssid=new Fl_Input(120,280,660,30,"Network name");
        ssid->when(FL_WHEN_CHANGED);ssid->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->saved_view=false;},this);
        security=new Fl_Choice(120,320,260,30,"Security");security->add("Open|WPA / WPA2 Personal|WPA3 Personal");security->value(1);
        password=new Fl_Secret_Input(120,360,660,30,"Password");
        password->when(FL_WHEN_CHANGED);password->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->saved_view=false;},this);
        country=new Fl_Input(565,320,70,30,"Country");country->maximum_size(2);
        hidden=new Fl_Check_Button(120,400,200,28,"Hidden network");
        new Fl_Box(330,400,450,28,"Connecting saves credentials in a private profile file.");
        auto *connect_btn=new Fl_Button(120,445,140,32,"Save & connect");connect_btn->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->connect_network();},this);
        auto *disconnect=new Fl_Button(275,445,130,32,"Disconnect");disconnect->callback([](Fl_Widget*,void*p){auto*s=(WifiConfig*)p;if(!s->ready())return;s->connecting=0;s->scan_ticks=0;wifi::stop_dhcp(s->iface);s->message(wifi::request(s->iface,"DISCONNECT")=="OK"?"Disconnected.":"Disconnect failed.");},this);
        auto *forget_btn=new Fl_Button(420,445,140,32,"Forget saved");forget_btn->callback([](Fl_Widget*,void*p){((WifiConfig*)p)->forget();},this);
        auto *status=new Fl_Button(575,445,100,32,"Status");status->callback([](Fl_Widget*,void*p){auto*s=(WifiConfig*)p;if(!s->ready())return;auto v=wifi::status_values(wifi::request(s->iface,"STATUS"));s->message(v["wpa_state"]+"  "+wifi::display_text(v["ssid"])+"  IP: "+v["ip_address"]);},this);
        state=new Fl_Box(20,490,760,65);state->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
        auto *hardware_btn=new Fl_Button(20,565,140,30,"Hardware");hardware_btn->callback([](Fl_Widget*,void*p){auto*s=(WifiConfig*)p;if(!s->hardware)s->hardware=new WifiHardware;else{s->hardware->win.show();}},this);
        auto *note=new Fl_Box(175,560,605,40,"Personal and open Wi-Fi networks. Profiles persist on installed systems\nand live sessions with persistent home storage.");note->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        win.end();win.callback([](Fl_Widget*,void*p){auto*s=(WifiConfig*)p;if(s->child>0||s->connecting)s->message("Wait for the operation to finish, or disconnect before closing.");else s->win.hide();},this);
        refresh();Fl::add_timeout(1,tick,this);win.show();
    }
    ~WifiConfig(){Fl::remove_timeout(tick,this);delete hardware;}
};
