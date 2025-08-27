#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "button_module.h"

// INPUT BUTTONS
#define BUTTON_UP 0
#define BUTTON_DOWN 3
#define BUTTON_LEFT 2
#define BUTTON_RIGHT 1

#define PRESS_TIMEOUT (100 * 1000) // microseconds, i.e., 100ms

typedef enum {
    BUTTON_PRESSED, // corresponds to falling edge read
    BUTTON_RELEASED // corresponds to rising edge read
} button_state;

struct button {
    const uint gpio;
    char *name;
    // state read on the last button state check
    button_state last_read_state;
    // current stable state
    button_state stable_state;
    // counts consecutive number of reads of last_read_state
    bool has_changed;
    int read_counter;
};
typedef struct button *button;

// the number of consecutive reads required to declare the button state stable
#define READ_COUNTER_CAP 5

struct button button_left = {BUTTON_LEFT, "BUTTON_LEFT"};
struct button button_right = {BUTTON_RIGHT, "BUTTON_RIGHT"};
struct button button_up = {BUTTON_UP, "BUTTON_UP"};
struct button button_down = {BUTTON_DOWN, "BUTTON_DOWN"};

button buttons[] = {&button_right, &button_down, &button_left, &button_up};
const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

button_state read_state(button b) {
    return gpio_get(b->gpio) ? BUTTON_RELEASED : BUTTON_PRESSED;
}

void press_button(button curr_button) {
    printf("%s pressed\n", curr_button->name);
}

void release_button(button curr_button) {
    printf("%s released\n", curr_button->name);
}

bool check_buttons_callback(struct repeating_timer *t) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_state curr_state = read_state(buttons[i]);
        if (curr_state != buttons[i]->last_read_state) {
            // initial switch or bounce
            buttons[i]->read_counter = 0;
        }
        else if (buttons[i]->read_counter < READ_COUNTER_CAP) {
            // consecutive read detected
            buttons[i]->read_counter++;
        }
        else if (curr_state != buttons[i]->stable_state) {
            // consecutive read detected, read_counter reached its cap
            // but stable_state is different
            buttons[i]->stable_state = curr_state;
            // button stable state switch detected, call the callback
            buttons[i]->has_changed = true;
        }
        buttons[i]->last_read_state = curr_state;
    }
}

void initialise_buttons() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_init(buttons[i]->gpio);
        gpio_set_dir(buttons[i]->gpio, GPIO_IN);
        gpio_pull_up(buttons[i]->gpio);
        
        // initialise state variables
        buttons[i]->last_read_state = read_state(buttons[i]); // Set initial state
        buttons[i]->stable_state = buttons[i]->last_read_state;
        buttons[i]->has_changed = false;
        buttons[i]->read_counter = 0;
    }
}

uint get_buttons_state() {
    uint state = 0;
    // for each button set ith bit of the state if the button is pressed
    // RIGHT - bit 0; DOWN - bit 1; LEFT - bit 2; UP - bit 3
    for (int i = 0; i < 4; i++) {
        state |= (buttons[i]->stable_state == BUTTON_PRESSED) << i;
    }
    return state;
}

void process_buttons() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        // if the button state has_changed, call the corresponding callback
        // and reset the has_changed flag
        if (buttons[i]->has_changed) {
            switch (buttons[i]->stable_state) {
                case BUTTON_PRESSED:
                    press_button(buttons[i]);
                    break;
                case BUTTON_RELEASED:
                    release_button(buttons[i]);
                    break;
            }
            buttons[i]->has_changed = false;
        }
    }
}
