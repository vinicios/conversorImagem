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

/**
 * mean_filter_2d - apply a 3x3 mean filter in a raw greyscale image
 * @param in pointer to input image
 * @param out pointer to output image
 * @param w width
 * @param h height
 */
static void mean_filter_2d(uint8_t *in, uint8_t *out, size_t w, size_t h)
{
	uint32_t l;
	uint32_t c;
	uint32_t i;
	uint32_t pix;

	for (l = 1; l < h; l++) {
		for (c = 1; c < w; c++) {
			i = (l * w) + c;
			pix = (uint32_t) in[i - w - 1];
			pix += (uint32_t) in[i - w];
			pix += (uint32_t) in[i - w + 1];
			pix += (uint32_t) in[i - 1];
			pix += (uint32_t) in[i];
			pix += (uint32_t) in[i + 1];
			pix += (uint32_t) in[i + w - 1];
			pix += (uint32_t) in[i + w];
			pix += (uint32_t) in[i + w + 1];
			out[i] = (uint8_t) (pix / 9);
		}
	}
}

int main(int argc,char **argv)
{
	uint8_t *raw_img;
	uint8_t *raw_out;
	size_t width;
	size_t height;
	size_t read;
	size_t written;
	size_t size;
	FILE *in;
	FILE *out;
	int err;

	err = 0;

	if (argc != 5) {
		printf("Usage: %s <raw_file_in> <raw_file_out> <width> "
				"<height>\n", argv[0]);
		err = -1;
		goto error1;
	}
	in = fopen(argv[1], "r");
	if (in == NULL) {
		printf("Could not open %s file\n", argv[1]);
		err = -1;
		goto error1;
	}
	out = fopen(argv[2], "w");
	if (out == NULL) {
		printf("Could not open %s file\n", argv[2]);
		err = -1;
		goto error2;
	}
	width = atoi(argv[3]);
	height = atoi(argv[4]);
	size = width * height;
	raw_img = (uint8_t *) malloc(size * sizeof(uint8_t));
	raw_out = (uint8_t *) malloc(size * sizeof(uint8_t));

	read = fread((void *) raw_img, 1, size, in);
	if (read != size) {
		printf("Only %lu/%lu bytes were read\n", read, size);
		err = -1;
		goto error3;
	}

	mean_filter_2d(raw_img, raw_out, width, height);

	written = fwrite((const void *) raw_out, 1, size, out);
	if (written != size)
		printf("Only %lu/%lu bytes were written\n", written, size);

error3:
	free(raw_img);
	free(raw_out);
	fclose(in);
error2:
	fclose(out);
error1:
	return err;
}
