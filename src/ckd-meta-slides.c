#include <poppler.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include "ckd-meta-slides.h"
#include "ckd-slide.h"

G_DEFINE_TYPE (CkdMetaSlides, ckd_meta_slides, G_TYPE_OBJECT);

#define CKD_META_SLIDES_GET_PRIVATE(o) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                     CKD_TYPE_META_SLIDES, \
                                     CkdMetaSlidesPriv))

typedef struct _CkdMetaSlidesPriv CkdMetaSlidesPriv;
struct _CkdMetaSlidesPriv {
        GFile *source;
        gchar *checksum;
        PopplerDocument *pdf_doc;
        CkdMetaSlidesCacheMode cache_mode;
        gchar *tmp_dir;
        GArray *cache;
        gdouble scale;
        gint n_of_slides;

        /* 在硬盘缓存模式中用于记录缓存线程的进度 */
        gint next_cached_slide_id;
        /* 配置 */
        GNode *script;
};

enum {
        PROP_CKD_META_SLIDES_0,
        PROP_CKD_META_SLIDES_SOURCE,
        PROP_CKD_META_SLIDES_PDF_DOC,
        PROP_CKD_META_SLIDES_CACHE_MODE,
        PROP_CKD_META_SLIDES_CACHE,
        PROP_CKD_META_SLIDES_SCALE,
        N_CKD_META_SLIDES_PROPS
};

static void
ckd_meta_slides_set_property (GObject *obj,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
        CkdMetaSlides *self = CKD_META_SLIDES (obj);
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);

        switch (property_id) {
        case PROP_CKD_META_SLIDES_SOURCE:
                priv->source = g_value_get_pointer (value);
                /* 获取 PDF 文件的 md5 校验码 */
                {
                        gsize length;
                        gchar *contents;
                        g_file_load_contents (priv->source,
                                              NULL,
                                              &contents,
                                              &length,
                                              NULL,
                                              NULL);
                        priv->checksum =
                                g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                                                      contents,
                                                                      length);
                        g_free (contents);
                }
                /* 读取配置文件 */
                {
                        gchar *path = g_file_get_path (priv->source);
                        gchar **splitted = g_strsplit (path, ".", 0);
                        GString *script_path = g_string_new (splitted[0]);
                        g_string_append (script_path, ".ckd");
                        GFile *script_file = g_file_new_for_path (script_path->str);
                        if (g_file_query_exists (script_file, NULL))
                                priv->script = ckd_slide_script_new (script_path->str);
                        g_strfreev (splitted);
                        g_string_free (script_path, TRUE);
                        g_free (path);
                }
                break;
        case PROP_CKD_META_SLIDES_PDF_DOC:
                priv->pdf_doc = g_value_get_pointer (value);
                priv->n_of_slides =
                        poppler_document_get_n_pages (priv->pdf_doc);
                /* 初始化 cache */
                {
                        CkdMetaEntry *meta_entry;
                        priv->cache = g_array_new (FALSE,
                                                   FALSE,
                                                   sizeof(CkdMetaEntry *));
                        for (gint i = 0; i < priv->n_of_slides; i++) {
                                meta_entry = g_slice_alloc (sizeof(CkdMetaEntry));
                                meta_entry->slide = NULL;
                                meta_entry->duration = 60;
                                meta_entry->enter = CKD_SLIDE_FADE_ENTER;
                                meta_entry->exit = CKD_SLIDE_FADE_EXIT;
                                meta_entry->text = NULL;
                                g_array_append_val (priv->cache, meta_entry);
                        }
                }
                break;
        case PROP_CKD_META_SLIDES_CACHE_MODE:
                priv->cache_mode = g_value_get_int (value);
                if (priv->cache_mode == CKD_META_SLIDES_DISK_CACHE) {
                        priv->tmp_dir = g_strdup_printf ("%s/cikada",
                                                         g_get_tmp_dir ());
                }
                break;
        case PROP_CKD_META_SLIDES_SCALE:
                priv->scale = g_value_get_double (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
                break;
        }
}

static void
ckd_meta_slides_get_property (GObject *obj,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
        CkdMetaSlides *self = CKD_META_SLIDES (obj);
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);

        switch (property_id) {
        case PROP_CKD_META_SLIDES_PDF_DOC:
                g_value_set_pointer (value, priv->pdf_doc);
                break;
        case PROP_CKD_META_SLIDES_CACHE_MODE:
                g_value_set_int (value, priv->cache_mode);
                break;
        case PROP_CKD_META_SLIDES_CACHE:
                g_value_set_pointer (value, priv->cache);
                break;
        case PROP_CKD_META_SLIDES_SCALE:
                g_value_set_double (value, priv->scale);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
                break;
        }
}

/* GObject 析构函数 */
static void
ckd_meta_slides_dispose (GObject *obj)
{
        CkdMetaSlides *self     = CKD_META_SLIDES (obj);
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);
        CkdMetaEntry *meta_entry;

        if (priv->source) {
                g_object_unref (priv->source);
                priv->source = NULL;
        }
        if (priv->checksum) {
                g_free (priv->checksum);
                priv->checksum = NULL;
        }
        if (priv->tmp_dir) {
                g_free (priv->tmp_dir);
                priv->tmp_dir = NULL;
        }
        if (priv->pdf_doc) {
                g_object_unref (priv->pdf_doc);
                priv->pdf_doc = NULL;
        }
        if (priv->cache) {
                for (gint i = 0; i < priv->cache->len; i++) {
                        meta_entry = g_array_index (priv->cache,
                                                    CkdMetaEntry *,
                                                    i);
                        g_object_unref (meta_entry->slide);
                        g_slice_free (CkdMetaEntry, meta_entry);
                }
                g_array_free (priv->cache, FALSE);
                priv->cache = NULL;
        }

        if (priv->script) {
                ckd_slide_script_free (priv->script);
                priv->script = NULL;
        }

        G_OBJECT_CLASS (ckd_meta_slides_parent_class)->dispose (obj);
}

static void
ckd_meta_slides_finalize (GObject *obj)
{
        G_OBJECT_CLASS (ckd_meta_slides_parent_class)->finalize (obj);
}

static void
ckd_meta_slides_class_init (CkdMetaSlidesClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdMetaSlidesPriv));

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->dispose      = ckd_meta_slides_dispose;
        base_class->finalize     = ckd_meta_slides_finalize;
        base_class->set_property = ckd_meta_slides_set_property;
        base_class->get_property = ckd_meta_slides_get_property;

        GParamSpec *props[N_CKD_META_SLIDES_PROPS] = {NULL,};
        props[PROP_CKD_META_SLIDES_SOURCE] =
                g_param_spec_pointer ("source", "Source", "PDF Source File",
                                      G_PARAM_WRITABLE
                                      | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_META_SLIDES_PDF_DOC] =
                g_param_spec_pointer ("pdf-doc", "PDF Document", "PDF Document",
                                      G_PARAM_WRITABLE
                                      | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_META_SLIDES_CACHE_MODE] =
                g_param_spec_int ("cache-mode", "Cache Mode", "Cache Mode",
                                  G_MININT,
                                  G_MAXINT,
                                  CKD_META_SLIDES_DISK_CACHE,
                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CKD_META_SLIDES_CACHE] =
                g_param_spec_pointer ("meta-data",
                                      "Meta Data",
                                      "Meta Data",
                                      G_PARAM_READABLE);
        props[PROP_CKD_META_SLIDES_SCALE] =
                g_param_spec_double ("scale", "Scale", "Scale",
                                     CKD_META_SLIDES_QUALITY_MIN,
                                     CKD_META_SLIDES_QUALITY_MAX,
                                     1.0,
                                     G_PARAM_READWRITE
                                     | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_properties (base_class,
                                           N_CKD_META_SLIDES_PROPS,
                                           props);
}

static void
ckd_meta_slides_init (CkdMetaSlides *self)
{
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);

        priv->source     = NULL;
        priv->checksum   = NULL;
        priv->pdf_doc    = NULL;
        priv->cache_mode = 0;
        priv->tmp_dir    = NULL;
        priv->cache      = NULL;
        priv->scale      = 1.0;
        priv->n_of_slides = 0;
        priv->next_cached_slide_id = 0;
        priv->script = NULL;
}

static gboolean
ckd_meta_slides_image_should_be_recreated (GFile *image, gdouble w, gdouble h)
{
        gboolean ret = FALSE;
        gchar *filename = g_file_get_path (image);

        if (g_file_query_exists (image, NULL)) {
                cairo_surface_t *cs =
                        cairo_image_surface_create_from_png (filename);
                gint iw = cairo_image_surface_get_width (cs);
                gint ih = cairo_image_surface_get_height (cs);
                cairo_surface_destroy (cs);

                if ((gint) w != iw || (gint) h != ih) {
                        if (!g_file_delete (image, NULL, NULL)) {
                                g_error ("I can not delete %s\n", filename);
                        }
                        ret = TRUE;
                }
        } else {
                ret = TRUE;
        }

        g_free (filename);

        return ret;
}

static void
ckd_meta_slides_disk_cache_init (CkdMetaSlides *self)
{
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);

        gchar *meta_slides_path =
                g_strdup_printf ("%s/%s", priv->tmp_dir, priv->checksum);
        GFile *meta_slides_dir  = g_file_new_for_path (meta_slides_path);
        GError *error = NULL;

        gboolean is_ok = g_file_make_directory_with_parents (meta_slides_dir,
                                                             NULL,
                                                             &error);
        if (!is_ok && error == NULL && error->code != G_IO_ERROR_EXISTS)
                g_error ("I can not create %s\n", meta_slides_path);
        g_object_unref (meta_slides_dir);
        if (error) {
                g_error_free (error);
        }

        CkdMetaEntry *entry;
        gchar *image_path = NULL;
        for (gint i = 0; i < priv->n_of_slides; i++) {
                entry = g_array_index (priv->cache, CkdMetaEntry *, i);
                image_path = g_strdup_printf ("%s/%d.png", meta_slides_path, i);
                entry->slide = g_file_new_for_path (image_path);
                g_free (image_path);
        }

        g_free (meta_slides_path);
}

static void
ckd_meta_slides_create_disk_cache (CkdMetaSlides *self)
{
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);
        CkdMetaEntry *entry;
        PopplerPage *page;
        gdouble w, h;
        gchar *image_path = NULL;
        cairo_surface_t *cs;
        cairo_t *cr;

        for (gint i = 0; i < priv->n_of_slides; i++) {
                entry = g_array_index (priv->cache, CkdMetaEntry *, i);
                page = poppler_document_get_page (priv->pdf_doc, i);
                poppler_page_get_size (page, &w, &h);
                w *= priv->scale;
                h *= priv->scale;
                if (ckd_meta_slides_image_should_be_recreated (entry->slide,
                                                               w,
                                                               h)) {
                        cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         (gint)w,
                                                         (gint)h);
                        cr = cairo_create (cs);
                        cairo_scale (cr, priv->scale, priv->scale);

                        /* @begin: 绘制白色背景，因为有些 PDF 页面是透
                         * 背景的 */
                        cairo_rectangle (cr, 0, 0, w, h);
                        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
                        cairo_fill (cr);
                        /* @end */

                        poppler_page_render (page, cr);

                        image_path = g_file_get_path (entry->slide);
                        cairo_surface_write_to_png (cs, image_path);
                        g_free (image_path);

                        cairo_surface_destroy (cs);
                        cairo_destroy (cr);
                }
                g_object_unref (page);
                priv->next_cached_slide_id ++;
        }
}

static gpointer
ckd_meta_slides_cache_thread (gpointer data)
{
        CkdMetaSlides *self = data;
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);
        if (!priv->pdf_doc)
                g_error ("I think you should give me a real pdf document!");

        GTimer *t = g_timer_new ();
        g_timer_start (t);
        ckd_meta_slides_create_disk_cache (self);
        g_timer_stop (t);
        g_print ("硬盘缓冲时间：%f 秒\n", g_timer_elapsed (t, NULL));
        g_timer_destroy (t);
}

void
ckd_meta_slides_create_cache (CkdMetaSlides *self)
{
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);
        CkdMetaEntry *entry;

        if (priv->script) {
                /* 幻灯片备注还未实现 */
                GList *list = ckd_slide_script_out_meta_entry_list (priv->script,
                                                                    priv->n_of_slides);
                gint i = 0;
                CkdMetaEntry *e1, *e2;
                GList *iter;
                for (iter = g_list_first (list);
                     iter != NULL;
                     iter = g_list_next (iter), i++) {
                        e1 = iter->data;
                        e2 = g_array_index (priv->cache, CkdMetaEntry *, i);
                        if (e1->duration > 0)
                                e2->duration = e1->duration;
                        if (e1->enter != CKD_SLIDE_NULL_ENTER)
                                e2->enter = e1->enter;
                        if (e1->exit != CKD_SLIDE_NULL_EXIT)
                                e2->exit = e1->exit;
                }
        }

        if (priv->cache_mode == CKD_META_SLIDES_MEM_CACHE) {
                for (gint i = 0; i < priv->n_of_slides; i++) {
                        entry = g_array_index (priv->cache, CkdMetaEntry *, i);
                        entry->slide =
                                poppler_document_get_page (priv->pdf_doc, i);
                }
        } else if (priv->cache_mode == CKD_META_SLIDES_DISK_CACHE) {
                ckd_meta_slides_disk_cache_init (self);
                GThread *thread = g_thread_new ("cache_thread",
                                                ckd_meta_slides_cache_thread,
                                                self);
                g_thread_unref (thread);
        }
}

ClutterActor *
ckd_meta_slides_get_slide (CkdMetaSlides *self, gint i)
{
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);
        CkdMetaEntry *entry = NULL;
        gpointer meta_slide = NULL;
        ClutterActor *slide = NULL;

        /* 页码约束 [0, (n_ofpages - 1)] */
        if (i < 0 || i >= priv->n_of_slides)
                return NULL;

        entry = g_array_index (priv->cache, CkdMetaEntry *, i);

        if (priv->cache_mode == CKD_META_SLIDES_MEM_CACHE) {
                meta_slide = entry->slide;
                slide = ckd_slide_new_for_poppler_page (meta_slide,
                                                        priv->scale);
        } else if (priv->cache_mode == CKD_META_SLIDES_DISK_CACHE) {
                /* 如果缓存文件尚未生成，就等那么一会 */
                while (i >= priv->next_cached_slide_id) {
                        g_usleep (0.01 *  G_USEC_PER_SEC);
                }
                meta_slide = entry->slide;
                slide = ckd_slide_new_for_image (meta_slide);
        }

        return slide;
}

CkdMetaEntry *
ckd_meta_slides_get_meta_entry (CkdMetaSlides *self, gint i)
{
        CkdMetaSlidesPriv *priv = CKD_META_SLIDES_GET_PRIVATE (self);
        CkdMetaEntry *entry = NULL;

        /* 页码约束 [0, (n_ofpages - 1)] */
        if (i < 0 || i >= priv->n_of_slides)
                return NULL;

        return g_array_index (priv->cache, CkdMetaEntry *, i);
}
