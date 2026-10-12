#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

static char* nexus_strdup(const char* src){
    if(!src)src="";
    size_t n=strlen(src);
    char* out=(char*)malloc(n+1);
    if(!out)return NULL;
    memcpy(out,src,n+1);
    return out;
}

void nexus_print_i64(int64_t x){
    printf("%lld\n",(long long)x);
}
void nexus_print_f64(double x){
    printf("%.15g\n",x);
}
void nexus_print_bool(bool x){
    puts(x?"true":"false");
}
void nexus_print_str(const char* x){
    puts(x?x:"null");
}
void* nexus_alloc(size_t n){
    return malloc(n);
}
void nexus_free(void* p){
    free(p);
}
void nexus_bounds_check(int64_t index,int64_t size){
    if(index<0||index>=size){
        fprintf(stderr,"Nexus runtime error: array index %lld out of bounds for size %lld\n",(long long)index,(long long)size);
        exit(1);
    }
}
char* nexus_str_concat(const char* a,const char* b){
    if(!a)a="";
    if(!b)b="";
    size_t la=strlen(a),lb=strlen(b);
    char* out=(char*)malloc(la+lb+1);
    if(!out)return NULL;
    memcpy(out,a,la);
    memcpy(out+la,b,lb);
    out[la+lb]=0;
    return out;
}
bool nexus_str_equal(const char* a,const char* b){
    if(a==b)return true;
    if(!a||!b)return false;
    return strcmp(a,b)==0;
}

static char* nexus_readline_alloc(void){

    size_t cap=256,len=0;
    char* buf=(char*)malloc(cap);
    if(!buf) return NULL;

    int ch;
    while((ch=getchar())!=EOF){
        if(ch=='\r') continue;
        if(ch=='\n') break;
        if(len+1>=cap){
            size_t ncap=cap*2;
            char* nb=(char*)realloc(buf,ncap);
            if(!nb){
                free(buf);
                return NULL;
            }
            buf=nb;
            cap=ncap;
        } buf[len++]=(char)ch;
    }
    if(ch==EOF && len==0){
        buf[0]=0;
        return buf;
    } buf[len]=0;
    return buf;

}
char* nexus_input(const char* prompt){
    if(prompt)fputs(prompt,stdout);
    fflush(stdout);
    return nexus_readline_alloc();
}
int64_t nexus_input_i64(const char* prompt){
    for(;;){
        char* s=nexus_input(prompt);
        if(!s)return 0;
        char* end=NULL;
        long long v=strtoll(s,&end,10);
        while(end&&*end==' ')++end;
        if(end&&*end=='\0'){
            free(s);
            return (int64_t)v;
        }
        fprintf(stderr,"Invalid integer. Try again.\n");
        free(s);
    }
}
double nexus_input_f64(const char* prompt){
    for(;;){
        char* s=nexus_input(prompt);
        if(!s)return 0.0;
        char* end=NULL;
        double v=strtod(s,&end);
        while(end&&*end==' ')++end;
        if(end&&*end=='\0'){
            free(s);
            return v;
        }
        fprintf(stderr,"Invalid number. Try again.\n");
        free(s);
    }
}

char* nexus_str_i64(int64_t value){
    char buf[64];
    snprintf(buf,sizeof(buf),"%lld",(long long)value);
    return nexus_strdup(buf);
}
char* nexus_str_f64(double value){
    char buf[96];
    snprintf(buf,sizeof(buf),"%.15g",value);
    return nexus_strdup(buf);
}
char* nexus_str_bool(bool value){
    return nexus_strdup(value?"true":"false");
}

void nexus_clear(void){
    fputs("\033[2J\033[H",stdout);
    fflush(stdout);
}
void nexus_sleep_ms(int64_t ms){
    if(ms<=0)return;

    #ifdef _WIN32
    Sleep((DWORD)ms);

    #else
    struct timespec ts;
    ts.tv_sec=(time_t)(ms/1000);
    ts.tv_nsec=(long)((ms%1000)*1000000);
    nanosleep(&ts,NULL);

    #endif
}
int64_t nexus_random_i64(int64_t min,int64_t max){
    static int seeded=0;
    if(!seeded){
        srand((unsigned)time(NULL));
        seeded=1;
    }
    if(min>max){
        int64_t t=min;
        min=max;
        max=t;
    }
    unsigned long long span=(unsigned long long)(max-min)+1ULL;
    if(span==0ULL)return min;
    unsigned long long r=((unsigned long long)(unsigned)rand()<<32) ^ (unsigned)rand();
    return min+(int64_t)(r%span);
}
int64_t nexus_time_ms(void){

    #ifdef _WIN32
    return (int64_t)GetTickCount64();

    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return (int64_t)ts.tv_sec*1000 + (int64_t)(ts.tv_nsec/1000000);

    #endif
}
int64_t nexus_system(const char* command){
    return command?system(command):-1;
}
char* nexus_file_read(const char* path){
    if(!path)return NULL;
    FILE* f=fopen(path,"rb");
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END)!=0){
        fclose(f);
        return NULL;
    }
    long n=ftell(f);
    if(n<0){
        fclose(f);
        return NULL;
    }
    rewind(f);
    char* out=(char*)malloc((size_t)n+1);
    if(!out){
        fclose(f);
        return NULL;
    }
    size_t got=fread(out,1,(size_t)n,f);
    fclose(f);
    out[got]=0;
    return out;
}
int64_t nexus_file_write(const char* path,const char* data){
    if(!path)return -1;
    FILE* f=fopen(path,"wb");
    if(!f)return -1;
    const char* src=data?data:"";
    size_t n=strlen(src);
    size_t wrote=fwrite(src,1,n,f);
    int ok=(wrote==n&&fclose(f)==0);
    return ok?0:-1;
}
bool nexus_file_exists(const char* path){
    if(!path)return false;
    FILE* f=fopen(path,"rb");
    if(!f)return false;
    fclose(f);
    return true;
}
char* nexus_env(const char* name){
    const char* v=name?getenv(name):NULL;
    return nexus_strdup(v?v:"");
}
void nexus_exit(int64_t code){
    exit((int)code);
}
void nexus_beep(void){
    fputs("\a",stdout);
    fflush(stdout);
}

#ifdef _WIN32
static int nexus_screen_started=0;

void nexus_screen_begin(void){
    nexus_screen_started=1;
}
void nexus_screen_end(void){
    if(nexus_screen_started){
        fputs("\033[0m\033[?25h",stdout);
        fflush(stdout);
        nexus_screen_started=0;
    }
}
bool nexus_key_pressed(void){
    return _kbhit()!=0;
}
int64_t nexus_read_key(void){
    return (int64_t)_getch();
}
void nexus_screen_set_title(const char* title){
    if(title)SetConsoleTitleA(title);
}
void nexus_screen_clear(void){
    nexus_clear();
}
#else
static struct termios nexus_old_termios;
static int nexus_raw=0;
static int nexus_screen_started=0;

void nexus_screen_begin(void){

    if(nexus_raw)return;
    if(tcgetattr(STDIN_FILENO,&nexus_old_termios)!=0)return;
    struct termios raw=nexus_old_termios;
    raw.c_lflag&=(tcflag_t)~(ICANON|ECHO);
    raw.c_cc[VMIN]=0;
    raw.c_cc[VTIME]=0;
    tcsetattr(STDIN_FILENO,TCSANOW,&raw);
    nexus_raw=1;
    nexus_screen_started=1;
    fputs("\033[?25l",stdout);
    fflush(stdout);

}
void nexus_screen_end(void){
    if(nexus_raw){
        tcsetattr(STDIN_FILENO,TCSANOW,&nexus_old_termios);
        nexus_raw=0;
    }
    if(nexus_screen_started){
        fputs("\033[0m\033[?25h",stdout);
        fflush(stdout);
        nexus_screen_started=0;
    }
}
bool nexus_key_pressed(void){
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO,&set);
    struct timeval tv={
        0,0
    };
    int rc=select(STDIN_FILENO+1,&set,NULL,NULL,&tv);
    return rc>0&&FD_ISSET(STDIN_FILENO,&set);
}
int64_t nexus_read_key(void){
    unsigned char c=0;
    if(!nexus_raw)nexus_screen_begin();
    if(read(STDIN_FILENO,&c,1)==1)return (int64_t)c;
    return -1;
}
void nexus_screen_set_title(const char* title){
    if(title)printf("\033]0;%s\007",title);
}
void nexus_screen_clear(void){
    nexus_clear();
}
#endif

void nexus_screen_put(int64_t x,int64_t y,const char* text){
    if(x<0)x=0;
    if(y<0)y=0;
    printf("\033[%lld;%lldH%s",(long long)(y+1),(long long)(x+1),text?text:"");
}
void nexus_screen_present(void){
    fflush(stdout);
}
void nexus_screen_width_fallback(void){
}
int64_t nexus_screen_width(void){

    #ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&info))return (int64_t)(info.srWindow.Right-info.srWindow.Left+1);
    return 80;

    #else
    struct winsize ws;
    if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)==0&&ws.ws_col)return ws.ws_col;
    return 80;

    #endif
}
int64_t nexus_screen_height(void){

    #ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&info))return (int64_t)(info.srWindow.Bottom-info.srWindow.Top+1);
    return 24;

    #else
    struct winsize ws;
    if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)==0&&ws.ws_row)return ws.ws_row;
    return 24;

    #endif
}
