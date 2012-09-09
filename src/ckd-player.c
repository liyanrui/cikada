#include <math.h>
#include <glib/gprintf.h>
#include "ckd-meta-slides.h"
#include "ckd-view.h"
#include "ckd-player.h"

G_DEFINE_TYPE (CkdPlayer, ckd_player, G_TYPE_OBJECT);

#define CKD_PLAYER_GET_PRIVATE(o) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((o), CKD_TYPE_PLAYER, CkdPlayerPriv))

typedef struct _CkdPlayerPriv CkdPlayerPriv;
struct _CkdPlayerPriv {
        CkdView  *view;

        gboolean slide_number_is_shown;
        ClutterActor *slide_number;
        ClutterActor *slide_number_bg;

        /* 动画持续时间 */
        guint am_time;

        /* 配置文件 */
        GFile *script_file;
        GFileMonitor *script_file_monitor;
        GNode *script;
};

enum {
        PROP_CKD_PLAYER_0,
        PROP_CKD_PLAYER_VIEW,
        PROP_CKD_PLAYER_SCRIPT,
        PROK_CKD_PLAYER_AM_TIME,
        N_CKD_PLAYER_PROPS
};

static gint
ckd_player_get_n_of_slides (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        CkdMetaSlides *meta_slides;
        g_object_get (priv->view, "meta-slides", &meta_slides, NULL);

        GArray *slides_cache;
        gint n_of_slides;
        g_object_get (meta_slides,
                      "n-of-slides", &n_of_slides,
                      NULL);

        return n_of_slides;
}

static GArray *
ckd_player_get_slides_cache (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        CkdMetaSlides *meta_slides;
        g_object_get (priv->view, "meta-slides", &meta_slides, NULL);

        GArray *slides_cache;
        GArray *cache = NULL;
        g_object_get (meta_slides,
                      "cache", &cache,
                      NULL);

        return cache;
}

static CkdMetaSlides *
ckd_player_get_meta_slides (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        CkdMetaSlides *meta_slides;
        g_object_get (priv->view, "meta-slides", &meta_slides, NULL);

        return meta_slides;
}

static gboolean
ckd_player_button_press (ClutterActor *a, ClutterEvent * e, gpointer data)
{
        CkdPlayer *player = data;

        gint button_pressed = clutter_event_get_button (e);

        switch (button_pressed) {
        case 1:
                ckd_player_step (player, 1);
                break;
        case 2:
                break;
        case 3:
                ckd_player_step (player, -1);
                break;
        default:
                break;
        }

        return FALSE;
}

static gboolean
ckd_player_key_press (ClutterActor *actor, ClutterEvent *event, gpointer data)
{

        CkdPlayer *player = data;
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (player);
        
        guint keyval = clutter_event_get_key_symbol (event);
        
        switch (keyval) {
        case CLUTTER_KEY_Left:
        case CLUTTER_KEY_Up:
                ckd_player_step (player, -1);
                break;
        case CLUTTER_KEY_Right:
        case CLUTTER_KEY_Down:
                ckd_player_step (player, 1);
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
                break;
        case CLUTTER_KEY_D:
        case CLUTTER_KEY_d:
                if (priv->slide_number_is_shown) {
                        priv->slide_number_is_shown = FALSE;
                        clutter_actor_hide (priv->slide_number_bg);
                        clutter_actor_hide (priv->slide_number);
                } else {
                        priv->slide_number_is_shown = TRUE;
                        clutter_actor_show (priv->slide_number_bg);
                        clutter_actor_show (priv->slide_number);
                }
                break;
        default:
                break;
        }
        
        return TRUE;
}

static gboolean
ckd_progress_bar_button_press (ClutterActor *a, ClutterEvent * event, gpointer data)
{
        CkdPlayer *player = data;
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (player);

        gfloat x;
        clutter_event_get_coords (event, &x, NULL);

        ClutterActorBox box;
        clutter_actor_get_allocation_box (a, &box);

        gfloat w = clutter_actor_get_width (a);
        gfloat tick = (x - box.x1) / w;

        CkdMetaSlides *meta_slides;
        g_object_get (priv->view, "meta-slides", &meta_slides, NULL);

        gint n_of_slides;
        g_object_get (meta_slides, "n-of-slides", &n_of_slides, NULL);
        
        CkdMetaEntry *e;
        gint next_slide_number;
        gfloat d = G_MAXFLOAT, t;
        for (gint i = 0; i < n_of_slides; i++) {
                e = ckd_meta_slides_get_meta_entry (meta_slides, i);
                t = fabsf (tick - e->tick);
                if (d > t) {
                        d = t;
                        next_slide_number = i;
                }
        }

        gint current_slide_number;
        g_object_get (priv->view, "slide-number", &current_slide_number, NULL);
        
        gint step = next_slide_number - current_slide_number;
        ckd_player_step (player, step);
        
        return TRUE;
}

static GNode *
ckd_player_generate_script (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        CkdMetaSlides *meta_slides = ckd_player_get_meta_slides (self);

        GFile *source;
        g_object_get (meta_slides, "source", &source, NULL);
        
        gchar *path = g_file_get_path (source);
        gchar **splitted = g_strsplit (path, ".", 0);
        GString *script_path = g_string_new (splitted[0]);
        g_string_append (script_path, ".ckd");
        
        priv->script_file = g_file_new_for_path (script_path->str);
        
        if (g_file_query_exists (priv->script_file, NULL)) {
                gint n_of_slides;
                g_object_get (meta_slides,
                              "n-of-slides", &n_of_slides,
                              NULL);
                priv->script = ckd_script_new (script_path->str, n_of_slides);
        }
        
        g_string_free (script_path, TRUE);
        g_strfreev (splitted);
        g_free (path);
}

static void
ckd_player_config_view (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        ClutterActor *stage;

        g_object_get (priv->view, "stage", &stage, NULL);

        /* \begin 设置幻灯片编号牌 */
        clutter_actor_add_child (stage, priv->slide_number_bg);
        clutter_actor_add_child (stage, priv->slide_number);
        clutter_actor_add_constraint (priv->slide_number_bg,
                                      clutter_align_constraint_new (stage,
                                                                    CLUTTER_ALIGN_X_AXIS,
                                                                    1.0));
        clutter_actor_add_constraint (priv->slide_number_bg,
                                      clutter_align_constraint_new (stage,
                                                                    CLUTTER_ALIGN_Y_AXIS,
                                                                    0.5));
        /* \end */
        
        g_signal_connect (stage,
                          "button-press-event",
                          G_CALLBACK (ckd_player_button_press),
                          self);

        g_signal_connect (stage,
                          "key-press-event",
                          G_CALLBACK (ckd_player_key_press),
                          self);

        ClutterActor *bar;
        g_object_get (priv->view, "bar", &bar, NULL);
        g_signal_connect (bar,
                          "button-press-event",
                          G_CALLBACK (ckd_progress_bar_button_press),
                          self);
}

static void
ckd_player_update_slides_cache (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        GArray *slides_cache = ckd_player_get_slides_cache (self);
        gint n_of_slides = ckd_player_get_n_of_slides (self);

        /* 幻灯片备注还未实现 */
        GList *list = ckd_script_output_meta_entry_list (priv->script);
        gint i = 0;
        CkdMetaEntry *e1, *e2;
        GList *iter = g_list_first (list);
        for (; iter != NULL; iter = g_list_next (iter), i++) {
                e1 = iter->data;
                e2 = g_array_index (slides_cache, CkdMetaEntry *, i);
                if (e1->am != CKD_SLIDE_AM_NULL)
                        e2->am = e1->am;
                e2->tick = e1->tick;
        }
}

static void
ckd_player_update_view (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        ClutterColor *bar_color = ckd_script_get_progress_bar_color (priv->script);
        if (bar_color)
                g_object_set (priv->view, "bar-color", bar_color, NULL);
        
        ClutterColor *nonius_color = ckd_script_get_nonius_color (priv->script);
        if (nonius_color)
                g_object_set (priv->view, "nonius-color", nonius_color, NULL);
        
        gfloat bar_vsize = ckd_script_get_progress_bar_vsize (priv->script);
        if (bar_vsize >= 0)
                g_object_set (priv->view, "bar-vsize", bar_vsize, NULL);

        /* \begin 重新调整场景布局并重绘图形 */
        ClutterActor *stage;
        g_object_get (priv->view, "stage", &stage, NULL);
        
        g_signal_emit_by_name(stage, "allocation-changed");
        clutter_actor_queue_redraw (stage);
        /* \end */
}

static gboolean
ckd_player_script_changed (GFileMonitor *monitor,
                           GFile *file,
                           GFile *other,
                           GFileMonitorEvent event_type,
                           gpointer user_data)
{        
        if (event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
                CkdPlayer *player = user_data;
                CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (player);
                
                gint n_of_slides = ckd_player_get_n_of_slides (player);
                
                gchar *script_path = g_file_get_path (file);
                GNode *new_script = ckd_script_new (script_path, n_of_slides);

                if (!ckd_script_equal (priv->script, new_script)) {
                        ckd_script_free (priv->script);
                        priv->script = new_script;

                        ckd_player_update_slides_cache (player);
                        ckd_player_update_view (player);
                }
        }

        return TRUE;
}

static void
ckd_player_watch_script (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        priv->script_file_monitor = g_file_monitor_file (priv->script_file,
                                                         G_FILE_MONITOR_NONE,
                                                         NULL,
                                                         NULL);
        g_signal_connect (priv->script_file_monitor,
                          "changed",
                          (GCallback)ckd_player_script_changed, self);
}

static void
ckd_player_set_property (GObject *o, guint prop, const GValue *v, GParamSpec *p)
{
        CkdPlayer *self = CKD_PLAYER (o);
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_PLAYER_VIEW:
                priv->view = g_value_get_pointer (v);
                ckd_player_config_view (self);
                ckd_player_generate_script (self);
                if (priv->script) {
                        ckd_player_update_slides_cache (self);
                        ckd_player_update_view (self);
                        ckd_player_watch_script (self);
                }
                break;
        case PROK_CKD_PLAYER_AM_TIME:
                priv->am_time = g_value_get_uint (v);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;
        }
}

static void
ckd_player_get_property (GObject *o, guint prop, GValue *v, GParamSpec *p)
{
        CkdPlayer *self = CKD_PLAYER (o);
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_PLAYER_SCRIPT:
                g_value_set_pointer (v, priv->script);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;
        }        
}

static void
ckd_player_dispose (GObject *o)
{
        CkdPlayer *self = CKD_PLAYER (o);
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        if (priv->view) {
                g_object_unref (priv->view);
                priv->view = NULL;
        }
        if (priv->script) {
                ckd_script_free (priv->script);
                priv->script = NULL;
        }
                
        G_OBJECT_CLASS (ckd_player_parent_class)->dispose (o);
}

static void
ckd_player_finalize (GObject *o)
{
        G_OBJECT_CLASS (ckd_player_parent_class)->finalize (o);
}

static void
ckd_player_class_init (CkdPlayerClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdPlayerPriv));

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->dispose      = ckd_player_dispose;
        base_class->finalize     = ckd_player_finalize;
        base_class->set_property = ckd_player_set_property;
        base_class->get_property = ckd_player_get_property;
        
        GParamSpec *props[N_CKD_PLAYER_PROPS] = {NULL,};
        GParamFlags w_co_flags = G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY;
        GParamFlags r_flag = G_PARAM_READABLE;
        
        props[PROP_CKD_PLAYER_VIEW] = g_param_spec_pointer ("view",
                                                            "View",
                                                            "View",
                                                            w_co_flags);
        props[PROP_CKD_PLAYER_SCRIPT] = g_param_spec_pointer ("script",
                                                              "Script",
                                                              "Script",
                                                              r_flag);
        props[PROK_CKD_PLAYER_AM_TIME] = g_param_spec_uint  ("am-time",
                                                             "Animation Time",
                                                             "Animation Time",
                                                             0, G_MAXUINT, 1000,
                                                             w_co_flags);   
        g_object_class_install_properties (base_class,
                                           N_CKD_PLAYER_PROPS,
                                           props);
}

static
void ckd_player_init (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        /* \begin 初始化幻灯片编号牌 */
        ClutterColor sncolor = {51, 51, 51, 100};
        priv->slide_number_bg = clutter_actor_new ();
        clutter_actor_set_size (priv->slide_number_bg, 240.0, 100.0);
        clutter_actor_set_background_color (priv->slide_number_bg, &sncolor);

        ClutterColor text_color = {255, 255, 255, 255};
        priv->slide_number = clutter_text_new ();
        clutter_text_set_color (CLUTTER_TEXT(priv->slide_number), &text_color);
        clutter_text_set_font_name (CLUTTER_TEXT(priv->slide_number), "Sans 32");
        clutter_text_set_text (CLUTTER_TEXT(priv->slide_number), "1");
        clutter_text_set_line_alignment (CLUTTER_TEXT (priv->slide_number), PANGO_ALIGN_CENTER);
        

        clutter_actor_add_constraint (priv->slide_number,
                                      clutter_align_constraint_new (priv->slide_number_bg,
                                                                    CLUTTER_ALIGN_X_AXIS,
                                                                    0.5));
        clutter_actor_add_constraint (priv->slide_number,
                                      clutter_align_constraint_new (priv->slide_number_bg,
                                                                    CLUTTER_ALIGN_Y_AXIS,
                                                                    0.5));

        priv->slide_number_is_shown = FALSE;
        clutter_actor_hide (priv->slide_number_bg);
        clutter_actor_hide (priv->slide_number);
        /* \end */
        
        priv->script = NULL;
        priv->am_time = 1000;
}

static void
ckd_player_update_progress (CkdPlayer *self, CkdMetaEntry *e)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *nonius;
        g_object_get (priv->view, "nonius", &nonius, NULL);

        gfloat x, y;
        ckd_view_get_nonius_position (priv->view, &x, &y);

        clutter_actor_save_easing_state (nonius);
        clutter_actor_set_easing_mode (nonius, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (nonius, priv->am_time);
        clutter_actor_set_position (nonius, x, y);
        clutter_actor_restore_easing_state (nonius);
}

static void
_slide_enter_from_left (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_w = clutter_actor_get_width (stage);

        gfloat x, y;
        clutter_actor_get_position (slide, &x, &y);
        clutter_actor_set_position (slide, - stage_w, y);        

        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_position (slide, x, y);
        clutter_actor_restore_easing_state (slide);
}

 static void
_slide_enter_from_right (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_w = clutter_actor_get_width (stage);

        gfloat x, y;
        clutter_actor_get_position (slide, &x, &y);
        clutter_actor_set_position (slide, stage_w, y);
        
        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_position (slide, x, y);
        clutter_actor_restore_easing_state (slide);
}

 static void
_slide_enter_from_top (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_h = clutter_actor_get_height (stage);

        gfloat x, y;
        clutter_actor_get_position (slide, &x, &y);
        clutter_actor_set_position (slide, x, - stage_h);
        
        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_position (slide, x, y);
        clutter_actor_restore_easing_state (slide);
}

 static void
_slide_enter_from_bottom (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_h = clutter_actor_get_height (stage);

        gfloat x, y;
        clutter_actor_get_position (slide, &x, &y);
        clutter_actor_set_position (slide, x, stage_h);
        
        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_position (slide, x, y);
        clutter_actor_restore_easing_state (slide);
}

static void
_slide_enter_from_enlargement (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        clutter_actor_set_scale (slide, 1.618, 1.618);
        clutter_actor_set_pivot_point (slide, 0.5, 0.5);
        clutter_actor_set_opacity (slide, 100);
        
        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_scale (slide, 1.0, 1.0);
        clutter_actor_set_opacity (slide, 255);
        clutter_actor_restore_easing_state (slide);
}

static void
_slide_enter_from_shrink (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        ClutterActorBox box;
        clutter_actor_get_allocation_box (slide, &box);
        
        clutter_actor_set_scale (slide, 0.618, 0.618);
        clutter_actor_set_pivot_point (slide, 0.5, 0.5);
        clutter_actor_set_opacity (slide, 100);

        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_scale (slide, 1.0, 1.0);
        clutter_actor_set_opacity (slide, 255);
        clutter_actor_restore_easing_state (slide);        
}

static void
_slide_enter_from_fade (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        clutter_actor_set_opacity (slide, 0);

        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_opacity (slide, 255);
        clutter_actor_restore_easing_state (slide);        
}

static void
_slide_enter_from_curl (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        ClutterActor *stage = clutter_actor_get_stage (slide);
        gfloat h = clutter_actor_get_height (stage);
        
        ClutterEffect *effect = clutter_page_turn_effect_new (1.0,
                                                              45.0,
                                                              0.15 * h);
        clutter_deform_effect_set_n_tiles (CLUTTER_DEFORM_EFFECT(effect),
                                           128,
                                           128);
        clutter_actor_add_effect_with_name (slide, "curl", effect);

        ClutterTransition *t = clutter_property_transition_new ("@effects.curl.period");

        clutter_timeline_set_duration (CLUTTER_TIMELINE (t), 1.5 * priv->am_time);
        clutter_actor_add_transition (slide, "curl", t);
        
        clutter_transition_set_from (t, G_TYPE_DOUBLE, 1.0);
        clutter_transition_set_to (t, G_TYPE_DOUBLE, 0.0);
        clutter_timeline_rewind (CLUTTER_TIMELINE (t));
        clutter_timeline_start (CLUTTER_TIMELINE (t));
}

static void
ckd_player_slide_enter (CkdPlayer *self, ClutterActor *slide, CkdMetaEntry *e)
{
        switch (e->am) {
        case CKD_SLIDE_AM_LEFT:
                _slide_enter_from_left (self, slide);
                break;
        case CKD_SLIDE_AM_RIGHT:
                _slide_enter_from_right (self, slide);
                break;
        case CKD_SLIDE_AM_TOP:
                _slide_enter_from_top (self, slide);
                break;
        case CKD_SLIDE_AM_BOTTOM:
                _slide_enter_from_bottom (self, slide);
                break;
        case CKD_SLIDE_AM_ENLARGEMENT:
                _slide_enter_from_enlargement (self, slide);
                break;
       case CKD_SLIDE_AM_SHRINK:
                _slide_enter_from_shrink (self, slide);
                break;
        case CKD_SLIDE_AM_FADE:
                _slide_enter_from_fade (self, slide);
                break;
        case CKD_SLIDE_AM_CURL:
                _slide_enter_from_curl (self, slide);
                break;
        default:
                break;
        }

        ckd_player_update_progress (self, e);
}

static void
_slide_exit_cb (ClutterAnimation *am, gpointer data)
{
        ClutterActor *slide = data;
        clutter_actor_destroy (slide);
}

static void
_slide_exit_from_left (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        guint am_time;
        g_object_get (priv->view, "stage", &stage, NULL);
        
        gfloat stage_w = clutter_actor_get_width (stage);

        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_translation (slide, - stage_w, 0.0, 0.0);
        clutter_actor_restore_easing_state (slide);

        /* 销毁幻灯片 */
        ClutterTransition *transition = clutter_actor_get_transition (slide, "translation-x");
        g_signal_connect (transition, "completed", G_CALLBACK (_slide_exit_cb), slide);
}

static void
_slide_exit_from_right (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        guint am_time;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_w = clutter_actor_get_width (stage);

        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_translation (slide, stage_w, 0.0, 0.0);
        clutter_actor_restore_easing_state (slide);

        /* 销毁幻灯片 */
        ClutterTransition *transition = clutter_actor_get_transition (slide, "translation-x");
        g_signal_connect (transition, "completed", G_CALLBACK (_slide_exit_cb), slide);        
}

static void
_slide_exit_from_top (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        guint am_time;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_h = clutter_actor_get_height (stage);

        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_translation (slide, 0.0, - stage_h, 0.0);
        clutter_actor_restore_easing_state (slide);

        /* 销毁幻灯片 */
        ClutterTransition *transition = clutter_actor_get_transition (slide, "translation-y");
        g_signal_connect (transition, "completed", G_CALLBACK (_slide_exit_cb), slide);   
}

static void
_slide_exit_from_bottom (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        guint am_time;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_h = clutter_actor_get_height (stage);
        
        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_EASE_IN_OUT_CUBIC);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_translation (slide, 0.0, stage_h, 0.0);
        clutter_actor_restore_easing_state (slide);

        /* 销毁幻灯片 */
        ClutterTransition *transition = clutter_actor_get_transition (slide, "translation-y");
        g_signal_connect (transition, "completed", G_CALLBACK (_slide_exit_cb), slide);   
}

static void
_slide_exit_from_enlargement (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        clutter_actor_set_pivot_point (slide, 0.5, 0.5);
        
        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_scale (slide, 1.618, 1.618);
        clutter_actor_set_opacity (slide, 0);
        clutter_actor_restore_easing_state (slide);

        /* 销毁幻灯片 */
        ClutterTransition *transition = clutter_actor_get_transition (slide, "scale-x");
        g_signal_connect (transition, "completed", G_CALLBACK (_slide_exit_cb), slide);
}

static void
_slide_exit_from_shrink (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        clutter_actor_set_pivot_point (slide, 0.5, 0.5);
        
        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_scale (slide, 0.618, 0.618);
        clutter_actor_set_opacity (slide, 0);
        clutter_actor_restore_easing_state (slide);

        /* 销毁幻灯片 */
        ClutterTransition *transition = clutter_actor_get_transition (slide, "scale-x");
        g_signal_connect (transition, "completed", G_CALLBACK (_slide_exit_cb), slide);
}

static void
_slide_exit_from_fade (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        clutter_actor_set_opacity (slide, 255);

        clutter_actor_save_easing_state (slide);
        clutter_actor_set_easing_mode (slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (slide, priv->am_time);
        clutter_actor_set_opacity (slide, 0);
        clutter_actor_restore_easing_state (slide);

        /* 销毁幻灯片 */
        ClutterTransition *transition = clutter_actor_get_transition (slide, "opacity");
        g_signal_connect (transition, "completed", G_CALLBACK (_slide_exit_cb), slide);  
}

static void
ckd_player_slide_exit (CkdPlayer *self, ClutterActor *slide, CkdMetaEntry *e)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        switch (e->am) {
        case CKD_SLIDE_AM_LEFT:
                _slide_exit_from_left (self, slide);
                break;
        case CKD_SLIDE_AM_RIGHT:
                _slide_exit_from_right (self, slide);
                break;
        case CKD_SLIDE_AM_TOP:
                _slide_exit_from_top (self, slide);
                break;
        case CKD_SLIDE_AM_BOTTOM:
                _slide_exit_from_bottom (self, slide);
                break;
        case CKD_SLIDE_AM_ENLARGEMENT:
                _slide_exit_from_enlargement (self, slide);
                break;
        case CKD_SLIDE_AM_SHRINK:
                _slide_exit_from_shrink (self, slide);
                break;
        case CKD_SLIDE_AM_FADE:
                _slide_exit_from_fade (self, slide);
                break;
        default:
                break;
        }

        ckd_player_update_progress (self, e);
}

/* \begin 如果当前幻灯片是卷轴退出，那么下一张幻灯片的动画必须放在当前幻灯片动画结束后进行 */
struct CkdSlideAmCurlData {
        CkdPlayer *player;
        ClutterActor *current_slide;
        
        ClutterActor *next_slide;
        CkdMetaEntry *next_meta_entry;
};

static void
_slide_exit_from_curl_cb (ClutterAnimation *am, gpointer data)
{
        struct CkdSlideAmCurlData *curl_data = data;
        
        clutter_actor_destroy (curl_data->current_slide);

        clutter_actor_show (curl_data->next_slide);
        ckd_player_slide_enter (curl_data->player,
                                curl_data->next_slide,
                                curl_data->next_meta_entry);

        g_slice_free (struct CkdSlideAmCurlData, curl_data);
}


static void
_slide_exit_from_curl (struct CkdSlideAmCurlData *data)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (data->player);

        clutter_actor_hide (data->next_slide);
        
        ClutterTransition *t = clutter_actor_get_transition (data->current_slide, "curl");

        if (t == NULL) {
                ClutterActor *stage = clutter_actor_get_stage (data->current_slide);
                gfloat h = clutter_actor_get_height (stage);
                ClutterEffect *effect = clutter_page_turn_effect_new (1.0,
                                                                      45.0,
                                                                      0.15 * h);
                clutter_deform_effect_set_n_tiles (CLUTTER_DEFORM_EFFECT(effect),
                                                   128,
                                                   128);
                clutter_actor_add_effect_with_name (data->current_slide, "curl", effect);
                
                t = clutter_property_transition_new ("@effects.curl.period");

                clutter_timeline_set_duration (CLUTTER_TIMELINE (t), 1.5 * priv->am_time);
                clutter_actor_add_transition (data->current_slide, "curl", t);
        }
        
        clutter_transition_set_from (t, G_TYPE_DOUBLE, 0.0);
        clutter_transition_set_to (t, G_TYPE_DOUBLE, 1.0);
        clutter_timeline_rewind (CLUTTER_TIMELINE (t));
        clutter_timeline_start (CLUTTER_TIMELINE (t));

        g_signal_connect (t, "completed", G_CALLBACK (_slide_exit_from_curl_cb), data);  
}
/* \end */

void
ckd_player_step (CkdPlayer *self, gint step)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        CkdMetaSlides *meta_slides;
        g_object_get (priv->view, "meta-slides", &meta_slides, NULL);
        
        ClutterActor *current_slide;
        gint current_slide_number;
        CkdMetaEntry *current_meta_entry;
        
        g_object_get (priv->view,
                      "slide", &current_slide,
                      "slide-number", &current_slide_number,
                      NULL);
        current_meta_entry = ckd_meta_slides_get_meta_entry (meta_slides,
                                                             current_slide_number);

        ClutterActor *next_slide;
        CkdMetaEntry *next_meta_entry;
        
        next_slide = ckd_view_load_ith_slide (priv->view,
                                              current_slide_number + step);
        next_meta_entry = ckd_meta_slides_get_meta_entry (meta_slides,
                                                          current_slide_number + step);

        if (next_slide && next_meta_entry) {
                /* 当前幻灯片如果是卷轴动画效果，需要将下一张幻灯片动画置于当前幻灯片动画结束后进行 */
                if (current_meta_entry->am == CKD_SLIDE_AM_CURL) {
                        struct CkdSlideAmCurlData *data =
                                g_slice_alloc (sizeof(struct CkdSlideAmCurlData));
                        data->player = self;
                        data->current_slide = current_slide;
                        data->next_slide = next_slide;
                        data->next_meta_entry = next_meta_entry;
                        
                        _slide_exit_from_curl (data);
                        
                } else {
                        ckd_player_slide_exit (self, current_slide, current_meta_entry);
                        ckd_player_slide_enter (self, next_slide, next_meta_entry);
                }
        }

        /* \begin 更新幻灯片编号牌 */
        g_object_get (priv->view,
                      "slide-number", &current_slide_number,
                      NULL);
        
        gchar index_text[255];
        g_sprintf (index_text, "%d", current_slide_number + 1);
        clutter_text_set_text (CLUTTER_TEXT(priv->slide_number), index_text);
        /* \end */
}
