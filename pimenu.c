/*  PiMenu for Raspberry Pi

	Copyright (C) 2013 Squid. 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL.h>
#include <glib.h>
#include <png.h>
#include <bcm_host.h>

#include "pimenu.h"
#include "keyconstants.h"

SDL_Event event;

#define MAXICONS 20
#define PNGSIZE 384

typedef struct IMAGE_T_
{
    int32_t width;
    int32_t height;
    int32_t pitch;
    uint16_t bitsPerPixel;
    uint32_t size;
    void *buffer;
} IMAGE_T;

IMAGE_T image;

//Actual number icons being displayed
int num_icons=0;

char g_string[255];

unsigned char joy_buttons[2][32];
unsigned char joy_axes[2][8];

int joyaxis_LR, joyaxis_UD;

unsigned char *sdl_keys;

static void fe_ProcessEvents (void);
unsigned long pi_joystick_read(void);

static void dispmanx_init(void);
static void dispmanx_deinit(void);
static void dispmanx_display(void);
static void initSDL(void);
int loadPNG(IMAGE_T* image, const char *file, int sizex, int sizey);
void freePNG(IMAGE_T* image);

#define NUMKEYS 256
static Uint16 pi_key[NUMKEYS];
static Uint16 pi_joy[NUMKEYS];

int icon_x=0;

int posx_diff;
int posy=0;
int iconsize=PNGSIZE;
float scalesize=0;  //scale ICONs on screen based on display size

int current_icon=0;
int current_icon_pos=0;
int next_icon=0;
int middle_position=0;

int icon_posx[20];

char icon_commands[MAXICONS][1000];
char icon_args[MAXICONS][1000];
char tmp_icon_commands[MAXICONS][1000];
char tmp_icon_args[MAXICONS][1000];

uint32_t display_width=0, display_height=0;

int zoomspeed=4;
int zoom=1;

char abspath[1000];

struct options
{
	int icon_count;
	int kiosk_mode;
} options;

static GKeyFile *gkeyfile=0;

static void open_config_file(void)
{
    GError *error = NULL;

    gkeyfile = g_key_file_new ();
    if (!(int)g_key_file_load_from_file (gkeyfile, "pimenu.cfg", G_KEY_FILE_NONE, &error))
    {
        gkeyfile=0;
    }
}

static void close_config_file(void)
{
    if(gkeyfile)
        g_key_file_free(gkeyfile);
}

static int get_integer_conf (const char *section, const char *option, int defval)
{
    GError *error=NULL;
    int tempint;

    if(!gkeyfile) return defval;

    tempint = g_key_file_get_integer(gkeyfile, section, option, &error);
    if (!error)
        return tempint;
    else
        return defval;
}

static char* get_string_conf (const char *section, const char *option)
{
    GError *error=NULL;

    if(!gkeyfile) return NULL;

    return g_key_file_get_string(gkeyfile, section, option, &error);
}

int exit_prog(void)
{
	dispmanx_deinit();

	SDL_JoystickClose(0);
	SDL_Quit();

	exit(0);
}

void init_title(void)
{
    FILE *fp;
    unsigned char filename[65535];
    int i;

    //Scan all the icon commands configured in the cfg file and check to 
    //see if they actually exist. Also check the icons exist, otherwise
    //simple skip them alltogether.
    for(i=0;i<options.icon_count;i++) {

        if ((fp = fopen( tmp_icon_commands[i] , "r")) == NULL) {
          continue;
        }

        //Check icon images exists, otherwise skip it
        sprintf(filename, "./ICON%d.png", i);    
        if ((fp = fopen( filename , "r")) != NULL) {
            fclose(fp);

            strcpy(icon_commands[num_icons], tmp_icon_commands[i]);
            strcpy(icon_args[num_icons], tmp_icon_args[i]);

            num_icons++;
        }
    }
}

void ss_prog_run(void)
{
	char cwd_dir[1000];
	char curr_dir[1000];
	char runcmd[2000];

	//Close down all displays and SDL Input so game has full control
 	dispmanx_deinit();	
	SDL_JoystickClose(0);
    SDL_Quit();

    //Set the cwd directory to where the binary is
    realpath(icon_commands[current_icon], curr_dir);
    char *dirsep = strrchr(curr_dir, '/');
    if( dirsep != 0 ) *dirsep = 0;

	getcwd(cwd_dir, 999);	
	chdir(curr_dir);

	strcpy(runcmd, icon_commands[current_icon]);
	if(strlen(icon_args[current_icon]) > 0) {
		strcat(runcmd, " ");
 		strcat(runcmd, icon_args[current_icon]);
	}

	//Run Program and wait
	system(runcmd);

	chdir(cwd_dir);

}


void pi_initialise()
{
	int i;
	
    memset(joy_buttons, 0, 32*2);
    memset(joy_axes, 0, 8*2);
    memset(pi_key, 0, NUMKEYS*2);
    memset(pi_joy, 0, NUMKEYS*2);

    //Open config file for reading below
    open_config_file();

    //Configure keys from config file or defaults
    pi_key[A_1] = get_integer_conf("Keyboard", "A_1", RPI_KEY_A);
    pi_key[START_1] = get_integer_conf("Keyboard", "START_1", RPI_KEY_START);
    pi_key[LEFT_1] = get_integer_conf("Keyboard", "LEFT_1", RPI_KEY_LEFT);
    pi_key[RIGHT_1] = get_integer_conf("Keyboard", "RIGHT_1", RPI_KEY_RIGHT);
    pi_key[UP_1] = get_integer_conf("Keyboard", "UP_1", RPI_KEY_UP);
    pi_key[DOWN_1] = get_integer_conf("Keyboard", "DOWN_1", RPI_KEY_DOWN);
	pi_key[QUIT] = get_integer_conf("Keyboard", "QUIT", RPI_KEY_QUIT);

    //Configure joysticks from config file or defaults
    pi_joy[A_1] = get_integer_conf("Joystick", "A_1", RPI_JOY_A);
    pi_joy[LEFT_1] = get_integer_conf("Joystick", "LEFT_1", RPI_JOY_LEFT);
    pi_joy[RIGHT_1] = get_integer_conf("Joystick", "RIGHT_1", RPI_JOY_RIGHT);
    pi_joy[UP_1] = get_integer_conf("Joystick", "UP_1", RPI_JOY_UP);
    pi_joy[DOWN_1] = get_integer_conf("Joystick", "DOWN_1", RPI_JOY_DOWN);

    pi_joy[START_1] = get_integer_conf("Joystick", "START_1", RPI_JOY_START);
    pi_joy[SELECT_1] = get_integer_conf("Joystick", "SELECT_1", RPI_JOY_SELECT);

    pi_joy[QUIT] = get_integer_conf("Joystick", "QUIT", RPI_JOY_QUIT);

	//Read joystick axis to use, default to 0 & 1
	joyaxis_LR = get_integer_conf("Joystick", "JA_LR", 0);
	joyaxis_UD = get_integer_conf("Joystick", "JA_UD", 1);

	options.kiosk_mode = get_integer_conf("General", "kioskmode", 0);	

    //Read command list
	options.icon_count = 0;
	for(i=0;i<MAXICONS;i++) {
		char tmpstr[50], *tempptr;
		sprintf(tmpstr, "icon_command_%d", i);
		tempptr = get_string_conf("General", tmpstr);
		strcpy(tmp_icon_commands[i], "");
		if(tempptr) 
			strcpy(tmp_icon_commands[i], tempptr);
        else 
            continue;

		sprintf(tmpstr, "icon_args_%d", i);
		tempptr = get_string_conf("General", tmpstr);
		strcpy(tmp_icon_args[i], "");
		if(tempptr)
			strcpy(tmp_icon_args[i], tempptr);

        options.icon_count++;
	}
		
    close_config_file();

}


int main(int argc, char *argv[])
{
	int Quit,ErrorQuit ;
	//int Write = 0;
	unsigned int zipnum;
	unsigned int y;
	int i;

    Uint32 Joypads=0;
    unsigned long keytimer=0;
    int keydirection=0, last_keydirection=0;
        
    //Set the cwd directory to where the binary is
    realpath(argv[0], abspath);
    char *dirsep = strrchr(abspath, '/');
    if( dirsep != 0 ) *dirsep = 0;
    chdir(abspath);

	while(1)
	{

		num_icons=0;
		posx_diff=PNGSIZE+32;

		pi_initialise();

		initSDL();
		init_title();

		if (num_icons == 0) {
			printf("\n\n      No Games found!!!\n");
			exit_prog();	
		}

		dispmanx_init();

		Quit = 0;
		while (!Quit)
		{

			//Perform the smooth transition to the different
		    //icon positions
			if(next_icon != current_icon) {
				if(next_icon > current_icon) {
					//current_icon_pos-=8*scalesize;
					current_icon_pos-=16*scalesize;
					if(current_icon_pos <= -(posx_diff)) {
						current_icon = next_icon;
						current_icon_pos = 0;
					}
				}
				if(next_icon < current_icon) {
					//current_icon_pos+=8*scalesize;
					current_icon_pos+=16*scalesize;
					if(current_icon_pos >= posx_diff) {
						current_icon = next_icon;
						current_icon_pos = 0;
					}
				}
			}
			
			for(i=0;i<num_icons;i++) {
				icon_posx[i] = middle_position + (i*posx_diff) - (current_icon*posx_diff) + current_icon_pos;
			}

			dispmanx_display();

	        while(next_icon == current_icon)
	        {
	            
	            fe_ProcessEvents();
	            Joypads = pi_joystick_read();
	            
	            last_keydirection=keydirection;
	            keydirection=0;
	            
	            //Any keyboard key pressed?
	            if(Joypads & GP2X_LEFT || Joypads & GP2X_RIGHT ||
	               Joypads & GP2X_UP || Joypads & GP2X_DOWN)
	            {
	                keydirection=1;
	                break;
	            }

	            //Game selected
	            //if(Joypads & GP2X_START || Joypads & GP2X_B) break;
				if(Joypads) break;
	            
	            //Used to delay the initial key press, but
	            //once pressed and held the delay will clear

			//Commented out, can't remember what this does, but it jerks the opening animation
	     //       usleep(10000);
	            
			dispmanx_display();
	        }
	        
	        if (Joypads & GP2X_RIGHT && next_icon == current_icon) {
				next_icon++;
				if(next_icon >= num_icons) next_icon = num_icons-1;
	        }
	        
	        if (Joypads & GP2X_LEFT && next_icon == current_icon) {
				next_icon--;
				if(next_icon < 0) next_icon = 0;
	        }
	
			if(!options.kiosk_mode) {
				if (Joypads & GP2X_SELECT || Joypads & GP2X_ESCAPE) {
					zoomspeed=-zoomspeed*2;
					while(zoom > 1) dispmanx_display();
					exit_prog();
	           	 Quit=1;
	        	}
			}
	                
	        if (Joypads & GP2X_A || Joypads & GP2X_START){
				Quit=1;
				ss_prog_run();
			}
	                
		}
	
	}

	return 0;
}

static void fe_ProcessEvents (void)
{
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_JOYBUTTONDOWN:
                joy_buttons[event.jbutton.which][event.jbutton.button] = 1;
                break;
            case SDL_JOYBUTTONUP:
                joy_buttons[event.jbutton.which][event.jbutton.button] = 0;
                break;
            case SDL_JOYAXISMOTION:
                if(event.jaxis.axis == joyaxis_LR) {
                    if(event.jaxis.value > -10000 && event.jaxis.value < 10000)
                        joy_axes[event.jbutton.which][joyaxis_LR] = CENTER;
                    else if(event.jaxis.value > 10000)
                        joy_axes[event.jbutton.which][joyaxis_LR] = RIGHT;
                    else
                        joy_axes[event.jbutton.which][joyaxis_LR] = LEFT;
                }
                if(event.jaxis.axis == joyaxis_UD) {
                    if(event.jaxis.value > -10000 && event.jaxis.value < 10000)
                        joy_axes[event.jbutton.which][joyaxis_UD] = CENTER;
                    else if(event.jaxis.value > 10000)
                        joy_axes[event.jbutton.which][joyaxis_UD] = DOWN;
                    else
                        joy_axes[event.jbutton.which][joyaxis_UD] = UP;
                }
                break;

            case SDL_JOYHATMOTION:
              switch(event.jhat.value) {
              case SDL_HAT_CENTERED:
                joy_axes[event.jbutton.which][joyaxis_LR] = CENTER;
                joy_axes[event.jbutton.which][joyaxis_UD] = CENTER;
                break;
              case SDL_HAT_LEFT:
                joy_axes[event.jbutton.which][joyaxis_LR] = LEFT;
                joy_axes[event.jbutton.which][joyaxis_UD] = CENTER;
                break;
              case SDL_HAT_RIGHT:
                joy_axes[event.jbutton.which][joyaxis_LR] = RIGHT;
                joy_axes[event.jbutton.which][joyaxis_UD] = CENTER;
                break;
              case SDL_HAT_RIGHTUP:
                joy_axes[event.jbutton.which][joyaxis_LR] = RIGHT;
                joy_axes[event.jbutton.which][joyaxis_UD] = UP;
                break;
              case SDL_HAT_LEFTUP:
                joy_axes[event.jbutton.which][joyaxis_LR] = LEFT;
                joy_axes[event.jbutton.which][joyaxis_UD] = UP;
                break;
              case SDL_HAT_RIGHTDOWN:
                joy_axes[event.jbutton.which][joyaxis_LR] = RIGHT;
                joy_axes[event.jbutton.which][joyaxis_UD] = DOWN;
                break;
              case SDL_HAT_LEFTDOWN:
                joy_axes[event.jbutton.which][joyaxis_LR] = LEFT;
                joy_axes[event.jbutton.which][joyaxis_UD] = DOWN;
                break;
              }

            case SDL_KEYDOWN:
                sdl_keys = SDL_GetKeyState(NULL);
                break;
            case SDL_KEYUP:
                sdl_keys = SDL_GetKeyState(NULL);
                break;
        }
    }

}

unsigned long pi_joystick_read(void)
{
    unsigned long val=0;

    if (joy_buttons[0][pi_joy[A_1]])       val |= GP2X_A;
    if (joy_buttons[0][pi_joy[START_1]])   val |= GP2X_START;
    if (joy_buttons[0][pi_joy[SELECT_1]])  val |= GP2X_SELECT;
	if (joy_axes[0][joyaxis_UD] == UP)         val |= GP2X_UP;
	if (joy_axes[0][joyaxis_UD] == DOWN)       val |= GP2X_DOWN;
	if (joy_axes[0][joyaxis_LR] == LEFT)       val |= GP2X_LEFT;
	if (joy_axes[0][joyaxis_LR] == RIGHT)      val |= GP2X_RIGHT;

        // DPAD special buttons
    if (joy_buttons[0][pi_joy[LEFT_1]])   val |= GP2X_LEFT;
    if (joy_buttons[0][pi_joy[RIGHT_1]])  val |= GP2X_RIGHT;
    if (joy_buttons[0][pi_joy[UP_1]])     val |= GP2X_UP;
    if (joy_buttons[0][pi_joy[DOWN_1]])   val |= GP2X_DOWN;

    if(sdl_keys)
    {
        if (sdl_keys[pi_key[A_1]] == SDL_PRESSED)       val |= GP2X_A;
        if (sdl_keys[pi_key[START_1]] == SDL_PRESSED)   val |= GP2X_START;
        if (sdl_keys[pi_key[SELECT_1]] == SDL_PRESSED)  val |= GP2X_SELECT;
        if (sdl_keys[pi_key[UP_1]] == SDL_PRESSED)      val |= GP2X_UP;
        if (sdl_keys[pi_key[DOWN_1]] == SDL_PRESSED)    val |= GP2X_DOWN;
        if (sdl_keys[pi_key[LEFT_1]] == SDL_PRESSED)    val |= GP2X_LEFT;
        if (sdl_keys[pi_key[RIGHT_1]] == SDL_PRESSED)   val |= GP2X_RIGHT;
        if (sdl_keys[pi_key[QUIT]] == SDL_PRESSED)      val |= GP2X_ESCAPE;
    }

    return(val);
}

DISPMANX_DISPLAY_HANDLE_T dx_display;
DISPMANX_UPDATE_HANDLE_T dx_update;

DISPMANX_RESOURCE_HANDLE_T dx_resource_bg;
DISPMANX_ELEMENT_HANDLE_T dx_element_bg;
DISPMANX_RESOURCE_HANDLE_T dx_resource_bg2;
DISPMANX_ELEMENT_HANDLE_T dx_element_bg2;

DISPMANX_RESOURCE_HANDLE_T dx_icon[MAXICONS];
DISPMANX_ELEMENT_HANDLE_T dx_element[MAXICONS];

static void initSDL(void)
{
	//Initialise everything SDL
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0 )
    {
        printf("Could not initialize SDL(%s)\n", SDL_GetError());
        exit(1);
    }

    SDL_SetVideoMode(0, 0, 16, SDL_SWSURFACE);
	SDL_JoystickOpen(0);

    SDL_EventState(SDL_ACTIVEEVENT,SDL_IGNORE);
    SDL_EventState(SDL_SYSWMEVENT,SDL_IGNORE);
    SDL_EventState(SDL_VIDEORESIZE,SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT,SDL_IGNORE);
    SDL_ShowCursor(SDL_DISABLE);
}

static void dispmanx_init(void)
{
    int ret;
	int i;
    uint32_t crap;
    unsigned char filename[65535];
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    bcm_host_init();

    graphics_get_display_size(0 /* LCD */, &display_width, &display_height);

	//Screen is based on a 480 height screen, scale up for anything else
	scalesize = (float)(display_height/480) * (float) 0.75;
	iconsize = PNGSIZE*scalesize;
	posx_diff = posx_diff*scalesize;

	//Find vertical centre and centre for an icon
	middle_position = (display_width/2)-(iconsize/2);
	posy = (display_height/2)-(iconsize/2);

	for(i=0;i<num_icons;i++) {
		icon_posx[i] = middle_position + (i*posx_diff) - (current_icon*posx_diff) + current_icon_pos;
	}

    dx_display = vc_dispmanx_display_open( 0 );

	//Read the PNG files and write to the dispmanx resources (surfaces)
    vc_dispmanx_rect_set( &dst_rect, 0, 0, PNGSIZE, PNGSIZE );
	for(i=0;i<num_icons;i++) {
        sprintf(filename, "./ICON%d.png", i);
        if(!loadPNG(&image, filename, PNGSIZE, PNGSIZE)) {
            printf("\n\n\n ERROR: ICON %d read error!\n",i);
            exit_prog();
        }
        dx_icon[i] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, PNGSIZE, PNGSIZE, &crap);
        vc_dispmanx_resource_write_data( dx_icon[i], VC_IMAGE_RGBA32, image.pitch, image.buffer, &dst_rect );
        freePNG(&image);
	}

    vc_dispmanx_rect_set( &src_rect, 0, 0, PNGSIZE << 16, PNGSIZE << 16);

    dx_update = vc_dispmanx_update_start( 0 );

    //PNGs use alpha for mask
    VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 255, 0 };

    // draw icons to screen
	for(i=0;i<num_icons;i++) {
		vc_dispmanx_rect_set( &dst_rect, icon_posx[i], posy, 1, 1);
    	dx_element[i] = vc_dispmanx_element_add(  dx_update,
							dx_display, 10, &dst_rect, dx_icon[i], &src_rect,
							DISPMANX_PROTECTION_NONE, &alpha, 0, (DISPMANX_TRANSFORM_T) 0 );
	}

///////////////////////
//    //Background image to cover whole screen
//#define SCRX 1024
//#define SCRY 768
//    dx_resource_bg2 = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, SCRX, SCRY, &crap);
//
//    vc_dispmanx_rect_set( &dst_rect, 0, 0, SCRX, SCRY );
//    loadPNG(&image, "scr.png", SCRX, SCRY);
//    vc_dispmanx_resource_write_data( dx_resource_bg2, VC_IMAGE_RGBA32, image.pitch, image.buffer, &dst_rect );
//    freePNG(&image);
//
//    vc_dispmanx_rect_set( &src_rect, 0, 0, SCRX<< 16, SCRY<<16);
//    vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width, display_height );
//    dx_element_bg2 = vc_dispmanx_element_add(  dx_update, dx_display, 5,
//                               &dst_rect, dx_resource_bg2, &src_rect,
//                               DISPMANX_PROTECTION_NONE, 0, 0, (DISPMANX_TRANSFORM_T) 0 );


    //White background layer to cover whole screen
    //Match element display resolution to avoid expensive stretching
    dx_resource_bg = vc_dispmanx_resource_create(VC_IMAGE_RGB565, display_width, display_height, &crap);

    //Just write white values to memory area
    unsigned char *tmpbitmap=malloc(display_width*display_height*2);
    memset(tmpbitmap, 255, display_width*display_height*2);
    vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width, display_height );
    vc_dispmanx_resource_write_data( dx_resource_bg, VC_IMAGE_RGB565, display_width*2, tmpbitmap, &dst_rect );
    free(tmpbitmap);

    vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width, display_height );
    vc_dispmanx_rect_set( &src_rect, 0, 0, display_width << 16, display_height << 16);
    dx_element_bg = vc_dispmanx_element_add(  dx_update, dx_display, 1,
                                          &dst_rect, dx_resource_bg, &src_rect,
                                          DISPMANX_PROTECTION_NONE, 0, 0, (DISPMANX_TRANSFORM_T) 0 );


	//Update the display
    ret = vc_dispmanx_update_submit_sync( dx_update );

}

static void dispmanx_deinit(void)
{
    int ret, i;

    dx_update = vc_dispmanx_update_start( 0 );
	for(i=0;i<num_icons;i++) {
    	ret = vc_dispmanx_element_remove( dx_update, dx_element[i] );
	}
    ret = vc_dispmanx_element_remove( dx_update, dx_element_bg );
    ret = vc_dispmanx_update_submit_sync( dx_update );
    ret = vc_dispmanx_resource_delete( dx_resource_bg );
	for(i=0;i<num_icons;i++) {
    	ret = vc_dispmanx_resource_delete( dx_icon[i] );
	}
    ret = vc_dispmanx_display_close( dx_display );

	bcm_host_deinit();
}


#define ELEMENT_CHANGE_DEST_RECT      (1<<2)

static void dispmanx_display(void)
{
	int i;
    int dst_x=0, dst_y=0;
	int32_t rc;
    VC_RECT_T src_rect, dst_rect;

    // begin display update
    dx_update = vc_dispmanx_update_start( 0 );

	//Move each icon as required
	for (i=0;i<num_icons;i++) {

		dst_x=icon_posx[i]+((iconsize-zoom)/2);
		dst_y=posy+((iconsize-zoom)/2);

		vc_dispmanx_rect_set( &src_rect, 0, 0, PNGSIZE << 16, PNGSIZE << 16);
    	vc_dispmanx_rect_set( &dst_rect, dst_x, dst_y, zoom, zoom );

        //If icon is not visible on left then move off right of screen
		if(dst_x <= -zoom){
    		vc_dispmanx_rect_set( &dst_rect, display_width+1, 0, PNGSIZE, PNGSIZE );
		}

    	rc = vc_dispmanx_element_change_attributes(
				dx_update, 
				dx_element[i], 
				ELEMENT_CHANGE_DEST_RECT,
				0, 
				0xff, 
				&dst_rect, 
				&src_rect, 
				0, 
				(DISPMANX_TRANSFORM_T) 0 );
	}

	if (zoom <= iconsize) zoom += (zoomspeed*scalesize);
	if (zoom > iconsize) zoom = iconsize;

    vc_dispmanx_update_submit_sync( dx_update );
}


#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x) ((x + 15) & ~15)
#endif

int loadPNG(IMAGE_T* image, const char *file, int sizex, int sizey)
{
    int width, height;

    FILE* fpin = fopen(file, "rb");

    if (fpin == NULL)
    {
        fprintf(stderr, "loadPNG: can't open file %s\n",file);
        return FALSE;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL) { return FALSE; }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL)
    {
        png_destroy_read_struct(&png_ptr, 0, 0);
        return FALSE;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        return FALSE;
    }

    png_init_io(png_ptr, fpin);
    png_read_info(png_ptr, info_ptr);

    png_byte colour_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    width =  png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);

    image->width = width;
    image->height =  height;
    if (width != sizex || height != sizey)
    {
        fprintf(stderr, "ERROR: PNG wrong size\n");
        exit_prog();
    }

    image->pitch = (ALIGN_TO_16(width) * 32) / 8;
    image->size = image->pitch * ALIGN_TO_16(height);
    image->buffer = calloc(1, image->size);

    if (image->buffer == NULL)
    {
        fprintf(stderr, "ERROR: memory exhausted\n");
        exit_prog();
    }

    png_read_update_info(png_ptr, info_ptr);
    png_bytepp row_pointers = malloc(image->height * sizeof(png_bytep));

    png_uint_32 j = 0;
    for (j = 0 ; j < (png_uint_32) image->height ; ++j)
    {
        row_pointers[j] = image->buffer + (j * image->pitch);
    }

    png_read_image(png_ptr, row_pointers);

    fclose(fpin);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    return TRUE;
}

void freePNG(IMAGE_T* image)
{
    free (image->buffer);
}
