/*
 * author: Diogo C. Luvizon
 * date: 2012-06-02
 * version: 1.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>

/* struct to store a grayscale raw image */
struct raw_img {
	uint8_t *img;
	size_t width;
	size_t height;
};

#define IMAGE_NCHANNELS 1

/**
 * jpeg_compress - compress a raw grayscale image to a jpeg file
 * @param file name of jpeg file
 * @param raw pointer to struct raw_img
 */
static int jpeg_compress(const char *file, struct raw_img *raw)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr; 
	JSAMPROW row_ptr[1];
	FILE* out_jpeg;
	uint8_t *image;
	int row_stride;
	int err;

	err = 0;
	image = raw->img;
	out_jpeg = fopen(file, "w+");
	if (out_jpeg == NULL) {
		printf("Could no open '%s' file\n", file);
		err = -1;
		goto error;
	}

	
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, out_jpeg);

	cinfo.image_width = raw->width;
	cinfo.image_height = raw->height;
	cinfo.input_components = IMAGE_NCHANNELS;
	cinfo.in_color_space = JCS_GRAYSCALE;
	cinfo.err = jpeg_std_error(&jerr); 

	jpeg_set_defaults(&cinfo);
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = raw->width * IMAGE_NCHANNELS;

	while (cinfo.next_scanline < cinfo.image_height) {
		row_ptr[0] = &image[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, row_ptr, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(out_jpeg);

error:
	return err;
}

/**
 * open_raw_image - open a raw grayscale image
 * @param file name of the input file
 * @param raw pointer to a struct raw_img
 * @return error code, 0 if success, negative if not
 */
static int open_raw_image(const char *file, struct raw_img *raw)
{
	FILE *raw_file;
	size_t read;
	size_t size;
	int err;

	raw_file = fopen(file, "r");
	if (raw_file == NULL) {
		printf("Could not open '%s' file\n", file);
		err = -1;
		goto error;
	}
	size = raw->width * raw->height;
	raw->img = (uint8_t *) malloc(size * sizeof(uint8_t));
	if (raw->img == NULL) {
		err = -2;
		goto error;
	}
	read = fread((void *) raw->img, 1, size, raw_file);
	if (read != size)
		printf("Only %lu/%lu bytes were read\n", read, size);

	fclose(raw_file);

	return 0;
error:
	return err;	
}

int main(int argc, char **argv)
{
	struct raw_img raw_img;
	int err;

	err = 0;

	if (argc != 5) {
		printf("Usage: %s <raw_file> <jpeg> <width> <height>\n",
				argv[0]);
		err = -1;
		goto error;
	}

	raw_img.width = atoi(argv[3]);
	raw_img.height = atoi(argv[4]);
	err = open_raw_image(argv[1], &raw_img);
	if (err == -1)
		goto error;
	if (err == -2)
		goto cleanup;
	jpeg_compress(argv[2], &raw_img);

cleanup:
	free(raw_img.img);
error:
	return err;
}
