#include <pebble.h>

#define STEPS 0

static Window *s_main_window;
static TextLayer *s_output_layer, *s_distance_layer, *s_steps_layer, *s_cadence_layer, *s_status_layer;
static time_t start_time;
int test_value = 0;

static void worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  static char s_cadence[32];
  static char s_distance[32];
  static char s_steps[32];
  int km = 0;
  int m = 0;
  int value = 0;
  switch (type) {
    case STEPS:
      if(test_value == 0) { value = data->data1; } else {value = test_value; }
      m = (value * 115 / 100);
      km = (m > 1000) ? m/1000 : 0;
    
      snprintf(s_distance, sizeof(s_distance), "Distance: %2d.%02dk", km, ((km > 0 ? m - (km * 1000) : m) & 0xFF));
      text_layer_set_text(s_distance_layer, s_distance);

      snprintf(s_steps, sizeof(s_steps), "Steps: %d", value);
      text_layer_set_text(s_steps_layer, s_steps);
      
      snprintf(s_cadence, sizeof(s_cadence), "Cadence: %d", data->data0/2);
      text_layer_set_text(s_cadence_layer, s_cadence);
      break;
    default:
      break;
  }
}

static void add_steps_handler(ClickRecognizerRef recognizer, void *context) {
  test_value += 100;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Check to see if the worker is currently active
  bool running = app_worker_is_running();

  // Toggle running state
  AppWorkerResult result;
  if(running) {
    result = app_worker_kill();

    if(result == APP_WORKER_RESULT_SUCCESS) {
      text_layer_set_text(s_status_layer, "Stopped");
    } else {
      text_layer_set_text(s_status_layer, "Error killing worker!");
    }
  } else {
    result = app_worker_launch();

    if(result == APP_WORKER_RESULT_SUCCESS) {
      text_layer_set_text(s_distance_layer, "Distance:  0.00k");
      text_layer_set_text(s_steps_layer, "Steps: 0");
      text_layer_set_text(s_cadence_layer, "Cadence: 0");
      text_layer_set_text(s_status_layer, "Stopped");
      text_layer_set_text(s_status_layer, "Running");
      test_value = 0;
      start_time = time(NULL);
    } else {
      text_layer_set_text(s_status_layer, "Error launching worker!");
    }
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Result: %d", result);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, add_steps_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const int inset = 8;

  // Create UI
  s_output_layer = text_layer_create(bounds);
  text_layer_set_text(s_output_layer, "SELECT: start/stop");
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
#ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(s_output_layer, inset);
#endif

  s_distance_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(5, 0), 45, bounds.size.w, 30));
  s_steps_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(5, 0), 75, bounds.size.w, 30));
  s_cadence_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(5, 0), 105, bounds.size.w, 30));
  s_status_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(5, 0), 145, bounds.size.w, 20));
  
  text_layer_set_font(s_distance_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_font(s_cadence_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_font(s_steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  
  text_layer_set_text_alignment(s_distance_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter));
  text_layer_set_text_alignment(s_steps_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter));
  text_layer_set_text_alignment(s_cadence_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  
  text_layer_set_text(s_distance_layer, "Distance:  0.00k");
  text_layer_set_text(s_steps_layer, "Steps: 0");
  text_layer_set_text(s_cadence_layer, "Cadence: 0");
  text_layer_set_text(s_status_layer, "Stopped");
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_cadence_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

#ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(s_cadence_layer, inset);
#endif
}

static void main_window_unload(Window *window) {
  // Destroy UI
  text_layer_destroy(s_output_layer);
  text_layer_destroy(s_cadence_layer);
  text_layer_destroy(s_distance_layer);
  text_layer_destroy(s_status_layer);
}

static void init(void) {
  // Setup main Window
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  // Subscribe to Worker messages
  app_worker_message_subscribe(worker_message_handler);
}

static void deinit(void) {
  // Destroy main Window
  window_destroy(s_main_window);

  // No more worker updates
  app_worker_message_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
