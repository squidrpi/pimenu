
// Default key and joystick mappings

enum {
    GP2X_UP=0x1,
    GP2X_UP_LEFT=1<<1,
    GP2X_LEFT=1<<2,
    GP2X_DOWN_LEFT=1<<3,
    GP2X_DOWN=1<<4,
    GP2X_DOWN_RIGHT=1<<5,
    GP2X_RIGHT=1<<6,
    GP2X_UP_RIGHT=1<<7,
    GP2X_START=1<<8,
    GP2X_SELECT=1<<9,
    GP2X_L=1<<10,
    GP2X_R=1<<11,
    GP2X_A=1<<12,
    GP2X_B=1<<13,
    GP2X_X=1<<14,
    GP2X_Y=1<<15,
    GP2X_ESCAPE=1<<16
};

#define RPI_KEY_A       SDLK_LCTRL
#define RPI_KEY_START   SDLK_RETURN
#define RPI_KEY_SELECT  SDLK_TAB
#define RPI_KEY_LEFT    SDLK_LEFT
#define RPI_KEY_RIGHT   SDLK_RIGHT
#define RPI_KEY_UP      SDLK_UP
#define RPI_KEY_DOWN    SDLK_DOWN
#define RPI_KEY_QUIT    SDLK_ESCAPE

#define RPI_JOY_A       1
#define RPI_JOY_START   7
#define RPI_JOY_SELECT  6
#define RPI_JOY_QUIT    5
#define RPI_JOY_LEFT    11
#define RPI_JOY_RIGHT   12
#define RPI_JOY_UP      13
#define RPI_JOY_DOWN    14

#define A_1     0
#define START_1 12
#define SELECT_1 13
#define UP_1    16
#define DOWN_1  17
#define LEFT_1  18
#define RIGHT_1 19

#define QUIT 51

// Axis positions
#define CENTER  0
#define LEFT    1
#define RIGHT   2
#define UP      1
#define DOWN    2
