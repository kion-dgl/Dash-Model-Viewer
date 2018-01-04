/*
* Copyright 2017,2018 Benjamin Collins
*
*This file is part of Dash-Model-Viewer
*
* Dash-Model-Viewer is free software: you can redistribute it and/or modify it 
* under the terms of the GNU General Public License as published by the Free 
* Software Foundation, either version 3 of the License, or (at your option) any 
* later version.
*
* Dash-Model-Viewer is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
* details.
*
* You should have received a copy of the GNU General Public License along with 
* Dash-Model-Viewer. If not, see http://www.gnu.org/licenses/.
*/


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <gtk/gtk.h>

static void on_realize(GtkGLArea *gl_area);
static void on_render(GtkGLArea *gl_area, GdkGLContext *context);
static void open_file_click(GtkButton *button, gpointer data);
static void copy_bin_file(char *filename);
static void row_select_callback(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);

struct ModelAddress {
	uint32_t enemy_type;
	uint32_t mesh_ofs;
	uint32_t anim_ofs;
	uint32_t bone_ofs;
};

struct {
	FILE *fp;
	uint8_t *buffer;
	uint32_t file_len;
	GtkWidget *listbox;
	uint32_t memory_ofs;
	struct ModelAddress *file_ofs;
} global;

int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *hbox;
	GtkWidget *open_file, *export_file;
	GtkWidget *list_frame, *gl_frame;
	GtkWidget *scrolled_window;
	GtkWidget *gl_area;

	gtk_init(&argc, &argv);

	// Create Window

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// Create Header and Add to Window

	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Dash Model Viewer");
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_window_set_titlebar(GTK_WINDOW(window), header);

	open_file = gtk_button_new_with_label("Open");
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), open_file);
	g_signal_connect(GTK_WIDGET(open_file), "clicked", G_CALLBACK(open_file_click), (gpointer)window);

	export_file = gtk_button_new_with_label("Export");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), export_file);

	// Create horizontal box and add to Window

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_bottom(GTK_WIDGET(hbox), 10);
	gtk_widget_set_margin_top(GTK_WIDGET(hbox), 10);
	gtk_widget_set_margin_start(GTK_WIDGET(hbox), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(hbox), 10);

	gtk_container_add(GTK_CONTAINER(window), hbox);

	// Create Listbox widget Frame

	list_frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), list_frame, FALSE, FALSE, 10);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	global.listbox = gtk_list_box_new();

	gtk_container_add(GTK_CONTAINER(list_frame), scrolled_window);
	gtk_container_add(GTK_CONTAINER(scrolled_window), global.listbox);
	gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scrolled_window), 110);

	g_signal_connect(GTK_WIDGET(global.listbox), "row-selected", G_CALLBACK(row_select_callback), NULL);

	// Create GLArea Frame

	gl_frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), gl_frame, TRUE, TRUE, 0);

	gl_area = gtk_gl_area_new();
	gtk_widget_set_vexpand(gl_area, TRUE);
	gtk_widget_set_hexpand(gl_area, TRUE);
	g_signal_connect(GTK_WIDGET(gl_area), "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(GTK_WIDGET(gl_area), "realize", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(gl_frame), gl_area);

	// Attempt to open default file

	copy_bin_file("ST00_01.BIN");

	// Show all widgets and start program

	gtk_widget_show_all(window);
	gtk_main();

	return 0;

}

static void on_realize(GtkGLArea *gl_area) {

	g_print("On Realize\n");

	gtk_gl_area_make_current(gl_area);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

}

static void on_render(GtkGLArea *gl_area, GdkGLContext *context) {

	g_print("On render\n");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}


static void open_file_click(GtkButton *button, gpointer data) {

	GtkWidget *dialog;
	gint res;

	dialog = gtk_file_chooser_dialog_new(
		"Open File (.BIN)",
		GTK_WINDOW(data),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		"Cancel",
		GTK_RESPONSE_CANCEL,
		"Open",
		GTK_RESPONSE_ACCEPT,
		NULL
	);

	res = gtk_dialog_run(GTK_DIALOG(dialog));

	if(res == GTK_RESPONSE_ACCEPT) {
		
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		g_print("Filename: %s\n", filename);
		copy_bin_file(filename);
		g_free(filename);

	}
	
	gtk_widget_destroy(dialog);

}


static void copy_bin_file(char *filename) {

	// Clear previous file (if exists)

	if(global.fp != NULL) {
		fclose(global.fp);
		free(global.buffer);
	} else {
		g_print("Global file pointer is currently null\n");
	}

	/*
	* TODO : Clear list box
	*/

	// Open new file

	FILE *fp = fopen(filename, "r");
	if(fp == NULL) {
		fprintf(stderr, "Could not open %s for reading\n", filename);
		return;
	}
	
	// Get file length

	fseek(fp, 0, SEEK_END);
	global.file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Read file as buffer

	global.buffer = malloc(global.file_len);
	fread(global.buffer, sizeof(uint8_t), global.file_len, fp);
	global.fp = fmemopen(global.buffer, global.file_len, "r");

	// Free memory and close file handler

	fclose(fp);
	g_print("File Open in memory\n");

	// Attempt to read EBD in Archive
	
	if(global.file_len < 0x800) {
		return;
	}

	char asset_name[0x20];
	fseek(global.fp, 0x40, SEEK_SET);
	fread(asset_name, sizeof(char), 0x20, global.fp);

	char *dot = strrchr(asset_name, '.');
	if(dot == NULL) {
		fprintf(stderr, "Could not find extension\n");
		return;
	}
	
	g_print("Asset Name: %s\n", asset_name);
	g_print("Extension: '%s'\n", dot);

	if(strcmp(dot, ".EBD") != 0) {
		fprintf(stderr, "Assets extension is not EBD\n");
		return;
	}

	fseek(global.fp, 0x0C, SEEK_SET);
	fread(&global.memory_ofs, sizeof(uint32_t), 1, global.fp);

	fseek(global.fp, 0x800, SEEK_SET);
	uint32_t nb_models;
	fread(&nb_models, sizeof(uint32_t), 1, global.fp);
	
	if(global.file_ofs != NULL) {
		free(global.file_ofs);
	}
	global.file_ofs = malloc(sizeof(struct ModelAddress) * nb_models);
	fread(global.file_ofs, sizeof(struct ModelAddress), nb_models, global.fp);

	char mdl_label[0x20];
	int i;

	g_print("Number of models: %d\n", nb_models);

	GtkWidget *row, *label;
	for(i = 0; i < nb_models; i++) {
		sprintf(mdl_label, "Model %03d", i);
		
		label = gtk_label_new(mdl_label);
		gtk_widget_set_halign (GTK_WIDGET(label), GTK_ALIGN_START);

		row = gtk_list_box_row_new();
		gtk_container_add(GTK_CONTAINER(row), label);
		gtk_container_add(GTK_CONTAINER(global.listbox), row);
	}

}

static void row_select_callback(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {

	g_print("New row selected\n");
	if(row == NULL) {
		return;
	}

	GList *children = gtk_container_get_children(GTK_CONTAINER(row));
	const gchar *text = gtk_label_get_text(children->data);
	char *space = strrchr(text, ' ');
	if(space == NULL) {
		return;
	}

	space++;
	int num = atoi(space);
	g_print("Label number: %d\n", num);
	g_print("Psx Memory Offset: 0x%08x\n", global.file_ofs[num].mesh_ofs);

}
