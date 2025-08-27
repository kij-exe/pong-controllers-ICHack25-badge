
extern void initialise_buttons();

// returns a byte representing current state of the buttons
// the code is UP LEFT DOWN RIGHT
// 0 - button released, 1 - button pressed
// e.g., 1000 - UP pressed, all other released
extern uint get_buttons_state();

extern void process_buttons();

extern bool check_buttons_callback(struct repeating_timer *t);
