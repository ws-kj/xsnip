/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <png.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "config.h"

Display* display;
Window   root;
Window   overlay;
XImage*  img;
Cursor   cursor;
Pixmap 	 pm;
GC 	     gc;
#if OPAQUE
GC 		 empty_gc;
#endif

char*    fpath;
uint8_t* buffer;
uint8_t* keymap;

int32_t exit_clean(char* err) {	
	if(img) XDestroyImage(img);
	if(pm)  XFreePixmap(display, pm);
	XUnmapWindow(display, overlay);
	XFreeCursor(display, cursor);
	XFreeGC(display, gc);
	#if OPAQUE
	XFreeGC(display, empty_gc);
	#endif

	if(fpath)  free(fpath);
	if(buffer) free(buffer);
	if(keymap) free(keymap);
	XCloseDisplay(display);

	if(err) {
		fprintf(stderr, "%s", err);
		return 1;
	}
	return 0;
}

// Pretty ugly but should be safe.
int32_t create_filename(bool save, char** ts) {
	time_t cur = time(NULL);

	if(save) {
		const char* home;
		if((home = getenv("HOME")) == NULL) {
			home = getpwuid(getuid())->pw_dir;
		}

		// 29 = ctime-4 + .png + \0
		uint32_t tsize = strlen(home) + strlen(SAVEDIR) + 29;		
		*ts = malloc(sizeof(char) * tsize);
		if(!*ts) 
			return exit_clean("Could not allocate filename\n");

		strncpy(*ts, home, strlen(home));
		strncat(*ts, SAVEDIR, strlen(SAVEDIR)+1);
	} else {
		// 34 = ctime-4 + /tmp/ + .png + \0
		*ts = malloc(sizeof(char) * 34);
		if(!*ts) 
			return exit_clean("Could not allocate filename\n");
		strncpy(*ts, "/tmp/", 6);
	}

	// intentionally cut off the last 4 bytes of ctime() 
	strncat(*ts, ctime(&cur), 24);

	strncat(*ts, ".png", 6);

	return 0;
}

int32_t create_png(uint8_t* buffer, uint32_t width, uint32_t height, bool save, char* ts) {
	printf("creating %s (save=%d)\n", ts, save);

	FILE *fp = fopen(ts, "wb");
	if(!fp) 
		return exit_clean("Could not open png for writing\n");

	png_bytep row_pointers[height];
	for(uint32_t i = 0; i < height; i++) {
		row_pointers[i] = (buffer + i * width * 3);
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) 
		return exit_clean("Could not create png write struct\n");

	png_infop info = png_create_info_struct(png);
	if (!info) 
		return exit_clean("Could not create png info struct\n");

	if(setjmp(png_jmpbuf(png)))
		return exit_clean("Could not set png jmpbuf\n");

	png_init_io(png, fp);

	png_set_IHDR(
		png, info, width, height, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);
	png_write_info(png, info);

	png_write_image(png, row_pointers);
	png_write_end(png, info);

	fclose(fp);
	png_destroy_write_struct(&png, &info);

	return 0;	
}

int main(int argc, char** argv) {
	display = XOpenDisplay(NULL);
	root = DefaultRootWindow(display);

	XWindowAttributes gwa;
	XGetWindowAttributes(display, root, &gwa);	
	XVisualInfo vinfo;

	// If there are compositor issues (black screen or blur) we can
	// create a 24 bit image and paint a temporary screenshot onto it
	// to achieve the same effect as a fully transparent 32 bit image.
	#if OPAQUE
		XMatchVisualInfo(display, DefaultScreen(display), 24, TrueColor, &vinfo);
	#else
		XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &vinfo);
	#endif

	XSetWindowAttributes attrs;
	attrs.override_redirect = true;
	attrs.colormap = XCreateColormap(display, root, vinfo.visual, AllocNone);
	attrs.background_pixel = 0;
	attrs.border_pixel = 0;

	// This is somewhat scuffed but we need to send an exposure event
	// otherwise the temp screenshot will sometimes be blank.
	#if OPAQUE
		XSelectInput(display, root, ExposureMask);
		XEvent evt;
		XSendEvent(display, root, true, ExposureMask, &evt);
		img = XGetImage(display, root, 0, 0, gwa.width, gwa.height, AllPlanes, ZPixmap);	
	#endif

	// We create a transparent window so that drawing the box
	// doesn't mess with running programs.  
	overlay = XCreateWindow(
		display, root,
		0, 0, gwa.width, gwa.height, 0,
		vinfo.depth, InputOutput,
		vinfo.visual,
		CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel,
		&attrs);

	// Must be done after overlay is created
	#if OPAQUE
		pm = XCreatePixmap(display, overlay, gwa.width, gwa.height, 24);
		empty_gc = XCreateGC(display, overlay, 0, 0);
		XPutImage(display, overlay, empty_gc, img, 0, 0, 0, 0, gwa.width, gwa.height);	
	#endif

	XMapWindow(display, overlay);

	XGCValues gcval;
	gcval.foreground = XWhitePixel(display, 0);
	gcval.function = GXxor;
	gcval.background = XBlackPixel(display, 0);
	gcval.plane_mask = gcval.background ^ gcval.foreground;
	gcval.subwindow_mode = IncludeInferiors;
	gc = XCreateGC(display, overlay, GCFunction | GCForeground | GCBackground | GCSubwindowMode, &gcval);

	cursor = XCreateFontCursor(display, CURSOR);
	XDefineCursor(display, overlay, cursor);

	uint32_t wx, wy;		// Junk
	Window cw, rw;			// Junk

	uint32_t startx, starty, endx, endy, temp;
	bool grabbing = false;
	uint32_t mousex, mousey, mask;
	uint32_t bsx, bsy, bex, bey;

	#if OPAQUE
		XSelectInput(display, overlay, ExposureMask);
	#endif

	bool save = false;
	keymap = malloc(sizeof(uint8_t) * 32);
	for(;;) {	
		// Safe exit on keypress instead of forcing user to take a
		// screenshot with no width/height.
		XQueryKeymap(display, keymap);
		KeyCode kc = XKeysymToKeycode(display, KQUIT);
		bool pressed = !!(keymap[kc>>3] & (1<<(kc&7))); 
		if(pressed) return exit_clean(NULL);

		XQueryPointer(display, root, &cw, &rw,
			&mousex, &mousey, &wx, &wy, &mask);

		if(mask == 256) {			// Left click
			if(!grabbing) {
				save = false;
				grabbing = true;
				startx = mousex;
				starty = mousey;
			} 
		} else if(mask == 1024) {	// Right click
			if(!grabbing) {
				save = true;
				grabbing = true;
				startx = mousex;
				starty = mousey;
			} 
		} else {
			if(grabbing) {
				grabbing = false;
				endx = mousex;
				endy = mousey;
				break;
			}
		}

		#if OPAQUE
			XPutImage(display, pm, empty_gc, img, 0, 0, 0, 0, gwa.width, gwa.height);	
		#else
			XClearArea(display, overlay, 0, 0, gwa.width, gwa.height, false);
		#endif

		if(grabbing) {
			// Due to the way XDrawRectange works, we always need to
			// pass the upper lefthand corner first.
			bsx = (mousex > startx) ? startx-1 : mousex-1;
			bsy = (mousey > starty) ? starty-1 : mousey-1;

			bex = (mousex > startx) ? mousex-startx+3 : startx-mousex+3;
			bey = (mousey > starty) ? mousey-starty+3 : starty-mousey+1;
			#if OPAQUE
				XDrawRectangle(display, pm, gc, bsx, bsy, bex, bey);
			#else
				XDrawRectangle(display, overlay, gc, bsx, bsy, bex, bey);
			#endif
		}

		#if OPAQUE
			XClearArea(display, overlay, 0, 0, gwa.width, gwa.height, false);
			XSetWindowBackgroundPixmap(display, overlay, pm);
		#endif

		XFlush(display);
		usleep(POLLRATE * 1000); // Experiment as needed.
	}

	// More corner flipping.
	if(startx > endx) {
		temp = startx;
		startx = endx;
		endx = temp;
	}
	if(starty > endy) {
		temp = starty;
		starty = endy;
		endy = temp;
	}

	uint32_t width = endx+1-startx;
	uint32_t height = endy-starty;
	if(width == 0 || height == 0) return exit_clean(NULL);

	img = XGetImage(display, root, 0, 0, gwa.width, gwa.height, AllPlanes, ZPixmap);
	uint32_t rmask = img->red_mask;
	uint32_t gmask = img->green_mask;
	uint32_t bmask = img->blue_mask;

	uint8_t* buffer = malloc(sizeof(uint8_t) * width * height * 3);
	if(!buffer) {
		return exit_clean("Could not allocate image buffer\n");
	}

	// Each pixel is encoded as an integer with colors at bit offset.
	uint32_t c = 0;
	for(uint32_t h = starty+1; h < endy+1; h++) {
		for(uint32_t w = startx+1; w < endx+2; w++) {
			uint32_t pix = XGetPixel(img, w, h);
			uint8_t r = (pix & rmask) >> 16;
			uint8_t g = (pix & gmask) >> 8;
			uint8_t b = pix & bmask;

			buffer[c++] = r;
			buffer[c++] = g;
			buffer[c++] = b;
		}
	} 

	create_filename(save, &fpath);

	int32_t png = create_png(buffer, width, height, save, fpath); 
	if(png != 0) return png;

	// I would have liked to do a custom clipboard implementation
	// but xlib selection handling has little documentation and
	// many features are completely broken (such as XA_CLIPBOARD).
	if(!save) {
		const char* com = "xclip -selection clipboard -target image/png -i '";
		char* command = malloc(sizeof(char) * (strlen(com) + strlen(fpath) + 2));
		if(!command)
			return exit_clean("Could not allocate command\n");

		strncpy(command, com, strlen(com)+1);
		strncat(command, fpath, strlen(fpath)+1);
		strncat(command, "'", 2);

		system(command);

		free(command);
	}

	exit_clean(NULL);
	return 0;
} 
