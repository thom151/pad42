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
int editorReader();
void editorClearScreen();
void editorDrawRows();
void initEditor();
int getCursorPosition( int *rows, int *cols);
void editorMoveCursor(int c);

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


enum editorKey {
    LEFT = 1000,
    RIGHT,
    UP,
    DOWN,
    PAGE_UP,
    PAGE_DOWN
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

    A.apBuf += "\x1b[" + std::to_string(E.cy + 1) + ";" + std::to_string(E.cx + 1) + "H";
    A.apBuf +="\x1b[?25h";

    write(STDOUT_FILENO, A.apBuf.c_str(), A.apBuf.size());
    A.apBuf.clear();
}


void editorDrawRows() {

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
    int c =editorReader();
    switch(c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2j", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case UP:
        case LEFT:
        case DOWN:
        case RIGHT:
            editorMoveCursor(c);
            break;
        case PAGE_UP:
        case PAGE_DOWN:
            {
                int move = E.screenRows;
                while(move > 0) {
                    if(c == PAGE_UP) editorMoveCursor(UP);
                    if(c == PAGE_DOWN) editorMoveCursor(DOWN);
                    --move;
                }

            }
            break;
    }
}

void editorMoveCursor(int key) {
    switch (key) {
        case UP:
            if(E.cy != 0) {
                --E.cy;
            }
            break;
        case LEFT:
            if(E.cx != 0) {
                --E.cx;
            }
            break;
        case DOWN:
            if(E.cy != E.screenRows -1 ) {
                ++E.cy;
            }
            break;
        case RIGHT:
            if(E.cx != E.screenCols-1) {
                ++E.cx;
            }
            break;
    }
}




/*** TERMINAL ***/

//read char and return
int editorReader() {
    char c;
    int returnCode;
    while((returnCode = read(STDIN_FILENO, &c, 1)) != 1) {
        if(returnCode == -1 && errno != EAGAIN) die("read editor reader");
    }
    if ( c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if ( seq[0] == '[') {
            if(seq[1] >= '0' && seq[1] <= '9') {
                std::cout<<"Here we are\r\n";
                if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if(seq[2] == '~') {
                    switch (seq[1]) {
                        case '5':
                            std::cout<<"Up sir\r\n";
                            return PAGE_UP;
                        case '6':
                            std::cout<<"Down sir\r\n";
                            return PAGE_DOWN;
                    }
                }

            } else {
                switch(seq[1]) {
                    case 'A': return UP;
                    case 'B': return DOWN;
                    case 'C': return RIGHT;
                    case 'D': return LEFT;
                }

            }
        }
       
        return '\x1b';
    } else {
        return c;
    }
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
    raw.c_cc[VTIME] = 50;

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
