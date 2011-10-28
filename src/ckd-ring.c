#include <math.h>
#include "ckd-ring.h"

G_DEFINE_TYPE (CkdRing, ckd_ring, CLUTTER_TYPE_ACTOR);

#define CKD_RING_DIAMETER 240
#define CKD_RING_WIDTH 24

#define CKD_RING_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_RING, CkdRingPriv))

typedef struct _CkdRingPriv {
        ClutterActor *ring;
        gint index;
        gint number_of_slides;
} CkdRingPriv;

enum {
        PROP_0,
        PROP_RING_INDEX,
        PROP_RING_NUMBER_OF_SLIDES,
        N_PROPS
};

static gboolean
ckd_ring_render (ClutterCairoTexture *actor, cairo_t *cr, gpointer user_data)
{
        CkdRing *self = CKD_RING (user_data);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);

        /* 清除上一次的画面 */
        clutter_cairo_texture_clear (actor);
 
        gfloat cx = 0.5 * clutter_actor_get_width (priv->ring);
        gfloat cy = 0.5 * clutter_actor_get_height (priv->ring);
        gfloat r_out  = (cx < cy) ? cx : cy;
        gfloat r_in   = r_out - CKD_RING_WIDTH;

        cairo_pattern_t *pat = cairo_pattern_create_radial (cx, cy, r_out, cx, cy, r_in);
        cairo_pattern_add_color_stop_rgba (pat, 0.0, 0, 0, 0, 0.0);
        cairo_pattern_add_color_stop_rgba (pat, 0.05, 0.1, 0.1, 0.1, 0.2);
        cairo_pattern_add_color_stop_rgba (pat, 0.5, 0.2, 0.4, 0.8, 1.0);
        cairo_pattern_add_color_stop_rgba (pat, 0.95, 0.1, 0.1, 0.1, 0.2);
        cairo_pattern_add_color_stop_rgba (pat, 1.0, 0, 0, 0, 0.0);
        cairo_set_source (cr, pat);
        cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_arc (cr, cx, cy, r_out, 0, 2 * G_PI);
        cairo_arc (cr, cx, cy, r_in, 0, 2 * G_PI);
        cairo_fill (cr);


        gfloat delta = 2 * G_PI / priv->number_of_slides;
        gfloat start = - G_PI / 2;
        gfloat r = 0.85 * r_in;
        gfloat mark_len = 0.05 * r_in;
        gfloat handle_len = 0.75 * r_in;
        gfloat theta = start + priv->index * delta;
        
        cairo_set_line_width (cr, 8);
        cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);        

        cairo_set_source_rgba (cr, 0.8, 0.4, 0.2, 0.8);
        cairo_move_to (cr, cx, cy - r_in + mark_len);
        cairo_line_to (cr, cx, cy - r_in);
        cairo_stroke (cr);
        
        cairo_set_source_rgba (cr, 0.74, 0.8, 0.075, 0.8);
        cairo_move_to (cr, cx, cy);
        cairo_line_to (cr, cx + r * cos (theta), cy + r * sin (theta));
        cairo_stroke (cr);

        return TRUE;
}

static void
ckd_ring_set_property (GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec)
{
        CkdRing *self = CKD_RING (obj);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);
        gint index;
        
        switch (prop_id) {
        case PROP_RING_INDEX:
                priv->index = g_value_get_int (value);
                clutter_cairo_texture_invalidate (CLUTTER_CAIRO_TEXTURE(priv->ring));
                break;
        case PROP_RING_NUMBER_OF_SLIDES:
                priv->number_of_slides = g_value_get_int (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
                break;
        }
}

static void
ckd_ring_get_property (GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec)
{
        CkdRing *self = CKD_RING (obj);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);

        switch (prop_id) {
        case PROP_RING_INDEX:
                g_value_set_int (value, priv->index);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
                break;
        }
}

static void
ckd_ring_get_preferred_width (ClutterActor *actor,
                              gfloat for_height, gfloat *min_width, gfloat *natural_width)
{
        CkdRing *self = CKD_RING (actor);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);
        
        clutter_actor_get_preferred_height (priv->ring,
                                            for_height,
                                            min_width,
                                            natural_width);
}

static void
ckd_ring_get_preferred_height (ClutterActor *actor,
                               gfloat for_width, gfloat *min_height, gfloat *natural_height)
{
        CkdRing *self = CKD_RING (actor);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);
        
        clutter_actor_get_preferred_height (priv->ring,
                                            for_width,
                                            min_height,
                                            natural_height);
}

static void
ckd_ring_destroy (ClutterActor *self)
{
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);

        if (priv->ring) {
                clutter_actor_destroy (priv->ring);
                priv->ring = NULL;
        }

        if (CLUTTER_ACTOR_CLASS (ckd_ring_parent_class)->destroy) {
                CLUTTER_ACTOR_CLASS (ckd_ring_parent_class)->destroy (self);
        }
}

static void
ckd_ring_allocate (ClutterActor *actor, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
        CkdRing *self = CKD_RING (actor);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);

        CLUTTER_ACTOR_CLASS (ckd_ring_parent_class)->allocate (actor, box, flags);

        gfloat w, h;
        w = clutter_actor_box_get_width (box);
        h = clutter_actor_box_get_height (box);

        ClutterActorBox bbox = {0.0, 0.0, w, h};
        clutter_actor_allocate (priv->ring, &bbox, flags);
}

static void
ckd_ring_paint (ClutterActor *actor)
{
        CkdRing *self = CKD_RING (actor);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);
        
        clutter_actor_paint (priv->ring);
}

static void
ckd_ring_pick (ClutterActor *actor, const ClutterColor *pick_color)
{
        CkdRing *self = CKD_RING (actor);
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);
        
        CLUTTER_ACTOR_CLASS (ckd_ring_parent_class)->pick (actor, pick_color);
}

static void
ckd_ring_class_init (CkdRingClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdRingPriv));

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->set_property = ckd_ring_set_property;
        base_class->get_property = ckd_ring_get_property;

        GParamSpec *pspec;
        pspec = g_param_spec_int ("index",
                                  "Index",
                                  "Current Slide Number",
                                  0,
                                  G_MAXINT,
                                  0,
                                  G_PARAM_WRITABLE);
        g_object_class_install_property (base_class, PROP_RING_INDEX, pspec);
        
        pspec = g_param_spec_int ("number-of-slides",
                                  "Number of Slides",
                                  "Number of Slides",
                                  0,
                                  G_MAXINT,
                                  0,
                                  G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (base_class, PROP_RING_NUMBER_OF_SLIDES, pspec);
        
        ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
        actor_class->destroy = ckd_ring_destroy;
        actor_class->paint = ckd_ring_paint;
        actor_class->pick = ckd_ring_pick;
        actor_class->get_preferred_height = ckd_ring_get_preferred_height;
        actor_class->get_preferred_width = ckd_ring_get_preferred_width;
        actor_class->allocate = ckd_ring_allocate;
}

static void
ckd_ring_init (CkdRing *self)
{
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);

        priv->ring = clutter_cairo_texture_new (CKD_RING_DIAMETER, CKD_RING_DIAMETER);
        clutter_actor_set_parent (priv->ring, CLUTTER_ACTOR(self));
        clutter_cairo_texture_set_auto_resize (CLUTTER_CAIRO_TEXTURE (priv->ring), TRUE);
        
        g_signal_connect (priv->ring, "draw", G_CALLBACK (ckd_ring_render), self);
}

gint
ckd_ring_get_polar_coordinates_map (CkdRing *self, gfloat x, gfloat y)
{
        CkdRingPriv *priv = CKD_RING_GET_PRIVATE (self);

        gfloat w = clutter_actor_get_width (CLUTTER_ACTOR(self));
        gfloat h = clutter_actor_get_height (CLUTTER_ACTOR(self));
        gfloat r = (w < h) ? w : h;
        gfloat cx = w / 2;
        gfloat cy = h / 2;
        
        gfloat outer  = (cx < cy) ? cx : cy;
        gfloat inner   = 0.5 * outer;
        
        x -= cx;
        y -= cy;

        /* 将 y 轴反向 */
        y = -y;
        gfloat m = sqrtf(x * x + y * y);
        if (m < inner || m > outer)
                return -1;
        
        gfloat theta = asinf (fabs (y) / m);
        if (x >= 0 && y > 0 )
                theta = (G_PI / 2) - theta;
        else if (x > 0 && y <= 0)
                theta += (G_PI / 2);
        else if (x < 0 && y <= 0)
                theta = (3 * G_PI / 2) - theta;
        else
                theta += (3 * G_PI / 2);

        gint index = (theta / (2 * G_PI)) * (gfloat)priv->number_of_slides + 1;
        
        return index;
}
