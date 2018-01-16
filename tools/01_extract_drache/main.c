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
};

struct TIM_Image {
	struct TIM_Header header;
	uint16_t *pallet;
	uint8_t *body;
};

int main(int argc, char *argv[]) {

	FILE *fp;
	
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

	uint32_t file_len, ofs, i;
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

	for(i = 0; i < nb_tim; i++) {
		printf("Reading TIM file at 0x%08x\n", tim_ofs[i]);
	}

	// Close and return

	fclose(fp);
	return 0;

}
