// Hardware identification and optional firmware. No guessed driver installs.
#include <fstream>
#include <cctype>
struct WifiHardware {
    Fl_Window win{780,470,"Wi-Fi hardware"};
    Fl_Hold_Browser list{20,45,740,220};
    Fl_Choice vendor{160,285,360,30,"Firmware family"};
    Fl_Box state{20,375,740,70};
    Fl_Button install{540,285,220,30,"Install firmware"};
    pid_t child=-1;
    bool reloading=false;
    struct Card {std::string label;int family;std::string pci;};
    std::vector<Card> cards;
    static std::string read(const std::string& path) {
        std::ifstream f(path);std::string s;std::getline(f,s);return s;
    }
    static int family(const std::string& id) {
        if(id=="0x8086"||id=="8087")return 0;
        if(id=="0x14e4"||id=="0a5c")return 1;
        if(id=="0x168c"||id=="0cf3")return 2;
        if(id=="0x10ec"||id=="0bda")return 3;
        if(id=="0x14c3"||id=="0x1814"||id=="148f"||id=="0e8d")return 4;
        return 5;
    }
    static const char *brand(int i) {
        static const char *names[]={"Intel","Broadcom","Qualcomm / Atheros","Realtek","MediaTek / Ralink","Other"};return names[i];
    }
    void detect() {
        list.clear();cards.clear();
        for(const char *bus:{"pci","usb"}) {
            std::string base=std::string("/sys/bus/")+bus+"/devices/";
            DIR *dir=opendir(base.c_str());if(!dir)continue;
            while(auto *entry=readdir(dir)) {
                if(entry->d_name[0]=='.')continue;
                std::string path=base+entry->d_name,id,device,description;
                if(!strcmp(bus,"pci")) {
                    if(read(path+"/class").substr(0,6)!="0x0280")continue;
                    id=read(path+"/vendor");device=read(path+"/device");
                } else {
                    description=read(path+"/product");std::string lower=description;
                    std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return std::tolower(c);});
                    if(lower.find("wireless")==std::string::npos && lower.find("wlan")==std::string::npos && lower.find("802.11")==std::string::npos)continue;
                    id=read(path+"/idVendor");device=read(path+"/idProduct");
                }
                char link[512];ssize_t n=readlink((path+"/driver").c_str(),link,sizeof(link)-1);
                std::string driver;
                if(n>0){link[n]=0;driver=link;driver=driver.substr(driver.rfind('/')+1);}
                // USB drivers bind interfaces, not necessarily the device node.
                std::string status=driver.empty()?"driver/firmware not confirmed":"driver: "+driver;
                int index=family(id);
                cards.push_back({std::string(brand(index))+"  "+id+":"+device+"  "+description+"  ["+status+"]",index,!strcmp(bus,"pci")?entry->d_name:""});
                list.add(cards.back().label.c_str());
            }
            closedir(dir);
        }
        if(cards.empty())state.copy_label("No identifiable Wi-Fi card found. Connect an adapter and reopen this window.\nIf you know its chipset, choose a firmware family. Firmware cannot add an unsupported kernel driver.");
        else {list.value(1);vendor.value(cards[0].family);state.copy_label("Select your card. A missing interface can mean missing firmware or an unsupported driver.\nThe IDs above identify the hardware; choosing a vendor does not change that identity.");}
    }
    static void tick(void *p) {
        auto *s=(WifiHardware*)p;int status=0;
        if(waitpid(s->child,&status,WNOHANG)==s->child) {
            s->child=-1;s->install.activate();s->vendor.activate();
            if(s->reloading) {
                s->reloading=false;
                s->state.copy_label(WIFEXITED(status)&&WEXITSTATUS(status)==0?"Driver reload requested. Refresh Wi-Fi to check for an interface.":"Driver reload failed. See /tmp/felix-firmware.log; the chipset may need another driver or firmware.");
                return;
            }
            if(WIFEXITED(status)&&WEXITSTATUS(status)==0) {
                for(const auto& card:s->cards)if(!card.pci.empty()) {
                    std::ofstream probe("/sys/bus/pci/drivers_probe");probe<<card.pci;
                }
            }
            s->state.copy_label(WIFEXITED(status)&&WEXITSTATUS(status)==0
                ? "Firmware installed; detected PCI cards were reprobed. Refresh Wi-Fi, or reconnect a USB adapter.\nInstalled systems keep firmware. Live-session firmware downloads are lost at shutdown."
                : "Firmware installation failed. Check Ethernet connectivity and /tmp/felix-firmware.log.");
        } else Fl::repeat_timeout(.25,tick,s);
    }
    void download() {
        static const char *packages[]={"linux-firmware-intel","linux-firmware-brcm","linux-firmware-ath9k_htc","linux-firmware-rtlwifi","linux-firmware-mediatek"};
        int i=vendor.value();if(child>0)return;
        if(i<0||i>4){state.copy_label("Use the hardware IDs to find a compatible driver. No generic firmware package is available.");return;}
        if(fl_choice("Download %s and dependencies? Internet access is required; firmware can be large.","Cancel","Download",nullptr,packages[i])!=1)return;
        child=fork();
        if(child==0){int fd=open("/tmp/felix-firmware.log",O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd>=0){dup2(fd,1);dup2(fd,2);close(fd);}execl("/sbin/apk","apk","add","--no-cache",packages[i],(char*)nullptr);_exit(127);}
        if(child<0){state.copy_label("Could not start firmware download.");return;}
        install.deactivate();vendor.deactivate();state.copy_label("Downloading firmware. Keep the network connected.");Fl::add_timeout(.25,tick,this);
    }
    void reload() {
        if(child>0)return;
        int i=list.value()-1;
        if(i<0||i>=(int)cards.size()){state.copy_label("Select a detected card first.");return;}
        std::string pci=cards[i].pci;
        if(pci.empty()){state.copy_label("Reconnect this USB adapter after installing firmware, then refresh Wi-Fi.");return;}
        if(fl_choice("Reload the selected Wi-Fi card? Its current wireless connection will disconnect.","Cancel","Reload",nullptr)!=1)return;
        child=fork();
        if(child==0) {
            int fd=open("/tmp/felix-firmware.log",O_WRONLY|O_CREAT|O_TRUNC,0600);
            if(fd>=0){dup2(fd,1);dup2(fd,2);close(fd);}
            bool ok=true;
            std::string unbind="/sys/bus/pci/devices/"+pci+"/driver/unbind";
            if(!access(unbind.c_str(),F_OK)){std::ofstream f(unbind);f<<pci;f.flush();ok=bool(f);}
            if(ok){std::ofstream f("/sys/bus/pci/drivers_probe");f<<pci;f.flush();ok=bool(f);}
            if(!ok)perror("driver reload");
            _exit(ok?0:1);
        }
        if(child<0){state.copy_label("Could not start the driver reload.");return;}
        reloading=true;install.deactivate();vendor.deactivate();state.copy_label("Reloading the selected Wi-Fi card...");Fl::add_timeout(.25,tick,this);
    }
    WifiHardware() {
        win.begin();new Fl_Box(20,10,740,25,"Detected Wi-Fi cards and firmware");
        list.format_char(0);vendor.add("Intel|Broadcom|Qualcomm / Atheros|Realtek|MediaTek / Ralink|Other");vendor.value(0);
        list.callback([](Fl_Widget*,void*p){auto*s=(WifiHardware*)p;int i=s->list.value()-1;if(i>=0&&i<(int)s->cards.size()&&s->child<0)s->vendor.value(s->cards[i].family);},this);
        install.callback([](Fl_Widget*,void*p){((WifiHardware*)p)->download();},this);
        auto *rescan=new Fl_Button(20,335,180,30,"Rescan hardware");
        rescan->callback([](Fl_Widget*,void*p){auto*s=(WifiHardware*)p;if(s->child<0)s->detect();},this);
        auto *retry=new Fl_Button(220,335,220,30,"Reload selected driver");retry->callback([](Fl_Widget*,void*p){((WifiHardware*)p)->reload();},this);
        auto *hint=new Fl_Box(450,335,310,30,"Choose your Wi-Fi chipset.");hint->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);hint->labelsize(12);
        state.align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
        win.end();win.callback([](Fl_Widget*,void*p){auto*s=(WifiHardware*)p;if(s->child<0)s->win.hide();else s->state.copy_label("Please wait for the firmware download to finish before closing.");},this);
        detect();win.show();
    }
    ~WifiHardware(){Fl::remove_timeout(tick,this);}
};
