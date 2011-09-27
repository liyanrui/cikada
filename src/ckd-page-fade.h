#ifndef CKD_PAGE_FADE_H
#define CKD_PAGE_FADE_H

#include "ckd-page-manager.h"

void ckd_page_fade (
        CkdPageManager *pm,
        ClutterActor *current_page,
        ClutterActor *next_page,
        gdouble time
        );

#endif
