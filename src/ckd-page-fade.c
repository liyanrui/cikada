#include "ckd-page-fade.h"

static CkdPageManager *_ckd_page_manager = NULL;

static void
_ckd_on_fade (ClutterAnimation *am, gpointer data)
{
        ckd_page_manager_uncache (_ckd_page_manager, data);
}

void
ckd_page_fade (CkdPageManager *pm,
               ClutterActor *current_page,
               ClutterActor *next_page,
               gdouble time)
{
        _ckd_page_manager = pm;
        clutter_actor_set_opacity (next_page, 255);
        clutter_actor_set_opacity (current_page, 255);

        /* 如果 next_page 已存在正在消隐的动画，那么就让它复位 */
        ClutterTimeline *timeline;
        ClutterTimelineDirection direction;
        ClutterAnimation *am = clutter_actor_get_animation (next_page);
        if (am) {
                timeline = clutter_animation_get_timeline (am);
                direction = clutter_timeline_get_direction (timeline);
                
                if (direction == CLUTTER_TIMELINE_FORWARD)
                        direction = CLUTTER_TIMELINE_BACKWARD;
                else
                        direction = CLUTTER_TIMELINE_FORWARD;
                
                clutter_timeline_set_direction (timeline, direction);
        }

        /* 让当前页消隐 */
        clutter_actor_animate (
                current_page,
                CLUTTER_LINEAR,
                1000 * ((guint)time),
                "opacity", 0,
                "signal-after::completed", _ckd_on_fade, current_page,
                NULL
                );
}
