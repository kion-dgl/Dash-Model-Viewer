"use strict";

const fs = require("fs");
const path = require("path");
const async = require("async");
const PNG = require("pngjs").PNG;

/*

Index Number of Primitives Bone Anim    Name     Image Page Pallet Page Texture
0     9                    yes  yes  arukoitan                          EM0200
1     5                    yes  yes  hokkoro                            EM0300
2     5                    yes  yes  shekuten                           EM0400
3     2                    yes  no   orudakoitan                        EM0e00
4     11                   yes  yes  junkshop                           EM1300B
5     1                    no   no   mouse                              MOUSE
6     4                    yes  yes  mirumijee                          EM0400
7     2                    yes  yes  chest                              EM3500

*/

var ofs = 0;
var fp = fs.readFileSync(process.argv[2]);
var framebuffer = [];
var model_list = [];
var offset = null;
var index = 0;

var mat_lookup = [3, 4, 5, 6, 7, 8, 5, 9];
var name_lookup = [
	"arukoitan.gltf",
	"hokkoro.gltf",
	"shekuten.gltf",
	"orudakoitan.gltf",
	"junkshop.gltf",
	"mouse.gltf",
	"mirumijee.gltf",
	"chest.gltf"
];

do {

	var str = fp.toString("ascii", ofs+0x40, ofs + 0x60);
	
	if(str.indexOf("..") !== 0) {
		continue;
	}

	var ext = path.extname(str);

	if(ext.localeCompare(".EBD") === 0) {
		offset = ofs;
	}

	if(ext.localeCompare(".TIM") !== 0) {
		continue;
	}
	
	framebuffer.push({
		"index" : index++,
		"offset": ofs,
		"pallet_x" : fp.readUInt32LE(ofs + 0x0c),
		"pallet_y" : fp.readUInt32LE(ofs + 0x10),
		"nb_colors" : fp.readUInt32LE(ofs + 0x14),
		"nb_pallet" : fp.readUInt32LE(ofs + 0x18),
		"pallet_x_bin" : fp.readUInt32LE(ofs + 0x0c).toString(2),
		"pallet_y_bin" : fp.readUInt32LE(ofs + 0x10).toString(2),
		"image_x" : fp.readUInt32LE(ofs + 0x1C),
		"image_y" : fp.readUInt32LE(ofs + 0x20),
		"width" : fp.readUInt32LE(ofs + 0x24),
		"height" : fp.readUInt32LE(ofs + 0x28),
		"image_x_bin" : fp.readUInt32LE(ofs + 0x1C).toString(2),
		"image_y_bin" : fp.readUInt32LE(ofs + 0x20).toString(2),
		"name" : str.replace(/\0/g, "")
	});

} while( (ofs += 0x400) < fp.length);

if(offset === null) {
	console.log("No ebd file located in this file");
	process.exit(1);
}

var json_str = JSON.stringify(framebuffer, null, "\t");
fs.writeFileSync("framebuffer.json", json_str);

var memory_ofs = fp.readUInt32LE(offset + 0x0C);
var nb_models = fp.readUInt32LE(offset + 0x800);

console.log("PSX memory offset: 0x%s", memory_ofs.toString(16));
console.log("Number of models: %d", nb_models);

ofs = offset + 0x804;
for(var i = 0; i < nb_models; i++) {
	
	var mdl = {
		"index" : i,
		"mesh_ofs" : fp.readUInt32LE(ofs + 0x04),
		"bone_ofs" : fp.readUInt32LE(ofs + 0x08),
		"anim_ofs" : fp.readUInt32LE(ofs + 0x0c)
	};

	if(mdl.mesh_ofs) {
		mdl.mesh_ofs -= memory_ofs;
		mdl.mesh_ofs += offset;
		mdl.mesh_ofs += 0x800;
	}

	if(mdl.bone_ofs) {
		mdl.bone_ofs -= memory_ofs;
		mdl.bone_ofs += offset;
		mdl.bone_ofs += 0x800;
	}

	if(mdl.anim_ofs) {
		mdl.anim_ofs -= memory_ofs;
		mdl.anim_ofs += offset;
		mdl.anim_ofs += 0x800;
	}

	model_list.push(mdl);

	ofs += 0x10;

}

model_list = [
	model_list[0],
	model_list[1],
	model_list[2],
	model_list[3],
	model_list[4],
	model_list[5],
	model_list[6],
	model_list[7]
];

async.eachSeries(model_list, function(model, nextModel) {

	var ofs = model.mesh_ofs;
	model.nb_prim = fp.readUInt8(ofs + 0x11);

	model.prim = [];
	model.mats = [];
	model.imgs = [];
	model.vert_list = [];
	model.face_list = [];

	console.log("\nSTARTING INDEX: %d", model.index);
	console.log("Number of primitives: %d", model.nb_prim);
	
	// Read primitive header

	ofs += 0x90;
	for(var i = 0; i < model.nb_prim; i++) {
		
		var prim = {
			"nb_tri" : fp.readUInt8(ofs + 0x00),
			"nb_quad" : fp.readUInt8(ofs + 0x01),
			"nb_vert" : fp.readUInt8(ofs + 0x02),
			"bone" : fp.readUInt8(ofs + 0x03) + 1,
			"tri_ofs" : fp.readUInt32LE(ofs + 0x04),
			"quad_ofs" : fp.readUInt32LE(ofs + 0x08),
			"image_page" : fp.readUInt16LE(ofs + 0x0c),
			"pallet_page" : fp.readUInt16LE(ofs + 0x0e),
			"texture" : fp.readUInt32LE(ofs + 0x0C),
			"vert_ofs" : fp.readUInt32LE(ofs + 0x10)
		};

		if(prim.tri_ofs) {
			prim.tri_ofs -= memory_ofs;
			prim.tri_ofs += offset;
			prim.tri_ofs += 0x800;
		}

		if(prim.quad_ofs) {
			prim.quad_ofs -= memory_ofs;
			prim.quad_ofs += offset;
			prim.quad_ofs += 0x800;
		}

		if(prim.vert_ofs) {
			prim.vert_ofs -= memory_ofs;
			prim.vert_ofs += offset;
			prim.vert_ofs += 0x800;
		}

		ofs += 0x14;
		model.prim.push(prim);

	}

	for(var i = 0; i < model.nb_prim; i++) {
		
		var image_page = model.prim[i].image_page;
		var pallet_page = model.prim[i].pallet_page;
		var texture = model.prim[i].texture;
		
		if(model.mats.indexOf(texture) === -1) {
			model.mats.push(texture);
		}

	}

	// Ideally we'd find the pallet and image, lookup table for now

	for(var i = 0; i < model.mats.length; i++) {
		
		var y, x, by, bx, pos, byte;
		var block_width, block_height, inc;

		var pallet_frame = framebuffer[mat_lookup[model.index]];
		var image_frame = framebuffer[mat_lookup[model.index]];

		var nb_colors = pallet_frame.nb_colors;
		var width = image_frame.width;
		var height = image_frame.height;

		var pallet = [];
		var image_body = [];
		ofs = pallet_frame.offset + 0x100;
		for(var k = 0; k < nb_colors; k++) {
			pallet.push(fp.readUInt16LE(ofs));
			ofs += 0x02;
		}
		
		console.log("number of colors: %d", nb_colors);

		switch(nb_colors) {
			case 16:
				
				width *= 4;
				inc = 2;
				block_height = 32;
				block_width = 128;

			break;
			case 256:
				
				width *= 2;
				inc = 1;
				block_height = 32;
				block_width = 64;

			break;
		}

		// Start body generate

		ofs = image_frame.offset + 0x800;

		for(y = 0; y < height; y += block_height) {
			for(x = 0; x < width; x += block_width) {
				for(by = 0; by < block_height; by++) {
					for(bx = 0; bx < block_width; bx += inc) {
						
						byte = fp.readUInt8(ofs++);

						switch(nb_colors) {
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
		
		// End body generate

		ofs = 0;
		var buffer = Buffer.alloc(image_body.length * 4);
		
		for(var k = 0; k < image_body.length; k++) {
			
			buffer[ofs++] = ((image_body[k] >> 0x00) & 0x1f) << 3;
			buffer[ofs++] = ((image_body[k] >> 0x05) & 0x1f) << 3;
			buffer[ofs++] = ((image_body[k] >> 0x0a) & 0x1f) << 3;

			if(image_body[k] === 0) {
				buffer[ofs++] = 0;
			} else {
				buffer[ofs++] = 0xFF;
			}

		}
		
		var image = {
			"texture" : model.mats[i],
			"width" : width,
			"height" : height,
			"name" : image_frame.name,
			"data" : buffer
		};

		var png_buffer = PNG.sync.write(image);
		image.base64 = png_buffer.toString("base64");
		model.imgs.push(image);
		
	}	
	
	// Read Vertice and Face Lists

	model.prim.forEach(function(prim) {

		var texture = prim.texture;
		for(var i = 0; i < model.imgs.length; i++) {
			if(model.imgs[i].texture !== texture) {
				continue;
			}

			prim.mat = i;
			break;
		}
		
		var image = model.imgs[prim.mat];

		var ofs = prim.vert_ofs;
		var vert_list = [];
		prim.face_list = [];
		var indices = new Array(4);

		for(var i = 0; i < prim.nb_vert; i++) {
			
			vert_list.push({
				"b" : prim.bone,
				"x" : fp.readInt16LE(ofs + 0x00) * -0.01,
				"y" : fp.readInt16LE(ofs + 0x02) * -0.01,
				"z" : fp.readInt16LE(ofs + 0x04) * -0.01,
			});
			
			ofs += 0x08;
		}
		

		if(prim.tri_ofs) {
			ofs = prim.tri_ofs;
			
			for(var i = 0; i < prim.nb_tri; i++) {
				
				var face = {
					"tex_coords" : [
						{
							"u" : fp.readUInt8(ofs + 0x00), 
							"v" : fp.readUInt8(ofs + 0x01)
						},
						{
							"u" : fp.readUInt8(ofs + 0x02), 
							"v" : fp.readUInt8(ofs + 0x03)
						},
						{
							"u" : fp.readUInt8(ofs + 0x04), 
							"v" : fp.readUInt8(ofs + 0x05)
						}
					],
					"indices" : [
						fp.readUInt8(ofs + 0x08),
						fp.readUInt8(ofs + 0x09),
						fp.readUInt8(ofs + 0x0a)
					]
				};
				
				ofs += 0x0c;
				
				for(var k = 0; k < 3; k++) {
					
					var v = {};
					v.b = vert_list[face.indices[k]].b;
					v.x = vert_list[face.indices[k]].x;
					v.y = vert_list[face.indices[k]].y;
					v.z = vert_list[face.indices[k]].z;
					v.u = face.tex_coords[k].u / image.width;
					v.v = face.tex_coords[k].v / image.height;
					
					var found = false;
					
					for(var j = 0; j < model.vert_list.length; j++) {
						
						if(model.vert_list[j].b != v.b) {
							continue;
						}
						
						if(model.vert_list[j].x != v.x) {
							continue;
						}
						
						if(model.vert_list[j].y != v.y) {
							continue;
						}
						
						if(model.vert_list[j].z != v.z) {
							continue;
						}
						
						if(model.vert_list[j].u != v.u) {
							continue;
						}
						
						if(model.vert_list[j].v != v.v) {
							continue;
						}
						
						found = true;
						indices[k] = j;
						break;
					}

					if(found) {
						continue;
					}
					
					indices[k] = model.vert_list.length;
					model.vert_list.push(v);

				}
				
				prim.face_list.push(
					indices[0],
					indices[1],
					indices[2]
				);


			}
			
		}

		if(prim.quad_ofs) {
			ofs = prim.quad_ofs;
			
			for(var i = 0; i < prim.nb_quad; i++) {
				
				var face = {
					"tex_coords" : [
						{
							"u" : fp.readUInt8(ofs + 0x00), 
							"v" : fp.readUInt8(ofs + 0x01)
						},
						{
							"u" : fp.readUInt8(ofs + 0x02), 
							"v" : fp.readUInt8(ofs + 0x03)
						},
						{
							"u" : fp.readUInt8(ofs + 0x04), 
							"v" : fp.readUInt8(ofs + 0x05)
						},
						{
							"u" : fp.readUInt8(ofs + 0x06), 
							"v" : fp.readUInt8(ofs + 0x07)
						}
					],
					"indices" : [
						fp.readUInt8(ofs + 0x08),
						fp.readUInt8(ofs + 0x09),
						fp.readUInt8(ofs + 0x0a),
						fp.readUInt8(ofs + 0x0b)
					]
				};
				
				ofs += 0x0c;
				
				for(var k = 0; k < 4; k++) {
					
					var v = {};
					v.b = vert_list[face.indices[k]].b;
					v.x = vert_list[face.indices[k]].x;
					v.y = vert_list[face.indices[k]].y;
					v.z = vert_list[face.indices[k]].z;
					v.u = face.tex_coords[k].u / image.width;
					v.v = face.tex_coords[k].v / image.height;
					
					var found = false;
					
					for(var j = 0; j < model.vert_list.length; j++) {
						
						if(model.vert_list[j].b != v.b) {
							continue;
						}
						
						if(model.vert_list[j].x != v.x) {
							continue;
						}
						
						if(model.vert_list[j].y != v.y) {
							continue;
						}
						
						if(model.vert_list[j].z != v.z) {
							continue;
						}
						
						if(model.vert_list[j].u != v.u) {
							continue;
						}
						
						if(model.vert_list[j].v != v.v) {
							continue;
						}
						
						found = true;
						indices[k] = j;
						break;
					}

					if(found) {
						continue;
					}
					
					indices[k] = model.vert_list.length;
					model.vert_list.push(v);

				}
				
				prim.face_list.push(
					indices[0],
					indices[1],
					indices[2]
				);

				prim.face_list.push(
					indices[1],
					indices[3],
					indices[2]
				);


			}
			
		}

	});
	
	// Create glTF to Export

	var gltf = {
		"asset" : {
			"generator" : "DashGL Exporter",
			"version" : "2.0"
		},
		"scene" : 0,
		"scenes" : [
			{
				"nodes" : [
					0
				]
			}
		],
		"nodes" : [
			{
				"mesh" : 0
			}
		],
		"meshes" : [
			{
				"primitives" : []
			}
		],
		"accessors" : [
			{
				"bufferView" : 0,
				"byteOffset" : 0,
				"componentType" : 5126,
				"count" : model.vert_list.length,
				"max" : [null,null,null],
				"min" : [null,null,null],
				"type" : "VEC3"
			}, {
				"bufferView" : 1,
				"byteOffset" : 0,
				"componentType" : 5126,
				"count" : model.vert_list.length,
				"type" : "VEC2"
			}
		],
		"materials" : [

		],
		"textures" : [

		],
		"images": [

		],
		"bufferViews" : [
			{
				"buffer" : 0,
				"byteOffset" : 0,
				"byteLength" : model.vert_list.length * 20 - 8,
				"byteStride" : 20
			},
			{
				"buffer" : 0,
				"byteOffset" : 12,
				"byteLength" : model.vert_list.length * 20 - 12,
				"byteStride" : 20
			}
		],
		"buffers" : [

		]
	};

	for(var i = 0; i < model.imgs.length; i++) {
		gltf.materials.push({
			"pbrMetallicRoughness" : {
				"baseColorTexture" : {
					"index" : i
				},
				"metallicFactor" : 0.0
			},
			"emissiveFactor" : [0.0,0.0,0.0],
			"name" : model.imgs[i].name
		});
		
		gltf.textures.push({
			"source" : i
		});

		gltf.images.push({
			"uri" : "data:image/png;base64," + model.imgs[i].base64
		});
	}

	var vert_ofs = model.vert_list.length * 20;
	var buff_len = vert_ofs;
	var face_ofs = 0;

	for(var i = 0; i < model.prim.length; i++) {
		
		gltf.meshes[0].primitives.push({
			"attributes" : {
				"POSITION" : 0,
				"TEXCOORD_0" : 1
			},
			"indices" : i + 2,
			"mode" : 4,
			"material" : model.prim[i].mat
		});

		buff_len += model.prim[i].face_list.length * 2;

		gltf.accessors.push({
			"bufferView" : i + 2,
			"byteOffset" : 0,
			"componentType" : 5123,
			"count" : model.prim[i].face_list.length,
			"type" : "SCALAR"
		});

		gltf.bufferViews.push({
			"buffer" : 0,
			"byteOffset" : vert_ofs + face_ofs,
			"byteLength" : model.prim[i].face_list.length * 2,
		});
		
		face_ofs += model.prim[i].face_list.length * 2;
		
	}
	
	var buffer = Buffer.alloc(buff_len);

	for(var i = 0; i < model.vert_list.length; i++) {

		if(gltf.accessors[0].min[0] === null) {
			gltf.accessors[0].min[0] = model.vert_list[i].x;
		}
		if(gltf.accessors[0].min[1] === null) {
			gltf.accessors[0].min[1] = model.vert_list[i].y;
		}
		if(gltf.accessors[0].min[2] === null) {
			gltf.accessors[0].min[2] = model.vert_list[i].z;
		}

		if(gltf.accessors[0].max[0] === null) {
			gltf.accessors[0].max[0] = model.vert_list[i].x;
		}
		if(gltf.accessors[0].max[1] === null) {
			gltf.accessors[0].max[1] = model.vert_list[i].y;
		}
		if(gltf.accessors[0].max[2] === null) {
			gltf.accessors[0].max[2] = model.vert_list[i].z;
		}

		if(model.vert_list[i].x < gltf.accessors[0].min[0]) {
			gltf.accessors[0].min[0] = model.vert_list[i].x;
		}
		if(model.vert_list[i].y < gltf.accessors[0].min[1]) {
			gltf.accessors[0].min[1] = model.vert_list[i].y;
		}
		if(model.vert_list[i].z < gltf.accessors[0].min[2]) {
			gltf.accessors[0].min[2] = model.vert_list[i].z;
		}

		if(model.vert_list[i].x > gltf.accessors[0].max[0]) {
			gltf.accessors[0].max[0] = model.vert_list[i].x;
		}
		if(model.vert_list[i].y > gltf.accessors[0].max[1]) {
			gltf.accessors[0].max[1] = model.vert_list[i].y;
		}
		if(model.vert_list[i].z > gltf.accessors[0].max[2]) {
			gltf.accessors[0].max[2] = model.vert_list[i].z;
		}
	
		buffer.writeFloatLE(model.vert_list[i].x, i * 20 + 0);
		buffer.writeFloatLE(model.vert_list[i].y, i * 20 + 4);
		buffer.writeFloatLE(model.vert_list[i].z, i * 20 + 8);
		buffer.writeFloatLE(model.vert_list[i].u, i * 20 + 12);
		buffer.writeFloatLE(model.vert_list[i].v, i * 20 + 16);

	}
	
	var ofs = model.vert_list.length * 20;

	for(var i = 0; i < model.prim.length; i++) {

		for(var k = 0; k < model.prim[i].face_list.length; k++) {
			
			buffer.writeUInt16LE(model.prim[i].face_list[k], ofs);
			ofs += 2;

		}

	}
	
	gltf.buffers[0] = {
		"byteLength" : buffer.length,
		"uri":"data:application/octet-stream;base64,"+buffer.toString("base64")
	};

	var str = JSON.stringify(gltf, null, "\t");
	fs.writeFileSync(name_lookup[model.index], str);

	nextModel();
	// process.exit(0);

}, function() {

	console.log("Process complete");

});

