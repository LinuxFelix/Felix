// FLTK installation wizard. The privileged backend revalidates every target.
struct Installer {
    struct Disk {std::string path,token,label;};
    Fl_Window win{780,590,"Install Felix"};
    Fl_Group *pages[5];Fl_Hold_Browser *drives;
    Fl_Input *name;Fl_Secret_Input *password,*confirm;
    Fl_Check_Button *erase;Fl_Box *summary,*state;
    Fl_Button *back,*next;Fl_Text_Display *log;Fl_Text_Buffer output;
    std::vector<Disk> disks;Disk selected;
    int page=0,fd=-1;pid_t child=-1;bool eof=false,reaped=false;int status=0;
    void show(int n){for(auto*p:pages)p->hide();page=n;pages[n]->show();back->activate();if(n==0)back->deactivate();next->copy_label(n==2?"Yes, use ext4":n==3?"Proceed and install":"Next");win.redraw();}
    void refresh() {
        disks.clear();drives->clear();
        FILE*f=popen("/usr/bin/felix-install --list","r");if(!f)return;
        char line[1024];while(fgets(line,sizeof(line),f)){
            auto fields=wifi::split(line,'\t');if(fields.size()<4)continue;
            double gib=strtod(fields[1].c_str(),nullptr)*512.0/1073741824.;char size[40];snprintf(size,sizeof(size),"%.1f GiB",gib);
            std::string model=fields[3];while(!model.empty()&&(model.back()=='\n'||model.back()=='\r'))model.pop_back();
            disks.push_back({fields[0],fields[2],fields[0]+"    "+size+"    "+model});drives->add(disks.back().label.c_str());
        }pclose(f);
        if(disks.empty())drives->add("No eligible disk. Mounted, busy, read-only and unsupported drives are excluded.");
    }
    void advance() {
        if(page==0){int i=drives->value()-1;if(i<0||i>=(int)disks.size()){fl_alert("Select a disk first.");return;}selected=disks[i];show(1);}
        else if(page==1){std::string user=name->value(),pass=password->value();
            if(user.empty()||user.size()>31||user[0]<'a'||user[0]>'z'||user.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos){fl_alert("Use a login name starting with a lowercase letter, followed by lowercase letters, digits, _ or -.");return;}
            if(pass.size()<8||pass.size()>128||pass.find_first_of("\r\n")!=std::string::npos){fl_alert("Use an 8 to 128 character password without line breaks.");return;}
            if(pass!=confirm->value()){fl_alert("The passwords do not match.");return;}show(2);
        }else if(page==2){summary->copy_label(("Install to: "+selected.label+"\nUser: "+name->value()+" (sudo administrator)\nLayout: entire disk, one ext4 partition, BIOS bootloader.\n\nALL FILES AND PARTITIONS ON THIS DISK WILL BE ERASED.\nThis does not preserve another operating system.\n\nConnect Ethernet to download the required installation tools.\nDownloads finish before your disk is erased.\nIf downloads fail, the disk is left untouched.").c_str());erase->value(0);show(3);
        }else if(page==3){if(!erase->value()){fl_alert("Check the erase confirmation for the selected disk.");return;}start();}
        else if(page==4 && child<0)win.hide();
    }
    void start() {
        int out[2],input[2];
        if(pipe(out)){fl_alert("Cannot start installation. Please try again.");return;}
        if(pipe(input)){close(out[0]);close(out[1]);fl_alert("Cannot start installation. Please try again.");return;}
        child=fork();if(child<0){close(out[0]);close(out[1]);close(input[0]);close(input[1]);fl_alert("Cannot start installer.");return;}
        if(!child){dup2(input[0],0);dup2(out[1],1);dup2(out[1],2);close(out[0]);close(out[1]);close(input[0]);close(input[1]);execl("/usr/bin/felix-install","felix-install","--erase-and-install",selected.path.c_str(),selected.token.c_str(),name->value(),(char*)nullptr);_exit(127);}
        close(out[1]);close(input[0]);std::string pass=std::string(password->value())+"\n";
        signal(SIGPIPE,SIG_IGN);write(input[1],pass.data(),pass.size());close(input[1]);std::fill(pass.begin(),pass.end(),'\0');password->value("");confirm->value("");
        fd=out[0];fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);show(4);back->deactivate();next->deactivate();state->copy_label("Installing. Keep power and Ethernet connected.");Fl::add_timeout(.1,tick,this);
    }
    static void tick(void*p){auto*s=(Installer*)p;char buffer[4096];ssize_t n;
        while((n=read(s->fd,buffer,sizeof(buffer)-1))>0){buffer[n]=0;s->output.append(buffer);s->log->insert_position(s->output.length());s->log->show_insert_position();}
        if(n==0)s->eof=true;
        if(!s->reaped&&waitpid(s->child,&s->status,WNOHANG)==s->child)s->reaped=true;
        if(s->eof&&s->reaped){close(s->fd);s->fd=-1;s->child=-1;s->next->activate();s->next->copy_label("Finish");s->state->copy_label(WIFEXITED(s->status)&&WEXITSTATUS(s->status)==0?"Installed. Remove the ISO and reboot to use Felix from disk.":"Installation failed. Read the log; a disk erased during this attempt may be incomplete.");}
        else Fl::repeat_timeout(.1,tick,s);
    }
    Installer(){win.begin();
        pages[0]=new Fl_Group(15,15,750,490);new Fl_Box(30,25,710,55,"What drive do you want to use to install Felix?");
        drives=new Fl_Hold_Browser(30,95,710,280);drives->format_char(0);auto*r=new Fl_Button(30,390,140,32,"Refresh drives");r->callback([](Fl_Widget*,void*p){((Installer*)p)->refresh();},this);
        auto*info=new Fl_Box(30,435,710,55,"BIOS/legacy boot, whole-disk installation. Supported disk size: 2 GiB to 2 TiB.\nBack up the selected disk before continuing.");info->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);pages[0]->end();
        pages[1]=new Fl_Group(15,15,750,490);new Fl_Box(30,30,710,45,"Create your user account");name=new Fl_Input(190,120,480,32,"Name (login)");password=new Fl_Secret_Input(190,175,480,32,"Password");confirm=new Fl_Secret_Input(190,230,480,32,"Confirm password");
        auto*account=new Fl_Box(40,305,690,110,"This account can manage software and system settings.\nYour password is required for administrator actions.\nFelix signs into this desktop account automatically at boot.\nText-console login requires the password.");account->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);pages[1]->end();
        pages[2]=new Fl_Group(15,15,750,490);auto*ext=new Fl_Box(50,90,680,230,"By default Felix will use an ext4 partition. Proceed?\n\nFelix will use the entire selected drive.\nYour applications, documents and settings will be saved there.\nAfter installation, remove the installation media and restart.");ext->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);pages[2]->end();
        pages[3]=new Fl_Group(15,15,750,490);summary=new Fl_Box(35,30,710,360);summary->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);erase=new Fl_Check_Button(35,420,710,36,"I confirm: erase the selected disk and install Felix.");pages[3]->end();
        pages[4]=new Fl_Group(15,15,750,490);log=new Fl_Text_Display(25,30,730,390);log->buffer(&output);log->textfont(FL_COURIER);log->textsize(12);state=new Fl_Box(25,435,730,60);state->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);pages[4]->end();
        back=new Fl_Button(455,535,130,32,"Back");back->callback([](Fl_Widget*,void*p){auto*s=(Installer*)p;if(s->page>0&&s->page<4)s->show(s->page-1);},this);
        next=new Fl_Button(595,535,165,32,"Next");next->callback([](Fl_Widget*,void*p){((Installer*)p)->advance();},this);
        win.end();win.callback([](Fl_Widget*,void*p){auto*s=(Installer*)p;if(s->child>0)fl_alert("Wait until installation finishes before closing.");else s->win.hide();},this);refresh();show(0);win.show();
    }
    ~Installer(){Fl::remove_timeout(tick,this);}
};
