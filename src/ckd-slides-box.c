#include <math.h>
#include "ckd-slides-box.h"

G_DEFINE_TYPE (CkdSlidesBox, ckd_slides_box, CLUTTER_TYPE_ACTOR);

#define CKD_SLIDES_BOX_GET_PRIVATE(o) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((o), CKD_TYPE_SLIDES_BOX, CkdSlidesBoxPriv))

typedef struct _CkdSlidesBoxPriv CkdSlidesBoxPriv;
struct _CkdSlidesBoxPriv {
        ClutterActor *slide;
        gfloat time_axis_width;
        gfloat padding;
};

enum {
        PROP_CKD_SLIDES_BOX_0,
        PROP_CKD_SLIDES_BOX_SLIDE,
        PROP_CKD_SLIDES_BOX_TIME_AXIS_WIDTH,
        PROP_CKD_SLIDES_BOX_PADDING,
        N_CKD_SLIDES_BOX_PROPS
};

static void
ckd_slides_box_set_property (GObject *o, guint prop, const GValue *v, GParamSpec *p)
{
        CkdSlidesBox *self = CKD_SLIDES_BOX (o);
        CkdSlidesBoxPriv *priv = CKD_SLIDES_BOX_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_SLIDES_BOX_SLIDE:
                priv->slide = g_value_get_pointer (v);
                clutter_actor_add_child (CLUTTER_ACTOR(self), priv->slide);
                break;
        case PROP_CKD_SLIDES_BOX_TIME_AXIS_WIDTH:
                priv->time_axis_width = g_value_get_float (v);
                break;
        case PROP_CKD_SLIDES_BOX_PADDING:
                priv->padding = g_value_get_float (v);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;
        }
}

static void
ckd_slides_box_get_property (GObject *o, guint prop, GValue *v, GParamSpec *p)
{
        CkdSlidesBox *self = CKD_SLIDES_BOX (o);
        CkdSlidesBoxPriv *priv = CKD_SLIDES_BOX_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_SLIDES_BOX_SLIDE:
                g_value_set_pointer (v, priv->slide);
                break;
        case PROP_CKD_SLIDES_BOX_TIME_AXIS_WIDTH:
                g_value_set_float (v, priv->time_axis_width);
                break;
        case PROP_CKD_SLIDES_BOX_PADDING:
                g_value_set_float (v, priv->padding);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;                
        }
}

static void
ckd_slides_box_destroy (ClutterActor *a)
{
        CkdSlidesBox *self = CKD_SLIDES_BOX (a);
        CkdSlidesBoxPriv *priv = CKD_SLIDES_BOX_GET_PRIVATE (self);

        if (priv->slide) {
                clutter_actor_destroy (priv->slide);
                priv->slide = NULL;
        }
        
        if (CLUTTER_ACTOR_CLASS (ckd_slides_box_parent_class)->destroy)
                CLUTTER_ACTOR_CLASS (ckd_slides_box_parent_class)->destroy (a);
}

static void
ckd_slides_allocate_slide (const ClutterActorBox *box,
                           ClutterActor *slide,
                           gfloat padding,
                           gfloat time_axis_width,
                           ClutterAllocationFlags f)
{
        gfloat inner_w, inner_h, slide_w, slide_h, slide_r;
        ClutterActorBox *slide_box = g_slice_alloc (sizeof(ClutterActorBox));
        
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
        
        slide_box->x1 = 0.5 * (box->x1 + box->x2 - slide_w);
        slide_box->y1 = 0.5 * (box->y1 + box->y2 - slide_h - time_axis_width);
        slide_box->x2 = slide_box->x1 + slide_w;
        slide_box->y2 = slide_box->y1 + slide_h;
        
        clutter_actor_allocate (slide, slide_box, f);
        
        g_slice_free (ClutterActorBox, slide_box);
}

static void
ckd_slides_box_allocate (ClutterActor *a, const ClutterActorBox *b, ClutterAllocationFlags f)
{
        CkdSlidesBox *self = CKD_SLIDES_BOX (a);
        CkdSlidesBoxPriv *priv = CKD_SLIDES_BOX_GET_PRIVATE (self);
        ClutterActorBox *slide_box = NULL;
        
        CLUTTER_ACTOR_CLASS (ckd_slides_box_parent_class)->allocate (a, b, f);

        if (priv->slide) {
                ckd_slides_allocate_slide (b,
                                           priv->slide,
                                           priv->padding,
                                           priv->time_axis_width,
                                           f);
        }
        if (priv->next_slide) {
                ckd_slides_allocate_slide (b,
                                           priv->next_slide,
                                           priv->padding,
                                           priv->time_axis_width,
                                           f);
        }
}

static void
ckd_slides_box_paint (ClutterActor *a)
{
        CkdSlidesBox *self = CKD_SLIDES_BOX (a);
        CkdSlidesBoxPriv *priv = CKD_SLIDES_BOX_GET_PRIVATE (self);

        if (priv->slide)
                clutter_actor_paint (priv->slide);
        if (priv->next_slide)
                clutter_actor_paint (priv->next_slide);
}

static void
ckd_slides_box_class_init (CkdSlidesBoxClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdSlidesBoxPriv));

        ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
        actor_class->destroy = ckd_slides_box_destroy;
        actor_class->paint = ckd_slides_box_paint;
        actor_class->allocate = ckd_slides_box_allocate;
        
        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->set_property = ckd_slides_box_set_property;
        base_class->get_property = ckd_slides_box_get_property;
        
        GParamSpec *props[N_CKD_SLIDES_BOX_PROPS] = {NULL,};
        props[PROP_CKD_SLIDES_BOX_SLIDE] =
                g_param_spec_pointer ("current-slide",
                                      "Current Slide",
                                      "Current Slide",
                                      G_PARAM_READWRITE);
        props[PROP_CKD_SLIDES_BOX_NEXT_SLIDE] =
                g_param_spec_pointer ("next-slide",
                                      "Next Slide",
                                      "Next Slide",
                                      G_PARAM_READWRITE);
        props[PROP_CKD_SLIDES_BOX_TIME_AXIS_WIDTH] =
                g_param_spec_float ("time-axis-width", "Time Axis Width", "Time Axis Width",
                                    G_MINFLOAT,
                                    G_MAXFLOAT,
                                    20.0,
                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_SLIDES_BOX_PADDING] =
                g_param_spec_float ("padding", "Padding", "Padding",
                                    G_MINFLOAT,
                                    G_MAXFLOAT,
                                    10.0,
                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_properties (base_class, N_CKD_SLIDES_BOX_PROPS, props);
}

static void
ckd_slides_box_init (CkdSlidesBox *self)
{
        CkdSlidesBoxPriv *priv = CKD_SLIDES_BOX_GET_PRIVATE (self);

        priv->slide = NULL;
        priv->next_slide  = NULL;
        priv->time_axis_width = 0.0;
        priv->padding = 0.0;
}

void
ckd_slides_box_transit_slide (CkdSlidesBox *self)
{
        CkdSlidesBoxPriv *priv = CKD_SLIDES_BOX_GET_PRIVATE (self);
        
        clutter_actor_animate (priv->slide, CLUTTER_EASE_IN_CUBIC, 500,
                       "x", 100.0,
                       "y", 100.0,
                       NULL);
        
        gdouble scale_x;
        gdouble scale_y;
        clutter_actor_get_scale (priv->next_slide, &scale_x, &scale_y);
        g_print ("%f\t%f\n", scale_x, scale_y);
        clutter_actor_animate   (priv->next_slide, CLUTTER_LINEAR, 5000,
                               "scale-x", scale_x * 0.5,
                               "scale-y", scale_y * 0.5);
}
