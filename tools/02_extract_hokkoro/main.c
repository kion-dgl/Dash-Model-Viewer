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

void psx_read_framebuffer(FILE *fp, PSX_Framebuffer *fb);
void psx_free_framebuffer(PSX_Framebuffer *fb);

void psx_read_ebd_file(FILE *fp, PSX_EBD_File *file);
void psx_free_ebd_file(PSX_EBD_File *file);

void glTF_read_model(FILE *fp, PSX_EBD_File *file, PSX_Framebuffer *fb);
void glTF_read_textures(FILE *fp, glTF_Model *model, PSX_Framebuffer *fb);

int main(int argc, char *argv[]) {

	FILE *fp;
	PSX_Framebuffer framebuffer;
	PSX_EBD_File file;

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
	psx_read_ebd_file(fp, &file);

	printf("Id: 0x%08x\n", file.model_list[1].id);
	printf("Mesh: 0x%08x\n", file.model_list[1].mesh_ofs);
	printf("Bone: 0x%08x\n", file.model_list[1].bone_ofs);
	printf("Anim: 0x%08x\n", file.model_list[1].anim_ofs);
	
	glTF_read_model(fp, &file, &framebuffer);

	psx_free_framebuffer(&framebuffer);
	psx_free_ebd_file(&file);
	fclose(fp);

	return 0;

}

void psx_read_framebuffer(FILE *fp, PSX_Framebuffer *fb) {

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

	fb->tex_list = malloc(sizeof(PSX_Tim_Image) * fb->nb_tex);

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
		
		i--;
		
		fseek(fp, ofs, SEEK_SET);
		fread(&fb->tex_list[i], sizeof(PSX_Tim_Image), 1, fp);
		fb->tex_list[i].offset = ofs;

		printf("%d Found image: %s\n\n", i, fb->tex_list[i].image_name);

		if(i != 16) {
			continue;
		}

		printf("Image x: %d\n",  fb->tex_list[i].image_x);
		printf("Image y: %d\n",  fb->tex_list[i].image_y);

		printf("Pallet x: %d\n",  fb->tex_list[i].pallet_x);
		printf("Pallet y: %d\n",  fb->tex_list[i].pallet_y);

	} while((ofs += 0x400) < len);

	// End

}


void psx_free_framebuffer(PSX_Framebuffer *fb) {

	free(fb->tex_list);

}

void psx_read_ebd_file(FILE *fp, PSX_EBD_File *file) {

	uint32_t len, ofs;
	char *dot, name[0x20];

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	
	ofs = 0;
	file->nb_model = 0;

	// Find EBD File Location

	do {

		fseek(fp, ofs + 0x40, SEEK_SET);
		fread(name, sizeof(char), 0x20, fp);

		if(name[0] != '.' || name[1] != '.') {
			continue;
		}

		dot = strrchr(name, '.');

		if(strcmp(dot, ".EBD") != 0) {
			continue;
		}
	
		file->nb_model = 1;
		printf("Found EBD File: %s\n", name);
		break;

	} while((ofs + 0x400) < len);

	// If not found set model count zero, and return

	if(file->nb_model == 0) {
		return;
	}

	// Read Memory Location and model offsets

	fseek(fp, ofs + 0x0C, SEEK_SET);
	file->offset = ofs;
	fread(&file->memory, sizeof(uint32_t), 1, fp);

	fseek(fp, ofs + 0x800, SEEK_SET);
	fread(&file->nb_model, sizeof(uint32_t), 1, fp);

	file->model_list = malloc(file->nb_model * sizeof(PSX_EBD_Model));
	fread(file->model_list, sizeof(PSX_EBD_Model), file->nb_model, fp);

	// End	

}

void psx_free_ebd_file(PSX_EBD_File *file) {

	free(file->model_list);

}

void glTF_read_model(FILE *fp, PSX_EBD_File *file, PSX_Framebuffer *fb) {

	uint8_t nb_prim;
	uint32_t i, k, j, found, tmp;
	uint32_t mesh_ofs, *mats;
	glTF_Model model;
	PSX_EBD_Mesh *psx_mesh;

	for(i = 1; i < file->nb_model; i++) {

		// Get Start of mesh

		mesh_ofs = file->model_list[i].mesh_ofs - file->memory;
		mesh_ofs += (file->offset + 0x800);
		
		printf("Start of mesh offset: 0x%08x\n", mesh_ofs);

		// Get Number of primitives

		fseek(fp, mesh_ofs + 0x11, SEEK_SET);
		fread(&nb_prim, sizeof(nb_prim), 1, fp);
		printf("Number of primitives: %d\n", nb_prim);
		nb_prim++;

		// Read the header to each primitive

		fseek(fp, mesh_ofs + 0x7c, SEEK_SET);
		psx_mesh = malloc(nb_prim * sizeof(PSX_EBD_Mesh));
		fread(psx_mesh, sizeof(PSX_EBD_Mesh), nb_prim, fp);

		// Set offsets realtive to local file

		for(k = 0; k < nb_prim; k++) {
			
			if(psx_mesh[k].tri_ofs) {
				tmp = psx_mesh[k].tri_ofs - file->memory;
				tmp += (file->offset + 0x800);
				psx_mesh[k].tri_ofs = tmp;
			}

			if(psx_mesh[k].quad_ofs) {
				tmp = psx_mesh[k].quad_ofs - file->memory;
				tmp += (file->offset + 0x800);
				psx_mesh[k].quad_ofs = tmp;
			}

			if(psx_mesh[k].vert_ofs) {
				tmp = psx_mesh[k].vert_ofs - file->memory;
				tmp += (file->offset + 0x800);
				psx_mesh[k].vert_ofs = tmp;
			}

		}

		// Determine Material

		mats = malloc(sizeof(uint32_t) * nb_prim);
		model.nb_mat = 0;

		for(k = 0; k < nb_prim; k++) {

			tmp = psx_mesh[k].tex_page;
			found = 0;

			for(j = 0; j < model.nb_mat; j++) {
				
				if(mats[j] != tmp) {
					continue;
				}

				found = 1;
				break;

			}
			
			if(found) {
				continue;
			}

			mats[model.nb_mat++] = tmp;
		}

		printf("Number of materials: %d\n", model.nb_mat);
		model.mat_list = malloc(model.nb_mat * sizeof(glTF_Material));

		for(k = 0; k < model.nb_mat; k++) {
			model.mat_list[k].tex_page = mats[k];
			printf("Material num: 0x%08x\n", mats[k]);
		}

		glTF_read_textures(fp, &model, fb);

		// End model parsing
		
		free(mats);
		free(psx_mesh);
		free(model.mat_list);
		break;

	}

}


void glTF_read_textures(FILE *fp, glTF_Model *model, PSX_Framebuffer *fb) {

	int i, k;

	uint16_t pallet_page, pallet_x, pallet_y;
	uint16_t image_page, image_x, image_y;
	uint16_t nb_color, *pallet;

	for(i = 0; i < model->nb_mat; i++) {

		image_page = model->mat_list[i].tex_page & 0xFFFF;
		pallet_page =  model->mat_list[i].tex_page >> 16;

		image_x = (image_page & 0xF) << 6;
		image_y = ((image_page >> 6) & 0x1) * 256;

		pallet_x = (pallet_page & 0x3f) << 4;
		pallet_y = pallet_page >> 6;
		
		// Locate pallet image

		for(k = 0; k < fb->nb_tex; k++) {
			
			if(pallet_x != fb->tex_list[k].pallet_x) {
				continue;
			}

			if(pallet_y != fb->tex_list[k].pallet_y) {
				continue;
			}
			


		}

	}


}
