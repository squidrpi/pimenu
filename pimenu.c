/*  CAPEX for FBA2X

    Copyright (C) 2007  JyCet
	Copyright (C) 2008 Juanvvc. Adapted from capex for cps2emu by Jycet
	Copyright (C) 2013 Squid. Rewritten for Raspberry Pi.

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

SDL_Surface *icon1;
SDL_Surface *icon2;
SDL_Surface *icon3;
SDL_Surface *tmp_icon;

int i;
unsigned int ii;
char g_string[255];
//char ar;
//char * path;
//unsigned char flag_save;
//unsigned char offset_x , offset_y ;

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

int posx_diff=192+25;
int posy=0;
int iconsize=192;
int	scalesize=0;

int current_icon=0;
int current_icon_pos=0;
int next_icon=0;
int middle_position=0;

int icon_posx[20];

char icon_commands[][1000] = {"/usr/local/bin/indiecity/InstalledApps/mame4all_pi/Full/mame", 
							  "/usr/local/bin/indiecity/InstalledApps/pisnes/Full/snes9x.gui",
							  "/usr/local/bin/indiecity/InstalledApps/pifba/Full/fbacapex"};

int num_icons=3;

char abspath[1000];

struct options
{
	unsigned int display_border;
} options;

struct capex
{
	unsigned char list;
	unsigned int sely;
	unsigned int selnum;
	unsigned int seloffset_num;
} capex;

struct selector
{
	unsigned int y;
	unsigned int crt_x;
	unsigned int num;
	unsigned int offset_num;
} selector;

void free_memory(void)
{
	SDL_FreeSurface(icon1); icon1=0;
	SDL_FreeSurface(icon2); icon2=0;
	SDL_FreeSurface(icon3); icon3=0;
}

int exit_prog(void)
{
	//menage avant execution
	free_memory();

	dispmanx_deinit();

	SDL_JoystickClose(0);
	SDL_Quit();

	exit(0);

}


void drawSprite(SDL_Surface* srcSurface, SDL_Surface* dstSurface, int srcX, int srcY, int dstX, int dstY, int width, int height)
{
    int error;

    SDL_Rect srcRect;
    srcRect.x = srcX;
    srcRect.y = srcY;
    srcRect.w = width;
    srcRect.h = height;

    SDL_Rect dstRect;
    dstRect.x = dstX;
    dstRect.y = dstY;
    dstRect.w = width;
    dstRect.h = height;

    error=SDL_BlitSurface(srcSurface, &srcRect, dstSurface, &dstRect);
}

void init_title(void)
{
	FILE *fp;
	unsigned char filename[255];

	strcpy(filename, "./MAME_icon.bmp");
	if ((fp = fopen( filename , "r")) != NULL){
		tmp_icon = SDL_LoadBMP( filename );
		fclose(fp);
		//Convert image to 16bit 565
		icon1 = SDL_CreateRGBSurface(SDL_SWSURFACE, tmp_icon->w, tmp_icon->h, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
		drawSprite( tmp_icon, icon1, 0, 0, 0, 0, 192, 192);
		SDL_FreeSurface(tmp_icon);
		//SDL_SetColorKey(icon1 ,SDL_SRCCOLORKEY,SDL_MapRGB(icon2 ->format,255,0,0));
	}
	
	strcpy(filename, "./SNES_icon.bmp");
	if ((fp = fopen( filename , "r")) != NULL){
		tmp_icon = SDL_LoadBMP( filename );
		fclose(fp);
		//Convert image to 16bit 565
		icon2 = SDL_CreateRGBSurface(SDL_SWSURFACE, tmp_icon->w, tmp_icon->h, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
		drawSprite( tmp_icon, icon2, 0, 0, 0, 0, 192, 192);
		SDL_FreeSurface(tmp_icon);
	}

	strcpy(filename, "./FBA_icon.bmp");
	if ((fp = fopen( filename , "r")) != NULL){
		tmp_icon = SDL_LoadBMP( filename );
		fclose(fp);
		//Convert image to 16bit 565
		icon3 = SDL_CreateRGBSurface(SDL_SWSURFACE, tmp_icon->w, tmp_icon->h, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
		drawSprite( tmp_icon, icon3, 0, 0, 0, 0, 192, 192);
		SDL_FreeSurface(tmp_icon);
	}

}

void ss_prog_run(void)
{
	char arglist[1000];
	FILE *fp2;

	strcpy(arglist, "./fba2x ");
	
	strcat(arglist, g_string);
	strcat(arglist, " ");

//    if (options.showfps){
		strcat(arglist, "--showfps");
		strcat(arglist, " ");
 //   }

    free_memory();
 	dispmanx_deinit();	
	SDL_JoystickClose(0);
    SDL_Quit();

	//Run FBA and wait
	//system(arglist);
	system(icon_commands[current_icon]);

}

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

void pi_initialize_input()
{
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
    pi_key[ACCEL] = get_integer_conf("Keyboard", "ACCEL", RPI_KEY_ACCEL);

    //Configure joysticks from config file or defaults
    pi_joy[A_1] = get_integer_conf("Joystick", "A_1", RPI_JOY_A);
    pi_joy[START_1] = get_integer_conf("Joystick", "START_1", RPI_JOY_START);
    pi_joy[SELECT_1] = get_integer_conf("Joystick", "SELECT_1", RPI_JOY_SELECT);

    pi_joy[QUIT] = get_integer_conf("Joystick", "QUIT", RPI_JOY_QUIT);

	//Read joystick axis to use, default to 0 & 1
	joyaxis_LR = get_integer_conf("Joystick", "JA_LR", 0);
	joyaxis_UD = get_integer_conf("Joystick", "JA_UD", 1);

	options.display_border = get_integer_conf("Graphics", "DisplayBorder", 40);

    close_config_file();

}


int main(int argc, char *argv[])
{
//    sleep(12);
    
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

	pi_initialize_input();

	while(1)
	{
	
		initSDL();
		init_title();
		dispmanx_init();
	
		//sq load_cfg();
		
		Quit = 0;
		while (!Quit)
		{
			dispmanx_display();


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
//	            keytimer = fe_timer_read() + (1000000/2);

	            usleep(10000);
	            
	        }
	        
	        //Key delay
	        //if(keydirection && last_keydirection && fe_timer_read() < keytimer) continue;
//	        if(fe_timer_read() < keytimer) continue;
	        
	        if (Joypads & GP2X_RIGHT && next_icon == current_icon) {
				next_icon++;
				if(next_icon >= num_icons) next_icon = num_icons-1;
	    //        keytimer = fe_timer_read() + (1000000/2);
	        }
	        
	        if (Joypads & GP2X_LEFT && next_icon == current_icon) {
				next_icon--;
				if(next_icon < 0) next_icon = 0;
	     //       keytimer = fe_timer_read() + (1000000/2);
	        }
	
			if (Joypads & GP2X_SELECT || Joypads & GP2X_ESCAPE) {
				exit_prog();
	            Quit=1;
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

    if (joy_buttons[0][pi_joy[L_1]])       val |= GP2X_L;
    if (joy_buttons[0][pi_joy[R_1]])       val |= GP2X_R;
    if (joy_buttons[0][pi_joy[X_1]])       val |= GP2X_X;
    if (joy_buttons[0][pi_joy[Y_1]])       val |= GP2X_Y;
    if (joy_buttons[0][pi_joy[B_1]])       val |= GP2X_B;
    if (joy_buttons[0][pi_joy[A_1]])       val |= GP2X_A;
    if (joy_buttons[0][pi_joy[START_1]])   val |= GP2X_START;
    if (joy_buttons[0][pi_joy[SELECT_1]])  val |= GP2X_SELECT;
	if (joy_axes[0][joyaxis_UD] == UP)         val |= GP2X_UP;
	if (joy_axes[0][joyaxis_UD] == DOWN)       val |= GP2X_DOWN;
	if (joy_axes[0][joyaxis_LR] == LEFT)       val |= GP2X_LEFT;
	if (joy_axes[0][joyaxis_LR] == RIGHT)      val |= GP2X_RIGHT;


    if(sdl_keys)
    {
        if (sdl_keys[pi_key[L_1]] == SDL_PRESSED)       val |= GP2X_L;
        if (sdl_keys[pi_key[R_1]] == SDL_PRESSED)       val |= GP2X_R;
        if (sdl_keys[pi_key[X_1]] == SDL_PRESSED)       val |= GP2X_X;
        if (sdl_keys[pi_key[Y_1]] == SDL_PRESSED)       val |= GP2X_Y;
        if (sdl_keys[pi_key[B_1]] == SDL_PRESSED)       val |= GP2X_B;
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


DISPMANX_RESOURCE_HANDLE_T dx_resource_bg;
DISPMANX_ELEMENT_HANDLE_T dx_element1;
DISPMANX_ELEMENT_HANDLE_T dx_element2;
DISPMANX_ELEMENT_HANDLE_T dx_element3;
DISPMANX_ELEMENT_HANDLE_T dx_element_bg;
DISPMANX_DISPLAY_HANDLE_T dx_display;
DISPMANX_UPDATE_HANDLE_T dx_update;

DISPMANX_RESOURCE_HANDLE_T dx_icon1;
DISPMANX_RESOURCE_HANDLE_T dx_icon2;
DISPMANX_RESOURCE_HANDLE_T dx_icon3;

// these are used for switching between the buffers
static DISPMANX_RESOURCE_HANDLE_T cur_res;
static DISPMANX_RESOURCE_HANDLE_T prev_res;
static DISPMANX_RESOURCE_HANDLE_T tmp_res;

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
    uint32_t display_width=0, display_height=0;

	//Dispmanx default lets black become transparent! This
	//switches that off.
	VC_DISPMANX_ALPHA_T alpha;
	alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	alpha.opacity = 0xFF;
	alpha.mask = 0; 

    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    bcm_host_init();

    graphics_get_display_size(0 /* LCD */, &display_width, &display_height);

	scalesize=(display_height/480);
	iconsize=192*scalesize;
	posx_diff = posx_diff*scalesize;

	middle_position = (display_width/2)-(iconsize/2);
	posy = (display_height/2)-(iconsize/2);

	for(i=0;i<num_icons;i++) {
		icon_posx[i] = middle_position + (i*posx_diff) - (current_icon*posx_diff) + current_icon_pos;
	}

    dx_display = vc_dispmanx_display_open( 0 );

    dx_icon1= vc_dispmanx_resource_create(VC_IMAGE_RGB565, 192, 192, &crap);
    dx_icon2= vc_dispmanx_resource_create(VC_IMAGE_RGB565, 192, 192, &crap);
    dx_icon3= vc_dispmanx_resource_create(VC_IMAGE_RGB565, 192, 192, &crap);

    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );
	SDL_LockSurface(icon1);
    vc_dispmanx_resource_write_data( dx_icon1, VC_IMAGE_RGB565, icon1->pitch, icon1->pixels, &dst_rect );
	SDL_UnlockSurface(icon1);

    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );
	SDL_LockSurface(icon2);
    vc_dispmanx_resource_write_data( dx_icon2, VC_IMAGE_RGB565, icon2->pitch, icon2->pixels, &dst_rect );
	SDL_UnlockSurface(icon2);

    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );
	SDL_LockSurface(icon3);
    vc_dispmanx_resource_write_data( dx_icon3, VC_IMAGE_RGB565, icon3->pitch, icon3->pixels, &dst_rect );
	SDL_UnlockSurface(icon3);

    vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);

    //vc_dispmanx_rect_set( &dst_rect, options.display_border, options.display_border, 
	//					display_width-(options.display_border*2), display_height-(options.display_border*2));

    dx_update = vc_dispmanx_update_start( 0 );

    // draw icon to screen
	vc_dispmanx_rect_set( &dst_rect, icon_posx[0], posy, iconsize, iconsize);
    dx_element1 = vc_dispmanx_element_add(  dx_update,
           dx_display, 10, &dst_rect, dx_icon1, &src_rect,
           DISPMANX_PROTECTION_NONE, &alpha, 0, (DISPMANX_TRANSFORM_T) 0 );

    // draw icon to screen
	vc_dispmanx_rect_set( &dst_rect, icon_posx[1], posy, iconsize, iconsize);
    dx_element2 = vc_dispmanx_element_add(  dx_update,
           dx_display, 10, &dst_rect, dx_icon2, &src_rect,
           DISPMANX_PROTECTION_NONE, &alpha, 0, (DISPMANX_TRANSFORM_T) 0 );

	vc_dispmanx_rect_set( &dst_rect, icon_posx[2], posy, iconsize, iconsize);
    dx_element3 = vc_dispmanx_element_add(  dx_update,
           dx_display, 10, &dst_rect, dx_icon3, &src_rect,
           DISPMANX_PROTECTION_NONE, &alpha, 0, (DISPMANX_TRANSFORM_T) 0 );

	vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width, display_height );
    vc_dispmanx_rect_set( &src_rect, 0, 0, 128 << 16, 128 << 16);

	//White background layer to cover whole screen
    dx_resource_bg = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 128, 128, &crap);
	unsigned char *tmpbitmap=malloc(128*128*2);
	memset(tmpbitmap, 255, 128*128*2); 
    vc_dispmanx_resource_write_data( dx_resource_bg, VC_IMAGE_RGB565, 256, tmpbitmap, &dst_rect );
	free(tmpbitmap);
    dx_element_bg = vc_dispmanx_element_add(  dx_update,
                                          dx_display,
                                          9,
                                          &dst_rect,
                                          dx_resource_bg,
                                          &src_rect,
                                          DISPMANX_PROTECTION_NONE,
                                          0,
                                          0,
                                          (DISPMANX_TRANSFORM_T) 0 );


    ret = vc_dispmanx_update_submit_sync( dx_update );

}

static void dispmanx_deinit(void)
{
    int ret;

    dx_update = vc_dispmanx_update_start( 0 );
    ret = vc_dispmanx_element_remove( dx_update, dx_element1 );
    ret = vc_dispmanx_element_remove( dx_update, dx_element2 );
    ret = vc_dispmanx_element_remove( dx_update, dx_element3 );
    ret = vc_dispmanx_element_remove( dx_update, dx_element_bg );
    ret = vc_dispmanx_update_submit_sync( dx_update );
    ret = vc_dispmanx_resource_delete( dx_resource_bg );
    ret = vc_dispmanx_resource_delete( dx_icon1 );
    ret = vc_dispmanx_resource_delete( dx_icon2 );
    ret = vc_dispmanx_resource_delete( dx_icon3 );
    ret = vc_dispmanx_display_close( dx_display );

	bcm_host_deinit();
}


#define ELEMENT_CHANGE_DEST_RECT      (1<<2)

static void dispmanx_display(void)
{
	int i;
	int32_t rc;
    VC_RECT_T src_rect, dst_rect;
	uint32_t change_flags;
 	change_flags = ELEMENT_CHANGE_DEST_RECT;

	vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);

    // begin display update
    dx_update = vc_dispmanx_update_start( 0 );

    vc_dispmanx_rect_set( &dst_rect, icon_posx[0], posy, iconsize, iconsize );
    rc = vc_dispmanx_element_change_attributes(dx_update, dx_element1, change_flags,
    	0, 0xff, &dst_rect, &src_rect, 0, (DISPMANX_TRANSFORM_T) 0 );

    vc_dispmanx_rect_set( &dst_rect, icon_posx[1], posy, iconsize, iconsize );
	rc = vc_dispmanx_element_change_attributes(dx_update, dx_element2, change_flags,
		0, 0xff, &dst_rect, &src_rect, 0, (DISPMANX_TRANSFORM_T) 0 );

    vc_dispmanx_rect_set( &dst_rect, icon_posx[2], posy, iconsize, iconsize );
	rc = vc_dispmanx_element_change_attributes(dx_update, dx_element3, change_flags,
		0, 0xff, &dst_rect, &src_rect, 0, (DISPMANX_TRANSFORM_T) 0 );

    vc_dispmanx_update_submit_sync( dx_update );

}
