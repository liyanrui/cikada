#include "ckd-slides.h"
#include "ckd-page-manager.h"
#include "ckd-page-fade.h"

struct _CkdSlides {
        ClutterActor *box;
        CkdPageManager *pm;
};

static ClutterActorBox *slide_bounding_box;

static void _ckd_on_slides_box_paint (ClutterActor *actor, gpointer user_data);
static void _ckd_page_raise (ClutterActor *slide_box, ClutterActor *page);

static void
_ckd_page_raise (ClutterActor *slide_box, ClutterActor *page)
{
        if (!clutter_actor_get_parent (page))
                clutter_container_add_actor (CLUTTER_CONTAINER(slide_box), page);
        else
                clutter_container_raise_child (CLUTTER_CONTAINER(slide_box), page, NULL);
}

static void
_ckd_on_slides_box_paint (ClutterActor *actor, gpointer user_data)
{
        CkdPageManager *pm = user_data;

        gfloat w, h;
        w = clutter_actor_get_width (actor);
        h = clutter_actor_get_height (actor);
        
        ckd_page_manager_set_page_size (pm, w, h);
}

CkdSlides *
ckd_slides_new_with_segment_length (ClutterActor *slide_box, const gchar * path, gint segment_length)
{
        gfloat w, h;
        ClutterActor *page;
        CkdSlides *slides = g_slice_new (CkdSlides);
        
        slides->box = slide_box;
        clutter_actor_get_size (slide_box, &w, &h);
        
        slides->pm = ckd_page_manager_new_with_page_size (path, w, h);
        ckd_page_manager_set_capacity (slides->pm, segment_length);
        
        page = ckd_page_manager_get_page (slides->pm, 0);
        ckd_page_manager_cache (slides->pm, page);
        clutter_container_add_actor (CLUTTER_CONTAINER(slide_box), page);
        
        g_signal_connect (slide_box, "paint",
                          G_CALLBACK(_ckd_on_slides_box_paint), slides->pm);

        return slides;
}

void
ckd_slides_switch_to_next_page (CkdSlides * slides, gint direction)
{
        ClutterActor *current_page, *next_page;
        
        current_page = ckd_page_manager_get_current_page (slides->pm);
        ckd_page_manager_cache (slides->pm, current_page);

        GTimer *timer = g_timer_new ();
        gdouble time  = 1.0;
        gdouble time_scale = 3.0;
        gdouble next_page_loading_time;
        
        g_timer_start (timer);
        {
                if (direction > 0)
                        next_page = ckd_page_manager_advance_page (slides->pm);
                if (direction < 0)
                        next_page = ckd_page_manager_retreat_page (slides->pm);
        }
        g_timer_stop (timer);
        next_page_loading_time = time_scale * g_timer_elapsed (timer, NULL);
        g_timer_destroy (timer);

        if (current_page == next_page) {
                ckd_page_manager_uncache (slides->pm, current_page);
                return;
        }

        if (time < next_page_loading_time)
                time =  next_page_loading_time;

        _ckd_page_raise (slides->box, next_page);
        _ckd_page_raise (slides->box, current_page);
        ckd_page_fade (slides->pm, current_page, next_page, time);
}
