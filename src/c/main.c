#include <pebble.h>

// Persistent storage key
#define SETTINGS_KEY 1

// Define our settings struct
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor HourColor;
  GColor MinuteColor;
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

// Main Window
static Window *s_main_window;

// Root (Parent) Window Layer
static Layer *s_window_layer;

// Global Values:
static int s_battery_level;
static bool s_battery_charging;
static bool s_bluetooth_connected;

// Set default settings
static void prv_default_settings() {
  settings.BackgroundColor = GColorBlack;
  settings.HourColor = GColorWhite;
  settings.MinuteColor = PBL_IF_COLOR_ELSE(GColorFromHEX(0xFF5555), GColorWhite);
  
  //settings.BackgroundColor = PBL_IF_COLOR_ELSE(GColorFromHEX(0x005555), GColorWhite);
  //settings.HourColor = PBL_IF_COLOR_ELSE(GColorFromHEX(0x00AAAA), GColorBlack);
  //settings.MinuteColor = PBL_IF_COLOR_ELSE(GColorFromHEX(0x55FFFF), GColorBlack);
}

// Save settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Read settings from persistent storage
static void prv_load_settings() {
  // Set defaults first
  prv_default_settings();
  // Then override with any saved values
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void window_update_proc(Layer *layer, GContext *ctx) {
  
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  
  window_set_background_color(s_main_window, settings.BackgroundColor);
  
  // Get Local Time and Day of Month
  int hour = tick_time->tm_hour % 12;
  int minute = tick_time->tm_min;
  
  //hour = 5;
  //minute = 45;
  
  int hour_angle = DEG_TO_TRIGANGLE (30*hour + 0.5*minute);
  int minute_angle = DEG_TO_TRIGANGLE ((12*minute)%360);
  
  // Get center of screen and max visible arm length for scaling
  GPoint center = GPoint((bounds.size.w-1)/2, (bounds.size.h-1)/2);
  int max_len = center.x>center.y? center.y : center.x;
  
  // Calculate Hand
  int hand_len = (max_len*12/14);
  GRect hand_rect = GRect ( center.x - hand_len, center.y - hand_len, 2*hand_len+1, 2*hand_len+1);
  GPoint hand_end = gpoint_from_polar(hand_rect, GOvalScaleModeFitCircle, hour_angle);
  
    // Actual Hands
  graphics_context_set_stroke_color(ctx, settings.MinuteColor);
  graphics_context_set_stroke_width(ctx, max_len/10);
  if (minute < 30){
    graphics_draw_arc(ctx, hand_rect, GOvalScaleModeFitCircle, hour_angle, (minute_angle + hour_angle));
  } else {
    graphics_draw_arc(ctx, hand_rect, GOvalScaleModeFitCircle, (minute_angle + hour_angle), hour_angle + TRIG_MAX_ANGLE);
  }
  
  graphics_context_set_stroke_color(ctx, settings.HourColor);
  graphics_draw_line(ctx, center, hand_end);
  

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_window_layer);
}

             
// AppMessage received handler
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  // Check for Clay settings data
  Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) {
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
    window_set_background_color(s_main_window, settings.BackgroundColor);
  }
  
  #ifdef PBL_COLOR
  Tuple *hour_color_t = dict_find(iterator, MESSAGE_KEY_HourColor);
  if (hour_color_t) {
    settings.HourColor = GColorFromHEX(hour_color_t->value->int32);
  }
  
  Tuple *min_color_t = dict_find(iterator, MESSAGE_KEY_MinuteColor);
  if (min_color_t) {
    settings.MinuteColor = GColorFromHEX(min_color_t->value->int32);
  }
  
  #else
  // For B/W watches, set the elements to the opposite color of the background. Element color selection is disabled for b/w watches in app config.js
  settings.HourColor = gcolor_equal(settings.BackgroundColor,GColorBlack) ? GColorWhite : GColorBlack;
  settings.MinuteColor = settings.HourColor;
  #endif

  // Save and apply if any settings were changed
  if ( PBL_IF_COLOR_ELSE(bg_color_t || hour_color_t || min_color_t, bg_color_t) ){
    prv_save_settings();
    layer_mark_dirty(s_window_layer);

  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area, void *context) {
  layer_mark_dirty(s_window_layer);
  //window_update_proc(Layer *layer, GContext *ctx);
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  layer_mark_dirty(s_window_layer);
  //window_update_proc(Layer *layer, GContext *ctx);
}

static void prv_unobstructed_did_change(void *context) {
  layer_mark_dirty(s_window_layer);
  //window_update_proc(Layer *layer, GContext *ctx);
}

static void main_window_load(Window *window) {
  GRect bounds = layer_get_bounds(window_get_root_layer(window));

  s_window_layer = layer_create(bounds);
  
  layer_set_update_proc(s_window_layer, window_update_proc);
  
  layer_add_child(window_get_root_layer(window), s_window_layer);
  
  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change,
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_window_layer);
}

static void init() {
  // Load settings before creating UI
  prv_load_settings();

  s_main_window = window_create();
  window_set_background_color(s_main_window, settings.BackgroundColor);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  layer_mark_dirty(s_window_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);


  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  const int inbox_size = 256;
  const int outbox_size = 256;
  app_message_open(inbox_size, outbox_size);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
