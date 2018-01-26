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
    File  : psx_types.h

-- */

#include <stdint.h>

struct PSX_Tim_Image {
	uint32_t type;
	uint32_t size;
	uint32_t mode;
	uint32_t pallet_x;
	uint32_t pallet_y;
	uint32_t nb_color;
	uint32_t nb_pallet;
	uint32_t image_x;
	uint32_t image_y;
	uint32_t width;
	uint32_t height;
	uint32_t offset;
	uint8_t nop[16];
	char image_name[0x20];
};

struct PSX_EBD_Model {
	uint32_t id;
	uint32_t mesh_ofs;
	uint32_t bone_ofs;
	uint32_t anim_ofs;
};

struct PSX_EBD_Model_List {
	uint32_t offset;
	uint32_t memory;
	uint32_t nb_model;
	struct PSX_EBD_Model *model_list;
};

struct PSX_EBD_Mesh {
	uint8_t nb_tri;
	uint8_t nb_quad;
	uint8_t nb_vert;
	uint8_t nop;
	uint32_t tri_ofs;
	uint32_t quad_ofs;
	uint16_t image_page;
	uint16_t pallet_page;
	uint32_t vert_ofs;
};

struct PSX_EBD_Vertex {
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t nop;
};

struct PSX_EBD_Face {
	struct {
		uint8_t u;
		uint8_t v;
	} coord[4];
	uint8_t indice[4];
};

struct PSX_Framebuffer {
	uint32_t nb_tex;
	struct PSX_Tim_Image *tex_list;
};