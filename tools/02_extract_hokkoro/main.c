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

#include <png.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "glTF_types.h"
#include "psx_types.h"
#include "b64codec.h"

#define MEMORY_SIZE 1024 * 1024

void psx_read_framebuffer(FILE *fp, PSX_Framebuffer *fb);
void psx_free_framebuffer(PSX_Framebuffer *fb);

void psx_read_ebd_file(FILE *fp, PSX_EBD_File *file);
void psx_free_ebd_file(PSX_EBD_File *file);

void glTF_read_model(FILE *fp, PSX_EBD_File *file, PSX_Framebuffer *fb);
void glTF_read_textures(FILE *fp, glTF_Model *model, PSX_Framebuffer *fb);

void glTF_export_model(glTF_Model *model, const char *filename);
void  glTF_export_obj(glTF_Model *model, const char *filename);

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
	i = 0;

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
		fread(&fb->tex_list[i], sizeof(PSX_Tim_Image), 1, fp);
		fb->tex_list[i].offset = ofs;

		/*
		printf("%d Found image: %s\n", i, fb->tex_list[i].image_name);
		printf("Image x: %d\n",  fb->tex_list[i].image_x);
		printf("Image y: %d\n",  fb->tex_list[i].image_y);
		printf("Pallet x: %d\n",  fb->tex_list[i].pallet_x);
		printf("Pallet y: %d\n",  fb->tex_list[i].pallet_y);
		printf("Number colors: %d\n\n",  fb->tex_list[i].nb_color);
		*/

		i++;

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
	uint16_t indice[4];
	uint32_t i, k, j, l, m, found, tmp, mat;
	uint32_t index, mesh_ofs, *mats, nb_face, f;
	glTF_Model model;
	PSX_EBD_Mesh *psx_mesh;
	PSX_EBD_Vertex *vert_list;
	PSX_EBD_Face *tri_list, *quad_list;
	float x, y, z, u, v, tex_u, tex_v;

	for(i = 1; i < file->nb_model; i++) {

		// Get Start of mesh

		mesh_ofs = file->model_list[i].mesh_ofs - file->memory;
		mesh_ofs += (file->offset + 0x800);
		
		printf("Start of mesh offset: 0x%08x\n", mesh_ofs);

		// Get Number of primitives

		fseek(fp, mesh_ofs + 0x11, SEEK_SET);
		fread(&nb_prim, sizeof(nb_prim), 1, fp);
		printf("Number of primitives: %d\n", nb_prim);

		// Read the header to each primitive

		fseek(fp, mesh_ofs + 0x90, SEEK_SET);
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

		// Convert glTF Models
		
		model.nb_vert = 0;
		model.nb_prim = nb_prim;
		model.prim_list = malloc(sizeof(glTF_Primitive) * nb_prim);
		
		for(k = 0; k < nb_prim; k++) {
			
			printf("Reading primitive: %d\n", k);

			// Find mat number

			for(j = 0; j < model.nb_mat; j++) {
				
				if(psx_mesh[k].tex_page != model.mat_list[j].tex_page) {
					continue;
				}
				
				printf("Found material: %d\n", j);
				model.prim_list[k].mat = j;
				mat = j;
				break;
			}

			// Allocate memory

			vert_list = malloc(psx_mesh[k].nb_vert * sizeof(PSX_EBD_Vertex));
			tri_list = malloc(psx_mesh[k].nb_tri * sizeof(PSX_EBD_Face));
			quad_list = malloc(psx_mesh[k].nb_quad * sizeof(PSX_EBD_Face));
			
			nb_face = psx_mesh[k].nb_tri + (psx_mesh[k].nb_quad * 2);
			model.prim_list[k].face_list = malloc(sizeof(glTF_Face) * nb_face);
			model.prim_list[k].nb_face = nb_face;

			// Read Original Values

			fseek(fp, psx_mesh[k].vert_ofs, SEEK_SET);
			fread(vert_list, sizeof(PSX_EBD_Vertex), psx_mesh[k].nb_vert, fp);

			fseek(fp, psx_mesh[k].tri_ofs, SEEK_SET);
			fread(tri_list, sizeof(PSX_EBD_Face), psx_mesh[k].nb_tri, fp);
			
			fseek(fp, psx_mesh[k].quad_ofs, SEEK_SET);
			fread(quad_list, sizeof(PSX_EBD_Face), psx_mesh[k].nb_quad, fp);
			
			// Convert Triangle List

			f = 0;
			tex_u = model.mat_list[mat].width;
			tex_v = model.mat_list[mat].height;

			for(j = 0; j < psx_mesh[k].nb_tri; j++) {
				
				for(l = 0; l < 3; l++) {
					
					index = tri_list[j].indice[l];
					x = -0.01f * (float)vert_list[index].x;
					y = -0.01f * (float)vert_list[index].y;
					z = -0.01f * (float)vert_list[index].z;
					u = (float)tri_list[j].coord[l].u / tex_u;
					v = (float)tri_list[j].coord[l].v / tex_v;
				
					printf("%.02f, %.02f, %.02f, %.02f, %.02f\n",x, y, z, u, v);

					found = 0;

					for(m = 0; m < model.nb_vert; m++) {
						
						if(model.vert_list[m].x != x) {
							continue;
						}
						
						if(model.vert_list[m].y != y) {
							continue;
						}
						
						if(model.vert_list[m].z != z) {
							continue;
						}
						
						if(model.vert_list[m].u != u) {
							continue;
						}

						if(model.vert_list[m].z != z) {
							continue;
						}
						
						found = 1;
						indice[l] = m;
						break;

					}

					if(found) {
						continue;
					}

					model.vert_list[model.nb_vert].x = x;
					model.vert_list[model.nb_vert].y = y;
					model.vert_list[model.nb_vert].z = z;
					model.vert_list[model.nb_vert].u = u;
					model.vert_list[model.nb_vert].v = v;

					indice[l] = model.nb_vert;
					model.nb_vert++;

				}
				
				model.prim_list[k].face_list[f].a = indice[0];
				model.prim_list[k].face_list[f].b = indice[1];
				model.prim_list[k].face_list[f].c = indice[2];

				f++;
			}

			for(j = 0; j < psx_mesh[k].nb_quad; j++) {
				
				for(l = 0; l < 4; l++) {
					
					index = quad_list[j].indice[l];
					x = -0.01f * (float)vert_list[index].x;
					y = -0.01f * (float)vert_list[index].y;
					z = -0.01f * (float)vert_list[index].z;
					u = (float)quad_list[j].coord[l].u / tex_u;
					v = (float)quad_list[j].coord[l].v / tex_v;
					
					found = 0;

					for(m = 0; m < model.nb_vert; m++) {
						
						if(model.vert_list[m].x != x) {
							continue;
						}
						
						if(model.vert_list[m].y != y) {
							continue;
						}
						
						if(model.vert_list[m].z != z) {
							continue;
						}
						
						if(model.vert_list[m].u != u) {
							continue;
						}

						if(model.vert_list[m].z != z) {
							continue;
						}
						
						found = 1;
						indice[l] = m;
						break;

					}

					if(found) {
						continue;
					}

					model.vert_list[model.nb_vert].x = x;
					model.vert_list[model.nb_vert].y = y;
					model.vert_list[model.nb_vert].z = z;
					model.vert_list[model.nb_vert].u = u;
					model.vert_list[model.nb_vert].v = v;

					indice[l] = model.nb_vert;
					model.nb_vert++;

				}
				
				model.prim_list[k].face_list[f].a = indice[0];
				model.prim_list[k].face_list[f].b = indice[1];
				model.prim_list[k].face_list[f].c = indice[2];
				f++;
				
				model.prim_list[k].face_list[f].a = indice[1];
				model.prim_list[k].face_list[f].b = indice[3];
				model.prim_list[k].face_list[f].c = indice[2];
				f++;
			}

			// Free Memory

			free(vert_list);
			free(tri_list);
			free(quad_list);
			
		}

		// Export Model
	
		glTF_export_model(&model, "output/hokkoro.gltf");
		glTF_export_obj(&model, "output/hokkoro");

		// End model parsing
		
		free(mats);
		free(psx_mesh);
		for(k = 0; k < model.nb_mat; k++) {
			free(model.mat_list[k].png);
		}
		for(k = 0; k < model.nb_prim; k++) {
			free(model.prim_list[k].face_list);
		}
		free(model.mat_list);
		free(model.prim_list);
		break;

	}

}


void glTF_read_textures(FILE *fp, glTF_Model *model, PSX_Framebuffer *fb) {

	int i, k;
	FILE *wp;

	int16_t index, x, y, bx, by, pos;
	uint16_t pallet_page, pallet_x, pallet_y;
	uint16_t image_page, image_x, image_y;
	uint16_t width, height, block_width, block_height;
	uint32_t width_height;
	uint16_t *pallet, *image_body;
	uint8_t byte, *bitmap, *c;
	char inc, *slash;
	
	png_structp png_ptr;
	png_infop info_ptr;
	png_byte **row_ptr;

	int pixel_size = 4;
	int depth = 8;
	uint8_t memory[MEMORY_SIZE];

	for(i = 0; i < model->nb_mat; i++) {

		image_page = model->mat_list[i].tex_page & 0xFFFF;
		pallet_page =  model->mat_list[i].tex_page >> 16;
		
		image_x = (image_page & 0xF) << 6;
		image_y = ((image_page >> 6) & 0x1) * 256;

		pallet_x = (pallet_page & 0x3f) << 4;
		pallet_y = pallet_page >> 6;
		
		// Locate pallet image
		
		index = 0;

		for(k = fb->nb_tex; k >= 0; --k) {
			
			if(pallet_x != fb->tex_list[k].pallet_x) {
				continue;
			}

			if(pallet_y != fb->tex_list[k].pallet_y) {
				continue;
			}
			
			index = 1;
			
			pallet = malloc(sizeof(uint16_t) * fb->tex_list[k].nb_color);
			fseek(fp, fb->tex_list[k].offset + 0x100, SEEK_SET);
			fread(pallet, sizeof(uint16_t), fb->tex_list[k].nb_color, fp);

		}

		if(index == 0) {
			fprintf(stderr, "Unable to find pallet page 0x%04x\n", pallet_page);
			exit(1);
		}

		index = -1;

		for(k = fb->nb_tex; k >= 0; --k) {
			
			if(image_x != fb->tex_list[k].image_x) {
				continue;
			}

			if(image_y != fb->tex_list[k].image_y) {
				continue;
			}
			
			index = k;
			break;

		}

		if(index == -1) {
			fprintf(stderr, "Unable to find image page 0x%04x\n", image_page);
			exit(1);
		}
		
		// Convert Image to bitmap

		fseek(fp, fb->tex_list[index].offset + 0x800, SEEK_SET);
		height = fb->tex_list[index].height;

		switch(fb->tex_list[index].nb_color) {
			case 16:
				
				width = fb->tex_list[index].width * 4;
				inc = 2;
				block_height = 32;
				block_width = 128;

			break;
			case 256:
				
				width = fb->tex_list[index].width * 2;
				inc = 1;
				block_height = 32;
				block_width = 64;

			break;
		}

		slash = strrchr(fb->tex_list[index].image_name, '\\');
		slash++;
		strcpy(model->mat_list[i].image_name, slash);
		model->mat_list[i].width = (float)width;
		model->mat_list[i].height = (float)height;
		model->mat_list[i].len = 0;
		image_body = malloc(sizeof(uint16_t) * width * height);

		// Read Image Body

		for(y = 0; y < height; y += block_height) {
			for(x = 0; x < width; x += block_width) {
				for(by = 0; by < block_height; by++) {
					for(bx = 0; bx < block_width; bx += inc) {
						
						fread(&byte, sizeof(uint8_t), 1, fp);

						switch(fb->tex_list[index].nb_color) {
							case 16:

								pos = ((y + by) * width) + (x + bx);
								image_body[pos] = pallet[byte & 0xf];
								pos = ((y + by) * width) + (x + bx + 1);
								image_body[pos] = pallet[byte >> 4];

							break;
							case 256:
								
								pos = ((y + by) * width) + (x + bx);
								image_body[pos] = pallet[byte];

							break;
						}

					}
				}
			}
		}

		// Create Bitmap

		width_height = width | (height << 16);
		bitmap = malloc(width * height * 4);
		c = bitmap;

		for(k = 0; k < width * height; k++) {
			
			*c++ = ((image_body[k] >> 0x00) & 0x1f) << 3;
			*c++ = ((image_body[k] >> 0x05) & 0x1f) << 3;
			*c++ = ((image_body[k] >> 0x0a) & 0x1f) << 3;

			if(image_body[k] == 0) {
				*c++ = 0;
			} else {
				*c++ = 0xFF;
			}
		}
		
		// Create PNG
		wp = fmemopen(memory, MEMORY_SIZE, "w");

		png_ptr = png_create_write_struct(
			PNG_LIBPNG_VER_STRING, 
			NULL, 
			NULL, 
			NULL
		);
		if(png_ptr == NULL) {
			fprintf(stderr, "Could not create png struct\n");
			exit(1);
		}

		info_ptr = png_create_info_struct(png_ptr);
		if(info_ptr == NULL) {
			fprintf(stderr, "Could not create info struct\n");
			exit(1);
		}

		if(setjmp(png_jmpbuf(png_ptr))) {
			fprintf(stderr, "Could not set jmpbuf\n");
			exit(1);
		}
	
		png_set_IHDR(
			png_ptr,
			info_ptr,
			width,
			height,
			depth,
			PNG_COLOR_TYPE_RGBA,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT
		);

		row_ptr = png_malloc(png_ptr, height * sizeof(png_byte*));
	
		k = 0;
		for(y = 0; y < height; y++) {
			
			png_byte *row = png_malloc(png_ptr, width * pixel_size);
			row_ptr[y] = row;

			for(x = 0; x < width; x++) {

				*row++ = bitmap[k++];
				*row++ = bitmap[k++];
				*row++ = bitmap[k++];
				*row++ = bitmap[k++];

			}

		}
		
		png_init_io(png_ptr, wp);
		png_set_rows(png_ptr, info_ptr, row_ptr);
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	
		for(y = 0; y < height; y++) {
			png_free(png_ptr, row_ptr[y]);
		}
		png_free(png_ptr, row_ptr);
	
		pos = ftell(fp);
		model->mat_list[i].len = pos;

		fclose(wp);

		model->mat_list[i].png = malloc(model->mat_list[i].len);
		memcpy(model->mat_list[i].png, memory, model->mat_list[i].len);

		// Free assets from loop

		free(pallet);
		free(image_body);
		free(bitmap);

	}

}

void glTF_export_model(glTF_Model *model, const char *filename) {
	
	int i, vert_len, face_ofs, face_len, count;
	float max_x, max_y, max_z;
	float min_x, min_y, min_z;
	size_t str_len, buffer_len, pos;
	uint8_t *buffer, memory[MEMORY_SIZE];

	FILE *fp = fopen(filename, "w");
	if(fp == NULL) {
		fprintf(stderr, "Unable to open %s for writing\n", filename);
		return;
	}
	

	max_x = 0.0f;
	max_y = 0.0f;
	max_z = 0.0f;

	min_x = 0.0f;
	min_y = 0.0f;
	min_z = 0.0f;

	vert_len = model->nb_vert * sizeof(glTF_Vertex);
	face_len = 0;
	for(i = 0; i < model->nb_prim; i++) {
		face_len += model->prim_list[i].nb_face * sizeof(glTF_Face);
	}
	face_ofs = vert_len;

	buffer_len = vert_len + face_len;
	buffer = malloc(buffer_len);

	memcpy(buffer, model->vert_list, vert_len);

	for(i = 0; i < model->nb_vert; i++) {

		if(model->vert_list[i].x < min_x) {
			min_x = model->vert_list[i].x;
		}

		if(model->vert_list[i].y < min_y) {
			min_y = model->vert_list[i].y;
		}

		if(model->vert_list[i].z < min_z) {
			min_z = model->vert_list[i].z;
		}

		if(model->vert_list[i].x > max_x) {
			max_x = model->vert_list[i].x;
		}

		if(model->vert_list[i].y > max_y) {
			max_y = model->vert_list[i].y;
		}

		if(model->vert_list[i].z > max_z) {
			max_z = model->vert_list[i].z;
		}

	}

	fprintf(fp, "{" 
		"\"asset\":{"
			"\"generator\":\"Dash glTF Exporter\","
			"\"version\":\"2.0\""
		"},"
		"\"scene\":0,"
		"\"scenes\":["
			"{\"nodes\":[0]}"
		"],"
		"\"nodes\":[{"
			"\"mesh\":0"
		"}],"
		"\"meshes\":[{"
			"\"name\":\"hokkoro\","
			"\"primitives\":[");

	for(i = 0; i < model->nb_prim; i++) {
		
		if(i) {
			fprintf(fp, ",");
		}

		fprintf(fp, "{"
			"\"attributes\":{"
				"\"POSITION\":0,"
				"\"TEXCOORD_0\":1"
			"},"
			"\"indices\":%d,"
			"\"mode\":4,"
			"\"material\":%d"
		"}", i+2, model->prim_list[i].mat);
		
	}
	
	fprintf(fp, "]}],\"accessors\":[");
	
	fprintf(fp, "{"
		"\"bufferView\":0,"
		"\"byteOffset\":0,"
		"\"componentType\":5126,"
		"\"count\":%d,"
		"\"type\":\"VEC3\","
		"\"min\":[%f,%f,%f],"
		"\"max\":[%f,%f,%f]"
	"}", model->nb_vert,
		min_x, min_y, min_z,
		max_x, max_y, max_z);
	fprintf(fp,",{"
		"\"bufferView\":1,"
		"\"byteOffset\":0,"
		"\"componentType\":5126,"
		"\"count\":%d,"
		"\"type\":\"VEC2\""
	"}", model->nb_vert);
	
	face_ofs = 0;
	for(i = 0; i < model->nb_prim; i++) {
		
		count = model->prim_list[i].nb_face * 3;
		/*
		if(i == 0) {
			count = 3;
		}
		*/

		fprintf(fp, ",{"
			"\"bufferView\":2,"
			"\"byteOffset\":%d,"
			"\"componentType\":5123,"
			"\"count\":%d,"
			"\"type\":\"SCALAR\""
		"}", face_ofs, count);
	
		memcpy(
			&buffer[vert_len + face_ofs], 
			model->prim_list[i].face_list, 
			model->prim_list[i].nb_face * sizeof(glTF_Face)
		);

		face_ofs += (model->prim_list[i].nb_face * sizeof(glTF_Face));

	}

	fprintf(fp, "],\"materials\":[");

	for(i = 0; i < model->nb_mat; i++) {
		
		if(i > 0) {
			fprintf(fp, ",");
		}

		fprintf(fp, "{"
			"\"pbrMetallicRoughness\":{"
				"\"baseColorTexture\":{"
					"\"index\":%d"
				"},"
				"\"metallicFactor\":0.0"
			"},"
			"\"emissiveFactor\":[0.0,0.0,0.0],"
			"\"name\":\"%s\""
		"}", i, model->mat_list[i].image_name);

	}

	fprintf(fp, "],\"textures\":[");
	
	for(i = 0; i < model->nb_mat; i++) {

		if(i > 0) {
			fprintf(fp, ",");
		}

		fprintf(fp, "{\"source\":%d}", i);
	}

	fprintf(fp, "],\"images\":[");

	for(i = 0; i < model->nb_mat; i++) {
		
		if(i > 0) {
			fprintf(fp, ",");
		}

		b64enc(
			model->mat_list[i].png, 
			model->mat_list[i].len + 1,
			memory,
			MEMORY_SIZE,
			&str_len
		);

		fprintf(fp, "{"
			"\"uri\":\"data:image/png;base64,%s\""
		"}", memory);

	}

	fprintf(fp, "],\"bufferViews\":[");

	fprintf(fp, "{"
		"\"buffer\":0,"
		"\"byteOffset\":0,"
		"\"byteLength\":%d,"
		"\"byteStride\":%ld"
	"}", vert_len - 8, sizeof(glTF_Vertex));

	fprintf(fp, ",{"
		"\"buffer\":0,"
		"\"byteOffset\":12,"
		"\"byteLength\":%d,"
		"\"byteStride\":%ld"
	"}", vert_len - 12, sizeof(glTF_Vertex));

	
	/*
	face_ofs = vert_len;
	for(i = 0; i < model->nb_prim; i++) {
	*/

		fprintf(fp, ",{"
			"\"buffer\":0,"
			"\"byteOffset\":%d,"
			"\"byteLength\":%d"
		"}", vert_len, face_len);

	/*
	}
	*/

	b64enc(
		buffer,
		buffer_len + 1,
		memory,
		MEMORY_SIZE,
		&str_len
	);

	fprintf(fp, "],\"buffers\":[");
	fprintf(fp, "{"
		"\"byteLength\":%ld,"
		"\"uri\":\"data:application/octet-stream;base64,%s\""
	"}]}", buffer_len, memory);

	fclose(fp);
	free(buffer);

}

void  glTF_export_obj(glTF_Model *model, const char *filename) {

	int i, k;
	char file_name[0x20], *dot;
	FILE *fp;

	for(i = 0; i < model->nb_mat; i++) {

		strcpy(file_name, "output/");
		strcat(file_name, model->mat_list[i].image_name);
		dot = strrchr(file_name, '.') + 1;
		*dot++ = 'p';
		*dot++ = 'n';
		*dot++ = 'g';
		printf("Outname: %s\n", file_name);

		fp = fopen(file_name, "w");
		fwrite(model->mat_list[i].png, model->mat_list[i].len, 1, fp);
		fclose(fp);

	}

	fp = fopen("output/hokkoro.obj", "w");

	for(i = 0; i < model->nb_vert; i++) {

		fprintf(fp, "v %f %f %f\n", 
			model->vert_list[i].x,
			model->vert_list[i].y,
			model->vert_list[i].z
		);

	}

	fprintf(fp, "\n");

	for(i = 0; i < model->nb_vert; i++) {

		fprintf(fp, "vt %f %f\n", 
			model->vert_list[i].u,
			model->vert_list[i].v
		);

	}

	fprintf(fp, "\n");

	for(i = 0; i < model->nb_prim; i++) {

		for(k = 0; k < model->prim_list[i].nb_face; k++) {
			fprintf(fp, "f %d//%d %d//%d %d//%d\n", 
				model->prim_list[i].face_list[k].a + 1,
				model->prim_list[i].face_list[k].a + 1,
				model->prim_list[i].face_list[k].b + 1,
				model->prim_list[i].face_list[k].b + 1,
				model->prim_list[i].face_list[k].c + 1,
				model->prim_list[i].face_list[k].c + 1
			);
		}

	}


	fclose(fp);

}
