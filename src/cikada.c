#include <config.h>
#include <glib/gi18n.h>
#include "ckd-slides.h"

static gboolean _ckd_fullscreen = FALSE;
static gint     _ckd_segment_length = 3;

static GOptionEntry _ckd_entries[] =
{
        {"fullscreen", 'f', 0, G_OPTION_ARG_NONE, &_ckd_fullscreen, N_("Set fullscreen mode"), NULL},
        {"segment-length", 's', 0, G_OPTION_ARG_INT, &_ckd_segment_length, N_("Set segment length of slides"), "N"},
        {NULL}
};

static gboolean
_on_stage_key_press (ClutterActor *actor, ClutterEvent *event, gpointer user_data)
{
        CkdSlides *slides = user_data;
        guint keyval = clutter_event_get_key_symbol (event);

        switch (keyval) {
        case CLUTTER_KEY_Up:
                ckd_slides_switch_to_next_page (slides, -1);
                break;
        case CLUTTER_KEY_Down:
                ckd_slides_switch_to_next_page (slides, 1);
                break;
        case CLUTTER_KEY_Escape:
                clutter_stage_set_fullscreen (CLUTTER_STAGE(actor), FALSE);
                break;
        case CLUTTER_KEY_F11:
                clutter_stage_set_fullscreen (CLUTTER_STAGE(actor), TRUE);
                break;
        case CLUTTER_KEY_o:
                /* 为概览视图准备的按键事件 */
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
                ckd_slides_switch_to_next_page (slides, 1);

        if (button_pressed == 3)
                ckd_slides_switch_to_next_page (slides, -1);

        return TRUE;
}

static void
_on_stage_allocation_change (ClutterActor          *actor,
                             ClutterActorBox       *box,
                             ClutterAllocationFlags flags,
                             gpointer               user_data)
{
        ClutterActor *slide_box = user_data;
        gfloat w, h;
        
        w = clutter_actor_box_get_width (box);
        h = clutter_actor_box_get_height (box);
        
        clutter_actor_set_size (slide_box, w, h);
}

int
main (int argc, char **argv)
{
        GOptionContext *context;

        ClutterActor *stage, *slide_box;
        ClutterLayoutManager *layout;
        CkdSlides *slides;
        
        ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };
        static gfloat w = 640, h = 480;
                
        if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
                return 1;

        bindtextdomain (PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (PACKAGE, "UTF-8");
        textdomain (PACKAGE);
        
        context = g_option_context_new (_("- Cikada is a presentation tool for PDF slides"));
        g_option_context_add_main_entries (context, _ckd_entries, PACKAGE);

        if (!g_option_context_parse (context, &argc, &argv, NULL))
                return -1;
        if (argv[1] == NULL) {
                g_print (_("You should input PDF file name!\n"));
                return -1;
        }
        
        stage = clutter_stage_get_default ();
        clutter_actor_set_size (stage, w, h);
        clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
        clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), TRUE);

        if (_ckd_fullscreen)
                clutter_stage_set_fullscreen (CLUTTER_STAGE(stage), TRUE);

        layout = clutter_fixed_layout_new ();
        slide_box = clutter_box_new (layout);
        clutter_actor_set_size (slide_box, w, h);
        clutter_container_add_actor (CLUTTER_CONTAINER(stage), slide_box);
        
        /* 创建 CkdSlides 实例 */
        slides = ckd_slides_new_with_segment_length (slide_box, argv[1], _ckd_segment_length);

        g_signal_connect (stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
        g_signal_connect (stage, "key-press-event", G_CALLBACK (_on_stage_key_press), slides);
        g_signal_connect (stage, "button-press-event", G_CALLBACK(_on_stage_button_press), slides);
        g_signal_connect (stage, "allocation-changed", G_CALLBACK(_on_stage_allocation_change), slide_box);
        
        clutter_actor_show_all (stage);

        clutter_main ();

        return 0;
}
