#include <pebble_worker.h>

#define STEPS 0

static uint16_t s_ticks = 0;
static time_t last_time;
static uint64_t s_steps_total = 0;

static void cadence_handler(struct tm *tick_timer, TimeUnits units_changed) {
  uint32_t numRcvd= 0;
  HealthMinuteData minute_data;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "cadence_handler: Enter");
    
  // Update value
  s_ticks++;
  
  // Get the number of steps since start time
  time_t time_now = time(NULL);  
  numRcvd = health_service_get_minute_history(&minute_data, 1, &last_time, &time_now);
  last_time = time(NULL);
  
  if (numRcvd == 0 || minute_data.is_invalid) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No health data to use");
    goto done;
  }
  
  s_steps_total += minute_data.steps;
  
  // Construct a data packet
  AppWorkerMessage msg_data = {
    .data0 = minute_data.steps,
    .data1 = s_steps_total
  };

  // Send the data to the foreground app
  app_worker_send_message(STEPS, &msg_data);

done:
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick Handler: Exit");
}

static void cadence_init() {
  last_time = time(NULL);
  // Use the TickTimer Service as a data source
  tick_timer_service_subscribe(MINUTE_UNIT, cadence_handler);
}

static void cadence_deinit() {
  // Stop using the TickTimerService
  tick_timer_service_unsubscribe();
}

int main(void) {
  cadence_init();
  worker_event_loop();
  cadence_deinit();
  return 0;
}