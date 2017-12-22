#include <gtk/gtk.h>

int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *hbox, *vbox, *rbox;
	GtkWidget *list_frame, *lod_frame, *gl_frame;
	GtkWidget *radio_low, *radio_med, *radio_high;
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

	// Create horizontal box and add to Window

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(window), hbox);

	// Create Listbox widget Frame

	list_frame = gtk_frame_new("Select");
	gtk_box_pack_start(GTK_BOX(hbox), list_frame, FALSE, FALSE, 10);

	// Create Vertical Box and add to Window

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	// Create Level of Detail Frame

	lod_frame = gtk_frame_new("Level of Detail");
	gtk_box_pack_start(GTK_BOX(vbox), lod_frame, FALSE, FALSE, 0);

	radio_low = gtk_radio_button_new_with_label(NULL, "Low");
	radio_med = gtk_radio_button_new_with_label(NULL, "Medium");
	radio_high = gtk_radio_button_new_with_label(NULL, "High");

	gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio_med), GTK_RADIO_BUTTON(radio_low));
	gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio_high), GTK_RADIO_BUTTON(radio_low));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_high), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_med), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio_low), FALSE);

	rbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(lod_frame), rbox);

	gtk_box_pack_start(GTK_BOX(rbox), radio_high, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(rbox), radio_med, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(rbox), radio_low, TRUE, TRUE, 0);

	// Create GLArea Frame

	gl_frame = gtk_aspect_frame_new("Preview", 0.5f, 0.5f, 4.0f/3.0f, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), gl_frame, TRUE, TRUE, 0);

	// Show all widgets and start program

	gtk_widget_show_all(window);
	gtk_main();

	return 0;

}
