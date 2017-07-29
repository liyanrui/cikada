#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "ckd-slide.h"

G_DEFINE_TYPE(CkdSlide, ckd_slide, CLUTTER_TYPE_ACTOR);

#define CKD_SLIDE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_SLIDE, CkdSlidePriv))

typedef struct _CkdSlidePriv {
        ClutterActor *content;
} CkdSlidePriv;

static void
ckd_slide_destroy (ClutterActor *self)
{
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);
        if (priv->content) {
                clutter_actor_destroy (priv->content);
                priv->content = NULL;
        }
        if (CLUTTER_ACTOR_CLASS (ckd_slide_parent_class)->destroy)
                CLUTTER_ACTOR_CLASS (ckd_slide_parent_class)->destroy (self);
}

static void
ckd_slide_get_preferred_width (ClutterActor *a, gfloat for_h, gfloat *min_w, gfloat *natural_w)
{
        CkdSlide *self = CKD_SLIDE (a);
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);

        clutter_actor_get_preferred_width (priv->content, for_h, min_w, natural_w);
}

static void
ckd_slide_get_preferred_height (ClutterActor *a, gfloat for_w, gfloat *min_h, gfloat *natural_h)
{
        CkdSlide *self = CKD_SLIDE (a);
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);

        clutter_actor_get_preferred_height (priv->content, for_w, min_h, natural_h);
}

static void
ckd_slide_allocate (ClutterActor *actor, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
        CkdSlide *self = CKD_SLIDE (actor);
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);
       
        CLUTTER_ACTOR_CLASS (ckd_slide_parent_class)->allocate (actor, box, flags);
 
        gfloat w = clutter_actor_box_get_width (box);
        gfloat h = clutter_actor_box_get_height (box);
        
        ClutterActorBox content_box = { 0, 0, w, h };
        clutter_actor_allocate (priv->content, &content_box, flags);
}

static void
ckd_slide_paint (ClutterActor *actor)
{
        CkdSlide *self = CKD_SLIDE (actor);
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);
        clutter_actor_paint (priv->content);
}

static void
ckd_slide_init (CkdSlide *self)
{
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);

        priv->content = NULL;
}

static void
ckd_slide_class_init (CkdSlideClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdSlidePriv));
        
        ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
        actor_class->destroy = ckd_slide_destroy;
        actor_class->paint = ckd_slide_paint;
        actor_class->get_preferred_height = ckd_slide_get_preferred_height;
        actor_class->get_preferred_width = ckd_slide_get_preferred_width;
        actor_class->allocate = ckd_slide_allocate;
}

ClutterActor *
ckd_slide_new_for_image (GFile *file)
{
        CkdSlide *self = g_object_new (CKD_TYPE_SLIDE, NULL);
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);

        GdkPixbuf *pixbuf;
        ClutterContent *image;

        /* 载入图像数据 */
        gchar *image_path = g_file_get_path (file);
        pixbuf = gdk_pixbuf_new_from_file (image_path, NULL);

        /* 获取图像尺寸 */
        gfloat w = gdk_pixbuf_get_width (pixbuf);
        gfloat h = gdk_pixbuf_get_height (pixbuf);
        
        image = clutter_image_new ();
        clutter_image_set_data (CLUTTER_IMAGE (image),
                                gdk_pixbuf_get_pixels (pixbuf),
                                gdk_pixbuf_get_has_alpha (pixbuf)
                                ? COGL_PIXEL_FORMAT_RGBA_8888
                                : COGL_PIXEL_FORMAT_RGB_888,
                                (guint)w,
                                (guint)h,
                                gdk_pixbuf_get_rowstride (pixbuf),
                                NULL);
        g_object_unref (pixbuf);
        g_free (image_path);
                
        priv->content = clutter_actor_new ();

        clutter_actor_set_size (priv->content, w, h);
        clutter_actor_set_content_scaling_filters (priv->content,
                                                   CLUTTER_SCALING_FILTER_TRILINEAR,
                                                   CLUTTER_SCALING_FILTER_LINEAR);
        clutter_actor_set_content (priv->content, image);
        g_object_unref (image);
  

        clutter_actor_add_child (CLUTTER_ACTOR(self), priv->content);
        
        return (ClutterActor *)self;
}

struct CkdDrawData {
        PopplerPage *page;
        gdouble scale;
};

static gboolean
draw_page (ClutterCanvas *canvas, cairo_t *cr, int width, int height, gpointer data)
{
        struct CkdDrawData *draw_data = data;
        
        cairo_save (cr);
        cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint (cr);
        cairo_restore (cr);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

        /* @begin: 绘制白色背景 */
        cairo_rectangle (cr, 0, 0, width, height);
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
        cairo_fill (cr);
        /* @end */

        cairo_scale (cr, draw_data->scale, draw_data->scale);
        poppler_page_render (POPPLER_PAGE(draw_data->page), cr);
        return TRUE;
}

ClutterActor *
ckd_slide_new_for_poppler_page (PopplerPage *page, gdouble scale)
{
        CkdSlide *self = g_object_new (CKD_TYPE_SLIDE, NULL);
        CkdSlidePriv *priv = CKD_SLIDE_GET_PRIVATE (self);
        ClutterContent *canvas;
        gdouble w, h;
        
        priv->content = clutter_actor_new ();
        canvas = clutter_canvas_new ();

        poppler_page_get_size (page, &w, &h);
        
        clutter_canvas_set_size (CLUTTER_CANVAS(canvas), w * scale, h * scale);
        clutter_actor_set_content (priv->content, canvas);
        clutter_actor_set_content_scaling_filters (priv->content,
                                                   CLUTTER_SCALING_FILTER_TRILINEAR,
                                                   CLUTTER_SCALING_FILTER_LINEAR);
        g_object_unref (canvas);

        clutter_actor_set_size (priv->content, w * scale, h * scale);
        clutter_actor_add_child (CLUTTER_ACTOR(self), priv->content);

        struct CkdDrawData data = {page, scale};
        g_signal_connect (canvas, "draw", G_CALLBACK (draw_page), &data);
        
        clutter_content_invalidate (canvas);

        
        return (ClutterActor *)self;
}
