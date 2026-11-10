/************************************************************
 * @file    gui.c
 * @brief   Filedescription
 * @author  dg
 * @date    02 Oct 2026
 ************************************************************/

/********************
 *     Includes    *
 ********************/
#include "gui.h"
#include "lvgl.h"
#ifndef LVGL_LIVE_PREVIEW
#include "input.h"
#endif

/************************************
 *     Private Macros / Defines    *
 ************************************/

/***************************
 *     Private Typedefs     *
 ***************************/

/*****************************************
 *     Private Function Declarations     *
 *****************************************/

void gui_create_counter(lv_obj_t *parent, const char *label_text, int32_t initialValue, lv_subject_t *subject);
#ifndef LVGL_LIVE_PREVIEW
void gui_init_input_device(void);
static void gui_keypad_read(lv_indev_t * indev, lv_indev_data_t * data);
#endif
static void gui_counter_increment_cb(lv_event_t *e);
static void gui_counter_decrement_cb(lv_event_t *e);

#ifdef LVGL_LIVE_PREVIEW
void lvgl_live_preview_init(void);
#endif
/****************************
 *     Public Variables     *
 ****************************/

/*****************************
 *     Private Variables     *
 *****************************/

static lv_theme_t *s_theme = NULL;

static lv_display_t *s_display = NULL;

static lv_group_t *s_input_group = NULL;

/*******************************************
 *     Public Function Implementations     *
 *******************************************/

/********************************************
 *     Private Function Implementations     *
 ********************************************/

void gui_init(void)
{
    s_display = lv_disp_get_default();

    lv_obj_t *screen = lv_scr_act();

#ifndef LVGL_LIVE_PREVIEW
    s_input_group = lv_group_create();
    lv_group_set_wrap(s_input_group, true);
    gui_init_input_device();
#endif

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101827), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "MPT");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 8);

    lv_obj_t *panel = lv_obj_create(screen);
    lv_obj_set_size(panel, 400, 100);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(panel, 8, 0);

    static lv_subject_t counter_1_subject;
    static lv_subject_t counter_2_subject;
    static lv_subject_t counter_3_subject;
    gui_create_counter(panel, "Counter 1", 0, &counter_1_subject);
    gui_create_counter(panel, "Counter 2", 0, &counter_2_subject);
    gui_create_counter(panel, "Counter 3", 0, &counter_3_subject);

}

#ifndef LVGL_LIVE_PREVIEW
void gui_init_input_device(void)
{
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, gui_keypad_read);
    lv_indev_set_group(indev, s_input_group);
}
#endif

void gui_create_counter(lv_obj_t *parent, const char *label_text, int32_t initialValue, lv_subject_t *subject)
{
    lv_subject_init_int(subject, initialValue);
    lv_subject_set_min_value_int(subject, 0);

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 100, 80);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(label, label_text);

    lv_obj_t *counter = lv_label_create(panel);
    lv_obj_align(counter, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(counter, &lv_font_montserrat_32, 0);
    lv_label_bind_text(counter, subject, "%d");

    lv_obj_t *btn_inc = lv_button_create(panel);
    lv_obj_set_size(btn_inc, 20, 20);
    lv_obj_set_style_pad_all(btn_inc, 0, 0);
    lv_obj_align(btn_inc, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_border_color(
        btn_inc,
        lv_color_hex(0x00AEEF),
        LV_STATE_FOCUSED
    );

    lv_obj_t *btn_inc_label = lv_label_create(btn_inc);
    lv_label_set_text(btn_inc_label, "+");
    lv_obj_align(btn_inc_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_inc, gui_counter_increment_cb, LV_EVENT_CLICKED, subject);

    lv_obj_t *btn_dec = lv_button_create(panel);
    lv_obj_set_size(btn_dec, 20, 20);
    lv_obj_align(btn_dec, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_border_color(
        btn_dec,
        lv_color_hex(0x00AEEF),
        LV_STATE_FOCUSED
    );

    lv_obj_t *btn_dec_label = lv_label_create(btn_dec);
    lv_obj_align(btn_dec_label, LV_ALIGN_CENTER, -1, -1);
    lv_label_set_text(btn_dec_label, "-");
    lv_obj_add_event_cb(btn_dec, gui_counter_decrement_cb, LV_EVENT_CLICKED, subject);

#ifndef LVGL_LIVE_PREVIEW
    if (s_input_group != NULL) {
        lv_group_add_obj(s_input_group, btn_dec);
        lv_group_add_obj(s_input_group, btn_inc);

        if (lv_group_get_focused(s_input_group) == NULL) {
            lv_group_focus_obj(btn_inc);
        }
    }
#endif
}

static void gui_counter_increment_cb(lv_event_t *e)
{
    lv_subject_t *subject = (lv_subject_t *)lv_event_get_user_data(e);
    lv_subject_set_int(subject, lv_subject_get_int(subject) + 1);
}

static void gui_counter_decrement_cb(lv_event_t *e)
{
    lv_subject_t *subject = (lv_subject_t *)lv_event_get_user_data(e);
    lv_subject_set_int(subject, lv_subject_get_int(subject) - 1);
}

#ifndef LVGL_LIVE_PREVIEW
static void gui_keypad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    bool up, down, left, right, enter, escape, prev, next;
    input_get_button_states(&up, &down, &left, &right, &enter, &escape, &prev, &next);

    if (up || left) {
        data->key = LV_KEY_PREV;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (down || right) {
        data->key = LV_KEY_NEXT;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (enter) {
        data->key = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (escape) {
        data->key = LV_KEY_ESC;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (prev) {
        data->key = LV_KEY_PREV;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (next) {
        data->key = LV_KEY_NEXT;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->key = 0; // No key pressed
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->continue_reading = false;
}
#endif

#ifdef LVGL_LIVE_PREVIEW
// gives the live preview tool a way to initialize your UI
void lvgl_live_preview_init(void)
{
    gui_init();
}
#endif