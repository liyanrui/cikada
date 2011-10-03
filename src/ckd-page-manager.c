#include "ckd-page-manager.h"

struct _CkdPageManager {
        PopplerDocument *pdf;
        GQueue *pages;
        GQueue *cache;
        GList  *thumbs;
        gint number_of_pages;
        gint head_page_number;
        gint tail_page_number;
        gint current_page_number;
        gfloat page_width;
        gfloat page_height;
};

static PopplerDocument * _ckd_page_manager_open_pdf (const gchar * path);
static ClutterActor *    _ckd_page_manager_create_page (CkdPageManager *pm, gint i);
static void              _ckd_page_destroy_in_pages (gpointer data, gpointer user_data);
static void              _ckd_page_is_in_cache (gpointer data, gpointer user_data);
static void              _ckd_page_destroy_in_cache (gpointer data, gpointer user_data);
static void              _ckd_page_manager_refresh_page_size (gpointer data, gpointer user_data);
static void              _ckd_page_manager_goto (CkdPageManager *pm, gint i);
static void              _ckd_page_hide (gpointer data, gpointer user_data);
static void              _ckd_page_show (gpointer data, gpointer user_data);

static PopplerDocument *
_ckd_page_manager_open_pdf (const gchar * path)
{
        PopplerDocument *pdf;
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

        pdf = poppler_document_new_from_file (pdf_file_uri, NULL, NULL);
        g_free (pdf_file_uri);

        g_assert (pdf != NULL);

        return pdf;
}

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

static void
_ckd_page_hide (gpointer data, gpointer user_data)
{
        clutter_actor_hide (data);
}

static void
_ckd_page_show (gpointer data, gpointer user_data)
{
        clutter_actor_show (data);
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
ckd_page_manager_set_capacity (CkdPageManager *pm, guint capacity)
{
        PopplerPage *pdf_page;
        ClutterActor *page;

        gint delta_len, delta_len_abs, d, h, t, i;
        gint n = pm->number_of_pages;
        
        if (capacity > n)
                capacity = n;

        delta_len = capacity - (pm->tail_page_number - pm->head_page_number + 1);
        delta_len_abs = abs (delta_len);

        if (delta_len == 0)
                return;
        
        h = delta_len_abs / 2;
        t = delta_len_abs - h;

        if (delta_len > 0) { /* 所设缓冲页面数量大于当前缓冲页面数量 */
                if (pm->head_page_number - h <= 0) {
                        h = 0;
                        t = delta_len;
                }
                if (pm->tail_page_number + t >= n) {
                        h = delta_len;
                        t = 0;
                }
                for (i = 1; i <= h; i++) {
                        page = _ckd_page_manager_create_page (pm, pm->head_page_number - i);
                        g_queue_push_head (pm->pages, page);
                }
                for (i = 1; i <= t; i++) {
                        page = _ckd_page_manager_create_page (pm, pm->tail_page_number + i);
                        g_queue_push_tail (pm->pages, page);
                }
                pm->head_page_number -= h;
                pm->tail_page_number += t;
        } else { /* 所设缓冲页面数量小于当前缓冲页面数量 */
                d = pm->current_page_number - pm->head_page_number;
                if (d < h) {
                        h = d;
                        t = delta_len_abs - h;
                }

                d = pm->tail_page_number - pm->current_page_number;
                if (d < t) {
                        t = d;
                        h = delta_len_abs - t;
                }
                
                for (i = h; i > 0; i--) {
                        page = g_queue_pop_head (pm->pages);
                        /* 被缓冲的页面不释放 */
                        if (g_queue_index (pm->cache, page) < 0)
                                clutter_actor_destroy (page);
                }
                for (i = t; i > 0; i--) {
                        page = g_queue_pop_tail (pm->pages);
                        /* 被缓冲的页面不释放 */
                        if (g_queue_index (pm->cache, page) < 0)
                                clutter_actor_destroy (page);
                }
                pm->head_page_number += h;
                pm->tail_page_number -= t;
        }
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

gint
ckd_page_manager_get_number_of_pages (CkdPageManager *pm)
{
        return pm->number_of_pages;
}

void
ckd_page_manager_hide_pages (CkdPageManager *pm)
{
        g_queue_foreach (pm->pages, _ckd_page_hide, NULL);
}

void
ckd_page_manager_show_pages (CkdPageManager *pm)
{
        g_queue_foreach (pm->pages, _ckd_page_show, NULL);
}

PopplerDocument *
ckd_page_manager_get_pdf (CkdPageManager *pm)
{
        return pm->pdf;
}
