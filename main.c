#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <gtk/gtk.h>

static void on_realize(GtkGLArea *gl_area);
static void on_render(GtkGLArea *gl_area, GdkGLContext *context);

int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *hbox;
	GtkWidget *open_file;
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

	open_file = gtk_file_chooser_button_new("Open", GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), open_file);

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
