#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static SimpleMenuLayer *menu_layer;
static SimpleMenuItem menu_items[100];
static SimpleMenuSection menu_section[1];

static void menu_select_callback(int index, void *contex) {
    
    menu_items[index].subtitle = "Running...";
    layer_mark_dirty(simple_menu_layer_get_layer(menu_layer));
    
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    Tuplet value = TupletInteger(1, index);
    dict_write_tuplet(iter, &value);
    app_message_outbox_send();
}

static void create_menu_with_data(DictionaryIterator *received){
    int i = 1;
    
    Tuple *text_tuple = dict_find(received, i);
    
    while (text_tuple != NULL){
        menu_items[i-1] = (SimpleMenuItem){
            .title = text_tuple->value->cstring,
            .callback = menu_select_callback
        };
        
        i = i + 1;
        text_tuple = dict_find(received, i);
    }
    
    menu_section[0] = (SimpleMenuSection){
        .items = menu_items,
        .num_items = i-1
    };
    
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    menu_layer = simple_menu_layer_create(bounds, window, menu_section, 1, NULL);
    
    layer_remove_from_parent((Layer*)text_layer);
    layer_add_child(window_layer, simple_menu_layer_get_layer(menu_layer));
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
    // outgoing message was delivered
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    // outgoing message failed
}

enum{
    kCommandMessageSceneData = 101,
    kCommandMessageSceneExecutionComplete = 102,
    kCommandMessageSceneExecutionError = 103,
    kCommandMessageConnectionFailure = 104
};

void in_received_handler(DictionaryIterator *received, void *context) {
    text_layer_set_text(text_layer, "Recieved Data...");
    
    Tuple *command_tuple = dict_find(received, 0);
    
    unsigned int command_message = command_tuple->value->uint8;
    
    switch (command_message) {
        case kCommandMessageSceneData:{
            create_menu_with_data(received);
            break;
        }
        case kCommandMessageSceneExecutionComplete:{
            Tuple *index_tuple = dict_find(received, 1);
            int index = index_tuple->value->uint8;
            menu_items[index].subtitle = "Completed.";
            layer_mark_dirty(simple_menu_layer_get_layer(menu_layer));
            break;
        }
        case kCommandMessageSceneExecutionError:{
            Tuple *index_tuple = dict_find(received, 1);
            int index = index_tuple->value->uint8;
            menu_items[index].subtitle = "Error.";
            layer_mark_dirty(simple_menu_layer_get_layer(menu_layer));
            break;
        }
        case kCommandMessageConnectionFailure:{
            text_layer_set_text(text_layer, "Cannot Find Vera.");
            break;
        }
        default:
            break;
    }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
    // incoming message dropped
}



static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "Loading...");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
    window = window_create();
    //window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
    });
    const bool animated = true;
    window_stack_push(window, animated);

    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_sent(out_sent_handler);
    app_message_register_outbox_failed(out_failed_handler);

    const uint32_t inbound_size = app_message_inbox_size_maximum();
    const uint32_t outbound_size = app_message_outbox_size_maximum();
    app_message_open(inbound_size, outbound_size);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
