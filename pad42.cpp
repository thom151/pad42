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
#include <vector>
#include <sstream>


/*** HEADERS ***/
void enableRawMode();
void disableRawMode();
void die(const char* s);
void editorProcessKeyPress();
char editorReader();
void editorClearScreen();
void editorDrawRows();
void initEditor();
int getCursorPosition( int *rows, int *cols);

#define CTRL_KEY(k) ((k) & 0x1f)

struct editorConfig {
    int cx, cy;
    int screenRows;
    int screenCols;
    struct termios orig_termios;
};

struct appendBuffer {
    std::string apBuf;
};

struct editorConfig E;
struct appendBuffer A;

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
    //
    A.apBuf +="\x1b[?25l";
    //A.apBuf +="\x1b[2J";
    A.apBuf += "\x1b[H";

    editorDrawRows();
    A.apBuf += "\x1b[H";
    A.apBuf +="\x1b[?25h";

    write(STDOUT_FILENO, A.apBuf.c_str(), A.apBuf.size());
    A.apBuf.clear();
}


void editorDrawRows() {

    std::cout<<"Drawing Rows"<<"\r\n";
    for(int i{1}; i<E.screenRows; i++) {
        if (i == E.screenRows /3) {
            std::string welcome = "Pad 42 Editor -- version 0.1";
            A.apBuf += "~";
            int spaces = (E.screenCols - welcome.length())/2-1;
            for(; spaces > 0; --spaces) {
                A.apBuf+=" ";
            }
    
            A.apBuf += welcome;
        } else {
            A.apBuf += "~";
        }
        A.apBuf += "\x1b[K";
        if (i < E.screenRows - 1) {
            A.apBuf += "\r\n";
        }
    }
}



//*** INPUT ***//
//process the input
void editorProcessKeyPress() {
    char c =editorReader();
    switch(c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2j", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
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


int getWindowsSizeFallBack(int *rows, int *cols) {
    struct winsize sz;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == -1 || sz.ws_col == 0) {
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = sz.ws_col;
        *rows = sz.ws_row;
    }

    return 0;


}

int getCursorPosition( int *rows, int *cols) {
    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    

    std::vector<char> buf(32);
    unsigned int i = 0;


      std::cout<<"\r\n";

    while (i < buf.size()) {
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;

        //if we reach the end of the STDIN
        if(buf[i] == 'R') break;
        ++i;
    }
    buf[i] = '\0';



    // example : <esc>[24;80
    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    std::istringstream ss(&buf[2]);
    ss >> *rows;
    ss.ignore(1);
    ss >> *cols;


    return 0;
}


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
    write(STDOUT_FILENO, "\x1b[H", 3);
    std::cerr<< s;
    exit(1);
}



/*** INIT ***/
void initEditor() {
    E.cx = 0;
    E.cy = 0;
    if(getWindowsSizeFallBack(&E.screenRows, &E.screenCols) == -1) die("getWindowSize init");
}
