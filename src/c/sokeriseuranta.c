// Sokeriseuranta Pebble Watchface 
// Copyright (C) 2017  Mika Haulo
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <pebble.h>

static Window *s_window;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static TextLayer *s_glucose_value;
static TextLayer *s_glucose_unit;
static TextLayer *s_current_glucose_time;

// Persistent storage key
#define SETTINGS_KEY 1

// Define our settings struct
typedef struct ClaySettings {
  char UserEmail[128];
  char AccessToken[64];
} ClaySettings;

static ClaySettings settings;

static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void load_settings() {
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read API credentials from settings
  
  Tuple *user_email_tuple = dict_find(iterator, MESSAGE_KEY_UserEmail);
  static char user_email_buffer[128];
  
  if(user_email_tuple) {
    snprintf(user_email_buffer, sizeof(user_email_buffer), "%s", user_email_tuple->value->cstring);
  }

  Tuple *access_token_tuple = dict_find(iterator, MESSAGE_KEY_AccessToken);
  static char access_token_buffer[64];
  
  if(access_token_tuple) {
    snprintf(access_token_buffer, sizeof(access_token_buffer), "%s", access_token_tuple->value->cstring);
  }
  
  // Save settings, they are needed below
  save_settings();
  
  
  // Update glucose info
  
  static char current_glucose_buffer[8];
  static char current_glucose_timestamp_buffer[8];

  Tuple *current_glucose_tuple = dict_find(iterator, MESSAGE_KEY_CURRENT_GLUCOSE);
  Tuple *current_glucose_timestamp_tuple = dict_find(iterator, MESSAGE_KEY_CURRENT_GLUCOSE_TIMESTAMP);

  if(current_glucose_tuple && current_glucose_timestamp_tuple) {
    snprintf(current_glucose_buffer, sizeof(current_glucose_buffer), "%s", current_glucose_tuple->value->cstring);
    text_layer_set_text(s_glucose_value, current_glucose_buffer);
    
    snprintf(current_glucose_timestamp_buffer, sizeof(current_glucose_timestamp_buffer), "%s", current_glucose_timestamp_tuple->value->cstring);
    text_layer_set_text(s_current_glucose_time, current_glucose_timestamp_buffer);
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


static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%d.%m.%y", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
  
  static char s_time_buffer[16];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H.%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get glucose update every 6 minutes
  if (tick_time->tm_min % 5 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    dict_write_uint8(iter, 0, 0);

    app_message_outbox_send();
  }
}

static void init(void) {
  load_settings();
  
	// Create the main window
	s_window = window_create();
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(s_window, GColorBlack);
	
  // Date layer
	s_date_layer = text_layer_create(bounds);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
	text_layer_set_text(s_date_layer, "01.01.17");
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_date_layer));
  text_layer_enable_screen_text_flow_and_paging(s_date_layer, 10);
  
  // Time layer
	s_time_layer = text_layer_create(bounds);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
	text_layer_set_text(s_time_layer, "00.00");
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_time_layer));
  text_layer_enable_screen_text_flow_and_paging(s_time_layer, 10);
  
  // Value layer
  s_glucose_value = text_layer_create(GRect(0, 60, bounds.size.w, 50));
  text_layer_set_background_color(s_glucose_value, GColorClear);
  text_layer_set_text_color(s_glucose_value, GColorClear);
  text_layer_set_font(s_glucose_value, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_glucose_value, GTextAlignmentCenter);
	text_layer_set_text(s_glucose_value, "...");
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_glucose_value));
  text_layer_enable_screen_text_flow_and_paging(s_glucose_value, 10);
  
  // Unit layer
  s_glucose_unit = text_layer_create(GRect(0, 135, bounds.size.w, 25));
  text_layer_set_background_color(s_glucose_unit, GColorClear);
  text_layer_set_text_color(s_glucose_unit, GColorClear);
  text_layer_set_font(s_glucose_unit, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_glucose_unit, GTextAlignmentLeft);
	text_layer_set_text(s_glucose_unit, "mmol/l");
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_glucose_unit));
  text_layer_enable_screen_text_flow_and_paging(s_glucose_unit, 10);
  
  // Current glucose value time
	s_current_glucose_time = text_layer_create(GRect(0, 135, bounds.size.w, 25));
  text_layer_set_background_color(s_current_glucose_time, GColorClear);
  text_layer_set_text_color(s_current_glucose_time, GColorClear);
  text_layer_set_font(s_current_glucose_time, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_current_glucose_time, GTextAlignmentRight);
	text_layer_set_text(s_current_glucose_time, "00.00");
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_current_glucose_time));
  text_layer_enable_screen_text_flow_and_paging(s_current_glucose_time, 10);
	

	// Push the window, setting the window animation to 'true'
	window_stack_push(s_window, true);
  
  // Register with TimeTickerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Initial time update
  update_time();
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");
}

static void deinit(void) {
  text_layer_destroy(s_date_layer);
	text_layer_destroy(s_time_layer);
  text_layer_destroy(s_glucose_value);
  text_layer_destroy(s_glucose_unit);
  text_layer_destroy(s_current_glucose_time);
	
	window_destroy(s_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
