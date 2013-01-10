#include <math.h>
#include "ckd-view.h"
#include "ckd-meta-slides.h"

G_DEFINE_TYPE (CkdView, ckd_view, G_TYPE_OBJECT);

#define CKD_VIEW_GET_PRIVATE(o) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((o), CKD_TYPE_VIEW, CkdViewPriv))

typedef struct _CkdViewPriv CkdViewPriv;
struct _CkdViewPriv {
        ClutterActor *stage;
        CkdMetaSlides *meta_slides;

        /* 当前幻灯片及其编号 */
        ClutterActor *slide;
        gint slide_number;

        /* 幻灯片与窗口之间的空白区域 */
        gfloat padding;

        /* \begin 进度条 */
        ClutterActor *bar;
        ClutterActor *nonius;
        ClutterColor *bar_color;
        ClutterColor *nonius_color;
        gfloat bar_vsize;
        /* \end */
};

enum {
        PROP_CKD_VIEW_0,
        PROP_CKD_VIEW_STAGE,
        PROP_CKD_VIEW_META_SLIDES,
        PROP_CKD_VIEW_SLIDE,
        PROP_CKD_VIEW_SLIDE_NUMBER,
        PROP_CKD_VIEW_BAR,
        PROP_CKD_VIEW_NONIUS,
        PROP_CKD_VIEW_PADDING,
        PROP_CKD_VIEW_BAR_COLOR,
        PROP_CKD_VIEW_BAR_VSIZE,
        PROP_CKD_VIEW_NONIUS_COLOR,
        N_CKD_VIEW_PROPS
};

static void
ckd_view_allocate_slide (CkdView *self, ClutterActor *slide)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        gfloat stage_w = clutter_actor_get_width (priv->stage);
        gfloat stage_h = clutter_actor_get_height (priv->stage);
        gfloat inner_w = stage_w - 2.0 * priv->padding;
        gfloat inner_h = stage_h - 2.0 * priv->padding - priv->bar_vsize;

        gfloat slide_w, slide_h, slide_r;
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
        x1 = 0.5 * (0.0 + stage_w - slide_w);
        y1 = 0.5 * (0.0 + stage_h - slide_h - priv->bar_vsize);

        clutter_actor_set_position (slide, x1, y1);
        clutter_actor_set_size (slide, slide_w, slide_h);
}

void
ckd_view_get_nonius_position (CkdView *self, gfloat *x, gfloat *y) 
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        
        gint n;
        g_object_get (priv->meta_slides, "n-of-slides", &n, NULL);

        CkdMetaEntry *e = ckd_meta_slides_get_meta_entry (priv->meta_slides, priv->slide_number);
        gfloat progress = e->tick;

        gfloat bar_x, bar_y, bar_w, bar_h;
        clutter_actor_get_position (priv->bar, &bar_x, &bar_y);
        clutter_actor_get_size (priv->bar, &bar_w, &bar_h);
         
        *x = bar_x + progress * (bar_w - priv->bar_vsize);
        *y = bar_y;
}

static void
ckd_view_allocate_progress_bar (CkdView *self)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        
        gfloat slide_w, slide_h, slide_x, slide_y;
        clutter_actor_get_size (priv->slide, &slide_w, &slide_h);
        clutter_actor_get_position (priv->slide, &slide_x, &slide_y);

        /*\begin 设置进度条的尺寸与位置 */
        gfloat bar_x, bar_y, bar_w, bar_h;
        bar_x = slide_x;
        bar_y = slide_y + slide_h;
        bar_w = slide_w;
        bar_h = priv->bar_vsize;
        clutter_actor_set_size (priv->bar, bar_w, bar_h);
        clutter_actor_set_position (priv->bar, bar_x, bar_y);
        /* \end */

        /* \begin 设置滑块的尺寸与位置 */
        gfloat nonius_x, nonius_y;
        ckd_view_get_nonius_position (self, &nonius_x, &nonius_y);
        clutter_actor_set_position (priv->nonius, nonius_x, nonius_y);
        /*\end */
}

static void
ckd_view_on_stage_allocation (ClutterActor *stage,
                              ClutterActorBox *box,
                              ClutterAllocationFlags flags,
                              gpointer data)
{
        CkdView *view = data;
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (view);

        /*\begin 调整当前幻灯片以及缓存中幻灯片的尺寸与位置 */
        ckd_view_allocate_slide (view, priv->slide);
        /*\end */
        
        ckd_view_allocate_progress_bar (view);
}

static void
ckd_view_config_stage (CkdView *self, ClutterActor *stage)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        
        priv->stage = stage;
        
        /* 将进度条置入场景 */
        clutter_actor_add_child (stage, priv->bar);
        clutter_actor_add_child (stage, priv->nonius);
        clutter_actor_set_child_above_sibling (stage, priv->nonius, priv->bar);
        
        /* 在设置 meta-slides 属性时，可能 stage 还未被设置，
           因此这里保证要将初始的幻灯片添加到 stage 中 */
        if (priv->slide) {
                clutter_actor_add_child (priv->stage, priv->slide);
        }
        
        g_signal_connect (priv->stage,
                          "allocation-changed",
                          G_CALLBACK(ckd_view_on_stage_allocation),
                          self);
}

static void
ckd_view_set_property (GObject *o, guint prop, const GValue *v, GParamSpec *p)
{
        CkdView *self = CKD_VIEW (o);
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_VIEW_STAGE:
                ckd_view_config_stage (self, g_value_get_pointer (v));
                break;
        case PROP_CKD_VIEW_META_SLIDES:
                priv->meta_slides = g_value_get_pointer (v);
                /*\begin 向视图内放置第一张幻灯片 */
                priv->slide = ckd_meta_slides_get_slide (priv->meta_slides,
                                                         priv->slide_number);
                if (priv->stage) {
                        clutter_actor_add_child (priv->stage, priv->slide);
                }
                /*\end */
                break;
        case PROP_CKD_VIEW_PADDING:
                priv->padding = g_value_get_float (v);
                break;
        case PROP_CKD_VIEW_BAR_COLOR:
                if (priv->bar_color)
                        clutter_color_free (priv->bar_color);
                priv->bar_color = g_value_get_pointer (v);
                clutter_actor_set_background_color (priv->bar, priv->bar_color);
                break;
        case PROP_CKD_VIEW_BAR_VSIZE:
                priv->bar_vsize = g_value_get_float (v);
                clutter_actor_set_height (priv->bar, priv->bar_vsize);
                /* 将 nonius 的尺寸也设为该值 */
                clutter_actor_set_size (priv->nonius,
                                        priv->bar_vsize,
                                        priv->bar_vsize);
                break;
        case PROP_CKD_VIEW_NONIUS_COLOR:
                if (priv->nonius_color)
                        clutter_color_free (priv->nonius_color);
                priv->nonius_color = g_value_get_pointer (v);
                clutter_actor_set_background_color (priv->nonius, priv->nonius_color);
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
        case PROP_CKD_VIEW_STAGE:
                g_value_set_pointer (v, priv->stage);
                break;
        case PROP_CKD_VIEW_META_SLIDES:
                g_value_set_pointer (v, priv->meta_slides);
                break;
        case PROP_CKD_VIEW_SLIDE:
                g_value_set_pointer (v, priv->slide);
                break;
        case PROP_CKD_VIEW_SLIDE_NUMBER:
                g_value_set_int (v, priv->slide_number);
                break;
        case PROP_CKD_VIEW_NONIUS:
                g_value_set_pointer (v, priv->nonius);
                break;
        case PROP_CKD_VIEW_BAR:
                g_value_set_pointer (v, priv->bar);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;
        }
}

static void
ckd_view_dispose (GObject *o)
{
        CkdView *self = CKD_VIEW (o);
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        if (priv->meta_slides) {
                g_object_unref (priv->meta_slides);
                priv->meta_slides = NULL;
        }

        if (priv->bar_color) {
                clutter_color_free (priv->bar_color);
                priv->bar_color = NULL;
        }

        if (priv->nonius_color) {
                clutter_color_free (priv->nonius_color);
                priv->nonius_color = NULL;
        }
                
        G_OBJECT_CLASS (ckd_view_parent_class)->dispose (o);
}

static void
ckd_view_finalize (GObject *o)
{      
        G_OBJECT_CLASS (ckd_view_parent_class)->finalize (o);
}

static void
ckd_view_class_init (CkdViewClass *c)
{
        g_type_class_add_private (c, sizeof (CkdViewPriv));

        GObjectClass *base_class = G_OBJECT_CLASS (c);
        base_class->set_property = ckd_view_set_property;
        base_class->get_property = ckd_view_get_property;
        base_class->dispose      = ckd_view_dispose;
        base_class->finalize     = ckd_view_finalize;
        
        GParamSpec *props[N_CKD_VIEW_PROPS] = {NULL,};
        
        GParamFlags r_flag = G_PARAM_READABLE;
        GParamFlags w_co_flags = G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY;
        GParamFlags rw_co_flags = G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY;
        GParamFlags rw_flags = G_PARAM_READWRITE;
        
        props[PROP_CKD_VIEW_STAGE] = g_param_spec_pointer ("stage",
                                                           "Stage",
                                                           "Stage",
                                                           rw_co_flags);
        
        props[PROP_CKD_VIEW_META_SLIDES] = g_param_spec_pointer ("meta-slides",
                                                                 "Meta Slides",
                                                                 "Meta Slides",
                                                                 rw_co_flags);

        props[PROP_CKD_VIEW_SLIDE] = g_param_spec_pointer ("slide",
                                                           "Slide",
                                                           "Slide",
                                                           r_flag);

        props[PROP_CKD_VIEW_SLIDE_NUMBER] = g_param_spec_int ("slide-number",
                                                                "Slide Number",
                                                                "Slide Number",
                                                                0, G_MAXINT, 0,
                                                                r_flag);
        
        props[PROP_CKD_VIEW_NONIUS] = g_param_spec_pointer ("nonius",
                                                           "Nonius",
                                                           "Nonius",
                                                           r_flag);

        props[PROP_CKD_VIEW_BAR] = g_param_spec_pointer ("bar",
                                                         "Bar",
                                                         "Bar",
                                                         r_flag);
        
        props[PROP_CKD_VIEW_PADDING] = g_param_spec_float ("padding",
                                                           "Padding",
                                                           "Padding",
                                                           0.0, G_MAXFLOAT, 10.0,
                                                           w_co_flags);
        
        props[PROP_CKD_VIEW_BAR_COLOR] = g_param_spec_pointer ("bar-color",
                                                               "Bar Color",
                                                               "Bar Color",
                                                               rw_flags);
        
        props[PROP_CKD_VIEW_BAR_VSIZE] = g_param_spec_float ("bar-vsize",
                                                             "Bar Vertical size",
                                                             "Bar Vertical size",
                                                             0.0, G_MAXFLOAT, 20.0,
                                                             rw_flags);

        props[PROP_CKD_VIEW_NONIUS_COLOR] = g_param_spec_pointer ("nonius-color",
                                                                  "Nonius Color",
                                                                  "Nonius Color",
                                                                  rw_flags);
        
        g_object_class_install_properties (base_class, N_CKD_VIEW_PROPS, props);
}

static void
ckd_view_init (CkdView *self)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);

        priv->slide_number = 0;
        
        priv->bar = clutter_actor_new ();
        clutter_actor_set_reactive (priv->bar, TRUE);
        
        priv->nonius = clutter_actor_new ();
}

ClutterActor *
ckd_view_load_ith_slide (CkdView *self, gint i)
{
        CkdViewPriv *priv = CKD_VIEW_GET_PRIVATE (self);
        ClutterActor *slide = ckd_meta_slides_get_slide (priv->meta_slides, i);
        
        if (slide) {
                clutter_actor_add_child (priv->stage, slide);
                clutter_actor_set_child_below_sibling (priv->stage, slide, priv->slide);
                ckd_view_allocate_slide (self, slide);                

                priv->slide = slide;
                priv->slide_number = i;
        }

        return slide;
}
