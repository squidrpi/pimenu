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
//#include <time.h>
#include <SDL.h>
#include <glib.h>
#include <bcm_host.h>

#include "pimenu.h"
#include "keyconstants.h"

SDL_Event event;

#define MAXICONS 10

SDL_Surface *icon[MAXICONS];
SDL_Surface *tmp_icon;

//Actual number icons being displayed
int num_icons=0;

int i;
unsigned int ii;
char g_string[255];

unsigned char joy_buttons[2][32];
unsigned char joy_axes[2][8];

int joyaxis_LR, joyaxis_UD;

unsigned char *sdl_keys;

static unsigned long fe_timer_read(void);
static void fe_ProcessEvents (void);
unsigned long pi_joystick_read(void);

static void dispmanx_init(void);
static void dispmanx_deinit(void);
static void dispmanx_display(void);
static void initSDL(void);

#define NUMKEYS 256
static Uint16 pi_key[NUMKEYS];
static Uint16 pi_joy[NUMKEYS];

int icon_x=0;

int posx_diff;
int posy=0;
int iconsize=192;
int	scalesize=0;

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
	unsigned char filename[255];
	int i;

	//Scan all the icon commands configured in the cfg file and check to 
    //see if they actually exist. Also check the icons exist, otherwise
	//simple skip them alltogether.
	for(i=0;i<options.icon_count;i++) {

		if ((fp = fopen( tmp_icon_commands[i] , "r")) == NULL) {
			continue;
		}

		//Check icon image exists
		sprintf(filename, "./ICON%d.bmp", i);	
		if ((fp = fopen( filename , "r")) != NULL) {

			tmp_icon = SDL_LoadBMP( filename );
			fclose(fp);
			//Validate read as sometimes gives errors!
			if(tmp_icon->w != 192 || tmp_icon == NULL) {
				printf("\n\n\n     ERROR: ICON %d read error!\n",i);
				exit_prog();
			}
			//Convert image to 16bit 565 ready for dispmanx Surface
			icon[num_icons] = SDL_CreateRGBSurface(SDL_SWSURFACE, tmp_icon->w, tmp_icon->h, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
			SDL_BlitSurface(tmp_icon, NULL,  icon[num_icons], NULL);
			SDL_FreeSurface(tmp_icon);

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
    pi_joy[START_1] = get_integer_conf("Joystick", "START_1", RPI_JOY_START);
    pi_joy[SELECT_1] = get_integer_conf("Joystick", "SELECT_1", RPI_JOY_SELECT);

    pi_joy[QUIT] = get_integer_conf("Joystick", "QUIT", RPI_JOY_QUIT);

	//Read joystick axis to use, default to 0 & 1
	joyaxis_LR = get_integer_conf("Joystick", "JA_LR", 0);
	joyaxis_UD = get_integer_conf("Joystick", "JA_UD", 1);

	options.kiosk_mode = get_integer_conf("General", "kioskmode", 0);	

	//Read icon info from config file
	options.icon_count = get_integer_conf("General", "icon_count", 0);	
	if (options.icon_count == 0) {
		printf("Configuration is incorrect, no icons!\n");
		exit_prog();
	}
	for(i=0;i<options.icon_count;i++) {
		char tmpstr[50], *tempptr;
		sprintf(tmpstr, "icon_command_%d", i);
		tempptr = get_string_conf("General", tmpstr);
		strcpy(tmp_icon_commands[i], "");
		if(tempptr)
			strcpy(tmp_icon_commands[i], tempptr);

		sprintf(tmpstr, "icon_args_%d", i);
		tempptr = get_string_conf("General", tmpstr);
		strcpy(tmp_icon_args[i], "");
		if(tempptr)
			strcpy(tmp_icon_args[i], tempptr);
	}
		
    close_config_file();

}


int main(int argc, char *argv[])
{
//    sleep(20);
    
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
		posx_diff=192+32;

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
					current_icon_pos-=8*scalesize;
					if(current_icon_pos <= -(posx_diff)) {
						current_icon = next_icon;
						current_icon_pos = 0;
					}
				}
				if(next_icon < current_icon) {
					current_icon_pos+=8*scalesize;
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

static unsigned long fe_timer_read(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    
    return ((unsigned long long)now.tv_sec * 1000000LL + (now.tv_nsec / 1000LL));
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
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

	//Dispmanx default lets black become transparent! This switches that off.
	VC_DISPMANX_ALPHA_T alpha;
	alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	alpha.opacity = 0xFF;
	alpha.mask = 0; 

    bcm_host_init();

    graphics_get_display_size(0 /* LCD */, &display_width, &display_height);

	//Screen is based on a 480 height screen, scale up for anything else
	scalesize = (display_height/480);
	iconsize = 192*scalesize;
	posx_diff = posx_diff*scalesize;

	//Find vertical centre and centre for an icon
	middle_position = (display_width/2)-(iconsize/2);
	posy = (display_height/2)-(iconsize/2);

	for(i=0;i<num_icons;i++) {
		icon_posx[i] = middle_position + (i*posx_diff) - (current_icon*posx_diff) + current_icon_pos;
	}

    dx_display = vc_dispmanx_display_open( 0 );

	//Write the SDL surface bitmaps to the dispmanx resources (surfaces) and free the SDL surface
    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );
	for(i=0;i<num_icons;i++) {
    	dx_icon[i] = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 192, 192, &crap);
		SDL_LockSurface(icon[i]);
    	vc_dispmanx_resource_write_data( dx_icon[i], VC_IMAGE_RGB565, icon[i]->pitch, icon[i]->pixels, &dst_rect );
		SDL_UnlockSurface(icon[i]);
		SDL_FreeSurface(icon[i]);
	}

    vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);

    dx_update = vc_dispmanx_update_start( 0 );

    // draw icons to screen
	for(i=0;i<num_icons;i++) {
		vc_dispmanx_rect_set( &dst_rect, icon_posx[i], posy, 1, 1);
    	dx_element[i] = vc_dispmanx_element_add(  dx_update,
							dx_display, 2, &dst_rect, dx_icon[i], &src_rect,
							DISPMANX_PROTECTION_NONE, &alpha, 0, (DISPMANX_TRANSFORM_T) 0 );
	}


	//White background layer to cover whole screen
    dx_resource_bg = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 128, 128, &crap);

	//Just write white values to memory area
	unsigned char *tmpbitmap=malloc(128*128*2);
	memset(tmpbitmap, 255, 128*128*2); 
	vc_dispmanx_rect_set( &dst_rect, 0, 0, 128, 128 );
    vc_dispmanx_resource_write_data( dx_resource_bg, VC_IMAGE_RGB565, 128*2, tmpbitmap, &dst_rect );

	vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width, display_height );
    vc_dispmanx_rect_set( &src_rect, 0, 0, 128 << 16, 128 << 16);
    dx_element_bg = vc_dispmanx_element_add(  dx_update, dx_display, 1,
                                          &dst_rect, dx_resource_bg, &src_rect,
                                          DISPMANX_PROTECTION_NONE, 0, 0, (DISPMANX_TRANSFORM_T) 0 );

	//Update the display
    ret = vc_dispmanx_update_submit_sync( dx_update );

	free(tmpbitmap);

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


#ifndef ELEMENT_CHANGE_LAYER
/* copied from interface/vmcs_host/vc_vchi_dispmanx.h of userland.git */
#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)
#endif

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

		//If none of the icon is on the screen then shift off the right screen
        //as dispmanx won't display it correctly when off the left completely.
		if(dst_x < -zoom){
  			dst_x = 3000;
		}

		vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);
    	vc_dispmanx_rect_set( &dst_rect, dst_x, dst_y, zoom, zoom );

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
