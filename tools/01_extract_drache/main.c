#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct TIM_Header {
	uint32_t type;
	uint32_t size;
	uint32_t unknown;
	uint32_t pallet_x;
	uint32_t pallet_y;
	uint32_t nb_colors;
	uint32_t nb_pallets;
	uint32_t image_x;
	uint32_t image_y;
	uint32_t width;
	uint32_t height;
	uint32_t offset;
	uint8_t nop[16];
	char image_name[0x20];
};

struct EBD_Mesh {
	uint32_t number;
	uint32_t mesh_ofs;
	uint32_t bone_ofs;
	uint32_t anim_ofs;
};

struct Mesh_Header {
	uint8_t nb_tri, nb_quad, nb_vert, nop;
	uint32_t tri_ofs;
	uint32_t quad_ofs;
	uint16_t tex_page;
	uint16_t pallet_page;
	uint32_t vert_ofs;
};

struct Vertex {
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t w;
};

struct TextureCoord {
	uint8_t u,v;
};

struct Face {
	struct TextureCoord coord[4];
	uint8_t indice[4];
};

void read_ebd_file(FILE *fp, struct TIM_Header list[]);

int main(int argc, char *argv[]) {

	FILE *fp;
	struct TIM_Header *tim_list;

	if(argc != 2) {
		fprintf(stderr, "Error: usage %s <file.BIN>\n", argv[0]);
		return 1;
	}

	fp = fopen(argv[1], "r");
	if(fp == NULL) {
		fprintf(stderr, "Error: could not open %s for reading\n", argv[1]);
		return 1;
	}

	// Read types of files in Archive

	uint32_t file_len, ofs, i, k;
	uint32_t nb_tim, tim_ofs[0x50];
	char *dot, asset_name[0x20];

	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ofs = 0;
	nb_tim = 0;

	do {
		
		fseek(fp, ofs + 0x40, SEEK_SET);
		fread(asset_name, sizeof(char), 0x20, fp);

		if(asset_name[0] != '.') {
			continue;
		}

		if(asset_name[1] != '.') {
			continue;
		}
		
		dot = strrchr(asset_name, '.') + 1;
		
		printf("Extension: %s\n", dot);

		if(strcmp(dot, "TIM") == 0) {
			tim_ofs[nb_tim++] = ofs;
		}

	} while((ofs += 0x400) < file_len);

	// Parse Each One of the TIM FILES

	printf("%d TIM files found in the archive\n", nb_tim);
	tim_list = malloc(nb_tim * sizeof(struct TIM_Header));

	for(i = 0; i < nb_tim; i++) {
		printf("Reading TIM file at 0x%08x\n", tim_ofs[i]);
		fseek(fp, tim_ofs[i], SEEK_SET);
		fread(&tim_list[i], sizeof(struct TIM_Header), 1, fp);
		tim_list[i].offset = tim_ofs[i];
		printf("Found file: %s\n", tim_list[i].image_name);
	}

	// Set File Position Back to Zero
	
	ofs = 0;
	fseek(fp, 0, SEEK_SET);

	do {
		
		fseek(fp, ofs + 0x40, SEEK_SET);
		fread(asset_name, sizeof(char), 0x20, fp);

		if(asset_name[0] != '.') {
			continue;
		}

		if(asset_name[1] != '.') {
			continue;
		}
		
		dot = strrchr(asset_name, '.') + 1;
		if(strcmp(dot, "EBD") == 0) {
			printf("Reading EBD file\n");
			fseek(fp, ofs, SEEK_SET);
			read_ebd_file(fp, tim_list);
		}

	} while((ofs += 0x400) < file_len);


	// Close and return

	free(tim_list);

	fclose(fp);
	return 0;

}

void read_ebd_file(FILE *fp, struct TIM_Header list[]) {

	struct EBD_Mesh *mesh_list;
	struct Mesh_Header *header;
	uint32_t ofs, memory, nb_mesh, i, k;
	uint8_t nb_poly;
	struct Vertex *vert_list;
	struct Face *face_list;


	ofs = ftell(fp);
	fseek(fp, ofs + 0x0C, SEEK_SET);
	fread(&memory, sizeof(uint32_t), 1, fp);

	fseek(fp, ofs + 0x800, SEEK_SET);
	fread(&nb_mesh, sizeof(uint32_t), 1, fp);

	printf("Number of meshes: %d\n", nb_mesh);
	mesh_list = malloc(nb_mesh * sizeof(struct EBD_Mesh));
	fread(mesh_list, sizeof(struct EBD_Mesh), nb_mesh, fp);

	for(i = 0; i < nb_mesh; i++) {

		if(mesh_list[i].mesh_ofs) {
			mesh_list[i].mesh_ofs -= memory;
			mesh_list[i].mesh_ofs += 0x800;
			mesh_list[i].mesh_ofs += ofs;
		}

		if(mesh_list[i].bone_ofs) {
			mesh_list[i].bone_ofs -= memory;
			mesh_list[i].bone_ofs += 0x800;
			mesh_list[i].bone_ofs += ofs;
		}

		if(mesh_list[i].anim_ofs) {
			mesh_list[i].anim_ofs -= memory;
			mesh_list[i].anim_ofs += 0x800;
			mesh_list[i].anim_ofs += ofs;
		}

	}


	for(i = 0; i < nb_mesh; i++) {
		
		if(mesh_list[i].number != 0x0c60) {
			continue;
		}

		printf("Reading Drache mesh 0x%08x\n", mesh_list[i].mesh_ofs);
		fseek(fp, mesh_list[i].mesh_ofs + 0x11, SEEK_SET);
		fread(&nb_poly, 1, 1, fp);

		printf("Number of polygons: %d\n", nb_poly);
		fseek(fp, mesh_list[i].mesh_ofs + 0x90, SEEK_SET);
		printf("Current position: 0x%08lx\n", ftell(fp));
		
		header = malloc(sizeof(struct Mesh_Header) * nb_poly);
		fread(header, sizeof(struct Mesh_Header), nb_poly, fp);
		
		for(k = 0; k < nb_poly; k++) {
			
			if(!header[k].vert_ofs) {
				continue;
			}

			header[k].vert_ofs -= memory;
			header[k].vert_ofs += 0x800;
			header[k].vert_ofs += ofs;
			
			if(header[k].tri_ofs) {
				header[k].tri_ofs -= memory;
				header[k].tri_ofs += 0x800;
				header[k].tri_ofs += ofs;
			} else {
				header[k].nb_tri = 0;
			}

			if(header[k].quad_ofs) {
				header[k].quad_ofs -= memory;
				header[k].quad_ofs += 0x800;
				header[k].quad_ofs += ofs;
			} else {
				header[k].nb_quad = 0;
			}

			vert_list = malloc(sizeof(struct Vertex) * header[k].nb_vert);
			fseek(fp, header[k].vert_ofs, SEEK_SET);
			fread(vert_list, sizeof(struct Vertex), header[k].nb_vert, fp);

			face_list = malloc(sizeof(struct Face) * (header[k].nb_tri +
			header[k].nb_quad));
			
			if(header[k].tri_ofs) {
				fseek(fp, header[k].tri_ofs, SEEK_SET);
				fread(face_list, sizeof(struct Face), header[k].nb_tri, fp);
			}

			if(header[k].quad_ofs) {
				fseek(fp, header[k].quad_ofs, SEEK_SET);
				fread(&face_list[header[k].nb_tri], sizeof(struct Face), header[k].nb_quad, fp);
			}

			free(vert_list);
			free(face_list);

		}

		free(header);

	}


	free(mesh_list);

}
