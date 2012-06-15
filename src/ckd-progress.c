#include <math.h>
#include "ckd-progress.h"

G_DEFINE_TYPE (CkdProgress, ckd_progress, CLUTTER_TYPE_ACTOR);
#define CKD_PROGRESS_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_PROGRESS, CkdProgressPriv))

typedef struct _CkdProgressPriv {
        ClutterActor *bar;
        ClutterActor *nonius;
        ClutterColor *bg;
        ClutterColor *fg;
        gfloat tick;
} CkdProgressPriv;

enum {
        PROP_CKD_PROGRESS_0,
        PROP_CKD_PROGRESS_BG,
        PROP_CKD_PROGRESS_FG,
        PROP_CKD_PROGRESS_TICK,
        N_CKD_PROGRESS_PROPS
};

static void
ckd_progress_set_property (GObject *o, guint prop, const GValue *v, GParamSpec *p)
{
        CkdProgress *self = CKD_PROGRESS (o);
        CkdProgressPriv *priv = CKD_PROGRESS_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_PROGRESS_BG:
                priv->bg = g_value_get_pointer (v);
                clutter_actor_set_background_color (priv->bar, priv->bg);
                break;
        case PROP_CKD_PROGRESS_FG:
                priv->fg = g_value_get_pointer (v);
                clutter_actor_set_background_color (priv->nonius, priv->fg);
                break;
        case PROP_CKD_PROGRESS_TICK:
                priv->tick = g_value_get_float (v);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;
        }
}

static void
ckd_progress_get_property (GObject *o, guint prop, GValue *v, GParamSpec *p)
{
        CkdProgress *self = CKD_PROGRESS (o);
        CkdProgressPriv *priv = CKD_PROGRESS_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_PROGRESS_BG:
                g_value_set_pointer (v, priv->bg);
                break;
        case PROP_CKD_PROGRESS_FG:
                g_value_set_pointer (v, priv->fg);
                break;
        case PROP_CKD_PROGRESS_TICK:
                g_value_set_float (v, priv->tick);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop, p);
                break;
        }
}

static void
ckd_progress_destroy (ClutterActor *a)
{
        CkdProgress *self = CKD_PROGRESS (a);
        CkdProgressPriv *priv = CKD_PROGRESS_GET_PRIVATE (self);

        if (priv->bar) {
                clutter_actor_destroy (priv->bar);
                priv->bar = NULL;
        }
        if (priv->nonius) {
                clutter_actor_destroy (priv->nonius);
                priv->nonius = NULL;
        }
        if (priv->bg) {
                g_slice_free (ClutterColor, priv->bg);
                priv->bg = NULL;
        }
        if (priv->fg) {
                g_slice_free (ClutterColor, priv->fg);
                priv->fg = NULL;
        }
        
        if (CLUTTER_ACTOR_CLASS (ckd_progress_parent_class)->destroy)
                CLUTTER_ACTOR_CLASS (ckd_progress_parent_class)->destroy (a);
}

static void
ckd_progress_allocate (ClutterActor *a,
                       const ClutterActorBox *b,
                       ClutterAllocationFlags f)
{
        CkdProgress *self = CKD_PROGRESS (a);
        CkdProgressPriv *priv = CKD_PROGRESS_GET_PRIVATE (self);
        
        CLUTTER_ACTOR_CLASS (ckd_progress_parent_class)->allocate (a, b, f);
        
       gfloat w = clutter_actor_box_get_width (b);
       gfloat h = clutter_actor_box_get_height (b);
       
       ClutterActorBox bar_box = { 0, 0, w, h };
       clutter_actor_allocate (priv->bar, &bar_box, f);

       gfloat x1, y1, x2, y2;
       x1 = priv->tick * w;
       y1 = 0;
       x2 = x1 + 20;
       y2 = 20;
       ClutterActorBox nonius_box = {x1, y1, x2, y2};
       clutter_actor_allocate (priv->nonius, &nonius_box, f);
}

static void
ckd_progress_paint (ClutterActor *a)
{
        CkdProgress *self = CKD_PROGRESS (a);
        CkdProgressPriv *priv = CKD_PROGRESS_GET_PRIVATE (self);

        clutter_actor_paint (priv->bar);
        clutter_actor_paint (priv->nonius);
}

static void
ckd_progress_class_init (CkdProgressClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdProgressPriv));

        ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
        actor_class->destroy = ckd_progress_destroy;
        actor_class->paint = ckd_progress_paint;
        actor_class->allocate = ckd_progress_allocate;

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->set_property = ckd_progress_set_property;
        base_class->get_property = ckd_progress_get_property;

        GParamSpec *props[N_CKD_PROGRESS_PROPS] = {NULL,};
        props[PROP_CKD_PROGRESS_BG] = g_param_spec_pointer ("background",
                                                            "Background",
                                                            "Background",
                                                            G_PARAM_READWRITE);
        props[PROP_CKD_PROGRESS_FG] = g_param_spec_pointer ("foreground",
                                                            "Foreground",
                                                            "Foreground",
                                                            G_PARAM_READWRITE);
        props[PROP_CKD_PROGRESS_TICK] = g_param_spec_float ("tick",
                                                             "Tick",
                                                             "Tick",
                                                             0.0,
                                                             1.0,
                                                             0.1,
                                                             G_PARAM_READWRITE);
        g_object_class_install_properties (base_class, N_CKD_PROGRESS_PROPS, props);
}

static void
ckd_progress_init (CkdProgress *self)
{
        CkdProgressPriv *priv = CKD_PROGRESS_GET_PRIVATE (self);

        priv->bar = clutter_actor_new ();
        clutter_actor_add_child (CLUTTER_ACTOR(self), priv->bar);

        priv->nonius = clutter_actor_new ();
        clutter_actor_set_size (priv->nonius, 20, 20);
        clutter_actor_add_child (CLUTTER_ACTOR(self), priv->nonius);
        
        priv->bg = NULL;
        priv->fg = NULL;
        priv->tick = 0.0;
}

static void
ckd_progress_nonius_am_cb (ClutterAnimation *animation, ClutterActor *actor)
{
        ClutterActor *source = clutter_clone_get_source (CLUTTER_CLONE(actor));
        clutter_actor_show (source);
        clutter_actor_destroy (actor);
}

void
ckd_progress_am (ClutterActor *self, gdouble tick)
{
        CkdProgressPriv *priv = CKD_PROGRESS_GET_PRIVATE (CKD_PROGRESS(self));

        priv->tick = tick;
        
        ClutterActor *nonius_clone = clutter_clone_new (priv->nonius);
        ClutterActor *stage = clutter_actor_get_stage (CLUTTER_ACTOR(self));

        clutter_actor_add_child (stage, nonius_clone);
        clutter_actor_set_child_above_sibling (stage, nonius_clone, NULL);

        gfloat x, y, w, h;
        clutter_actor_get_size (self, &w, &h);
        clutter_actor_get_position (self, &x, &y);

        clutter_actor_set_position (nonius_clone, x + (tick - 0.1) * w, y);

        clutter_actor_hide (priv->nonius);
        clutter_actor_queue_relayout (self);

        clutter_actor_animate (nonius_clone,
                               CLUTTER_EASE_IN_OUT_CUBIC,
                               1000,
                               "x", x + tick * w,
                               "signal::completed",
                               ckd_progress_nonius_am_cb,
                               nonius_clone,
                               NULL);
}
