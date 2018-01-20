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

struct glTF_Header {
	uint32_t magic;
	uint32_t version;
	uint32_t length;
};

struct glTF_Chunk {
	uint32_t length;
	uint32_t type;
};

struct glTF_Primitive {
	uint32_t nb_vert;
	uint32_t nb_face;
	float vert_list[1024 * 5];
	uint16_t face_list[1024 * 3];
	float max[3];
	float min[3];
};

void read_ebd_file(FILE *fp, struct TIM_Header list[]);
void glTF_convert_primitive(struct Mesh_Header h, struct Vertex v_list[], struct Face f_list[], struct glTF_Primitive *p);
void glTF_export(struct glTF_Primitive *p, uint32_t type);

int main(int argc, char *argv[]) {

	FILE *fp;
	struct glTF_Primitive prim;
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
	struct glTF_Primitive prim;

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

			
			glTF_convert_primitive(header[k], vert_list, face_list, &prim);
			
			printf("Number of verices: %d\n", prim.nb_vert);
			printf("Number of faces: %d\n", prim.nb_face);

			free(vert_list);
			free(face_list);

		}

		free(header);
		glTF_export(&prim, mesh_list[i].number);

	}


	free(mesh_list);

}

void glTF_convert_primitive(struct Mesh_Header h, struct Vertex v_list[], struct Face f_list[], struct glTF_Primitive *p) {

	int i, k, j, found;
	uint16_t index, indice[4];
	float x, y, z, u, v;

	p->nb_vert = 0;
	p->nb_face = 0;

	for(i = 0; i < h.nb_tri; i++) {

		for(k = 0; k < 3; k++) {
			index = f_list[i].indice[k];
			x = -0.01f * (float)v_list[index].x;
			y = -0.01f * (float)v_list[index].y;
			z = -0.01f * (float)v_list[index].z;
			u = (float)f_list[i].coord[k].u / 256.0f;
			v = (float)f_list[i].coord[k].v / 128.0f;

		
			found = 0;

			for(j = 0; j < p->nb_vert; j++) {
			
				if(p->vert_list[j*5+0] != x) {
					continue;
				}
				if(p->vert_list[j*5+0] != y) {
					continue;
				}
				if(p->vert_list[j*5+0] != z) {
					continue;
				}
				if(p->vert_list[j*5+0] != u) {
					continue;
				}
				if(p->vert_list[j*5+0] != v) {
					continue;
				}
			
				indice[k] = j;
				found = 1;
				break;
			}

			if(found) {
				continue;
			}

			p->vert_list[p->nb_vert*5+0] = x;
			p->vert_list[p->nb_vert*5+1] = y;
			p->vert_list[p->nb_vert*5+2] = z;
			p->vert_list[p->nb_vert*5+3] = u;
			p->vert_list[p->nb_vert*5+4] = v;
		
			indice[k] = p->nb_vert;
			p->nb_vert++;
		}

		p->face_list[p->nb_face*3+0] = indice[0];
		p->face_list[p->nb_face*3+1] = indice[1];
		p->face_list[p->nb_face*3+2] = indice[2];
		p->nb_face++;

	}

	for(i = h.nb_tri; i < h.nb_tri + h.nb_quad; i++) {

		for(k = 0; k < 4; k++) {
			index = f_list[i].indice[k];
			x = -0.01f * (float)v_list[index].x;
			y = -0.01f * (float)v_list[index].y;
			z = -0.01f * (float)v_list[index].z;
			u = (float)f_list[i].coord[k].u / 256.0f;
			v = (float)f_list[i].coord[k].v / 128.0f;

		
			found = 0;

			for(j = 0; j < p->nb_vert; j++) {
			
				if(p->vert_list[j*5+0] != x) {
					continue;
				}
				if(p->vert_list[j*5+0] != y) {
					continue;
				}
				if(p->vert_list[j*5+0] != z) {
					continue;
				}
				if(p->vert_list[j*5+0] != u) {
					continue;
				}
				if(p->vert_list[j*5+0] != v) {
					continue;
				}
			
				indice[k] = j;
				found = 1;
				break;
			}

			if(found) {
				continue;
			}

			p->vert_list[p->nb_vert*5+0] = x;
			p->vert_list[p->nb_vert*5+1] = y;
			p->vert_list[p->nb_vert*5+2] = z;
			p->vert_list[p->nb_vert*5+3] = u;
			p->vert_list[p->nb_vert*5+4] = v;
		
			indice[k] = p->nb_vert;
			p->nb_vert++;
		}

		p->face_list[p->nb_face*3+0] = indice[0];
		p->face_list[p->nb_face*3+1] = indice[1];
		p->face_list[p->nb_face*3+2] = indice[2];
		p->nb_face++;

		p->face_list[p->nb_face*3+0] = indice[1];
		p->face_list[p->nb_face*3+1] = indice[2];
		p->face_list[p->nb_face*3+2] = indice[3];
		p->nb_face++;

	}

	/*
	FILE *fp = fopen("drache.obj", "w");

	for(i = 0; i < p->nb_vert; i++) {
		fprintf(fp, "v %.02f %.02f %.02f\n", 
			p->vert_list[i*5+0],
			p->vert_list[i*5+1],
			p->vert_list[i*5+2]
		);
	}

	for(i = 0; i < p->nb_face; i++) {
		fprintf(fp, "f %d %d %d\n", 
			p->face_list[i*3+0] + 1,
			p->face_list[i*3+1] + 1,
			p->face_list[i*3+2] + 1
		);
	}

	fclose(fp);
	*/

}

void glTF_export(struct glTF_Primitive *p, uint32_t type) {
	
	char filename[0x20], json_str[1024*10];
	sprintf(filename, "%04x.glb", type);
	struct glTF_Header header;
	struct glTF_Chunk json_chunk, bin_chunk;
	uint32_t json_len, bin_length;
	uint8_t *json_data, *bin_data;
	uint8_t padding;

	FILE *str;
	str = fmemopen(json_str, 1024*10, "w");
	fprintf(str, "{");
	fprintf(str, "\"asset\":{\"generator\":\"Dash glTF Exporter\",\"version\":\"2.0\"},");
	fprintf(str, "\"scene\":0,\"scenes\":[{\"nodes\":[0]}],\"nodes\":[{\"mesh\":0}],");
	fprintf(str, "\"meshes\":[{\"name\":\"drache\",\"primitives\":[");

	// START Primitive
	fprintf(str, "{\"attributes\":{\"POSITION\":0,\"TEXCOORD_0\":1},\"indices\":2,\"mode\":4,\"material\":0}");
	// END Primitive

    fprintf(str, "]}],\"accessors\":[");

	// START Accessors
	fprintf(str, "{\"bufferView\":0,\"byteOffset\":0,\"componentType\":5126,\"count\":%d,",0);
	fprintf(str, "\"type\":\"VEC3\",\"max\":[%.02f,%.02f,%.02f],\"min\":[%.02f,%.02f,%.02f]},", 0.0,0.0,0.0,0.0,0.0,0.0);
	fprintf(str, "{\"bufferView\":1,\"byteOffset\":0,\"componentType\":5126,\"count\":%d,\"type\":\"VEC2\"},",0);
	fprintf(str, "{\"bufferView\":2,\"byteOffset\":0,\"componentType\":5123,\"count\":%d,\"type\":\"SCALAR\"}", 0);
	// END Accessors

	fprintf(str, "],\"materials\":[");
	// Start Materials
	fprintf(str, "{\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":%d},",0);
	fprintf(str, "\"metallicFactor\":0.0},\"emissiveFactor\":[0.0,0.0,0.0],\"name\": \"SH1500.TIM\"}");
	// End Materials

	fprintf(str, "],\"textures\":[{\"source\":0}],\"images\":[{\"bufferView\":3,\"mimeType\":\"image/png\"}],");
    fprintf(str, "\"bufferViews\":[");

	// Start Buffer Views
	fprintf(str, "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":%d,\"byteStride\":20},", 0);
	fprintf(str, "{\"buffer\":0,\"byteOffset\":12,\"byteLength\":%d,\"byteStride\":20},", 0);
	fprintf(str, "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":%d},", 0, 0);
	fprintf(str, "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":%d}", 0, 0);
	// End Buffer Views

	fprintf(str, "],\"buffers\":[{\"byteLength\":%d}]}", 0);

	fclose(str);

	printf("%s\n", json_str);
	
	padding = strlen(json_str)%4;
	if(padding == 0) {
		padding = 4;
	}
	json_len = strlen(json_str) + padding;
	json_data = malloc(json_len);
	memset(json_data, 0, json_len);
	strcpy(json_data, json_str);

	bin_length = p->nb_vert * 20 + p->nb_face * 6;
	bin_data = malloc(bin_length);
	str = fmemopen(bin_data, bin_length, "w");
	fwrite(p->vert_list, sizeof(float) * 5, p->nb_vert, str);
	fwrite(p->face_list, sizeof(uint16_t) * 3, p->nb_face, str);
	fclose(str);

	FILE *fp = fopen(filename, "w");
	
	if(fp == NULL) {
		fprintf(stderr, "Could not open %s for writing\n", filename);
		return;
	}
	
	header.magic = 0x46546C67;
	header.version = 2;
	header.length = json_len + bin_length;
	
	json_chunk.length = json_len;
	json_chunk.type = 0x4E4F534A;

	bin_chunk.length = bin_length;
	bin_chunk.type = 0x004E4942;

	fwrite(&header, sizeof(struct glTF_Header), 1, fp);
	fwrite(&json_chunk, sizeof(struct glTF_Chunk), 1, fp);
	fwrite(json_data, json_len, 1, fp);
	fwrite(&bin_chunk, sizeof(struct glTF_Chunk), 1, fp);
	fwrite(bin_data, bin_length, 1, fp);

	fclose(fp);
	
	free(json_data);
	free(bin_data);

}
