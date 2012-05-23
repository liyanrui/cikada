#include <math.h>
#include "ckd-progress.h"

G_DEFINE_TYPE (CkdProgress, ckd_progress, CLUTTER_TYPE_ACTOR);
#define CKD_PROGRESS_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_PAGE, CkdProgressPriv))

typedef struct _CkdProgressPriv {
        ClutterActor *bar;
        ClutterActor *nonius;
} CkdProgressPriv;
