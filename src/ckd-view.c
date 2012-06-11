#include <math.h>
#include "ckd-view.h"

G_DEFINE_TYPE (CkdView, ckd_view, CLUTTER_TYPE_ACTOR);

#define CKD_VIEW_GET_PRIVATE(o) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((o), CKD_TYPE_VIEW, CkdViewPriv))

typedef struct _CkdViewPriv CkdViewPriv;
struct _CkdViewPriv {
        ClutterActor *slide;
        gfloat time_axis_width;
        gfloat padding;
        guint duration_base;
        
        CkdViewSlideInEffect slide_in_effect;
        CkdViewSlideOutEffect slide_out_effect;
        
        gfloat slide_w;
        gfloat slide_h;
        gfloat slide_x;
        gfloat slide_y;
};

enum {
        PROP_CKD_VIEW_0,
        PROP_CKD_VIEW_SLIDE,
        PROP_CKD_VIEW_TIME_AXIS_WIDTH,
        PROP_CKD_VIEW_PADDING,
        PROP_CKD_VIEW_DURATION_BASE,
        PROP_CKD_VIEW_SLIDE_IN_EFFECT,
        PROP_CKD_VIEW_SLIDE_OUT_EFFECT,
        N_CKD_VIEW_PROPS
};

typedef void (*CkdViewAnimationFunc) (CkdView *, gpointer);

static void
ckd_view_set_property (GObject *o, guint prop, const GValue *v, GParamSpec *p)
{
        CkdView *self = CKD_VIEW (o);
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_VIEW_SLIDE:
                priv->slide = g_value_get_pointer (v);
                clutter_actor_add_child (CLUTTER_ACTOR(self), priv->slide);
                clutter_actor_queue_relayout (CLUTTER_ACTOR(self));
                break;
        case PROP_CKD_VIEW_TIME_AXIS_WIDTH:
                priv->time_axis_width = g_value_get_float (v);
                break;
        case PROP_CKD_VIEW_PADDING:
                priv->padding = g_value_get_float (v);
                break;
        case PROP_CKD_VIEW_DURATION_BASE:
                priv->duration_base = g_value_get_uint (v);
                break;
        case PROP_CKD_VIEW_SLIDE_IN_EFFECT:
                priv->slide_in_effect = g_value_get_int (v);
                break;
        case PROP_CKD_VIEW_SLIDE_OUT_EFFECT:
                priv->slide_out_effect = g_value_get_int (v);
                break;                
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;
        }
}

static void
ckd_view_get_property (GObject *o, guint prop, GValue *v, GParamSpec *p)
{
        CkdView *self = CKD_VIEW (o);
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_VIEW_SLIDE:
                g_value_set_pointer (v, priv->slide);
                break;
        case PROP_CKD_VIEW_TIME_AXIS_WIDTH:
                g_value_set_float (v, priv->time_axis_width);
                break;
        case PROP_CKD_VIEW_PADDING:
                g_value_set_float (v, priv->padding);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;                
        }
}

static void
ckd_view_destroy (ClutterActor *a)
{
        CkdView *self = CKD_VIEW (a);
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        if (priv->slide) {
                clutter_actor_destroy (priv->slide);
                priv->slide = NULL;
        }
        
        if (CLUTTER_ACTOR_CLASS (ckd_view_parent_class)->destroy)
                CLUTTER_ACTOR_CLASS (ckd_view_parent_class)->destroy (a);
}

static void
ckd_slides_allocate_slide (CkdView *self,
                           const ClutterActorBox *box,
                           ClutterAllocationFlags f)
{

        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = priv->slide;
        gfloat time_axis_width = priv->time_axis_width;
        gfloat padding = priv->padding;
        
        gfloat inner_w, inner_h, slide_w, slide_h, slide_r;

        inner_w = clutter_actor_box_get_width (box) - 2.0 * padding;
        inner_h = clutter_actor_box_get_height (box) - time_axis_width - 2.0 * padding;

        slide_w = clutter_actor_get_width (slide);
        slide_h = clutter_actor_get_height (slide);
        slide_r = slide_w / slide_h;
        
        if (inner_w < slide_r * inner_h) {
                slide_w = inner_w;
                slide_h = inner_w / slide_r;
        } else {
                slide_h = inner_h;
                slide_w = inner_h * slide_r;
        }
        
        gfloat x1, y1, x2, y2;
        x1 = 0.5 * (box->x1 + box->x2 - slide_w);
        y1 = 0.5 * (box->y1 + box->y2 - slide_h - time_axis_width);
        x2 = x1 + slide_w;
        y2 = y1 + slide_h;
        
        ClutterActorBox *slide_box = clutter_actor_box_new (x1, y1, x2, y2);
        clutter_actor_allocate (slide, slide_box, f);
        clutter_actor_box_free (slide_box);

        priv->slide_w = slide_w;
        priv->slide_h = slide_h;
        priv->slide_x = x1;
        priv->slide_y = y1;
}

static void
ckd_view_allocate (ClutterActor *a, const ClutterActorBox *b, ClutterAllocationFlags f)
{
        CkdView *self = CKD_VIEW (a);
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        CLUTTER_ACTOR_CLASS (ckd_view_parent_class)->allocate (a, b, f);
        
        ckd_slides_allocate_slide (self, b, f);
}

static void
ckd_view_paint (ClutterActor *a)
{
        CkdView *self = CKD_VIEW (a);
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        clutter_actor_paint (priv->slide);
}

static void
ckd_view_class_init (CkdViewClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdViewPriv));

        ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
        actor_class->destroy = ckd_view_destroy;
        actor_class->paint = ckd_view_paint;
        actor_class->allocate = ckd_view_allocate;
        
        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->set_property = ckd_view_set_property;
        base_class->get_property = ckd_view_get_property;
        
        GParamSpec *props[N_CKD_VIEW_PROPS] = {NULL,};
        props[PROP_CKD_VIEW_SLIDE] =
                g_param_spec_pointer ("slide", "Slide", "Slide", G_PARAM_READWRITE);
        props[PROP_CKD_VIEW_TIME_AXIS_WIDTH] =
                g_param_spec_float ("time-axis-width", "Time Axis Width", "Time Axis Width",
                                    G_MINFLOAT,
                                    G_MAXFLOAT,
                                    20.0,
                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_VIEW_PADDING] =
                g_param_spec_float ("padding", "Padding", "Padding",
                                    G_MINFLOAT,
                                    G_MAXFLOAT,
                                    10.0,
                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_VIEW_DURATION_BASE] =
                g_param_spec_uint ("duration-base", "Duration Base", "Duration Base",
                                  0,
                                  G_MAXUINT,
                                  1000,
                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_VIEW_SLIDE_IN_EFFECT] =
                g_param_spec_int ("slide-in-effect", "Slide In Effect", "Slide In Effect",
                                  G_MININT,
                                  G_MAXINT,
                                  CKD_SLIDE_FADE_ENTERING,
                                  G_PARAM_READWRITE);
        props[PROP_CKD_VIEW_SLIDE_OUT_EFFECT] =
                g_param_spec_int ("slide-out-effect", "Slide Out Effect", "Slide Out Effect",
                                  G_MININT,
                                  G_MAXINT,
                                  CKD_SLIDE_FADE_EXIT,
                                  G_PARAM_READWRITE);
        g_object_class_install_properties (base_class, N_CKD_VIEW_PROPS, props);
}

static void
ckd_view_init (CkdView *self)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        priv->slide = NULL;
        priv->time_axis_width = 0.0;
        priv->padding = 0.0;

        priv->slide_in_effect = CKD_SLIDE_FADE_ENTERING;
        priv->slide_out_effect = CKD_SLIDE_FADE_EXIT;
        
        priv->slide_w = 0.0;
        priv->slide_h = 0.0;
        priv->slide_x = 0.0;
        priv->slide_y = 0.0;
}

static void
ckd_view_slide_enter_cb (ClutterAnimation *animation, ClutterActor *actor)
{
        /* @begin: 偷梁换柱 */
        ClutterActor *source = clutter_clone_get_source (CLUTTER_CLONE(actor));
        clutter_actor_show (source);

        clutter_actor_destroy (actor);
        /* @end */
}

static void
ckd_view_slide_exit_cb (ClutterAnimation *animation, ClutterActor *actor)
{
        clutter_actor_destroy (actor);
}

static void
ckd_view_slide_left_exit (CkdView *self, gpointer data) 
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = data;
        
        gfloat view_w, view_h;
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        
        /* @begin: 当前幻灯片的移出动画 */
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        clutter_actor_animate (slide, CLUTTER_EASE_IN_OUT_CUBIC, priv->duration_base,
                               "x", -view_w,
                               "opacity", 0,
                               "signal::completed",
                               ckd_view_slide_exit_cb,
                               slide,
                               NULL);
        /* @end */

}

static void
ckd_view_slide_right_exit (CkdView *self, gpointer data) 
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = data;
        
        gfloat view_w, view_h;
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        
        /* @begin: 当前幻灯片的移出动画 */
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        clutter_actor_animate (slide, CLUTTER_EASE_IN_OUT_CUBIC, priv->duration_base,
                               "x", view_w,
                               "opacity", 0,
                               "signal::completed",
                               ckd_view_slide_exit_cb,
                               slide,
                               NULL);
        /* @end */

}

static void
ckd_view_slide_up_exit (CkdView *self, gpointer data) 
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = data;
        
        gfloat view_w, view_h;
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        
        /* @begin: 当前幻灯片的移出动画 */
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        clutter_actor_animate (slide, CLUTTER_EASE_IN_OUT_CUBIC, priv->duration_base,
                               "y", -view_h,
                               "opacity", 0,
                               "signal::completed",
                               ckd_view_slide_exit_cb,
                               slide,
                               NULL);
        /* @end */

}

static void
ckd_view_slide_down_exit (CkdView *self, gpointer data) 
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = data;
        
        gfloat view_w, view_h;
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        
        /* @begin: 当前幻灯片的移出动画 */
        clutter_actor_get_size (CLUTTER_ACTOR(self), &view_w, &view_h);
        clutter_actor_animate (slide, CLUTTER_EASE_IN_OUT_CUBIC, priv->duration_base,
                               "y", view_h,
                               "opacity", 0,
                               "signal::completed",
                               ckd_view_slide_exit_cb,
                               slide,
                               NULL);
        /* @end */
}

static void
ckd_view_slide_fade_exit (CkdView *self, gpointer data)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = data;

        clutter_actor_set_opacity (slide, 255);
        clutter_actor_animate (slide, CLUTTER_LINEAR, priv->duration_base,
                               "opacity", 0,
                               "signal::completed",
                               ckd_view_slide_exit_cb,
                               slide,
                               NULL);
}

static void
ckd_view_slide_scale_enter (CkdView *self, gpointer data)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActorBox view_box;
        ClutterActor *slide = data;

        clutter_actor_get_allocation_box (CLUTTER_ACTOR(self), &view_box);
        clutter_actor_set_scale_full (slide,
                                      0.7,
                                      0.7,
                                      0.5 * (view_box.x1 + view_box.x2),
                                      priv->slide_y + 0.5 * priv->slide_h);
        clutter_actor_set_opacity (slide, 0);
        clutter_actor_animate (slide, CLUTTER_LINEAR, priv->duration_base,
                               "scale-x", 1.0,
                               "scale-y", 1.0,
                               "opacity", 255,
                               "signal::completed",
                               ckd_view_slide_enter_cb,
                               slide,
                               NULL);
}

static void
ckd_view_slide_fade_enter (CkdView *self, gpointer data)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = data;

        clutter_actor_set_opacity (slide, 0);
        clutter_actor_animate (slide, CLUTTER_LINEAR, priv->duration_base,
                               "opacity", 255,
                               "signal::completed",
                               ckd_view_slide_enter_cb,
                               slide,
                               NULL);
}

static void
ckd_view_slide_enter (CkdView *self,
                      CkdViewAnimationFunc ckd_view_animate,
                      gpointer data)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActorBox view_box;
        ClutterActor *slide = data;
        
        g_object_set (self, "slide", slide, NULL);
        
        /*
         * @begin: 为当前的幻灯片创建一个可以参与动画过程的分身
         * 这个分身不受视图的尺寸约束
         * 在分身参与动画的过程中，当前幻灯片处于隐藏状态
         * 待分身动画完成后，再将当前幻灯片显示出来
        */
        ClutterActor *slide_clone = clutter_clone_new (priv->slide);
        ClutterActor *stage = clutter_actor_get_parent (CLUTTER_ACTOR(self));
        
        clutter_actor_add_child (stage, slide_clone);
        clutter_actor_hide (slide);
        
        clutter_actor_set_size (slide_clone, priv->slide_w, priv->slide_h);
        clutter_actor_set_position (slide_clone, priv->slide_x, priv->slide_y);
        /* @end */
        
        ckd_view_animate (self,  slide_clone);
}

static void
ckd_view_slide_exit (CkdView *self,
                     CkdViewAnimationFunc ckd_view_animate,
                     gpointer data)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *stage = clutter_actor_get_parent (CLUTTER_ACTOR(self));

        /* @begin:
         * 让当前幻灯片脱离视图，使之进入场景。
         * 如果不这样做，那么在幻灯片移动过程中，会被视图框住而动弹不得
         */
        ClutterActor *slide = data;
        g_object_ref (slide);
        clutter_actor_remove_child (CLUTTER_ACTOR(self), slide);

        clutter_actor_set_size (slide, priv->slide_w, priv->slide_h);
        clutter_actor_set_position (slide, priv->slide_x, priv->slide_y);
        clutter_actor_add_child (stage, slide);
        clutter_actor_set_child_above_sibling (stage, slide, NULL);
        /* @end */

        ckd_view_animate (self, slide);
}

void
ckd_view_transit_slide (CkdView *self, ClutterActor *new_slide)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        switch (priv->slide_out_effect) {
        case CKD_SLIDE_LEFT_EXIT:
                ckd_view_slide_exit (self, ckd_view_slide_left_exit, priv->slide);
                break;
        case CKD_SLIDE_RIGHT_EXIT:
                ckd_view_slide_exit (self, ckd_view_slide_right_exit, priv->slide);
                break;
        case CKD_SLIDE_UP_EXIT:
                ckd_view_slide_exit (self, ckd_view_slide_up_exit, priv->slide);
                break;
        case CKD_SLIDE_DOWN_EXIT:
                ckd_view_slide_exit (self, ckd_view_slide_down_exit, priv->slide);
                break;
        case CKD_SLIDE_FADE_EXIT:
                ckd_view_slide_exit (self, ckd_view_slide_fade_exit, priv->slide);
                break;
        default:
                g_error ("I can not find this slide exit effect!");
        }

        switch (priv->slide_in_effect) {
        case CKD_SLIDE_SCALE_ENTERING:
                ckd_view_slide_enter (self, ckd_view_slide_scale_enter, new_slide);
                break;
        case CKD_SLIDE_FADE_ENTERING:
                ckd_view_slide_enter (self, ckd_view_slide_fade_enter, new_slide);
                break;
        default:
                g_error ("I can not find this slide entering effect!");
        }
}
