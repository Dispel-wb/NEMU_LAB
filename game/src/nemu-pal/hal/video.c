#include "hal.h"
#include "device/video.h"
#include "device/palette.h"

#include <string.h>
#include <stdlib.h>

int get_fps();

void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, 
		SDL_Surface *dst, SDL_Rect *dstrect) {
	assert(dst && src);

	int sx = (srcrect == NULL ? 0 : srcrect->x);
	int sy = (srcrect == NULL ? 0 : srcrect->y);
	int dx = (dstrect == NULL ? 0 : dstrect->x);
	int dy = (dstrect == NULL ? 0 : dstrect->y);
	int w = (srcrect == NULL ? src->w : srcrect->w);
	int h = (srcrect == NULL ? src->h : srcrect->h);
	if(dst->w - dx < w) { w = dst->w - dx; }
	if(dst->h - dy < h) { h = dst->h - dy; }
	if(dstrect != NULL) {
		dstrect->w = w;
		dstrect->h = h;
	}

	assert(src->format && dst->format);
	assert(src->format->BitsPerPixel == dst->format->BitsPerPixel);
	int bytes_per_pixel = src->format->BitsPerPixel >> 3;
	assert(bytes_per_pixel > 0);
	assert(sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0);
	if(src->w - sx < w) { w = src->w - sx; }
	if(src->h - sy < h) { h = src->h - sy; }
	if(w <= 0 || h <= 0) return;

	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	int row;
	for(row = 0; row < h; row ++) {
		uint8_t *from = src->pixels + (sy + row) * src->pitch + sx * bytes_per_pixel;
		uint8_t *to = dst->pixels + (dy + row) * dst->pitch + dx * bytes_per_pixel;
		memmove(to, from, w * bytes_per_pixel);
	}
	if(dstrect != NULL) {
		dstrect->w = w;
		dstrect->h = h;
	}
}

void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
	assert(dst);
	assert(color <= 0xff);

	assert(dst->format && dst->format->BitsPerPixel == 8);
	int x = (dstrect == NULL ? 0 : dstrect->x);
	int y = (dstrect == NULL ? 0 : dstrect->y);
	int w = (dstrect == NULL ? dst->w : dstrect->w);
	int h = (dstrect == NULL ? dst->h : dstrect->h);
	assert(x >= 0 && y >= 0);
	if(dst->w - x < w) w = dst->w - x;
	if(dst->h - y < h) h = dst->h - y;
	if(w <= 0 || h <= 0) return;

	int row;
	for(row = 0; row < h; row ++) {
		memset(dst->pixels + (y + row) * dst->pitch + x, color, w);
	}
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, 
		int firstcolor, int ncolors) {
	assert(s);
	assert(s->format);
	assert(s->format->palette);
	assert(firstcolor == 0);

	if(s->format->palette->colors == NULL || s->format->palette->ncolors != ncolors) {
		if(s->format->palette->ncolors != ncolors && s->format->palette->colors != NULL) {
			/* If the size of the new palette is different 
			 * from the old one, free the old one.
			 */
			free(s->format->palette->colors);
		}

		/* Get new memory space to store the new palette. */
		s->format->palette->colors = malloc(sizeof(SDL_Color) * ncolors);
		assert(s->format->palette->colors);
	}

	/* Set the new palette. */
	s->format->palette->ncolors = ncolors;
	memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);

	if(s->flags & SDL_HWSURFACE) {
		write_palette(colors, ncolors);
	}
}

/* ======== The following functions are already implemented. ======== */

void SDL_UpdateRect(SDL_Surface *screen, int x, int y, int w, int h) {
	assert(screen);
	assert(screen->pitch == 320);

	// this should always be true in NEMU-PAL
	assert(screen->flags & SDL_HWSURFACE);

	if(x == 0 && y == 0) {
		/* Draw FPS */
		vmem = VMEM_ADDR;
		char buf[80];
		sprintf(buf, "%dFPS", get_fps());
		draw_string(buf, 0, 0, 10);
	}
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, 
		SDL_Surface *dst, SDL_Rect *dstrect) {
	assert(src && dst);
	int x = (srcrect == NULL ? 0 : srcrect->x);
	int y = (srcrect == NULL ? 0 : srcrect->y);
	int w = (srcrect == NULL ? src->w : srcrect->w);
	int h = (srcrect == NULL ? src->h : srcrect->h);

	assert(dstrect);
	if(w == dstrect->w && h == dstrect->h) {
		/* The source rectangle and the destination rectangle
		 * are of the same size. If that is the case, there
		 * is no need to stretch, just copy. */
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = w;
		rect.h = h;
		SDL_BlitSurface(src, &rect, dst, dstrect);
	}
	else {
		/* No other case occurs in NEMU-PAL. */
		assert(0);
	}
}

SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
		uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
	SDL_Surface *s = malloc(sizeof(SDL_Surface));
	assert(s);
	s->format = malloc(sizeof(SDL_PixelFormat));
	assert(s);
	s->format->palette = malloc(sizeof(SDL_Palette));
	assert(s->format->palette);
	s->format->palette->ncolors = 0;
	s->format->palette->colors = NULL;

	s->format->BitsPerPixel = depth;
	s->format->BytesPerPixel = depth >> 3;
	s->format->Rmask = Rmask;
	s->format->Gmask = Gmask;
	s->format->Bmask = Bmask;
	s->format->Amask = Amask;

	s->flags = flags;
	s->w = width;
	s->h = height;
	s->pitch = (width * depth) >> 3;
	s->clip_rect.x = 0;
	s->clip_rect.y = 0;
	s->clip_rect.w = width;
	s->clip_rect.h = height;
	s->pixels = (flags & SDL_HWSURFACE ? (void *)VMEM_ADDR : malloc(s->pitch * height));
	assert(s->pixels);

	return s;
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
	return SDL_CreateRGBSurface(flags,  width, height, bpp,
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
}

void SDL_FreeSurface(SDL_Surface *s) {
	if(s != NULL) {
		if(s->format != NULL) {
			if(s->format->palette != NULL) {
				if(s->format->palette->colors != NULL) {
					free(s->format->palette->colors);
				}
				free(s->format->palette);
			}

			free(s->format);
		}
		
		if(s->pixels != NULL && s->pixels != VMEM_ADDR) {
			free(s->pixels);
		}

		free(s);
	}
}
