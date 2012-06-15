#include <config.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "ckd-meta-slides.h"
#include "ckd-view.h"
#include "ckd-player.h"
#include "ckd-slide-script.h"

#define CKD_STAGE_WIDTH  640
#define CKD_STAGE_HEIGHT 480

static gboolean _ckd_fullscreen = FALSE;
static gdouble _ckd_quality = 0.5;

static GOptionEntry _ckd_entries[] =
{
        {"fullscreen", 'f', 0, G_OPTION_ARG_NONE, &_ckd_fullscreen,
         N_("Set fullscreen mode"), NULL},
        {"quality", 'q', 0, G_OPTION_ARG_DOUBLE, &_ckd_quality,
         N_("Set image quality with the given factors"), NULL},
        {NULL}
};

static void
ckd_stage_allocate (ClutterActor *stage,
                    ClutterActorBox *box,
                    ClutterAllocationFlags flags,
                    gpointer view)
{
        gfloat w = clutter_actor_box_get_width (box);
        gfloat h = clutter_actor_box_get_height (box);

        clutter_actor_set_size (CLUTTER_ACTOR(view), w, h);
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

        /* @begin: create stage */
        ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };
        ClutterActor *stage = clutter_stage_get_default ();
        clutter_stage_set_minimum_size(CLUTTER_STAGE(stage), CKD_STAGE_WIDTH, CKD_STAGE_HEIGHT);
        clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), TRUE);
        clutter_actor_set_background_color (stage, &stage_color);

        if (_ckd_fullscreen) {
                clutter_stage_set_fullscreen (CLUTTER_STAGE(stage), TRUE);
        }
        /* @end */

        /* @begin: 获取 pdf 文档 */
        GFile *source = g_file_new_for_path (argv[1]);
        gchar *pdf_uri = g_file_get_uri (source);
        PopplerDocument *pdf_doc = poppler_document_new_from_file (pdf_uri, NULL, NULL);
        g_free (pdf_uri);
        /* @end */

        /* @begin: 创建幻灯片元集及其缓存 */
        CkdMetaSlides *meta_slides = g_object_new (CKD_TYPE_META_SLIDES,
                                                   "source",
                                                   source,
                                                   "pdf-doc",
                                                   pdf_doc,
                                                   "cache-mode",
                                                   CKD_META_SLIDES_DISK_CACHE,
                                                   "scale",
                                                   _ckd_quality * CKD_META_SLIDES_QUALITY_DELTA,
                                                   NULL);
        ckd_meta_slides_create_cache (meta_slides);
        /* @end */

        /* @begin: 创建幻灯片视图 */
        ClutterActor *slide = ckd_meta_slides_get_slide (meta_slides, 0);
        ClutterActor *view = g_object_new (CKD_TYPE_VIEW,
                                           "slide", slide,
                                           NULL);
        clutter_actor_set_size (CLUTTER_ACTOR(view), CKD_STAGE_WIDTH, CKD_STAGE_HEIGHT);
        clutter_actor_add_child (stage, view);
        /* @end */

        /* @begin: 创建 plaer */
        CkdPlayer *player = g_object_new (CKD_TYPE_PLAYER,
                                          "meta-slides", meta_slides,
                                          "view", view,
                                          NULL);

        g_signal_connect (stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
        g_signal_connect (stage, "allocation-changed", G_CALLBACK(ckd_stage_allocate), view);

        clutter_actor_show_all (stage);

        clutter_main ();

        g_object_unref (player);

        return 0;
}
