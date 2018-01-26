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
	File  : glTF_types.h

-- */

#include <stdint.h>

struct glTF_Vertex {
	float x;
	float y;
	float z;
	float u;
	float v;
};

struct glTF_Face {
	uint16_t a;
	uint16_t b;
	uint16_t c;
};

struct glTF_Primitive {
	uint32_t nb_vert;
	struct glTF_Vertex *vert_list;
	uint32_t nb_face;
	struct glTF_Face *face_list;
	uint32_t mat;
};

struct glTF_Material {
	uint32_t pallet_x;
	uint32_t pallet_y;
	uint32_t image_x;
	uint32_t image_y;
	uint32_t image_name[0x20];
	float width;
	float height;
	uint8_t *png;
};

struct glTF_Model {
	uint32_t nb_prim;
	struct glTF_Primitve *prim_list;
	uint32_t nb_mat;
	struct glTF_Material *mat_list;
};
