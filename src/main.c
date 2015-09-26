#include "pebble.h"

static Window *s_main_window;
static TextLayer *s_date_layer, *s_time_layer, *s_day_layer;
static Layer *s_line_layer, *s_battery_outline_layer, *s_battery_fill_layer;

static void battery_outline_layer_update_callback(Layer *layer, GContext* gtx) {
  graphics_context_set_stroke_color(gtx, GColorWhite);
  graphics_draw_rect(gtx, layer_get_bounds(layer));
}

static void battery_fill_layer_update_callback(Layer *layer, GContext* gtx) {
  BatteryChargeState charge_state = battery_state_service_peek();
  
  graphics_context_set_fill_color(gtx, GColorWhite);
  graphics_fill_rect(gtx, GRect(0, 0, charge_state.charge_percent / 10, 4), 0, GCornerNone);
}

static void line_layer_update_callback(Layer *layer, GContext* ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, 128 * t->tm_sec / 60, 2), 0, GCornerNone);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char s_time_text[] = "00:00";
  static char s_date_text[] = "Xxxxxxxxx 00";
  static char s_day_text[] = "Xxxxxxxxx";

  strftime(s_date_text, sizeof(s_date_text), "%B %e", tick_time);
  text_layer_set_text(s_date_layer, s_date_text);

  char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);

  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (s_time_text[0] == '0')) {
    memmove(s_time_text, &s_time_text[1], sizeof(s_time_text) - 1);
  }
  text_layer_set_text(s_time_layer, s_time_text);
  
  strftime(s_day_text, sizeof(s_day_text), "%A", tick_time);
  text_layer_set_text(s_day_layer, s_day_text);
  
  layer_mark_dirty(window_get_root_layer(s_main_window));
}

static void handle_battery_change() {
  layer_mark_dirty(window_get_root_layer(s_main_window));
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  s_date_layer = text_layer_create(GRect(8, 68, 136, 100));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_time_layer = text_layer_create(GRect(7, 92, 137, 76));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  s_day_layer = text_layer_create(GRect(8, 68 - 23, 136, 100));
  text_layer_set_text_color(s_day_layer, GColorWhite);
  text_layer_set_background_color(s_day_layer, GColorClear);
  text_layer_set_font(s_day_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

  GRect line_frame = GRect(8, 97, 128, 2);
  s_line_layer = layer_create(line_frame);
  layer_set_update_proc(s_line_layer, line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer);
  
  GRect battery_outline_frame = GRect(125, 4, 14, 8);
  s_battery_outline_layer = layer_create(battery_outline_frame);
  layer_set_update_proc(s_battery_outline_layer, battery_outline_layer_update_callback);
  layer_add_child(window_layer, s_battery_outline_layer);
  
  GRect battery_fill_frame = GRect(127, 6, 10, 4);
  s_battery_fill_layer = layer_create(battery_fill_frame);
  layer_set_update_proc(s_battery_fill_layer, battery_fill_layer_update_callback);
  layer_add_child(window_layer, s_battery_fill_layer);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_day_layer);

  layer_destroy(s_line_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  battery_state_service_subscribe(handle_battery_change);
  
  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_second_tick(t, SECOND_UNIT);
}

static void deinit() {
  window_destroy(s_main_window);

  tick_timer_service_unsubscribe();
}

int main() {
  init();
  app_event_loop();
  deinit();
}