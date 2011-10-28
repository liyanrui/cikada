#include <math.h>
#include "ckd-page.h"

G_DEFINE_TYPE (CkdPage, ckd_page, CLUTTER_TYPE_ACTOR);

#define DEFAULT_PAGE_QUALITY 1.0

/*
  如果 PDF 页面分辨率比较小，那么在将其转化为 Cairo 纹理之前，
  缩放倍数可以设的较大一些，保证 Cairo 纹理清晰
*/
#define LOW_RESOLUTION_H 640
#define LOW_RESOLUTION_V 480
#define LOW_RESOLUTION_FACTOR 3.0

/*
  如果 PDF 页面分辨率比较小，那么在将其转化为 Cairo 纹理之前，
  缩放倍数可以设的较小一些，保证 Cairo 纹理渲染效率
*/
#define MEDIUM_RESOLUTION_H 800
#define MEDIUM_RESOLUTION_V 600
#define MEDIUM_RESOLUTION_FACTOR 1.5

#define CKD_PAGE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_PAGE, CkdPagePriv))

typedef struct _CkdPagePriv {
        PopplerPage *pdf_page;
        gfloat width;
        gfloat height;
        gfloat quality;
        gfloat border;
        gdouble pdf_width;
        gdouble pdf_height;
        ClutterActor *cairo_texture;
} CkdPagePriv;

enum {
        PROP_0,
        PROP_PDF_PAGE,
        PROP_QUALITY,
        PROP_SURFACE_WIDTH,
        PROP_SURFACE_HEIGHT,
        PROP_BORDER
};

static void _ckd_page_fit_size (CkdPage *self, gfloat *width, gfloat *height);

static gboolean
ckd_page_render (ClutterCairoTexture *actor, cairo_t *cr, gpointer user_data)
{
        CkdPage *self = CKD_PAGE (user_data);
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);
        
        guint w, h;
        ClutterCairoTexture *ct = CLUTTER_CAIRO_TEXTURE(priv->cairo_texture);
        clutter_cairo_texture_get_surface_size (ct, &w, &h);

        clutter_cairo_texture_clear (actor);
        cairo_rectangle (cr, 0, 0, w, h);
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
        cairo_fill (cr);
        cairo_scale (cr, priv->quality, priv->quality);
        poppler_page_render (priv->pdf_page, cr);
        
        return TRUE;
}

static void
ckd_page_paint (ClutterActor *actor)
{
        CkdPage *self = CKD_PAGE (actor);
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);
        clutter_actor_paint (priv->cairo_texture);
}

static void
ckd_page_set_property (GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec)
{
        CkdPage *self = CKD_PAGE (obj);
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);
        gdouble w, h;
        
        switch (prop_id) {
        case PROP_PDF_PAGE:
                priv->pdf_page = g_value_get_pointer (value);
                if (!priv->pdf_page)
                        break;
                if (priv->cairo_texture)
                        clutter_actor_destroy (priv->cairo_texture);
                poppler_page_get_size (priv->pdf_page, &priv->pdf_width, &priv->pdf_height);
                priv->cairo_texture = clutter_cairo_texture_new (priv->quality * w, priv->quality * h);
                clutter_actor_set_parent (priv->cairo_texture, CLUTTER_ACTOR (self));
                g_signal_connect (priv->cairo_texture, "draw", G_CALLBACK (ckd_page_render), self);
                break;
        case PROP_QUALITY:
                priv->quality = g_value_get_float (value);
                poppler_page_get_size (priv->pdf_page, &w, &h);
                clutter_cairo_texture_set_surface_size (CLUTTER_CAIRO_TEXTURE(priv->cairo_texture),
                                                        priv->quality * w, priv->quality * h);
                break;
        case PROP_BORDER:
                priv->border = g_value_get_float (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
                break;
        }
}

static void
ckd_page_get_property (GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec)
{
        CkdPage *self = CKD_PAGE (obj);
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);

        switch (prop_id) {
        case PROP_PDF_PAGE:
                g_value_set_pointer (value, priv->pdf_page);
                break;
        case PROP_QUALITY:
                g_value_set_float (value, priv->quality);
                break;
        case PROP_BORDER:
                g_value_set_float (value, priv->border);
                break;
        case PROP_SURFACE_WIDTH:
                g_value_set_float (value, priv->width);
                break;
        case PROP_SURFACE_HEIGHT:
                g_value_set_float (value, priv->height);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
                break;
        }
}

static void
ckd_page_get_preferred_width (ClutterActor *actor,
                              gfloat for_height, gfloat *min_width, gfloat *natural_width)
{
        CkdPage *self = CKD_PAGE (actor);
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);

        *min_width = 0.0;
        *natural_width = priv->width;
}

static void
ckd_page_get_preferred_height (ClutterActor *actor,
                               gfloat for_width, gfloat *min_height, gfloat *natural_height)
{
        CkdPage *self = CKD_PAGE (actor);
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);

        *min_height = 0.0;
        *natural_height = priv->height;
}

static void
ckd_page_destroy (ClutterActor *self)
{
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);

        if (priv->cairo_texture) {
                clutter_actor_destroy (priv->cairo_texture);
                priv->cairo_texture = NULL;
        }

        if (CLUTTER_ACTOR_CLASS (ckd_page_parent_class)->destroy) {
                CLUTTER_ACTOR_CLASS (ckd_page_parent_class)->destroy (self);
        }
}

static void
_ckd_page_fit_size (CkdPage *self, gfloat *width, gfloat *height)
{
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);
        
        gdouble w, h, a;
        gdouble pdf_w, pdf_h, pdf_a;

        w = *width;
        h = *height;

        if (h < G_MINFLOAT)
                h = G_MINFLOAT;
        a = w / h;
        
        poppler_page_get_size (priv->pdf_page, &pdf_w, &pdf_h);
        
        if (pdf_h < G_MINFLOAT)
                pdf_h = G_MINFLOAT;
        
        pdf_a = pdf_w / pdf_h;
        
        if (pdf_a < G_MINFLOAT)
                pdf_a = G_MINFLOAT;
        
        if (a >= pdf_a)
                w = h * pdf_a;
        else
                h = w / pdf_a;
        
        *width = w;
        *height = h;
}

static void
ckd_page_allocate (ClutterActor *actor, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
        CkdPage *self = CKD_PAGE (actor);
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);

        CLUTTER_ACTOR_CLASS (ckd_page_parent_class)->allocate (actor, box, flags);

        gfloat w, h, a;
        w = clutter_actor_box_get_width (box);
        h = clutter_actor_box_get_height (box);

        /* 调整页面空间尺寸 */
        _ckd_page_fit_size (self, &w, &h);
        priv->width = w;
        priv->height = h;
        
        gfloat dx, dy, b;
        dx = (box->x1 + box->x2 - w) / 2.0;
        dy = (box->y1 + box->y2 - h) / 2.0;
        b  = priv->border;

        ClutterActorBox inner_box = {dx + b, dy + b, w + dx - b, h + dy - b};
        clutter_actor_allocate (priv->cairo_texture, &inner_box, flags);
}

static void
ckd_page_class_init (CkdPageClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdPagePriv));

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->set_property = ckd_page_set_property;
        base_class->get_property = ckd_page_get_property;

        GParamSpec *pspec;
        pspec = g_param_spec_pointer ("pdf-page",
                                      "PDF Page",
                                      "A PDF Page which to be convert to CairoTexture",
                                      G_PARAM_READWRITE);
        g_object_class_install_property (base_class, PROP_PDF_PAGE, pspec);

        pspec = g_param_spec_float ("quality",
                                    "Page Quality",
                                    "Page Quality",
                                    0.1,
                                    4.0,
                                    DEFAULT_PAGE_QUALITY,
                                    G_PARAM_READWRITE);
        g_object_class_install_property (base_class, PROP_QUALITY, pspec);
        pspec = g_param_spec_float ("surface-width",
                                    "Cairo Surface Width",
                                    "Cairo Surface Width",
                                    0.0,
                                    G_MAXFLOAT,
                                    0.0,
                                    G_PARAM_READWRITE);
        g_object_class_install_property (base_class, PROP_SURFACE_WIDTH, pspec);
        pspec = g_param_spec_float ("surface-height",
                                    "Cairo Surface Height",
                                    "Cairo Surface Height",
                                    0.0,
                                    G_MAXFLOAT,
                                    0.0,
                                    G_PARAM_READWRITE);
        g_object_class_install_property (base_class, PROP_SURFACE_HEIGHT, pspec);
        pspec = g_param_spec_float ("border",
                                    "Page Border",
                                    "Page border size which around page",
                                    0.0,
                                    100,
                                    10.0,
                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
        g_object_class_install_property (base_class, PROP_BORDER, pspec);
        
        ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
        actor_class->destroy = ckd_page_destroy;
        actor_class->paint = ckd_page_paint;
        actor_class->get_preferred_height = ckd_page_get_preferred_height;
        actor_class->get_preferred_width = ckd_page_get_preferred_width;
        actor_class->allocate = ckd_page_allocate;
}

static void
ckd_page_init (CkdPage *self)
{
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (self);
        
        priv->quality = DEFAULT_PAGE_QUALITY;
        priv->cairo_texture = NULL;
        priv->pdf_page = NULL;
}

ClutterActor *
ckd_page_new_with_default_quality (PopplerPage *page)
{
        ClutterActor *ckd_page;
        CkdPagePriv *priv;
        gfloat level = DEFAULT_PAGE_QUALITY;
        
        ckd_page = g_object_new (CKD_TYPE_PAGE, "pdf-page", page, NULL);
        priv = CKD_PAGE_GET_PRIVATE (CKD_PAGE(ckd_page));

        if (priv->pdf_width <= LOW_RESOLUTION_H || priv->pdf_height <= LOW_RESOLUTION_V)
                level = LOW_RESOLUTION_FACTOR * DEFAULT_PAGE_QUALITY;
        if ((priv->pdf_width > LOW_RESOLUTION_H && priv->pdf_width <= MEDIUM_RESOLUTION_H)
            || (priv->pdf_height > LOW_RESOLUTION_V && priv->pdf_height <= MEDIUM_RESOLUTION_V))
                level = MEDIUM_RESOLUTION_FACTOR * DEFAULT_PAGE_QUALITY;

        ckd_page_set_quality (CKD_PAGE(ckd_page), level);
        
        return ckd_page;
}

ClutterActor *
ckd_page_new (PopplerPage *page)
{
        ClutterActor *ckd_page;
        
        ckd_page = g_object_new (CKD_TYPE_PAGE, "pdf-page", page, NULL);
        
        return ckd_page;        
}

void
ckd_page_set_border (CkdPage *page, gfloat border)
{
        g_object_set (G_OBJECT(page), "border", border, NULL);
}

/* 仅在在设置图像质量时，会进行页面渲染 */
void
ckd_page_set_quality (CkdPage *page, gfloat quality)
{
        CkdPagePriv *priv = CKD_PAGE_GET_PRIVATE (page);
        
        g_object_set (G_OBJECT(page), "quality", quality, NULL);
        clutter_cairo_texture_invalidate (CLUTTER_CAIRO_TEXTURE(priv->cairo_texture));
}
