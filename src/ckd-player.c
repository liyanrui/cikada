#include <math.h>
#include "ckd-meta-slides.h"
#include "ckd-view.h"
#include "ckd-player.h"

G_DEFINE_TYPE (CkdPlayer, ckd_player, G_TYPE_OBJECT);

#define CKD_PLAYER_GET_PRIVATE(o) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((o), CKD_TYPE_PLAYER, CkdPlayerPriv))

typedef struct _CkdPlayerPriv CkdPlayerPriv;
struct _CkdPlayerPriv {
        CkdView  *view;
        
        /* 动画持续时间 */
        guint am_time;
};

enum {
        PROP_CKD_PLAYER_0,
        PROP_CKD_PLAYER_VIEW,
        PROK_CKD_PLAYER_AM_TIME,
        N_CKD_PLAYER_PROPS
};

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

static void
ckd_player_set_property (GObject *o, guint prop, const GValue *v, GParamSpec *p)
{
        CkdPlayer *self = CKD_PLAYER (o);
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_PLAYER_VIEW:
                priv->view = g_value_get_pointer (v);
                {
                        ClutterActor *stage;
                        g_object_get (priv->view, "stage", &stage, NULL);
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
ckd_player_dispose (GObject *o)
{
        CkdPlayer *self = CKD_PLAYER (o);
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        if (priv->view) {
                g_object_unref (priv->view);
                priv->view = NULL;
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

        GParamSpec *props[N_CKD_PLAYER_PROPS] = {NULL,};
        GParamFlags w_co_flags = G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY;
        
        props[PROP_CKD_PLAYER_VIEW] = g_param_spec_pointer ("view",
                                                            "View",
                                                            "View",
                                                            w_co_flags);
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
}

static void
ckd_player_update_progress (CkdPlayer *self, CkdMetaEntry *e)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *nonius;
        g_object_get (priv->view, "nonius", &nonius, NULL);

        gfloat x, y;
        ckd_view_get_nonius_position (priv->view, &x, &y);

        clutter_actor_animate (nonius,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "x",
                               x,
                               NULL);
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
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "x",
                               x,
                               NULL);
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
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "x",
                               x,
                               NULL);
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
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "y",
                               y,
                               NULL);
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
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "y",
                               y,
                               NULL);
}

static void
_slide_enter_from_enlargement (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        ClutterActorBox box;
        clutter_actor_get_allocation_box (slide, &box);
        
        clutter_actor_set_scale_full (slide,
                                      1.618,
                                      1.618,
                                      0.5 * (box.x1 + box.x2),
                                      0.5 * (box.y1 + box.y2));
        clutter_actor_set_opacity (slide, 100);
        clutter_actor_animate (slide, CLUTTER_LINEAR, priv->am_time,
                               "scale-x", 1.0,
                               "scale-y", 1.0,
                               "opacity", 255,
                               NULL);
}

static void
_slide_enter_from_shrink (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        ClutterActorBox box;
        clutter_actor_get_allocation_box (slide, &box);
        
        clutter_actor_set_scale_full (slide,
                                      0.618,
                                      0.618,
                                      0.5 * (box.x1 + box.x2),
                                      0.5 * (box.y1 + box.y2));
        clutter_actor_set_opacity (slide, 100);
        clutter_actor_animate (slide, CLUTTER_LINEAR, priv->am_time,
                               "scale-x", 1.0,
                               "scale-y", 1.0,
                               "opacity", 255,
                               NULL);
}

static void
_slide_enter_from_fade (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        clutter_actor_set_opacity (slide, 150);
        clutter_actor_animate (slide,
                               CLUTTER_LINEAR,
                               priv->am_time,
                               "opacity",
                               255,
                               NULL);
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
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_OUT_QUAD,
                               2 * priv->am_time,
                                "@effects.curl.period", 0.0,
                                NULL);
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
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "x",
                               - stage_w,
                               "signal-after::completed",
                               _slide_exit_cb,
                               slide,
                               NULL);
}

static void
_slide_exit_from_right (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        guint am_time;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_w = clutter_actor_get_width (stage);
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "x",
                               stage_w,
                               "signal-after::completed",
                               _slide_exit_cb,
                               slide,
                               NULL);
}

static void
_slide_exit_from_top (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        guint am_time;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_h = clutter_actor_get_height (stage);
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "y",
                               - stage_h,
                               "signal-after::completed",
                               _slide_exit_cb,
                               slide,
                               NULL);
}

static void
_slide_exit_from_bottom (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        
        ClutterActor *stage;
        guint am_time;
        g_object_get (priv->view, "stage", &stage, NULL);

        gfloat stage_h = clutter_actor_get_height (stage);
        
        clutter_actor_animate (slide,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               priv->am_time,
                               "y",
                               stage_h,
                               "signal-after::completed",
                               _slide_exit_cb,
                               slide,
                               NULL);
}

static void
_slide_exit_from_enlargement (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        ClutterActorBox box;
        clutter_actor_get_allocation_box (slide, &box);
        
        gdouble scale_x, scale_y;
        clutter_actor_get_scale (slide, &scale_x, &scale_y);
        clutter_actor_set_scale_full (slide,
                                      scale_x,
                                      scale_y,
                                      0.5 * (box.x1 + box.x2),
                                      0.5 * (box.y1 + box.y2));
        clutter_actor_animate (slide, CLUTTER_LINEAR, priv->am_time,
                               "scale-x", 1.618,
                               "scale-y", 1.618,
                               "opacity", 0,
                               "signal-after::completed",
                               _slide_exit_cb,
                               slide,
                               NULL);
}

static void
_slide_exit_from_shrink (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        ClutterActorBox box;
        clutter_actor_get_allocation_box (slide, &box);
        
        gdouble scale_x, scale_y;
        clutter_actor_get_scale (slide, &scale_x, &scale_y);
        clutter_actor_set_scale_full (slide,
                                      scale_x,
                                      scale_y,
                                      0.5 * (box.x1 + box.x2),
                                      0.5 * (box.y1 + box.y2));

        clutter_actor_animate (slide, CLUTTER_LINEAR, priv->am_time,
                               "scale-x", 0.618,
                               "scale-y", 0.618,
                               "opacity", 0,
                               "signal-after::completed",
                               _slide_exit_cb,
                               slide,
                               NULL);
}

static void
_slide_exit_from_fade (CkdPlayer *self, ClutterActor *slide)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        clutter_actor_set_opacity (slide, 255);
        clutter_actor_animate (slide,
                               CLUTTER_LINEAR,
                               priv->am_time,
                               "opacity",
                               0,
                               "signal-after::completed",
                               _slide_exit_cb,
                               slide,
                               NULL);
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
        
        ClutterActor *stage = clutter_actor_get_stage (data->current_slide);
        gfloat h = clutter_actor_get_height (stage);
        ClutterEffect *effect = clutter_page_turn_effect_new (0.0,
                                                              45.0,
                                                              0.15 * h);
        
        clutter_deform_effect_set_n_tiles (CLUTTER_DEFORM_EFFECT(effect),
                                           128,
                                           128);
        
        clutter_actor_add_effect_with_name (data->current_slide, "curl", effect);
        clutter_actor_animate (data->current_slide,
                               CLUTTER_EASE_IN_QUAD,
                               2 * priv->am_time,
                               "@effects.curl.period", 1.0,
                               "signal-after::completed",
                               _slide_exit_from_curl_cb,
                               data,
                               NULL);
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
}
