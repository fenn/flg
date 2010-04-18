/*
 * Copyright 2002-2008 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

png_bytep * read_png_file(char* file_name)
{
	unsigned char header[8];	// 8 is the maximum size that can be checked

	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if (!fp)
		abort_("[read_png_file] File %s could not be opened for reading", file_name);
	if (fread(header, 1, 8, fp) != 0){};
	if ( png_sig_cmp(header, 0, 8))
		abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
        printf("png_ptr: %x, info_ptr: %d\n", (unsigned int) png_ptr, (unsigned int) info_ptr);	

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);
        printf("png_ptr: %x, info_ptr: %d\n", (unsigned int) png_ptr, (unsigned int) info_ptr);	

	width = info_ptr->width;
	height = info_ptr->height;
	color_type = info_ptr->color_type;
	bit_depth = info_ptr->bit_depth;

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);


	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
		row_pointers[y] = (png_byte*) malloc(info_ptr->rowbytes);

	png_read_image(png_ptr, row_pointers);

        fclose(fp);
        return row_pointers;
}

int get_png_height(void){
    return height;
}

int get_png_width(void){
    return width;
}

void run_png_sequence(void)
{
	if (info_ptr->color_type != PNG_COLOR_TYPE_RGB)
		abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGB (is %d)", info_ptr->color_type);
        printf("height: %d, width: %d\n", height, width);
#ifdef command_line
	for (y=0; y<height; y++) {
		png_byte* row = row_pointers[y];
		for (x=0; x<width; x++) {
			png_byte* ptr = &(row[x*3]);
                        printf("Pixel [x: %d, y: %d] R:%d G:%d B:%d\n",
			       x, y, ptr[0], ptr[1], ptr[2]);
		}
	}
#endif //command_line
}


#ifdef command_line
int main(int argc, char **argv)
{
	if (argc != 2)
		abort_("Usage: pngpattern foo.png");

	read_png_file(argv[1]);
	run_png_sequence();

        return 0;
}
#endif

