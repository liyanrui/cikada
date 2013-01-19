#include <config.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "ckd-meta-slides.h"
#include "ckd-view.h"
#include "ckd-player.h"
#include "ckd-script.h"

#define CKD_STAGE_WIDTH  640
#define CKD_STAGE_HEIGHT 480

static gboolean _ckd_fullscreen = FALSE;
static gdouble _ckd_scale = 1.5;
static gchar  *_ckd_cache = "on";

static GOptionEntry _ckd_entries[] =
{
        {"fullscreen", 'f', 0, G_OPTION_ARG_NONE, &_ckd_fullscreen,
         N_("Set fullscreen mode"), NULL},
        {"scale=NUMBER", 's', 0, G_OPTION_ARG_DOUBLE, &_ckd_scale,
         N_("Scales slides with the given factors"), NULL},
        {"cache", 'c', 0, G_OPTION_ARG_STRING, &_ckd_cache,
         N_("Set slides cache mode."), NULL},
        {NULL}
};

static gdouble
ckd_get_slides_scale (PopplerDocument *pdf_doc)
{
        gdouble scale = 1.0;
        
        gint n_of_pages = poppler_document_get_n_pages (pdf_doc);
        gdouble min_size = G_MAXDOUBLE, w, h, size;
        PopplerPage *page;
        for (gint i = 0; i < n_of_pages; i++) {
                page = poppler_document_get_page (pdf_doc, i);
                poppler_page_get_size (page, &w, &h);
                size = (w > h) ? w : h;
                if (min_size > size)
                        min_size = size;
                
                g_object_unref (page);
        }
        
        if (min_size > 0.0 && min_size < CKD_META_SLIDES_LOWEST_RESOLUTION) {
                scale = CKD_META_SLIDES_LOWEST_RESOLUTION / min_size;
        }
        
        return scale;
}

int
main (int argc, char **argv)
{
        GOptionContext *context;

        if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
                return 1;

        bindtextdomain (PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (PACKAGE, "UTF-8");
        textdomain (PACKAGE);

        context = g_option_context_new (_("filename.pdf\n\nDescription:\n"
                                          "Cikada is a presentation tool for PDF slides"));
        g_option_context_add_main_entries (context, _ckd_entries, PACKAGE);

        if (!g_option_context_parse (context, &argc, &argv, NULL))
                return -1;
        if (argv[1] == NULL) {
                g_print (_("You should input PDF file name!\n"));
                return -1;
        }

        /* \begin create stage */
        ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };
        ClutterActor *stage = clutter_stage_new ();
        clutter_stage_set_minimum_size(CLUTTER_STAGE(stage), CKD_STAGE_WIDTH, CKD_STAGE_HEIGHT);
        clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), TRUE);
        clutter_actor_set_background_color (stage, &stage_color);

        if (_ckd_fullscreen) {
                clutter_stage_set_fullscreen (CLUTTER_STAGE(stage), TRUE);
        }
        /* \end */

        /* \begin 获取 pdf 文档并确定页面默认缩放比例基准 */
        GFile *source = g_file_new_for_path (argv[1]);
        gchar *pdf_uri = g_file_get_uri (source);
        PopplerDocument *pdf_doc = poppler_document_new_from_file (pdf_uri, NULL, NULL);
        g_free (pdf_uri);
        /* \end */

        /* \begin 创建幻灯片元集及其缓存 */
        CkdMetaSlidesCacheMode cache_mode;
        if (g_str_equal (_ckd_cache, "off"))
                cache_mode = CKD_META_SLIDES_NO_CACHE;
        else
                cache_mode = CKD_META_SLIDES_DISK_CACHE;

        CkdMetaSlides *meta_slides = g_object_new (CKD_TYPE_META_SLIDES,
                                                   "source",
                                                   source,
                                                   "pdf-doc",
                                                   pdf_doc,
                                                   "cache-mode",
                                                   cache_mode,
                                                   "scale",
                                                   _ckd_scale * ckd_get_slides_scale (pdf_doc),
                                                   NULL);
        ckd_meta_slides_create_cache (meta_slides);
        /* \end */
        
        /* \begin 创建幻灯片视图 */
        ClutterColor *progress_bar_color = clutter_color_new (51, 51, 51, 255);
        ClutterColor *nonius_color = clutter_color_new (151, 0, 0, 255);
        gdouble progress_bar_vsize = 16.0;
        
        CkdView *view = g_object_new (CKD_TYPE_VIEW,
                                      "stage", stage,
                                      "meta-slides", meta_slides,
                                      "bar-color", progress_bar_color,
                                      "nonius-color", nonius_color,
                                      "bar-vsize", progress_bar_vsize,
                                      NULL);
        /* \end */

        /* \begin 创建 player */
        CkdPlayer *player = g_object_new (CKD_TYPE_PLAYER,
                                          "view", view,
                                          "am-time", 1000,
                                          NULL);
        /* \end */
        
        g_signal_connect (stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);

        clutter_actor_show (stage);
        clutter_main ();

        g_object_unref (player);

        return 0;
}
