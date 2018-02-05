"use strict";

const fs = require("fs");
const path = require("path");
const async = require("async");

var ofs = 0;
var fp = fs.readFileSync(process.argv[2]);
var framebuffer = [];
var model_list = [];
var offset = null;

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

	var x_bin = fp.readUInt32LE(ofs + 0x1C).toString(2);
	var y_bin = fp.readUInt32LE(ofs + 0x20).toString(2);

	while(x_bin.length < 16) {
		x_bin = "0" + x_bin;
	}

	while(y_bin.length < 16) {
		y_bin = "0" + y_bin;
	}

	framebuffer.push({
		"offset": ofs,
		"pallet" : {
			"x" : fp.readUInt32LE(ofs + 0x0c),
			"y" : fp.readUInt32LE(ofs + 0x10),
			"nb_colors" : fp.readUInt32LE(ofs + 0x14),
			"nb_pallet" : fp.readUInt32LE(ofs + 0x18),
		},
		"image" : {
			"x" : fp.readUInt32LE(ofs + 0x1C),
			"y" : fp.readUInt32LE(ofs + 0x20),
			"width" : fp.readUInt32LE(ofs + 0x24),
			"height" : fp.readUInt32LE(ofs + 0x28),
			"x_bin" : x_bin,
			"y_bin" : y_bin
		},
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

async.eachSeries(model_list, function(model, nextModel) {

	var ofs = model.mesh_ofs;
	model.nb_prim = fp.readUInt8(ofs + 0x11);
	model.prim = [];

	// Read primitive header

	ofs += 0x90;
	for(var i = 0; i < model.nb_prim; i++) {
		
		var prim = {
			"nb_tri" : fp.readUInt8(ofs + 0x00),
			"nb_quad" : fp.readUInt8(ofs + 0x01),
			"nb_vert" : fp.readUInt8(ofs + 0x02),
			"bone" : fp.readUInt8(ofs + 0x03),
			"tri_ofs" : fp.readUInt32LE(ofs + 0x04),
			"quad_ofs" : fp.readUInt32LE(ofs + 0x08),
			"image_page" : fp.readUInt32LE(ofs + 0x0c),
			"pallet_page" : fp.readUInt32LE(ofs + 0x0e),
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

	

	// Read Vertice and Face Lists

	model.prim.forEach(function(prim) {

		var ofs = prim.vert_ofs;
		prim.vert_list = [];

		for(var i = 0; i < prim.nb_vert; i++) {
			
			prim.vert_list.push({
				"x" : fp.readInt16LE(ofs + 0x00) * -0.01,
				"y" : fp.readInt16LE(ofs + 0x02) * -0.01,
				"z" : fp.readInt16LE(ofs + 0x04) * -0.01,
			});
			
			ofs += 0x08;
		}
		
		prim.face_list = [];
		if(prim.tri_ofs) {
			ofs = prim.tri_ofs;
			
			for(var i = 0; i < prim.nb_tri; i++) {
				
				prim.face_list.push({
					"type" : "tri",
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
				});
				
				ofs += 0x0c;

			}

		}
		
		if(prim.quad_ofs) {
			ofs = prim.quad_ofs;
			
			for(var i = 0; i < prim.nb_quad; i++) {
				
				prim.face_list.push({
					"type" : "quad",
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
				});
				
				ofs += 0x0c;

			}

		}

	});

	console.log(model);
	// nextModel();
	process.exit(0);

}, function() {

	console.log("Process complete");

});

