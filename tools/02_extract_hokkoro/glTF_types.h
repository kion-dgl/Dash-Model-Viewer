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

typedef struct {
	float x;
	float y;
	float z;
	float u;
	float v;
} glTF_Vertex;

typedef struct {
	uint16_t a;
	uint16_t b;
	uint16_t c;
} glTF_Face;

typedef struct {
	uint32_t nb_face;
	glTF_Face *face_list;
	uint32_t mat;
} glTF_Primitive;

typedef struct {
	uint32_t tex_page;
	float width;
	float height;
	uint8_t *png;
	uint32_t len;
	char image_name[0x20];
} glTF_Material;

typedef struct {
	uint32_t nb_mat;
	glTF_Material *mat_list;
	uint32_t nb_prim;
	glTF_Primitive *prim_list;
	uint32_t nb_vert;
	glTF_Vertex vert_list[1000];
} glTF_Model;
