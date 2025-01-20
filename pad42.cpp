#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <iostream>
#include <errno.h>

struct termios orig_termios;


/*** HEADERS ***/
void enableRawMode();
void disableRawMode();
void die(const char* s);



/*** MAIN LOOP ***/
int main() {
    enableRawMode();

    while(true) {
        char c = '\0';
       if ( read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read main");
        if(std::iscntrl(c)) {
            std::cout<< static_cast<int>(c) << "\r\n";
        }else {
            std::cout<< static_cast<int>(c) << "('" << c <<"')" << "\r\n";
        }

        if (c=='q') break; 
    }
 return 0;
}




/*** TERMINAL ***/
void enableRawMode() {

    //copy the settings of the standard input into raw
   if  (tcgetattr(STDIN_FILENO, &orig_termios) == -1)  die("tcgetattr (enable raw mode)");
    atexit(disableRawMode);

    //create a raw structure that gets terminal settings
    struct termios raw = orig_termios;


    //disable the echo flag, Ctrl-C, Ctrl-Z, Ctrl-S, Ctrl-Q, Ctrl-V
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);   


    //so this isthe minimum number of bytes that our read func will return
    raw.c_cc[VMIN] = 0;

    //max time to wait before c returns; it's in milliseconds
    raw.c_cc[VTIME] = 1;

    //set the configures settings to standard input
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr (enable raw mode)");
}


void disableRawMode() {
  if(  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        die("tcsetattr");
    }
}

void die(const char *s) {
    std::cerr<< s;
    exit(1);
}
