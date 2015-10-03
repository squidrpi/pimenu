
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <png.h>

#include <SDL.h>
#include <glib.h>
#include <bcm_host.h>

#include "pibounce.h"
#include "keyconstants.h"

SDL_Event event;

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
int num_icons=24;
int num_images=0;
int current_image=0;
int next_image=0;

unsigned char *sdl_keys;

static void fe_ProcessEvents (void);
unsigned long pi_joystick_read(void);

static void dispmanx_init(void);
static void dispmanx_deinit(void);
static void dispmanx_display(void);
static void initSDL(void);
int loadPNG(IMAGE_T* image, const char *file);
void freePNG(IMAGE_T* image);

#define NUMKEYS 256
static Uint16 pi_key[NUMKEYS];
static Uint16 pi_joy[NUMKEYS];


int icon_x=0;

int iconsize=192;
int	scalesize=1;

int current_icon=0;
int current_icon_pos=0;
int next_icon=0;

#define MAXICONS 30

int icon_posx[MAXICONS];
int icon_posy[MAXICONS];
int icon_diffx[MAXICONS];
int icon_diffy[MAXICONS];

DISPMANX_DISPLAY_HANDLE_T dx_display;
DISPMANX_UPDATE_HANDLE_T dx_update;

DISPMANX_RESOURCE_HANDLE_T dx_resource[MAXICONS], dx_resource_bg;
DISPMANX_ELEMENT_HANDLE_T dx_element[MAXICONS], dx_element_bg;

uint32_t display_width=0, display_height=0;

Uint32 Joypads=0;
unsigned char joy_buttons[2][32];
int key_down=0;

int exit_prog(void)
{
	dispmanx_deinit();

	SDL_JoystickClose(0);
	SDL_Quit();

	exit(0);
}

void pi_initialise()
{

    memset(joy_buttons, 0, 32*2);
    memset(pi_key, 0, NUMKEYS*2);
    memset(pi_joy, 0, NUMKEYS*2);

    pi_joy[QUIT] = RPI_JOY_QUIT;
    pi_joy[START_1] = RPI_JOY_START;

    memset(pi_key, 0, NUMKEYS*2);

	pi_key[QUIT] = RPI_KEY_QUIT;
    pi_key[START_1] = RPI_KEY_START;
}



int main(int argc, char *argv[])
{
    
	int Quit,ErrorQuit ;
	unsigned int y;
	int i;

    if(argc > 1) {
        num_icons = strtol(argv[1],0,10);
    } 

	while(1)
	{

		pi_initialise();

		initSDL();

		for(i=0;i<num_icons;i++) {
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
			for(i=0;i<num_icons;i++) {
				icon_posx[i] = icon_posx[i] + icon_diffx[i];
				icon_posy[i] = icon_posy[i] + icon_diffy[i];
				if(icon_posx[i] >= ((int)display_width-iconsize) || icon_posx[i] <= 10) icon_diffx[i] = -icon_diffx[i];
				if(icon_posy[i] >= ((int)display_height-iconsize) || icon_posy[i] <= 10) icon_diffy[i] = -icon_diffy[i];
			}

			dispmanx_display();

	        fe_ProcessEvents();
	        Joypads = pi_joystick_read();
	            
	        usleep(10000);
	        
			if (Joypads & GP2X_ESCAPE) {
                printf("\nquit\n");
				exit_prog();
	        	Quit=1;
	       	}

            if (Joypads & GP2X_START && !key_down) {
                next_image++;
                if(next_image >= num_images) next_image=0;

                printf("\nnext %d\n",next_image);

                key_down=1;
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
                key_down=0;
                break;

            case SDL_KEYDOWN:
                sdl_keys = SDL_GetKeyState(NULL);
                break;
            case SDL_KEYUP:
                sdl_keys = SDL_GetKeyState(NULL);
                key_down=0;
                break;
        }
    }

}

unsigned long pi_joystick_read(void)
{
    unsigned long val=0;

    if (joy_buttons[0][pi_joy[QUIT]])  val |= GP2X_ESCAPE;

    if (joy_buttons[0][pi_joy[START_1]])  val |= GP2X_START;


    if(sdl_keys)
    {
        if (sdl_keys[pi_key[QUIT]] == SDL_PRESSED)      val |= GP2X_ESCAPE;
        if (sdl_keys[pi_key[START_1]] == SDL_PRESSED)   val |= GP2X_START;
    }

    return(val);
}

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
    uint32_t vc_image_ptr;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    bcm_host_init();

    graphics_get_display_size(0 /* LCD */, &display_width, &display_height);

	//Screen is based on a 480 height screen, scale up for anything else
//	scalesize = (display_height/480);
	iconsize = 192*scalesize;

    dx_display = vc_dispmanx_display_open( 0 );

    //Load PNGs which have alpha channel and are 192x192 in size
    //Must free PNG after we've copied it

	//Write the PNG bitmap to the dispmanx resources (surfaces)
    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );
	for(i=0;i<MAXICONS;i++) {
        char filename[255];

        sprintf(filename, "./ICON%d.png", i);
        if(!loadPNG(&image, filename)) break;

    	dx_resource[i] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, 192, 192, &vc_image_ptr);
    	vc_dispmanx_resource_write_data( dx_resource[i], VC_IMAGE_RGBA32, image.pitch, image.buffer, &dst_rect );
        freePNG(&image);

        num_images++;
	}

    dx_update = vc_dispmanx_update_start( 0 );

    //PNGs use alpha for mask
    VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 255, 0 };

    vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);
    vc_dispmanx_rect_set( &dst_rect, 0, 0, 192, 192 );

    // draw icons to screen
	for(i=0;i<num_icons;i++) {
		vc_dispmanx_rect_set( &dst_rect, icon_posx[i], icon_posy[i], iconsize, iconsize);
    	dx_element[i] = vc_dispmanx_element_add( dx_update,
				dx_display, 2+i, &dst_rect, dx_resource[current_image], &src_rect,
				DISPMANX_PROTECTION_NONE, &alpha, 0, (DISPMANX_TRANSFORM_T) 0 );
	}

    //White background layer to cover whole screen
    //Match element display resolution to avoid expensive stretching
    dx_resource_bg = vc_dispmanx_resource_create(VC_IMAGE_RGB565, display_width, display_height, &vc_image_ptr);

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
    	ret = vc_dispmanx_resource_delete( dx_resource[num_images] );
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

    // begin display update
    dx_update = vc_dispmanx_update_start( 0 );

	//Move the icons if required
	vc_dispmanx_rect_set( &src_rect, 0, 0, 192 << 16, 192 << 16);

	for(i=0;i<num_icons;i++) {
    	vc_dispmanx_rect_set( &dst_rect, icon_posx[i], icon_posy[i], iconsize, iconsize );
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

        if(current_image != next_image) {
           vc_dispmanx_element_change_source( dx_update, dx_element[i], dx_resource[next_image] );
        }
    
	}

    current_image = next_image;

    vc_dispmanx_update_submit_sync( dx_update );

}


#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x) ((x + 15) & ~15)
#endif

int loadPNG(IMAGE_T* image, const char *file)
{
    int width, height;

    FILE* fpin = fopen(file, "rb");

    if (fpin == NULL)
    {
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
    image->pitch = (ALIGN_TO_16(width) * 32) / 8;
    image->size = image->pitch * ALIGN_TO_16(height);
    image->buffer = calloc(1, image->size);

    if (image->buffer == NULL)
    {
        fprintf(stderr, "image: memory exhausted\n");
        exit(EXIT_FAILURE);
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

void freePNG(IMAGE_T* image )
{
    free (image->buffer);
}
