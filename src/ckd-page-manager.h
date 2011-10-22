#ifndef CKD_PAGE_MANAGER_H
#define CKD_PAGE_MANAGER_H

#include "ckd-page.h"


#define CKD_TYPE_PAGE_MANAGER (ckd_page_manager_get_type ())
#define CKD_PAGE_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CKD_TYPE_PAGE_MANAGER, CkdPageManager))
#define CKD_IS_PAGE_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CKD_TYPE_PAGE_MANAGER))
#define CKD_PAGE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CKD_TYPE_PAGE_MANAGER, CkdPageManagerClass))
#define CKD_IS_PAGE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CKD_TYPE_PAGE_MANAGER))
#define CKD_PAGE_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CKD_TYPE_PAGE_MANAGER, CkdPageManagerClass))

typedef struct _CkdPageManager CkdPageManager;
typedef struct _CkdPageManagerClass CkdPageManagerClass;

struct _CkdPageManager {
        GObject parent_instance;
};

struct _CkdPageManagerClass {
        GObjectClass parent_class;
};

GType ckd_page_manager_get_type (void);

ClutterActor * ckd_page_manager_get_page (CkdPageManager *pm, gint i);
ClutterActor * ckd_page_manager_advance_page (CkdPageManager *pm);
ClutterActor * ckd_page_manager_retreat_page (CkdPageManager *pm);
ClutterActor * ckd_page_manager_get_current_page (CkdPageManager *pm);


void ckd_page_manager_cache (CkdPageManager *pm, ClutterActor *page);
void ckd_page_manager_uncache (CkdPageManager *pm, ClutterActor *page);

#endif
