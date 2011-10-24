#include <math.h>
#include "ckd-page-manager.h"

/* 页面队列默认容量 */
#undef  CKD_PAGE_MANAGER_CAPACITY
#define CKD_PAGE_MANAGER_CAPACITY (3)

/* 默认页面尺寸 */
#undef  CKD_CPM_PAGE_WIDTH
#define CKD_CPM_PAGE_WIDTH (640.0)
#undef  CKD_CPM_PAGE_HEIGHT
#define CKD_CPM_PAGE_HEIGHT (480.0)

G_DEFINE_TYPE (CkdPageManager, ckd_page_manager, G_TYPE_OBJECT);

#define CKD_PAGE_MANAGER_GET_PRIVATE(obj) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_PAGE_MANAGER, CkdPageManagerPriv))

typedef struct _CkdPageManagerPriv CkdPageManagerPriv;
struct  _CkdPageManagerPriv {
        PopplerDocument *doc;
        GString *doc_uri;
        gint capacity;
        GQueue  *pages;
        GQueue  *cache;
        gint number_of_pages;
        gint head_page_number;
        gint current_page_number;
        gfloat page_width;
        gfloat page_height;
};

enum {
        PROP_CPM_0,
        PROP_CPM_DOC_PATH,
        PROP_CPM_CAPACITY,
        PROP_CPM_DOC,
        PROP_CPM_NUMBER_OF_PAGES,
        PROP_CPM_HEAD_PAGE_NUMBER,
        PROP_CPM_CURRENT_PAGE_NUMBER,
        PROP_CPM_PAGE_WIDTH,
        PROP_CPM_PAGE_HEIGHT,
        N_CPM_PROPS
};

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
_ckd_page_manager_refresh_size (gpointer data, gpointer user_data)
{
        ClutterActor *page = data;
        CkdPageManager *pm = user_data;
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (pm);
        clutter_actor_set_size (page, priv->page_width, priv->page_height);
}

static ClutterActor *
_ckd_page_manager_create_page (CkdPageManager *self, gint i)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        PopplerPage *pdf_page;
        ClutterActor *page;

        /* 如果页码为负数，则从尾页向前计算 */
        if (i < 0)
                i += priv->number_of_pages;

        /* 页码整除以页面总数，是防止页码溢出。页码溢出时，就从首页向后计算 */
        pdf_page = poppler_document_get_page (priv->doc, i % priv->number_of_pages);
        
        page = ckd_page_new_with_default_quality (pdf_page);
        clutter_actor_set_size (page, priv->page_width, priv->page_height);
        
        return page;
}

static void
_ckd_page_destroy_in_queue (gpointer data, gpointer user_data)
{
        
        ClutterActor *page = data;
        CkdPageManager *self = user_data;
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        if (g_queue_index (priv->cache, page) < 0)
                clutter_actor_destroy (page);
}

static void
_ckd_page_manager_set_capacity (CkdPageManager *self, guint capacity)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        PopplerPage *pdf_page;
        ClutterActor *page;
        gint d, tail_page_number, left_rest, right_rest;
        gint first_page_number = 0, last_page_number = priv->number_of_pages - 1;

        if (capacity > priv->number_of_pages)
                capacity = priv->number_of_pages;
        
        tail_page_number = priv->head_page_number + priv->capacity -1;
        d = capacity - priv->capacity;

        if (d == 0) {
                return;
        }
        
        left_rest = d / 2;
        right_rest = d - left_rest;
        
        if (d > 0) {
                /* 左侧与右侧的空位 */
                
                for (int i = 1; i <= left_rest; i++) {
                        page = _ckd_page_manager_create_page (self, priv->head_page_number - i);
                        g_queue_push_head (priv->pages, page);                                
                }
                
                for (int i = 1; i <= right_rest; i++) {
                        page = _ckd_page_manager_create_page (self, tail_page_number + i);
                        g_queue_push_tail (priv->pages, page);
                }
        } else {
                /* 左侧与右侧的余量 */
                left_rest = d / 2;
                right_rest = d - left_rest;
                
                for (int i = left_rest; i < 0; i++) {
                        page = g_queue_pop_head (priv->pages);
                        /* 被缓冲的页面不释放 */
                        if (g_queue_index (priv->cache, page) < 0)
                                clutter_actor_destroy (page);
                }
                for (int i = right_rest; i < 0; i++) {
                        page = g_queue_pop_tail (priv->pages);
                        /* 被缓冲的页面不释放 */
                        if (g_queue_index (priv->cache, page) < 0)
                                clutter_actor_destroy (page);
                }
        }
        
        /* 更新页面队列首页面编号 */
        priv->head_page_number -= left_rest;
        
        /* 更新页面队列容量 */
        priv->capacity = capacity;
}

static void
_ckd_page_manager_goto (CkdPageManager *self, gint i)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        ClutterActor *page;
        PopplerPage *pdf_page;
        gint first_page_number = 0, last_page_number = priv->number_of_pages - 1;

        /* 原页面队列的首页与尾页 */
        gint p0 = priv->head_page_number;
        gint p1 = priv->head_page_number + priv->capacity - 1;

        /* 新页面队列的首页与尾页编码 */
        gint q0 = i - priv->capacity / 2;
        gint q1 = q0 + priv->capacity - 1;

        /* 新旧页面队列重叠区域处理 */
        if (q0 >= p0 && q0 <= p1) {
                for (int i = 1; i <= (q1 - p1); i++) {
                        page = g_queue_pop_head (priv->pages);
                        /* Cache 内的页面不释放 */
                        if (g_queue_index (priv->cache, page) < 0)
                                clutter_actor_destroy (page);

                        page = _ckd_page_manager_create_page (self, p1 + i);
                        g_queue_push_tail (priv->pages, page);
                }
        } else if (q1 >= p0 && q1 <= p1) {
                for (int i = 1; i <= (p0 - q0); i++) {
                        page = g_queue_pop_tail (priv->pages);
                        /* Cache 内的页面不释放 */
                        if (g_queue_index (priv->cache, page) < 0)
                                clutter_actor_destroy (page);     

                        page = _ckd_page_manager_create_page (self, p0 - i);
                        g_queue_push_head (priv->pages, page);
                }
        } else {
                g_queue_foreach (priv->pages, _ckd_page_destroy_in_queue, self);
                g_queue_clear (priv->pages);
                for (int i = 0; i < priv->capacity; i++) {
                        page = _ckd_page_manager_create_page (self, q0 + i);
                        g_queue_push_tail (priv->pages, page);
                }
        }

        /* 更新页面队列首页页码 */
        priv->head_page_number = q0;
}

ClutterActor *
ckd_page_manager_get_page (CkdPageManager *self, gint i)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        ClutterActor *page;

        gint p0, p1;

        enum {
                PM_INIT,
                PM_PAGE_LOST,
                PM_PAGE_OK,
                PM_PAGE_NULL
        } pm_status = PM_INIT;
        
        while (TRUE) {
                p0= priv->head_page_number;
                p1 = priv->head_page_number + priv->capacity - 1;
                
                switch (pm_status) {
                case PM_INIT:
                        if (i >= p0 && i <= p1)
                                pm_status = PM_PAGE_OK;
                        else
                                pm_status = PM_PAGE_LOST;
                        break;
                case PM_PAGE_OK:
                        page = g_queue_peek_nth (priv->pages, i - p0); /* GQueue 元素序号从 0 开始 */
                        priv->current_page_number = i;
                        goto exit_status_machine;
                case PM_PAGE_LOST:
                        _ckd_page_manager_goto (self, i);
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

static void
ckd_page_manager_set_property (GObject *obj,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
        CkdPageManager *self = CKD_PAGE_MANAGER (obj);
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);

        gchar *uri = NULL;
        gint capacity = 0;
        gfloat width, height;
        
        switch (property_id) {
        case PROP_CPM_DOC_PATH:
                uri = _ckd_page_manager_get_uri_from_path (g_value_get_string (value));
                priv->doc_uri = g_string_new (uri);
                g_free (uri);

                priv->doc = poppler_document_new_from_file (priv->doc_uri->str, NULL, NULL);
                g_assert (priv->doc != NULL);

                /* 确保页面队列长度不会大于 PDF 文档页面数量 */
                priv->number_of_pages = poppler_document_get_n_pages (priv->doc);
                if (priv->capacity > priv->number_of_pages)
                        priv->capacity = priv->number_of_pages;
                
                /* 页面队列初始化 */
                for (int i = 0; i < priv->capacity; i++)
                        g_queue_push_tail (priv->pages, _ckd_page_manager_create_page (self, i));
                break;
        case PROP_CPM_CAPACITY:
                _ckd_page_manager_set_capacity (self,g_value_get_int (value));
                break;
        case PROP_CPM_PAGE_WIDTH:
                width = g_value_get_float (value);
                if (fabs (priv->page_width - width) > G_MINFLOAT) {
                        priv->page_width = width;
                        g_queue_foreach (priv->pages, _ckd_page_manager_refresh_size, self);
                }
                break;
        case PROP_CPM_PAGE_HEIGHT:
                height = g_value_get_float (value);
                if (fabs (priv->page_height - height) > G_MINFLOAT) {
                        priv->page_height = height;
                        g_queue_foreach (priv->pages, _ckd_page_manager_refresh_size, self);
                }
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
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
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
                break;
        }        
}

/* GObject 析构函数 */
static void
ckd_page_manager_dispose (GObject *obj)
{
        CkdPageManager *self     = CKD_PAGE_MANAGER (obj);
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        if (priv->doc){
                g_object_unref (priv->doc);
                priv->doc = NULL;
        }
        G_OBJECT_CLASS (ckd_page_manager_parent_class)->dispose (obj);
}

static void
ckd_page_manager_finalize (GObject *obj)
{      
        CkdPageManager *self        = CKD_PAGE_MANAGER (obj);
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        g_string_free (priv->doc_uri, TRUE);
        
        g_queue_foreach (priv->pages, _ckd_page_destroy_in_queue, self);
        g_queue_foreach (priv->cache, _ckd_page_destroy_in_queue, self);

        g_queue_free (priv->pages);
        g_queue_free (priv->cache);
        
        G_OBJECT_CLASS (ckd_page_manager_parent_class)->finalize (obj);
}

static void
ckd_page_manager_class_init (CkdPageManagerClass *klass)
{
        g_type_class_add_private (klass, sizeof (CkdPageManagerPriv));

        GObjectClass *base_class = G_OBJECT_CLASS (klass);
        base_class->dispose      = ckd_page_manager_dispose;
        base_class->finalize     = ckd_page_manager_finalize;
        base_class->set_property = ckd_page_manager_set_property;
        base_class->get_property = ckd_page_manager_get_property;
        
        GParamSpec *props[N_CPM_PROPS] = {NULL,};
        props[PROP_CPM_DOC_PATH] =
                g_param_spec_string ("document-path", "Document Path", "PDF document path",
                                     NULL, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
        props[PROP_CPM_CAPACITY] =
                g_param_spec_int ("capacity", "Capacity",
                                  "Page manager capacity",
                                  0, G_MAXINT, 0, G_PARAM_READWRITE);
        props[PROP_CPM_DOC] =
                g_param_spec_pointer ("document", "Document", "PDF document object",
                                      G_PARAM_READABLE);
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
                                    0.0, G_MAXFLOAT, 0.0, G_PARAM_READWRITE);
        props[PROP_CPM_PAGE_HEIGHT] =
                g_param_spec_float ("page-height", "Page Height", "Page Height",
                                    0.0, G_MAXFLOAT, 0.0, G_PARAM_READWRITE);
        g_object_class_install_properties (base_class, N_CPM_PROPS, props);
}
 
static void
ckd_page_manager_init (CkdPageManager *self)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);

        priv->pages = g_queue_new ();
        priv->cache = g_queue_new ();
        
        priv->capacity = CKD_PAGE_MANAGER_CAPACITY;
        priv->head_page_number = 0;
        priv->current_page_number = 0;
        priv->page_width = CKD_CPM_PAGE_WIDTH;
        priv->page_height = CKD_CPM_PAGE_HEIGHT;
}

ClutterActor *
ckd_page_manager_get_current_page (CkdPageManager *self)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        return ckd_page_manager_get_page (self, priv->current_page_number);
}

ClutterActor *
ckd_page_manager_advance_page (CkdPageManager *self)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        ClutterActor *page = ckd_page_manager_get_page (self, ++priv->current_page_number);

        return page;
}

ClutterActor *
ckd_page_manager_retreat_page (CkdPageManager *self)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        ClutterActor *page = ckd_page_manager_get_page (self, --priv->current_page_number);
        
        return page;
}

void
ckd_page_manager_cache (CkdPageManager *self, ClutterActor *page)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        g_queue_push_tail (priv->cache, page);
}

void
ckd_page_manager_uncache (CkdPageManager *self, ClutterActor *page)
{
        CkdPageManagerPriv *priv = CKD_PAGE_MANAGER_GET_PRIVATE (self);
        
        g_queue_remove_all (priv->cache, page);
        
        if (g_queue_index (priv->pages, page) < 0)
                clutter_actor_destroy (page);
}
