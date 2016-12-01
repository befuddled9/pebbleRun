#include <pebble_worker.h>

#define WORKER_TICKS 0

static uint16_t s_ticks = 0;
static time_t start_time;
static time_t last_time;

static void tick_handler(struct tm *tick_timer, TimeUnits units_changed) {
  int numRcvd = 0;
  
  HealthMinuteData minute_data;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick Handler: Enter");
    
  // Update value
  s_ticks++;
  
  // Get the number of steps since start time
  time_t time_now = time(NULL);
  char time_str[80];
  strftime(time_str, 80, "%T", localtime(&last_time));
  APP_LOG(APP_LOG_LEVEL_INFO, "Last time : %s", time_str);
  
  strftime(time_str, 80, "%T", localtime(&time_now));
  APP_LOG(APP_LOG_LEVEL_INFO, "Time now: %s", time_str);
  
  numRcvd = health_service_get_minute_history(&minute_data, 1, &last_time, &time_now);
  last_time = time(NULL);
  
  APP_LOG(APP_LOG_LEVEL_INFO, "numRcvd: %d", numRcvd);
  APP_LOG(APP_LOG_LEVEL_INFO, "Steps: %d", minute_data.steps);

  if (minute_data.is_invalid) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health data is invalid!");
    goto done;
  }
  
  // Construct a data packet
  AppWorkerMessage msg_data = {
    .data0 = s_ticks,
    .data1 = minute_data.steps
  };

  // Send the data to the foreground app
  app_worker_send_message(WORKER_TICKS, &msg_data);

done:
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick Handler: Exit");
}

static void worker_init() {
  start_time = time(NULL);
  last_time = time(NULL);
  // Use the TickTimer Service as a data source
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void worker_deinit() {
  // Stop using the TickTimerService
  tick_timer_service_unsubscribe();
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
}