#ifndef CKD_PAGE_MANAGER_H
#define CKD_PAGE_MANAGER_H

#include "ckd-page.h"

typedef struct _CkdPageManager CkdPageManager;

CkdPageManager * ckd_page_manager_new_with_page_size (
        const gchar *pdf_name, gfloat width, gfloat height
        );

void ckd_page_manager_set_capacity (CkdPageManager *pm, guint capacity);

void ckd_page_manager_set_page_size (
        CkdPageManager *pm, gfloat width, gfloat height);

ClutterActor * ckd_page_manager_get_page (CkdPageManager *pm, gint i);

ClutterActor * ckd_page_manager_advance_page (CkdPageManager *pm);
ClutterActor * ckd_page_manager_retreat_page (CkdPageManager *pm);
ClutterActor * ckd_page_manager_get_current_page (CkdPageManager *pm);
gint ckd_page_manager_get_number_of_pages (CkdPageManager *pm);

void ckd_page_manager_cache (CkdPageManager *pm, ClutterActor *page);
void ckd_page_manager_uncache (CkdPageManager *pm, ClutterActor *page);

void ckd_page_manager_hide_pages (CkdPageManager *pm);
void ckd_page_manager_show_pages (CkdPageManager *pm);

PopplerDocument *ckd_page_manager_get_pdf (CkdPageManager *pm);

#endif
