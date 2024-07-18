#include "gblp-app-window.h"

static void close_window(GtkWindow *window)
{
}

static GBLPAppWindow *gblp_application_present_new_window(GtkApplication *app)
{
	GBLPAppWindow *const window = gblp_app_window_new(app);
	gtk_window_present(GTK_WINDOW(window));
	return window;
}

static void gblp_application_action_new(GSimpleAction *action, GVariant *parameter, gpointer app)
{
	gblp_application_present_new_window(GTK_APPLICATION(app));
}

static void gblp_application_action_open(GSimpleAction *action, GVariant *parameter, gpointer app)
{
	gblp_app_window_load_dialog(gblp_application_present_new_window(GTK_APPLICATION(app)), "Open File", "_Open");
}

static void gblp_application_action_quit(GSimpleAction *action, GVariant *parameter, gpointer app)
{
	g_application_quit(G_APPLICATION(app));
}

static const GActionEntry gblp_application_action_entries[] =
{
	{ "new", gblp_application_action_new },
	{ "open", gblp_application_action_open },
	{ "quit", gblp_application_action_quit }
};

static void gblp_application_on_startup(GtkApplication *app)
{
//	G_APPLICATION_CLASS(gblp_application_parent_class)->startup(app);
	GtkBuilder *const builder = gtk_builder_new_from_resource("/gblp/gblp-app-menu.ui");
	gtk_application_set_menubar(app, G_MENU_MODEL(gtk_builder_get_object(builder, "menubar")));
	g_object_unref(builder);
	g_action_map_add_action_entries(G_ACTION_MAP(app), gblp_application_action_entries, G_N_ELEMENTS(gblp_application_action_entries), app);
}

static void gblp_application_on_activate(GtkApplication *app)
{
	gblp_application_present_new_window(app);
	/*
	gtk_window_set_title(window, "GBLP");
	g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);
	*/
}

static void gblp_application_on_open(GApplication *app, GFile **files, gint n_files, const gchar *hint, gpointer user_data)
{
	printf("gblp_application_on_open app=%p files=%p n_files=%i hint=%s user_data=%p\n", app, files, n_files, hint, user_data);
	for (int f = 0; f < n_files; f++)
	{
		printf("gblp_application_on_open open file=\"%s\"\n", g_file_get_path(files[f]));
		gblp_app_window_load_blp(gblp_application_present_new_window(GTK_APPLICATION(app)), files[f]);
	}
}

int main(int argc, char *argv[])
{
/*
/home/chipzz/blp/ChipzzCraftedBLP.blp			// RAW3
/home/chipzz/blp/OilSlickEnvA.blp			// RAW3 flags=1
/home/chipzz/blp/SunGlare.blp				// RAW3 flags=136
/home/chipzz/blp/LoadingScreen_BlackrockFoundry.blp	// DXT1 RGB
/home/chipzz/blp/BuyoutIcon.blp				// DXT1 RGBA
/home/chipzz/blp/UI-CharacterCreate-Factions.blp	// DXT3
/home/chipzz/blp/UI-DialogBox-Gold-Corner.blp		// DXT3
/home/chipzz/blp/UI-DialogBox-Header.blp		// DXT3
/home/chipzz/blp/Ability_Rogue_Shadowstep.blp		// DXT5 mips=17
/home/chipzz/blp/Loading-BarGlow.blp			// RAW1 no alpha
/home/chipzz/blp/Cursor/2709/Attack.blp			// RAW1 1 bit alpha
/home/chipzz/blp/TaurenFemaleSkin00_01_Extra.blp	// RAW1 4 bit alpha
/home/chipzz/blp/Cursor/3902/Buy.blp			// RAW1 8 bit alpha
*/
	GtkApplication *const gblp_application = gtk_application_new("com.example.GtkApplication", G_APPLICATION_HANDLES_OPEN);
	g_signal_connect(gblp_application, "open", G_CALLBACK(gblp_application_on_open), NULL);
	g_signal_connect(gblp_application, "startup", G_CALLBACK(gblp_application_on_startup), NULL);
	g_signal_connect(gblp_application, "activate", G_CALLBACK(gblp_application_on_activate), NULL);
	return g_application_run(G_APPLICATION(gblp_application), argc, argv);
}
