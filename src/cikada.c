#include <config.h>
#include <glib/gi18n.h>
#include "ckd-slides.h"

#define CKD_STAGE_WIDTH  640
#define CKD_STAGE_HEIGHT 480

static gboolean _ckd_fullscreen = FALSE;
static gint     _ckd_segment_length = 3;

static GOptionEntry _ckd_entries[] =
{
        {"fullscreen", 'f', 0, G_OPTION_ARG_NONE, &_ckd_fullscreen, N_("Set fullscreen mode"), NULL},
        {"segment-length", 's', 0, G_OPTION_ARG_INT, &_ckd_segment_length, N_("Set segment length of slides"), "N"},
        {NULL}
};


static void
_on_stage_allocation_change (ClutterActor          *actor,
                             ClutterActorBox       *box,
                             ClutterAllocationFlags flags,
                             gpointer               user_data)
{
        gfloat w, h;
        w = clutter_actor_box_get_width (box);
        h = clutter_actor_box_get_height (box);

        CkdSlides *slides = user_data;
        ClutterActor *slides_box;

        g_object_get (slides, "box", &slides_box, NULL);
        clutter_actor_set_size (slides_box, w, h);

        /* 如果视图模式是概览视图，那么就将其关闭 */
        gboolean is_overview;
        g_object_get (slides, "is-overview", &is_overview, NULL);
        if (is_overview)
                ckd_slides_overview_off (slides);
}

static gboolean
_on_stage_key_press (ClutterActor *actor, ClutterEvent *event, gpointer user_data)
{
        CkdSlides *slides = user_data;
        ClutterActor *box;
        guint keyval = clutter_event_get_key_symbol (event);
        gboolean is_overview = FALSE;
        
        switch (keyval) {
        case CLUTTER_KEY_Left:
        case CLUTTER_KEY_Up:
                ckd_slides_switch_to_next_slide (slides, -1);
                break;
        case CLUTTER_KEY_Right:
        case CLUTTER_KEY_Down:
                ckd_slides_switch_to_next_slide (slides, 1);
                break;
        case CLUTTER_KEY_Escape:
                break;
        case CLUTTER_KEY_F11:
                if (clutter_stage_get_fullscreen (CLUTTER_STAGE(actor)))
                        clutter_stage_set_fullscreen (CLUTTER_STAGE(actor), FALSE);
                else
                        clutter_stage_set_fullscreen (CLUTTER_STAGE(actor), TRUE);
                break;
        case CLUTTER_KEY_O:
        case CLUTTER_KEY_o:
                g_object_get (slides, "is-overview", &is_overview, NULL);
                if (!is_overview) 
                        ckd_slides_overview_on (slides);
                else
                        ckd_slides_overview_off (slides);
                break;
        default:
                break;
        }
        
        return TRUE;
}

static gboolean
_on_stage_button_press (ClutterActor *actor, ClutterEvent * event, gpointer user_data)
{
        guint button_pressed;
        CkdSlides *slides = user_data;

        button_pressed = clutter_event_get_button (event);

        if (button_pressed == 1)
                ckd_slides_switch_to_next_slide (slides, 1);

        if (button_pressed == 3)
                ckd_slides_switch_to_next_slide (slides, -1);

        return TRUE;
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
        
        context = g_option_context_new (_("filename.pdf - Cikada is a presentation tool for PDF slides"));
        g_option_context_add_main_entries (context, _ckd_entries, PACKAGE);

        if (!g_option_context_parse (context, &argc, &argv, NULL))
                return -1;
        if (argv[1] == NULL) {
                g_print (_("You should input PDF file name!\n"));
                return -1;
        }
        
        ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };
        ClutterActor *stage = clutter_stage_get_default ();
        clutter_stage_set_minimum_size(stage, CKD_STAGE_WIDTH, CKD_STAGE_HEIGHT);
        clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
        clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), TRUE);

        /* 设置全屏。此处的全屏，只对 clutter_stage_get_default 有效，这是 Clutter 自身的问题！！！ */
        if (_ckd_fullscreen) {
                clutter_stage_set_fullscreen (CLUTTER_STAGE(stage), TRUE);
        }
        
        ClutterLayoutManager *layout = clutter_fixed_layout_new ();
        ClutterActor *slides_box = clutter_box_new (layout);
        clutter_actor_set_size (slides_box, CKD_STAGE_WIDTH, CKD_STAGE_HEIGHT);
        clutter_actor_set_reactive (slides_box, TRUE);
        clutter_container_add_actor (CLUTTER_CONTAINER(stage), slides_box);
        
        /* 创建 CkdSlides 实例 */
        CkdSlides *slides = g_object_new (CKD_TYPE_SLIDES,
                                          "document-path", argv[1],
                                          "capacity", _ckd_segment_length,
                                          "box", slides_box,
                                          NULL);
        
        g_signal_connect (stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
        g_signal_connect (stage, "key-press-event", G_CALLBACK (_on_stage_key_press), slides);
        g_signal_connect (stage, "allocation-changed", G_CALLBACK(_on_stage_allocation_change), slides);
        g_signal_connect (slides_box, "button-press-event", G_CALLBACK(_on_stage_button_press), slides);
        
        clutter_actor_show_all (stage);

        clutter_main ();

        g_object_unref (G_OBJECT(slides));
        
        return 0;
}
