
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL.h>
#include <glib.h>
#include <bcm_host.h>

#include "pibounce.h"
#include "keyconstants.h"

SDL_Event event;

#define MAXICONS 23

SDL_Surface *icon[MAXICONS];
SDL_Surface *tmp_icon;

//Actual number icons being displayed
int num_icons=MAXICONS;

int i;
unsigned int ii;
char g_string[255];

unsigned char *sdl_keys;

static void fe_ProcessEvents (void);
unsigned long pi_joystick_read(void);

static void dispmanx_init(void);
static void dispmanx_deinit(void);
static void dispmanx_display(void);
static void initSDL(void);

#define NUMKEYS 256
static Uint16 pi_key[NUMKEYS];

int icon_x=0;

int iconsize=192;
int	scalesize=1;

int current_icon=0;
int current_icon_pos=0;
int next_icon=0;

int icon_posx[MAXICONS];
int icon_posy[MAXICONS];
int icon_diffx[MAXICONS];
int icon_diffy[MAXICONS];

uint32_t display_width=0, display_height=0;

Uint32 Joypads=0;

int exit_prog(void)
{
	dispmanx_deinit();

	SDL_JoystickClose(0);
	SDL_Quit();

	exit(0);
}

void init_title(void)
{
	tmp_icon = SDL_LoadBMP( "./ICON0.bmp" );
	//Convert image to 16bit 565 ready for dispmanx Surface
	icon[0] = SDL_CreateRGBSurface(SDL_SWSURFACE, tmp_icon->w, tmp_icon->h, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
	SDL_BlitSurface(tmp_icon, NULL,  icon[0], NULL);
	SDL_FreeSurface(tmp_icon);
}

void pi_initialise()
{
	int i;
	
    memset(pi_key, 0, NUMKEYS*2);

	pi_key[QUIT] = RPI_KEY_QUIT;

}


int main(int argc, char *argv[])
{
//    sleep(20);
    
	int Quit,ErrorQuit ;
	unsigned int y;
	int i;

	while(1)
	{

		pi_initialise();

		initSDL();
		init_title();
	
		srand(time(NULL));

		for(i=0;i<MAXICONS;i++) {
			icon_posx[i] = rand() / (RAND_MAX / 450) + 10;
			icon_posy[i] = rand() / (RAND_MAX / 250) + 10;
			icon_diffx[i] = (rand() / (RAND_MAX / 5))+2;
			icon_diffy[i] = (rand() / (RAND_MAX / 5))+2;
			if(rand() > (RAND_MAX / 2)) icon_diffx[i] = -icon_diffx[i];
			if(rand() > (RAND_MAX / 2)) icon_diffy[i] = -icon_diffy[i];
		}

		dispmanx_init();

		Quit = 0;
		while (!Quit)
		{
			for(i=0;i<MAXICONS;i++) {
				icon_posx[i] = icon_posx[i] + icon_diffx[i];
				icon_posy[i] = icon_posy[i] + icon_diffy[i];
				if(icon_posx[i] >= ((int)display_width-iconsize) || icon_posx[i] <= 10) icon_diffx[i] = -icon_diffx[i];
				if(icon_posy[i] >= ((int)display_height-iconsize) || icon_posy[i] <= 10) icon_diffy[i] = -icon_diffy[i];
			}

			dispmanx_display();

	        fe_ProcessEvents();
	        Joypads = pi_joystick_read();
	            
	        //usleep(10000);
	        
			if (Joypads & GP2X_SELECT || Joypads & GP2X_ESCAPE) {
				exit_prog();
	        	Quit=1;
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

    if(sdl_keys)
    {
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


    bcm_host_init();

    graphics_get_display_size(0 /* LCD */, &display_width, &display_height);

	//Screen is based on a 480 height screen, scale up for anything else
//	scalesize = (display_height/480);
	iconsize = 192*scalesize;

    dx_display = vc_dispmanx_display_open( 0 );

	//Write the SDL surface bitmaps to the dispmanx resources (surfaces) and free the SDL surface
    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );
	for(i=0;i<MAXICONS;i++) {
    	dx_icon[i] = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 192, 192, &crap);
		SDL_LockSurface(icon[0]);
    	vc_dispmanx_resource_write_data( dx_icon[i], VC_IMAGE_RGB565, icon[0]->pitch, icon[0]->pixels, &dst_rect );
		SDL_UnlockSurface(icon[0]);
	}
	SDL_FreeSurface(icon[0]);

    vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);

    dx_update = vc_dispmanx_update_start( 0 );

	//Dispmanx default lets black become transparent! This switches that off.
	VC_DISPMANX_ALPHA_T alpha;
	alpha.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_PREMULT;
	alpha.opacity = 0xFF;
	alpha.mask = 0; 

    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );

    // draw icons to screen
	for(i=0;i<MAXICONS;i++) {
		vc_dispmanx_rect_set( &dst_rect, icon_posx[i], icon_posy[i], iconsize, iconsize);
    	dx_element[i] = vc_dispmanx_element_add( dx_update,
				dx_display, 2+i, &dst_rect, dx_icon[i], &src_rect,
				DISPMANX_PROTECTION_NONE, &alpha, 0, 0);
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
	for(i=0;i<MAXICONS;i++) {
    	ret = vc_dispmanx_element_remove( dx_update, dx_element[i] );
	}
    ret = vc_dispmanx_element_remove( dx_update, dx_element_bg );
    ret = vc_dispmanx_update_submit_sync( dx_update );
    ret = vc_dispmanx_resource_delete( dx_resource_bg );
	for(i=0;i<MAXICONS;i++) {
    	ret = vc_dispmanx_resource_delete( dx_icon[i] );
	}
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

	vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);

    // begin display update
    dx_update = vc_dispmanx_update_start( 0 );

	//Move the icons if required
 	change_flags = ELEMENT_CHANGE_DEST_RECT;

	for(i=0;i<MAXICONS;i++) {
    	vc_dispmanx_rect_set( &dst_rect, icon_posx[i], icon_posy[i], iconsize, iconsize );
    	rc = vc_dispmanx_element_change_attributes(dx_update, dx_element[i], change_flags,
    		0, 0xff, &dst_rect, &src_rect, 0, (DISPMANX_TRANSFORM_T) 0 );
	}

    vc_dispmanx_update_submit_sync( dx_update );
}
