#include "pebble.h"

static Window *window;

static ScrollLayer *scroll_layer;
static TextLayer *text_layer;

AppTimer *command_timer = NULL;

char text_buffer[4096];
char final_buffer[8196];
//char *text_buffer = (char *)malloc(1024);

int current_level = 0, current_slot = 0, current_command = 0;

char recv_buffer[1024];

void send_requestx(int slot, int command);

static char content[] = "\
test text\n\
\
";

/**************************/

enum {
	KEY_TEST = 0,
	KEY_TEXT = 1,
	KEY_TEXT_APPEND = 2,
	KEY_TEXT_CLEAR = 3,
};

/***************************/

void process_tuple(Tuple *t)
{
	//Get key
	int key = t->key;

	//Get integer value, if present
	int value = t->value->int32;

	//Get string value, if present
	strcpy(recv_buffer, t->value->cstring);

	//Decide what to do
	switch(key) {
		case KEY_TEST:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: %s", "Javascript");
			snprintf(text_buffer, sizeof(recv_buffer), "%s", recv_buffer);
			text_layer_set_text(text_layer, (char*) &text_buffer);
			break;
			
		case KEY_TEXT:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: %s", "stuff received");
			snprintf(text_buffer, sizeof(recv_buffer), "%s", recv_buffer);
			text_layer_set_text(text_layer, (char*) &text_buffer);
			break;

		case KEY_TEXT_APPEND:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: %s", "Append request received");
			snprintf(text_buffer, sizeof(recv_buffer), "%s", recv_buffer);
			strcat(final_buffer, text_buffer);
			text_layer_set_text(text_layer, (char*) &final_buffer);
			break;

		case KEY_TEXT_CLEAR:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: %s", "CLEAR request received");
			text_layer_set_text(text_layer, (char*)"");
			break;

		case 9999:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: %s", "Shit received");
			snprintf(text_buffer, sizeof(recv_buffer), "%s", recv_buffer);
			text_layer_set_text(text_layer, (char*) &text_buffer);
			break;
	}
}

static void in_received_handler(DictionaryIterator *iter, void *context)
{
	(void) context;
	
	//Get data
	Tuple *t = dict_read_first(iter);
	if(t)
	{
		process_tuple(t);
	}
	
	//Get next
	while(t != NULL)
	{
		t = dict_read_next(iter);
		if(t)
		{
			process_tuple(t);
		}
	}
}

static void in_dropped_handler(AppMessageResult reason, void *context)
{
	APP_LOG(APP_LOG_LEVEL_ERROR, "Dropped message! Reason given: %i", reason);
}

/*old implementation*/
/*void send_int(uint8_t key, uint8_t cmd)
{
	DictionaryIterator *iter;
 	app_message_outbox_begin(&iter);
 	
 	Tuplet value = TupletInteger(key, cmd);
 	dict_write_tuplet(iter, &value);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: %d %d", key, cmd);
 	
 	app_message_outbox_send();
}*/

static void handle_resend(void* data)
{
	command_timer = NULL;
	send_requestx(current_slot, current_command);
}

void send_requestx(int slot, int command)
{
	AppMessageResult result;
	DictionaryIterator *dict;
	result = app_message_outbox_begin(&dict);

	if (result == APP_MSG_OK)
	{
		dict_write_uint8(dict, slot, command);
		dict_write_end(dict);
		result = app_message_outbox_send();

		if (result == APP_MSG_OK)
		{
			app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		}
	}
	else
	{
		current_slot = slot;
		current_command = command;
		if (command_timer == NULL)
		{
			command_timer = app_timer_register(250, handle_resend, NULL);
		}
	}
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context)
{
	//Window *window = (Window *)context; // This context defaults to the window, but may be changed with \ref window_set_click_context.
	text_layer_set_text(text_layer, "Middle Button has been Clicked!");
	send_requestx(5,5);
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context)
{
	Window *window = (Window *)context;
	text_layer_set_text(text_layer, "Middle Button has been Clicked for long!");
}

void message_click_config_provider(void* context) {
	//config[BUTTON_ID_SELECT]->click.handler = message_click;
	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler, 0);
}

static void window_load(Window *window)
{
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	GRect max_text_bounds = GRect(1, 0, bounds.size.w, 3000);

	scroll_layer = scroll_layer_create(bounds);

	scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks){
		.click_config_provider = message_click_config_provider
	});

	scroll_layer_set_click_config_onto_window(scroll_layer, window);


	// Initialize the text layer
	text_layer = text_layer_create(max_text_bounds);
	text_layer_set_text(text_layer, content);

	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

	// Trim text layer and scroll content to fit text box
	GSize max_size = text_layer_get_content_size(text_layer);
	text_layer_set_size(text_layer, max_size);

	int vert_content_padding = 1;
	scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, max_size.h + vert_content_padding));

	// Add the layers for display
	scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_layer));

	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));
}

static void window_unload(Window *window) 
{
	text_layer_destroy(text_layer);
	scroll_layer_destroy(scroll_layer);
}

int main(void)
{
	window = window_create();
	window_set_fullscreen(window, true);
	window_set_window_handlers(window, (WindowHandlers) 
	{
		.load = window_load,
		.unload = window_unload,
	});

	//Register AppMessage events
	app_message_register_inbox_received(in_received_handler);
	//app_message_register_inbox_dropped();
				 
	const int inbound_size = app_message_inbox_size_maximum();
	const int outbound_size = 1024;
	app_message_open(inbound_size, outbound_size);
	
	window_stack_push(window, false);

	//window_set_click_config_provider(window, (ClickConfigProvider) config_provider);	

	app_event_loop();

	window_destroy(window);
}

