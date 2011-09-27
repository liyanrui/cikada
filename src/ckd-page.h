#ifndef CKD_PAGE_H
#define CKD_PAGE_H

#include <glib.h>
#include <clutter/clutter.h>
#include <poppler.h>

#define CKD_TYPE_PAGE (ckd_page_get_type ())
#define CKD_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CKD_TYPE_PAGE, CkdPage))
#define CKD_IS_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CKD_TYPE_PAGE))
#define CKD_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CKD_TYPE_PAGE, CkdPageClass))
#define CKD_IS_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CKD_TYPE_PAGE))
#define CKD_PAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CKD_TYPE_PAGE, CkdPageClass))

typedef struct _CkdPage CkdPage;
typedef struct _CkdPageClass CkdPageClass;

struct _CkdPage {
        ClutterActor parent_instance;
};

struct _CkdPageClass {
        ClutterActorClass parent_class;
};

GType ckd_page_get_type (void);

/* Public functons */
ClutterActor *ckd_page_new (PopplerPage *page);
ClutterActor *ckd_page_new_with_default_quality (PopplerPage *page);
void ckd_page_set_quality (CkdPage *page, gfloat quality);
void ckd_page_set_border (CkdPage *page, gfloat border);

#endif
