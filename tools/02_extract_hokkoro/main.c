/* ++

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author: Benjamin Collins
    File  : main.c

-- */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "glTF_types.h"
#include "psx_types.h"

void psx_read_framebuffer(FILE *fp, struct PSX_Framebuffer *fb);
void psx_free_framebuffer(struct PSX_Framebuffer *fb);

void psx_read_model_list(FILE *fp, struct PSX_EBD_Model_List *ml);
void psx_free_model_list(struct PSX_EBD_Model_List *ml);

int main(int argc, char *argv[]) {

	FILE *fp;
	struct PSX_Framebuffer framebuffer;
	struct PSX_EBD_Model_List model_list;

	if(argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	fp = fopen(argv[1], "r");
	if(fp == NULL) {
		fprintf(stderr, "Could not open %s for reading\n", argv[1]);
		return 1;
	}

	psx_read_framebuffer(fp, &framebuffer);
	psx_read_model_list(fp, &model_list);

	psx_free_framebuffer(&framebuffer);
	psx_free_model_list(&model_list);
	fclose(fp);

	return 0;

}

void psx_read_framebuffer(FILE *fp, struct PSX_Framebuffer *fb) {

	uint32_t len, ofs;
	char *dot, name[0x20], i;
	
	fb->nb_tex = 0;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Determine number of Textures
	
	ofs = 0;

	do {

		fseek(fp, ofs + 0x40, SEEK_SET);
		fread(name, sizeof(char), 0x20, fp);

		if(name[0] != '.' || name[1] != '.') {
			continue;
		}

		dot = strrchr(name, '.');

		if(strcmp(dot, ".TIM") != 0) {
			continue;
		}

		fb->nb_tex++;

	} while((ofs += 0x400) < len);

	// Allocate Memory

	fb->tex_list = malloc(sizeof(struct PSX_Tim_Image) * fb->nb_tex);

	// Read Textures to framebuffer
	
	ofs = 0;
	i = fb->nb_tex;

	do {

		fseek(fp, ofs + 0x40, SEEK_SET);
		fread(name, sizeof(char), 0x20, fp);

		if(name[0] != '.' || name[1] != '.') {
			continue;
		}

		dot = strrchr(name, '.');

		if(strcmp(dot, ".TIM") != 0) {
			continue;
		}
		
		fseek(fp, ofs, SEEK_SET);
		fread(&fb->tex_list[--i], sizeof(struct PSX_Tim_Image), 1, fp);

	} while((ofs += 0x400) < len);

	// End

}


void psx_free_framebuffer(struct PSX_Framebuffer *fb) {

	free(fb->tex_list);

}

void psx_read_model_list(FILE *fp, struct PSX_EBD_Model_List *ml) {

}

void psx_free_model_list(struct PSX_EBD_Model_List *ml) {

}
