#include <asm-generic/ioctls.h>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include <string>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct editorConfig {
    int screenRows;
    int screenCols;
    struct termios orig_termios;
};

struct editorConfig E;


/*** HEADERS ***/
void enableRawMode();
void disableRawMode();
void die(const char* s);
void editorProcessKeyPress();
char editorReader();
void editorClearScreen();
void editorDrawRows();
void initEditor();



/*** MAIN LOOP ***/
int main() {
    enableRawMode();
    initEditor();
    while(true) {
        editorClearScreen();
        editorProcessKeyPress();
    }
 return 0;
}

/*** OUTPUT ***/
void editorClearScreen() {

    //writing 4 bytes to the termina;
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}


void editorDrawRows() {
    for(int i{1}; i<E.screenRows; i++) {
        std::string rowNums = std::to_string(i) + "\r\n";
        write(STDOUT_FILENO, rowNums.c_str(), rowNums.size());
    }
}



//*** INPUT ***//
//process the input
void editorProcessKeyPress() {
    char c =editorReader();
    switch(c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2j", 4);

            exit(0);
            break;
    }
}




/*** TERMINAL ***/

//read char and return
char editorReader() {
    char c;
    int returnCode;
    while((returnCode = read(STDIN_FILENO, &c, 1)) != 1) {
        if(returnCode == -1 && errno != EAGAIN) die("read editor reader");
    }

    return c;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize sz;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == -1 || sz.ws_col == 0) {
        return -1;
    } else {
        *cols = sz.ws_col;
        *rows = sz.ws_row;
    }

    return 0;

}

/* **TODO**
int getWindowsSizeFallBack(int *rows, int *cols) {
    
}
*/

void enableRawMode() {

    //copy the settings of the standard input into E.orig_termios struct man
   if  (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)  die("tcgetattr (enable raw mode)");
    atexit(disableRawMode);

    //create a raw structure that gets terminal settings
    struct termios raw = E.orig_termios;


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
  if(  tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2j", 4);

    std::cerr<< s;
    exit(1);
}



/*** INIT ***/
void initEditor() {
    if(getWindowSize(&E.screenRows, &E.screenCols) == -1) die("getWindowSize init");
}
