#include "ckd-meta-slides.h"
#include "ckd-view.h"
#include "ckd-player.h"

G_DEFINE_TYPE (CkdPlayer, ckd_player, G_TYPE_OBJECT);

#define CKD_PLAYER_GET_PRIVATE(o) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((o), CKD_TYPE_PLAYER, CkdPlayerPriv))

typedef struct _CkdPlayerPriv CkdPlayerPriv;
struct _CkdPlayerPriv {
        CkdMetaSlides *meta_slides;
        CkdView  *view;
        gint i; /* 当前幻灯片编号 */
};

enum {
        PROP_CKD_PLAYER_0,
        PROP_CKD_PLAYER_META_SLIDES,
        PROP_CKD_PLAYER_VIEW,
        N_CKD_PLAYER_PROPS
};

static gboolean
ckd_player_button_press (ClutterActor *a, ClutterEvent * e, gpointer data)
{
        guint button_pressed;
        CkdPlayer *player = data;

        button_pressed = clutter_event_get_button (e);

        switch (button_pressed) {
        case 1:
                ckd_player_forward (player);
                break;
        case 2:
                break;
        case 3:
                ckd_player_rewind (player);
                break;
        default:
                g_print ("You should have a three key mouse!\n");
                break;
        }

        return TRUE;
}

static void
ckd_player_set_property (GObject *o, guint prop, const GValue *v, GParamSpec *p)
{
        CkdPlayer *self = CKD_PLAYER (o);
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        switch (prop) {
        case PROP_CKD_PLAYER_META_SLIDES:
                priv->meta_slides = g_value_get_pointer (v);
                break;
        case PROP_CKD_PLAYER_VIEW:
                priv->view = g_value_get_pointer (v);
                clutter_actor_set_reactive (CLUTTER_ACTOR(priv->view), TRUE);
                g_signal_connect (priv->view, "button-press-event",
                                  G_CALLBACK (ckd_player_button_press), self);
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
        case PROP_CKD_PLAYER_VIEW:
                g_value_set_pointer (v, priv->view);
                break;
        case PROP_CKD_PLAYER_META_SLIDES:
                g_value_set_pointer (v, priv->meta_slides);
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

        if (priv->meta_slides) {
                g_object_unref (priv->meta_slides);
                priv->meta_slides = NULL;
        }
        if (priv->view) {
                clutter_actor_destroy (CLUTTER_ACTOR(priv->view));
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
        base_class->get_property = ckd_player_get_property;

        GParamSpec *props[N_CKD_PLAYER_PROPS] = {NULL,};
        props[PROP_CKD_PLAYER_VIEW] =
                g_param_spec_pointer ("view",
                                      "View",
                                      "View",
                                      G_PARAM_WRITABLE
                                      | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_PLAYER_META_SLIDES] =
                g_param_spec_pointer ("meta-slides",
                                      "Meta Slides",
                                      "Meta Slides",
                                      G_PARAM_WRITABLE
                                      | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_properties (base_class,
                                           N_CKD_PLAYER_PROPS,
                                           props);
}

static
void ckd_player_init (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);

        priv->meta_slides = NULL;
        priv->view = NULL;


        priv->i = 0;
}

void
ckd_player_forward (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        ClutterActor *slide = NULL;
        CkdMetaEntry *e0 = NULL;
        CkdMetaEntry *e1 = NULL;

        slide = ckd_meta_slides_get_slide (priv->meta_slides, priv->i + 1);

        if (slide) {
                e0 = ckd_meta_slides_get_meta_entry (priv->meta_slides, priv->i);
                e1 = ckd_meta_slides_get_meta_entry (priv->meta_slides, priv->i + 1);

                priv->i++;
                g_object_set (priv->view,
                              "slide-out-effect",
                              e0->exit, NULL);
                g_object_set (priv->view,
                              "slide-in-effect",
                              e1->enter, NULL);

                ckd_view_transit_slide (priv->view, slide, priv->i);
        }
}

void
ckd_player_rewind (CkdPlayer *self)
{
        CkdPlayerPriv *priv = CKD_PLAYER_GET_PRIVATE (self);
        ClutterActor *slide = NULL;

        CkdMetaEntry *e0 = NULL;
        CkdMetaEntry *e1 = NULL;

        e0 = ckd_meta_slides_get_meta_entry (priv->meta_slides, priv->i);
        e1 = ckd_meta_slides_get_meta_entry (priv->meta_slides, priv->i - 1);

        slide = ckd_meta_slides_get_slide (priv->meta_slides, priv->i - 1);
        if (slide) {
                priv->i--;
                g_object_set (priv->view, "slide-out-effect", e0->exit, NULL);
                g_object_set (priv->view, "slide-in-effect", e1->enter, NULL);
                ckd_view_transit_slide (priv->view, slide, priv->i);
        }
}
