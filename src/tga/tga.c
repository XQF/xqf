/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/* taken from quake2 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#ifdef STANDALONE
#define xqf_warning(x) fputs(x, stderr)
#else
#include "debug.h"
#endif

static inline unsigned short LittleShort(const unsigned char* buf)
{
    return buf[0] + (buf[1]<<8);
}

typedef unsigned char byte;


typedef struct _TargaHeader
{
    unsigned char 	id_length, colormap_type, image_type;
    unsigned short	colormap_index, colormap_length;
    unsigned char	colormap_size;
    unsigned short	x_origin, y_origin, width, height;
    unsigned char	pixel_size, attributes;
} TargaHeader;

#define CHECK(x) do { if(buf_p+x>=buffer+length) goto error_out; } while(0)

char* LoadTGA (const unsigned char* buffer, size_t length, unsigned *width, unsigned *height)
{
    int         columns = 0, rows = 0, numPixels = 0;
    byte        *pixbuf = NULL;
    int         row = 0, column = 0;
    const byte  *buf_p = NULL;
    TargaHeader	targa_header = {0};
    byte        *targa_rgba = NULL;

    buf_p = buffer;

    CHECK(18);

    targa_header.id_length = *buf_p++;
    targa_header.colormap_type = *buf_p++;
    targa_header.image_type = *buf_p++;

    targa_header.colormap_index = LittleShort(buf_p);
    buf_p+=2;
    targa_header.colormap_length = LittleShort(buf_p);
    buf_p+=2;
    targa_header.colormap_size = *buf_p;
    ++buf_p;
    targa_header.x_origin = LittleShort(buf_p);
    buf_p+=2;
    targa_header.y_origin = LittleShort(buf_p);
    buf_p+=2;
    targa_header.width =  LittleShort(buf_p);
    buf_p+=2;
    targa_header.height = LittleShort(buf_p);
    buf_p+=2;
    targa_header.pixel_size = *buf_p++;
    targa_header.attributes = *buf_p++;

#if 0
    printf("id_length: %hhu\n", targa_header.id_length);
    printf("colormap_type: %hhu\n", targa_header.colormap_type);
    printf("image_type: %hhu\n", targa_header.image_type);
    printf("colormap_index: %hu\n", targa_header.colormap_index);
    printf("colormap_length: %hu\n", targa_header.colormap_length);
    printf("colormap_size: %hhu\n", targa_header.colormap_size);
    printf("x_origin: %hu\n", targa_header.x_origin);
    printf("y_origin: %hu\n", targa_header.y_origin);
    printf("width: %hx\n", targa_header.width);
    printf("height: %hx\n", targa_header.height);
    printf("pixel_size: %hhu\n", targa_header.pixel_size);
    printf("attributes: %hhu\n", targa_header.attributes);
#endif

    if (targa_header.image_type!=2 
	    && targa_header.image_type!=10) 
    {
	xqf_warning("Only type 2 and 10 targa RGB images supported");
	return NULL;
    }

    if (targa_header.colormap_type !=0 
	    || (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
    {
	xqf_warning("Only 32 or 24 bit images supported (no colormaps)");
	return NULL;
    }

    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows;

    if (width)
	*width = columns;
    if (height)
	*height = rows;

    targa_rgba = g_malloc (numPixels*4);


    CHECK(targa_header.id_length);

    if (targa_header.id_length != 0)
	buf_p += targa_header.id_length;  // skip TARGA image comment

    if (targa_header.image_type==2) {  // Uncompressed, RGB images
	for(row=rows-1; row>=0; row--) {
	    pixbuf = targa_rgba + row*columns*4;
	    for(column=0; column<columns; column++) {
		unsigned char red,green,blue,alphabyte;
		switch (targa_header.pixel_size) {
		    case 24:

			CHECK(3);
			blue = *buf_p++;
			green = *buf_p++;
			red = *buf_p++;
			*pixbuf++ = red;
			*pixbuf++ = green;
			*pixbuf++ = blue;
			*pixbuf++ = 255;
			break;
		    case 32:
			CHECK(4);
			blue = *buf_p++;
			green = *buf_p++;
			red = *buf_p++;
			alphabyte = *buf_p++;
			*pixbuf++ = red;
			*pixbuf++ = green;
			*pixbuf++ = blue;
			*pixbuf++ = alphabyte;
			break;
		}
	    }
	}
    }
    else if (targa_header.image_type==10) {   // Runlength encoded RGB images
	unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
	for(row=rows-1; row>=0; row--) {
	    pixbuf = targa_rgba + row*columns*4;
	    for(column=0; column<columns; ) {
		CHECK(1);
		packetHeader= *buf_p++;
		packetSize = 1 + (packetHeader & 0x7f);
		if (packetHeader & 0x80) {        // run-length packet
		    switch (targa_header.pixel_size) {
			case 24:
			    CHECK(3);
			    blue = *buf_p++;
			    green = *buf_p++;
			    red = *buf_p++;
			    alphabyte = 255;
			    break;
			case 32:
			    CHECK(4);
			    blue = *buf_p++;
			    green = *buf_p++;
			    red = *buf_p++;
			    alphabyte = *buf_p++;
			    break;
			default:
			    blue = 0;
			    green = 0;
			    red = 0;
			    alphabyte = 0;
			    break;
		    }

		    for(j=0;j<packetSize;j++) {
			*pixbuf++=red;
			*pixbuf++=green;
			*pixbuf++=blue;
			*pixbuf++=alphabyte;
			column++;
			if (column==columns) { // run spans across rows
			    column=0;
			    if (row>0)
				row--;
			    else
				goto breakOut;
			    pixbuf = targa_rgba + row*columns*4;
			}
		    }
		}
		else {                            // non run-length packet
		    for(j=0;j<packetSize;j++) {
			switch (targa_header.pixel_size) {
			    case 24:
				CHECK(3);
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			    case 32:
				CHECK(4);
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alphabyte = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alphabyte;
				break;
			}
			column++;
			if (column==columns) { // pixel packet run spans across rows
			    column=0;
			    if (row>0)
				row--;
			    else
				goto breakOut;
			    pixbuf = targa_rgba + row*columns*4;
			}						
		    }
		}
	    }
breakOut:;
	}
    }

    return targa_rgba;
error_out:
    g_free(targa_rgba);
    return NULL;
}
