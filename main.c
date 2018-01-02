#include <stdint.h>
#include <stdlib.h>
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <gtk/gtk.h>

static void on_realize(GtkGLArea *gl_area);
static void on_render(GtkGLArea *gl_area, GdkGLContext *context);
static void open_file_click(GtkButton *button, gpointer data);
static void copy_bin_file(char *filename);

struct {
	FILE *fp;
	uint32_t file_len;
} global;

int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *hbox;
	GtkWidget *open_file, *export_file;
	GtkWidget *list_frame, *gl_frame;
	GtkWidget *listbox, *scrolled_window;
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
	listbox = gtk_list_box_new();

	gtk_container_add(GTK_CONTAINER(list_frame), scrolled_window);
	gtk_container_add(GTK_CONTAINER(scrolled_window), listbox);
	gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scrolled_window), 80);

	// Create GLArea Frame

	gl_frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), gl_frame, TRUE, TRUE, 0);

	gl_area = gtk_gl_area_new();
	gtk_widget_set_vexpand(gl_area, TRUE);
	gtk_widget_set_hexpand(gl_area, TRUE);
	g_signal_connect(GTK_WIDGET(gl_area), "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(GTK_WIDGET(gl_area), "realize", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(gl_frame), gl_area);

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

	uint8_t *buffer;
	buffer = malloc(global.file_len);
	fread(buffer, sizeof(uint8_t), global.file_len, fp);
	global.fp = fmemopen(buffer, global.file_len, "r");

	// Free memory and close file handler

	free(buffer);
	fclose(fp);

}
