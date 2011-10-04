#include "ckd-page-manager.h"

/* 页面队列默认容量 */
#define CKD_PAGE_MANAGER_CAPACITY 3

G_DEFINE_TYPE (CkdPageManager, ckd_page_manager, G_TYPE_OBJECT);

#define CKD_PAGE_MANAGER_GET_PRIVATE(obj) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_PAGE_MANAGER, CkdPageManagerPriv))

typedef struct _CkdPageManagerPriv CkdPageManagerPriv;
struct  _CkdPageManagerPriv {
        PopplerDocument *doc;
        GString *doc_uri;
        gint capacity;
        GQueue  *pages;
        GQueue *cache;
        GList  *thumbs;
        gint number_of_pages;
        gint head_page_number;
        gint current_page_number;
        gfloat page_width;
        gfloat page_height;
};

enum {
        PROP_CPM_0,
        PROP_CPM_DOC_URI,
        PROP_CPM_CAPACITY,
        PROP_CPM_DOC,
        PROP_CPM_THUMBS,
        PROP_CPM_NUMBER_OF_PAGES,
        PROP_CPM_HEAD_PAGE_NUMBER,
        PROP_CPM_CURRENT_PAGE_NUMBER,
        PROP_CPM_PAGE_WIDTH,
        PROP_CPM_PAGE_HEIGHT,
        N_PROPS
};

static gchar *           _ckd_page_manager_get_uri_from_path (const gchar * path);
static void              _ckd_page_manager_set_capacity (CkdPageManager *pm, guint capacity);
static ClutterActor *    _ckd_page_manager_create_page (CkdPageManager *pm, gint i);
static void              _ckd_page_destroy_in_pages (gpointer data, gpointer user_data);
static void              _ckd_page_is_in_cache (gpointer data, gpointer user_data);
static void              _ckd_page_destroy_in_cache (gpointer data, gpointer user_data);
static void              _ckd_page_manager_refresh_page_size (gpointer data, gpointer user_data);
static void              _ckd_page_manager_goto (CkdPageManager *pm, gint i);

/* GObject 属性设置函数 */
static void ckd_page_manager_set_property (GObject *obj,
                                           guint property_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void ckd_page_manager_get_property (GObject *obj,
                                           guint property_id,
                                           GValue *value,
                                           GParamSpec *pspec);
static gchar *
_ckd_page_manager_get_uri_from_path (const gchar * path)
{
        gchar *pdf_file_uri = NULL;
        gchar *rel_path = NULL;
        gchar *abs_path = NULL;
        gchar *home_path = NULL;

        if (g_path_is_absolute (path)) {
                pdf_file_uri = g_filename_to_uri (path, NULL, NULL);
        } else {
                rel_path = g_get_current_dir ();
                abs_path = g_strdup_printf ("%s/%s", rel_path, path);
                pdf_file_uri = g_filename_to_uri (abs_path, NULL, NULL);
                g_free (abs_path);
                g_free (rel_path);
        }

        return pdf_file_uri;
}

static void
_ckd_page_manager_set_capacity (CkdPageManager *self, guint capacity)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        PopplerPage *pdf_page;
        ClutterActor *page;
        gint i, d, tail_page_number, left_rest, right_rest;

        if (capacity > priv->number_of_pages)
                capacity = priv->number_of_pages;

        d = capacity - priv->capacity;
        if (d == 0)
                return;
                
        tail_page_number = priv->head_page_number + priv->capacity -1;

        if (d > 0) {
                left_rest =  (priv->head_page_number < d / 2) ? priv->head_page_number : d / 2;
                right_rest = d - left_rest;
                
                for (i = 1; i <= left_rest; i++) {
                        page = _ckd_page_manager_create_page (self, priv->head_page_number - i);
                        g_queue_push_head (priv->pages, page);                                
                }
                
                for (i = 1; i <= right_rest; i++) {
                        page = _ckd_page_manager_create_page (self, tail_page_number + i);
                        g_queue_push_tail (priv->pages, page);
                }
                
        } else {
                left_rest = (priv->current_page_number + d / 2
                             < priv->head_page_number) ? (priv->head_page_number - priv->current_page_number) : d / 2;
                right_rest = d - left_rest;
                
                for (i = left_reset; i < 0; i++) {
                        page = g_queue_pop_head (priv->pages);
                        /* 被缓冲的页面不释放 */
                        if (g_queue_index (priv->cache, page) < 0)
                                clutter_actor_destroy (page);
                }
                for (i = right_rest; i < 0; i++) {
                        page = g_queue_pop_tail (priv->pages);
                        /* 被缓冲的页面不释放 */
                        if (g_queue_index (priv->cache, page) < 0)
                                clutter_actor_destroy (page);
                }
        }
        
        priv->head_page_number -= left_rest;
        priv->capacity = capacity;
}

static void
ckd_page_manager_set_property (GObject *obj,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
        CkdPageManager *self = CKD_PAGE_MANAGER (obj);
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);

        gint i;
        gchar *uri = NULL;
        gint capacity = 0;
        
        switch (property_id) {
        case PROP_CPM_DOC_PATH:
                uri = _ckd_page_manager_get_uri_from_path (g_value_get_string (value));
                priv->doc_path = g_string_new (uri);
                g_string_free (uri, TRUE);

                priv->doc = poppler_document_new_from_file (priv->uri, NULL, NULL);
                g_assert (priv->doc != NULL);

                /* 确保页面队列长度不会大于 PDF 文档页面数量 */
                priv->number_of_pages = poppler_document_get_n_pages (priv->doc);
                if (priv->capacity > priv->number_of_pages)
                        priv->capacity = priv->number_of_pages;
                
                /* 页面队列初始化 */
                for (i = 0; i < priv->capacity; i++)
                        g_queue_push_tail (self->pages, _ckd_page_manager_create_page (pm, i));
                break;
        case PROP_CPM_CAPACITY:
                _ckd_page_manager_set_capacity (self,g_value_get_int (value));
                break;
        case PROP_CPM_PAGE_WIDTH:
                priv->page_width = g_value_get_float (value);
                break;
        case PROP_CPM_PAGE_HEIGHT:
                priv->page_height = g_value_get_float (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
ckd_page_manager_get_property (GObject *obj,
                               guint property_id,
                               GValue *value,
                               GParamSpec *pspec)
{
        CkdPageManager *self = CKD_PAGE_MANAGER (obj);
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        switch (property_id) {
        case PROP_CPM_DOC:
                g_value_set_pointer (value, priv->doc);
                break;
        case PROP_CPM_PAGE_WIDTH:
                g_value_set_float (value, priv->page_width);
                break;
        case PROP_CPM_PAGE_HEIGHT:
                g_value_set_float (value, priv->page_height);
                break;
        case PROP_CPM_NUMBER_OF_PAGES:
                g_value_set_int (value, priv->number_of_pages);
                break;
        case PROP_CPM_HEAD_PAGE_NUMBER:
                g_value_set_int (value, priv->head_page_number);
                break;
        case PROP_CPM_CAPACITY:
                g_value_set_int (value, priv->capacity);
                break;
        case PROP_CPM_CURRENT_PAGE_NUMBER:
                g_value_set_int (value, priv->current_page_number);
                break;
        case PROP_CPM_THUMBS:
                g_value_set_pointer (value, priv->thumbs);
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }        
}

static void
ckd_page_manager_class_init (CkdPageManagerClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdPageManagerPrivate));

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->set_property = ckd_page_manager_set_property;
        base_class->get_property = ckd_page_manager_get_property;
        
        GParamSpec *props[N_PROPS] = {NULL,};
        props[PROP_CPM_DOC_URI] =
                g_param_spec_string ("document-uri", "Document URI", "PDF document uri",
                                     NULL, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CPM_CAPACITY] =
                g_param_spec_int ("capacity", "Capacity",
                                  "Page manager capacity",
                                  0, G_MAXINT, 0, G_PARAM_READWRITE);
        props[PROP_CPM_DOC] =
                g_param_spec_pointer ("document", "Document", "PDF document object",
                                      G_PARAM_READWRITE);
        props[PROP_CPM_THUMBS] =
                g_param_spec_pointer ("thumbs", "Thumbs", "Thumbs of PDF document",
                                      G_PARAM_READWRITE);
        props[PROP_CPM_NUMBER_OF_PAGES] =
                g_param_spec_int ("number-of-pages", "Number of pages",
                                  "Number of pages in PDF document",
                                  0, G_MAXINT, 0, G_PARAM_READABLE);
        props[PROP_CPM_HEAD_PAGE_NUMBER] =
                g_param_spec_int ("head-page-number", "Head page number",
                                  "Number of head page in queue",
                                  0, G_MAXINT, 0, G_PARAM_READABLE);
        props[PROP_CPM_CURRENT_PAGE_NUMBER] =
                g_param_spec_int ("current-page-number", "Current page number",
                                  "Number of current page in queue",
                                  0, G_MAXINT, 0, G_PARAM_READABLE);
        props[PROP_CPM_PAGE_WIDTH] =
                g_param_spec_float ("page-width", "Page Width", "Page Width",
                                    1.0, G_MAXFLOAT, 1.0, G_PARAM_READWRITE);
        props[PROP_CPM_PAGE_HEIGHT] =
                g_param_spec_float ("page-height", "Page Height", "Page Height",
                                    1.0, G_MAXFLOAT, 1.0, G_PARAM_READWRITE);
        g_object_class_install_properties (base_class, N_PROPS, props);
}
 
static void
ckd_page_manager_init (CkdPageManager *self)
{
        CkdPageManagerPrivate *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);

        priv->pages = g_queue_new ();
        priv->cache = g_queue_new ();
        priv->capcity = CKD_PAGE_MANAGER_CAPACITY;
}





/******************************************/
/* 以下是重构之前的代码 */
/*********************/
static void
_ckd_page_manager_refresh_page_size (gpointer data, gpointer user_data)
{
        ClutterActor *page = data;
        CkdPageManager *pm = user_data;
        
        clutter_actor_set_size (page, pm->page_width, pm->page_height);
}

static ClutterActor *
_ckd_page_manager_create_page (CkdPageManager *pm, gint i)
{
        PopplerPage *pdf_page;
        ClutterActor *page;
        pdf_page = poppler_document_get_page (pm->pdf, i);
        page = ckd_page_new_with_default_quality (pdf_page);
        _ckd_page_manager_refresh_page_size (page, pm);
        
        return page;
}

static void
_ckd_page_destroy_in_pages (gpointer data, gpointer user_data)
{
        ClutterActor *page = data;
        CkdPageManager *pm = user_data;

        if (g_queue_index (pm->cache, page) < 0)
                clutter_actor_destroy (page);
}

CkdPageManager *
ckd_page_manager_new_with_page_size (const gchar *pdf_name, gfloat width, gfloat height)
{
        CkdPageManager *pm  = g_slice_new (CkdPageManager);
        
        pm->pdf             = _ckd_page_manager_open_pdf (pdf_name);
        pm->number_of_pages = poppler_document_get_n_pages (pm->pdf);
        pm->pages           = g_queue_new ();
        pm->cache           = g_queue_new ();
        pm->head_page_number     = 0;
        pm->tail_page_number     = 0;
        pm->current_page_number  = 0;
        pm->page_width      = width;
        pm->page_height     = height;

        g_queue_push_tail (pm->pages, _ckd_page_manager_create_page (pm, 0));
        
        return pm;
}



void
ckd_page_manager_set_page_size (CkdPageManager *pm, gfloat width, gfloat height)
{
        pm->page_width = width;
        pm->page_height = height;
        g_queue_foreach (pm->pages, _ckd_page_manager_refresh_page_size, pm);
}

static void
_ckd_page_manager_goto (CkdPageManager *pm, gint i)
{
        gint queue_len, new_head_page_number, new_tail_page_number;

        /* 新滑块的起点与终点 */
        queue_len = pm->tail_page_number - pm->head_page_number + 1;
        new_head_page_number = i - queue_len / 2;
        new_tail_page_number = new_head_page_number + queue_len - 1;

        ClutterActor *page;
        PopplerPage *pdf_page;
        gint d, k;
        gint n = pm->number_of_pages - 1;
        
        /* 检测尾部是否碰壁，若有则对 head_page_number 与 tail_page_number 加以修正 */
        if (new_tail_page_number > n) {
                new_head_page_number -= (new_tail_page_number - n);
                new_tail_page_number = n;
        }

        /* 检测头部是否碰壁，若有则对 head_page_number 与 tail_page_number 加以修正 */
        if (new_head_page_number < 0) {
                new_head_page_number = 0;
                new_tail_page_number += (- new_head_page_number);
        }
        
        if (new_head_page_number >= pm->head_page_number && new_head_page_number <= pm->tail_page_number) {
                d = new_tail_page_number - pm->tail_page_number;
                for (k = 1; k <= d; k++) {
                        page = g_queue_pop_head (pm->pages);
                        /* Cache 内的页面不释放 */
                        if (g_queue_index (pm->cache, page) < 0)
                                clutter_actor_destroy (page);
                }
                for (k = 1; k <= d; k++) {
                        page = _ckd_page_manager_create_page (pm, pm->tail_page_number + k);
                        g_queue_push_tail (pm->pages, page);
                }
        } else if (new_tail_page_number >= pm->head_page_number && new_tail_page_number <= pm->tail_page_number) {
                d = abs (new_head_page_number - pm->head_page_number);
                for (k = 1; k <= d; k++) {
                        page = g_queue_pop_tail (pm->pages);
                        /* Cache 内的页面不释放 */
                        if (g_queue_index (pm->cache, page) < 0)
                                clutter_actor_destroy (page);     
                }
                for (k = 1; k <= d; k++) {
                        page = _ckd_page_manager_create_page (pm, pm->head_page_number - k);
                        g_queue_push_head (pm->pages, page);
                }
        } else {
                g_queue_foreach (pm->pages, _ckd_page_destroy_in_pages, pm);
                g_queue_clear (pm->pages);
                for (k = 0; k < queue_len; k++) {
                        page = _ckd_page_manager_create_page (pm, new_head_page_number + k);
                        g_queue_push_tail (pm->pages, page);
                }
        }

        pm->head_page_number = new_head_page_number;
        pm->tail_page_number = new_tail_page_number;
}

ClutterActor *
ckd_page_manager_get_page (CkdPageManager *pm, gint i)
{
        ClutterActor *page;
        guint d;
        
        enum {
                PM_INIT,
                PM_PAGE_VALID,
                PM_PAGE_LOST,
                PM_PAGE_OK,
                PM_PAGE_NULL
        } pm_status = PM_INIT;
        
        while (TRUE) {
                switch (pm_status) {
                case PM_INIT:
                        if (i >= 0 && i < pm->number_of_pages)
                                pm_status = PM_PAGE_VALID;
                        else
                                pm_status = PM_PAGE_NULL;
                        break;
                case PM_PAGE_VALID:
                        if (i >= pm->head_page_number && i <= pm->tail_page_number)
                                pm_status = PM_PAGE_OK;
                        else
                                pm_status = PM_PAGE_LOST;
                        break;
                case PM_PAGE_OK:
                        d = i - pm->head_page_number;
                        page = g_queue_peek_nth (pm->pages, d); /* GQueue 元素序号从 0 开始 */
                        pm->current_page_number = i;
                        goto exit_status_machine;
                case PM_PAGE_LOST:
                        _ckd_page_manager_goto (pm, i);
                        pm_status = PM_PAGE_OK;
                        break;
                case PM_PAGE_NULL:
                        page = NULL;
                        goto exit_status_machine;
                        break;
                default:
                        g_print ("I don't know what you want to do :(\n");
                        break;
                }
        }
        
exit_status_machine:
        return page;
}

ClutterActor *
ckd_page_manager_get_current_page (CkdPageManager *pm)
{
        return ckd_page_manager_get_page (pm, pm->current_page_number);
}

ClutterActor *
ckd_page_manager_advance_page (CkdPageManager *pm)
{
        ClutterActor *page;

        if (pm->current_page_number >= pm->number_of_pages - 1)
                page = ckd_page_manager_get_page (pm, pm->number_of_pages - 1);
        else
                page = ckd_page_manager_get_page (pm, ++pm->current_page_number);

        return page;
}

ClutterActor *
ckd_page_manager_retreat_page (CkdPageManager *pm)
{
        ClutterActor *page;
        
        if (pm->current_page_number <= 0)
                page = ckd_page_manager_get_page (pm, 0);
        else
                page = ckd_page_manager_get_page (pm, --pm->current_page_number);
        
        return page;
}

void
ckd_page_manager_cache (CkdPageManager *pm, ClutterActor *page)
{
        g_queue_push_tail (pm->cache, page);
}

void
ckd_page_manager_uncache (CkdPageManager *pm, ClutterActor *page)
{
        g_queue_remove_all (pm->cache, page);
        if (g_queue_index (pm->pages, page) < 0)
                clutter_actor_destroy (page);
}
