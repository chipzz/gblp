#ifndef __GBLP_APP_WINDOW_H__
#define __GBLP_APP_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GBLP_TYPE_APP_WINDOW (gblp_app_window_get_type())

G_DECLARE_FINAL_TYPE(GBLPAppWindow, gblp_app_window, GBLP, APP_WINDOW, GtkApplicationWindow)

GBLPAppWindow *gblp_app_window_new(GtkApplication *app);

void gblp_app_window_load_blp(GBLPAppWindow *const window, GFile *const blpfile);
void gblp_app_window_load_dialog(GBLPAppWindow *const window, const char *const title_text, const char *const button_text);

G_END_DECLS

#endif /* __GBLP_APP_WINDOW_H__ */
