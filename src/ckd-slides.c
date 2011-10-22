#include <math.h>
#include "ckd-slides.h"
#include "ckd-ring.h"

G_DEFINE_TYPE (CkdSlides, ckd_slides, CKD_TYPE_PAGE_MANAGER);

#define CKD_SLIDES_GET_PRIVATE(obj) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_SLIDES, CkdSlidesPriv))

typedef struct _CkdSlidesPriv CkdSlidesPriv;
struct  _CkdSlidesPriv {
        ClutterActor *box;
        ClutterActor *current_slide;
        ClutterActor *next_slide;
        ClutterActor *index_ring;
        gboolean is_overview;
};

enum {
        PROP_SLIDES_0,
        PROP_SLIDES_BOX,
        PROP_SLIDES_CURRENT_SLIDE,
        PROP_SLIDES_NEXT_SLIDE,
        PROP_SLIDES_IS_OVERVIEW,
        N_SLIDES_PROPS
};

static gint
_ckd_slides_get_index (ClutterActor *slide)
{
        PopplerPage *page;
        g_object_get (slide, "pdf-page", &page, NULL);
        
        return poppler_page_get_index (page);
}

static void
_ckd_slides_box_paint (ClutterActor *actor, gpointer user_data)
{
        CkdSlides *self = CKD_SLIDES (user_data);
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        
        gfloat w, h;
        w = clutter_actor_get_width (actor);
        h = clutter_actor_get_height (actor);
        
        g_object_set (self, "page-width", w, "page-height", h, NULL);
}

static void
_ckd_slides_set_box (GObject *obj, GParamSpec *pspec, gpointer user_data)
{
        CkdSlides *self = CKD_SLIDES (obj);
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        ClutterActor *page;
        
        g_object_ref (priv->box);
        g_signal_connect (priv->box, "paint", G_CALLBACK(_ckd_slides_box_paint), self);
        
        page = ckd_page_manager_get_page (CKD_PAGE_MANAGER(self), 0);
        ckd_page_manager_cache (CKD_PAGE_MANAGER(self), page);

        clutter_actor_show (page);
        clutter_container_add_actor (CLUTTER_CONTAINER(priv->box), page);

        priv->current_slide = page;

        /* 创建索引环 */
        gint n;
        g_object_get (self, "number-of-pages", &n, NULL);
        priv->index_ring = g_object_new (CKD_TYPE_RING, "number-of-slides", n, NULL);
}

static void
ckd_slides_set_property (GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec)
{
        CkdSlides *self = CKD_SLIDES (obj);
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);

        switch (prop_id) {
        case PROP_SLIDES_BOX:
                priv->box = g_value_get_pointer (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
                break;
        }
}

static void
ckd_slides_get_property (GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec)
{
        CkdSlides *self = CKD_SLIDES (obj);
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        
        switch (prop_id) {
        case PROP_SLIDES_BOX:
                g_value_set_pointer (value, priv->box);
                break;
        case PROP_SLIDES_CURRENT_SLIDE:
                g_value_set_pointer (value, priv->current_slide);
                break;
        case PROP_SLIDES_NEXT_SLIDE:
                g_value_set_pointer (value, priv->next_slide);
                break;
        case PROP_SLIDES_IS_OVERVIEW:
                g_value_set_boolean (value, priv->is_overview);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
                break;
        }
}

static void
ckd_slides_finalize (GObject *obj)
{
        CkdSlides *self     = CKD_SLIDES (obj);
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);

        clutter_actor_destroy (priv->box);
        
        G_OBJECT_CLASS (ckd_slides_parent_class)->dispose (obj);
}

static void
ckd_slides_class_init (CkdSlidesClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdSlidesPriv));

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->finalize      = ckd_slides_finalize;
        base_class->set_property = ckd_slides_set_property;
        base_class->get_property = ckd_slides_get_property;

        GParamSpec *props[N_SLIDES_PROPS] = {NULL,};
        props[PROP_SLIDES_BOX] =
                g_param_spec_pointer ("box", "Box", "Slides Box", G_PARAM_READWRITE);
        props[PROP_SLIDES_CURRENT_SLIDE] =
                g_param_spec_pointer ("current-slide", "Current Slide", "Current Slide",
                                      G_PARAM_READABLE);
        props[PROP_SLIDES_NEXT_SLIDE] =
                g_param_spec_pointer ("next-slide", "Next Slide", "Next Slide",
                                      G_PARAM_READABLE);
        props[PROP_SLIDES_IS_OVERVIEW] =
                g_param_spec_boolean ("is-overview",
                                      "Slides Overview Status",
                                      "Slides Overview Status",
                                      FALSE,
                                      G_PARAM_READABLE);

        g_object_class_install_properties (base_class, N_SLIDES_PROPS, props);
}

static void
ckd_slides_init (CkdSlides *self)
{
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        
        g_signal_connect (self, "notify::box", G_CALLBACK (_ckd_slides_set_box), NULL);
}

/********************************************************************************/
/*                              CkdSlides 页面切换的淡入淡出效果
/********************************************************************************/
static void
_ckd_page_raise (ClutterActor *box, ClutterActor *page)
{
        if (!clutter_actor_get_parent (page))
                clutter_container_add_actor (CLUTTER_CONTAINER(box), page);
        else
                clutter_container_raise_child (CLUTTER_CONTAINER(box), page, NULL);
}

static void
_ckd_slides_on_fade (ClutterAnimation *am, gpointer data)
{
        CkdPageManager *self = CKD_PAGE_MANAGER (data);
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);

        ckd_page_manager_uncache (self, priv->current_slide);
        priv->current_slide = priv->next_slide;
}

static void
_ckd_slides_fade_out_and_in (CkdSlides *self, gdouble time)
{
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        
        clutter_actor_set_opacity (priv->next_slide, 255);
        clutter_actor_set_opacity (priv->current_slide, 255);

        /* 华丽的逆转 */
        ClutterTimeline *timeline;
        ClutterTimelineDirection direction;
        ClutterAnimation *am = clutter_actor_get_animation (priv->next_slide);
        if (am) {
                timeline = clutter_animation_get_timeline (am);
                direction = clutter_timeline_get_direction (timeline);
                
                if (direction == CLUTTER_TIMELINE_FORWARD)
                        direction = CLUTTER_TIMELINE_BACKWARD;
                else
                        direction = CLUTTER_TIMELINE_FORWARD;
                
                clutter_timeline_set_direction (timeline, direction);
        }

        /* 让当前页消隐 */
        clutter_actor_animate (
                priv->current_slide,
                CLUTTER_LINEAR,
                1000 * ((guint)time),
                "opacity", 0,
                "signal-after::completed", _ckd_slides_on_fade, self,
                NULL);
}

void
ckd_slides_switch_to_next_slide (CkdSlides *self, gint direction)
{
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        
        priv->current_slide = ckd_page_manager_get_current_page (CKD_PAGE_MANAGER(self));
        ckd_page_manager_cache (CKD_PAGE_MANAGER(self), priv->current_slide);
        
        /* 因为页面载入有时会较慢，会破坏动画效果，所以要根据页面加载时间来推延动画持续时间 */
        GTimer *timer = g_timer_new ();
        gdouble time  = 1.0;
        gdouble time_scale = 3.0;
        gdouble next_slide_loading_time;
        
        g_timer_start (timer);
        {
                if (direction > 0)
                        priv->next_slide = ckd_page_manager_advance_page (CKD_PAGE_MANAGER(self));
                if (direction < 0)
                        priv->next_slide = ckd_page_manager_retreat_page (CKD_PAGE_MANAGER(self));
        }
        g_timer_stop (timer);
        next_slide_loading_time = time_scale * g_timer_elapsed (timer, NULL);
        g_timer_destroy (timer);

        if (priv->current_slide == priv->next_slide) {
                ckd_page_manager_uncache (CKD_PAGE_MANAGER(self), priv->current_slide);
                return;
        }

        if (time < next_slide_loading_time)
                time =  next_slide_loading_time;
        
        _ckd_page_raise (priv->box, priv->next_slide);
        _ckd_page_raise (priv->box, priv->current_slide);
        _ckd_slides_fade_out_and_in (self, time);

        /* 更新索引环的当前页面索引值 */
        g_object_set (priv->index_ring, "index", _ckd_slides_get_index (priv->next_slide), NULL);
}

/********************************************************************************/
/*                              CkdSlides 概览视图
/********************************************************************************/
static gfloat slide_offset_x_on_overview = 0;

void
ckd_slides_overview_on  (CkdSlides *self)
{
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        priv->is_overview = TRUE;
        
        ClutterActor *stage = clutter_actor_get_stage (priv->box);
        /* clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), FALSE); */

        if (clutter_actor_get_stage (priv->index_ring) != stage) {
                clutter_container_add_actor (CLUTTER_CONTAINER(stage), priv->index_ring);
        }
        g_object_set (priv->index_ring, "index", _ckd_slides_get_index (priv->current_slide), NULL);
        clutter_actor_show (priv->index_ring);

        /* 让索引环位于 Slides 盒子之后 */
        clutter_container_lower_child (CLUTTER_CONTAINER(stage), priv->index_ring, NULL);
        
        /* --Begin: Slide */
        gfloat stage_w, stage_h;
        stage_w = clutter_actor_get_width (stage);
        stage_h = clutter_actor_get_height (stage);
        
        gfloat cx = 0.5 * stage_w;
        gfloat cy = 0.5 * stage_h;
        gfloat s = 0.5;
        g_object_set (priv->box,
                      "scale-center-x", cx,
                      "scale-center-y", cy,
                      "scale-gravity", CLUTTER_GRAVITY_CENTER,
                      NULL);

        gfloat slide_w, slide_h, offset;
        g_object_get (priv->current_slide,
                      "surface-width", &slide_w,
                      "surface-height", &slide_h,
                      NULL);
        offset = 0.25 * slide_w;
        
        clutter_actor_animate (priv->box, CLUTTER_LINEAR, 1000,
                               "scale-x", s,
                               "scale-y", s,
                               "x", offset,
                               NULL);
        /* --End*/

        /* --Begin: 索引环 */
        gfloat d = 0.5 * ((slide_w < slide_h) ? slide_w : slide_h);
        gfloat r = 0.5 * d;
        clutter_actor_set_size (priv->index_ring, d, d);
        clutter_actor_set_position (priv->index_ring,
                                    0.5 * (stage_w - slide_w),
                                    cy - r);
        clutter_actor_set_opacity (priv->index_ring, 0);
        clutter_actor_animate (priv->index_ring, CLUTTER_LINEAR, 1000, "opacity", 255, NULL);
        /* --End */
}

void
ckd_slides_overview_off (CkdSlides *self)
{
        CkdSlidesPriv *priv = CKD_SLIDES_GET_PRIVATE (self);
        ClutterActor *stage = clutter_actor_get_stage (priv->box);        

        priv->is_overview = FALSE;
        
        /* clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), TRUE); */

        /* 华丽的逆转 */
        ClutterTimeline *timeline;
        ClutterTimelineDirection direction;
        ClutterAnimation *slides_am = clutter_actor_get_animation (priv->box);
        ClutterAnimation *index_ring_am = clutter_actor_get_animation (priv->index_ring);
        if (slides_am) {
                timeline = clutter_animation_get_timeline (slides_am);
                direction = clutter_timeline_get_direction (timeline);
                
                if (direction == CLUTTER_TIMELINE_FORWARD)
                        direction = CLUTTER_TIMELINE_BACKWARD;
                else
                        direction = CLUTTER_TIMELINE_FORWARD;
                
                clutter_timeline_set_direction (timeline, direction);
        } else {
                clutter_actor_animate (priv->box, CLUTTER_LINEAR, 1000,
                                       "scale-x", 1.0,
                                       "scale-y", 1.0,
                                       "x", 0.0,
                                       NULL);
        }
        
        if (index_ring_am) {
                timeline = clutter_animation_get_timeline (index_ring_am);
                direction = clutter_timeline_get_direction (timeline);
                
                if (direction == CLUTTER_TIMELINE_FORWARD)
                        direction = CLUTTER_TIMELINE_BACKWARD;
                else
                        direction = CLUTTER_TIMELINE_FORWARD;
                
                clutter_timeline_set_direction (timeline, direction);
        } else {
                clutter_actor_animate (priv->index_ring, CLUTTER_LINEAR, 500, "opacity", 0, NULL);
        }
        
}
