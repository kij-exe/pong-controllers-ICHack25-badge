#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

#include "button_module.h"

// define the UART and GPIO pins to use for communication with the ESP-01
#define UART_ID uart1
#define BAUD_RATE 115200
// using the board's UART1
#define UART_TX_PIN 8
#define UART_RX_PIN 9

#define INPUT_BUFFER_SIZE 256

void send_at_command(const char* cmd) {
#ifdef DEBUG
    printf("Sending command: %s\n", cmd);
#endif
    uart_puts(UART_ID, cmd);
    uart_puts(UART_ID, "\r\n");
}

#define RESPONSE_BUFFER_SIZE 256

bool wait_for_esp_response(const char* expected_response, uint32_t timeout) {
    char response_buf[RESPONSE_BUFFER_SIZE];
    int response_pos = 0;

    // gets timestamp RESPONSE_TIMEOUT after current time
    absolute_time_t timeout_time = make_timeout_time_ms(timeout);

    // wait for the response
    while (absolute_time_diff_us(get_absolute_time(), timeout_time) > 0) {
        if (uart_is_readable(UART_ID)) {
            char ch = uart_getc(UART_ID);
#ifdef DEBUG
            putchar(ch); // echo ESP response to the USB serial for debugging
#endif
            // guard from buffer overrun
            if (response_pos < (RESPONSE_BUFFER_SIZE - 1)) {
                response_buf[response_pos++] = ch;
                response_buf[response_pos] = '\0'; // keep it null-terminated
            }

            // Check if the expected response is in our buffer
            if (strstr(response_buf, expected_response) != NULL) {
#ifdef DEBUG
                printf("\nFound expected response '%s'\n", expected_response);
#endif
                return true;
            }
        }
    }

    // timeout exit
#ifdef DEBUG
    printf("\nTIMEOUT: Did not receive '%s' within %lu ms\n", expected_response, timeout);
#endif
    return false;
}

void process_usb_serial_in(char *buf, int length) {
    send_at_command(buf);
}

bool read_usb_serial_in(char *buf, int buf_size, int *next_char_pos) {
    // try reading the character immediately
    int c = getchar_timeout_us(0);
    bool command_read = false;

    // a character was received.
    if (c != PICO_ERROR_TIMEOUT) {
        if (c == '\n' || c == '\r') {
            putchar('\n');
            // end of line received
            buf[*next_char_pos] = '\0'; // null-terminate the string
            command_read = true;
        } else {
            putchar(c);
            // add character to buffer (with overflow check)
            if (*next_char_pos < buf_size) {
                buf[(*next_char_pos)++] = (char)c;
            }
        }
    }
    return command_read;
}

#define SERVER_LINK_ID "0"

#define SEND_BYTE "AT+CIPSEND="SERVER_LINK_ID",1"

#define REQUEST_INTERVAL 20 // ms

#define SET_MODE "AT+CWMODE=1"

#define SSID "C-project-45"
#define PASSWORD "password"
#define CONNECT_NETWORK "AT+CWJAP=\""SSID"\",\""PASSWORD"\""

#define MODE_SETTING "AT+CIPMUX=1"

#define CHECK_LINK "AT+CIPSTATUS"
#define ALREADY_CONNECTED "+CIPSTATUS:"SERVER_LINK_ID",\"UDP\""

#define CHECK_NETWORK "AT+CWJAP?"

#define REMOTE_ADDRESS "192.168.157.37"
#define REMOTE_PORT "8888"
#define LOCAL_PORT "8888"
#define OPEN_LINK "AT+CIPSTART="SERVER_LINK_ID",\"UDP\",\""REMOTE_ADDRESS"\","REMOTE_PORT","LOCAL_PORT",0"

#define RECONNECT_INTERVAL 3000

void setup_network_connection(void) {
    // attempt network connection until it succeeds
    while (1) {
        sleep_ms(RECONNECT_INTERVAL);

        send_at_command(SET_MODE);
        if (!wait_for_esp_response("OK", 2000)) continue;

        // check if already connected to the required network
        send_at_command(CHECK_NETWORK);
        // connect only if not yet connected
        if (!wait_for_esp_response(SSID, 1000)) {
            send_at_command(CONNECT_NETWORK);
            if (!wait_for_esp_response("OK", 20000)) continue;
        }
        
        send_at_command(MODE_SETTING);
        if (!wait_for_esp_response("OK", 2000)) continue;

        // only create link connection if it has not yet been established
        send_at_command(CHECK_LINK);
        if (!wait_for_esp_response(ALREADY_CONNECTED, 1000)) {
            send_at_command(OPEN_LINK);
            if (!wait_for_esp_response("OK", 5000)) continue;
        }

        process_buttons();
        break;
    }
    
#ifdef DEBUG
    printf("\nSETUP COMPLETE: Sending data\n");
#endif
}

void send_byte(uint byte_data) {
#ifdef DEBUG
    printf("\nSending Data Packet\n");
#endif  
    send_at_command(SEND_BYTE);

    if (wait_for_esp_response(">", 2000)) {
#ifdef DEBUG
        printf("Sending byte: b'%d'\n", byte_data);
#endif  
        uart_putc_raw(UART_ID, byte_data);

        wait_for_esp_response("SEND OK", 5000);
    }
}

void initialise_ports(void) {
    // initialize standard I/O for printf() over USB
    stdio_init_all();
    
    // give time to open the serial monitor
    sleep_ms(5000);
#ifdef DEBUG
    printf("Type 'start' to setup network connection:\n");
#endif

    // initialize the UART for the ESP-01
    uart_init(UART_ID, BAUD_RATE);
    
    // set the GPIO pins to UART function
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // small delay to let the ESP-01 boot up
    sleep_ms(1000);
    
#ifdef DEBUG
    // flush initial garbage input 
    while (uart_is_readable(UART_ID)) {
        char ch = uart_getc(UART_ID);
        putchar(ch);
    }
    putchar('\n');
#endif
}

#define BUTTON_CHECK_INTERVAL 5 // ms

int main() {
    initialise_ports();

    initialise_buttons();

    char input_buffer[INPUT_BUFFER_SIZE];
    int next_char_pos = 0;
    
    // create a repeating timer that calls the callback function every 5ms
    struct repeating_timer timer; // timer needs to be in main to not get destroyed
    add_repeating_timer_ms(BUTTON_CHECK_INTERVAL, &check_buttons_callback, NULL, &timer); 

#ifdef DEBUG
    // first wait for "start" to input
    while (1) {
        bool command_read = read_usb_serial_in(input_buffer, INPUT_BUFFER_SIZE, &next_char_pos);
        if (command_read) {
            next_char_pos = 0; // reset buffer for the next command
            if (strncmp(input_buffer, "start", 5) == 0) {
                break;
            }
        }
    }
#endif
    
    setup_network_connection();
    
    uint64_t last_request_time_ms = 0;

    while (1) {
#ifdef DEBUG
        // check if there is data to read from the ESP-01
        while (uart_is_readable(UART_ID)) {
            // read one character and print it to the USB serial monitor
            char ch = uart_getc(UART_ID);
            putchar(ch);
        }
#endif
        process_buttons();
        
        uint64_t current_time_ms = time_us_64() / 1000;

        if (current_time_ms - last_request_time_ms > REQUEST_INTERVAL) {
            send_byte(get_buttons_state());
            last_request_time_ms = current_time_ms;
        }
    }

    return 0;
}
