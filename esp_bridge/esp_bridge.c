#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

// define the UART and GPIO pins to use for communication with the ESP-01
#define UART_ID uart1
#define BAUD_RATE 115200
// using the board's UART1
#define UART_TX_PIN 8
#define UART_RX_PIN 9

#define INPUT_BUFFER_SIZE 256

void send_at_command(const char* cmd) {
    printf("Sending command: %s\n", cmd);
    uart_puts(UART_ID, cmd);
    uart_puts(UART_ID, "\r\n");
}

void process_usb_serial_in(char *buf, int length) {
    send_at_command(buf);
}

void read_usb_serial_in(char *buf, int buf_size, int *next_char_pos) {
    // try reading the character immediately
    int c = getchar_timeout_us(0);

    // a character was received.
    if (c != PICO_ERROR_TIMEOUT) {
        if (c == '\n' || c == '\r') {
            putchar('\n');
            // end of line received
            buf[*next_char_pos] = '\0'; // null-terminate the string
            process_usb_serial_in(buf, *next_char_pos);

            *next_char_pos = 0; // reset buffer for the next command
        } else {
            putchar(c);
            // add character to buffer (with overflow check)
            if (*next_char_pos < buf_size) {
                buf[(*next_char_pos)++] = (char)c;
            }
        }
    }
}

int main() {
    // Initialize standard I/O for printf() over USB
    stdio_init_all();
    
    // Give us time to open the serial monitor
    sleep_ms(5000);
    printf("RP2040 to ESP-01 AT Command Tester\n");

    // Initialize the UART for the ESP-01
    uart_init(UART_ID, BAUD_RATE);
    
    // Set the GPIO pins to UART function
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Small delay to let the ESP-01 boot up if it was just powered on
    sleep_ms(1000);
    
    // Flash initial garbage input 
    while (uart_is_readable(UART_ID)) {
        char ch = uart_getc(UART_ID);
        putchar(ch);
    }
    putchar('\n');

    char input_buffer[INPUT_BUFFER_SIZE];
    int next_char_pos = 0;

    while (1) {
        read_usb_serial_in(input_buffer, INPUT_BUFFER_SIZE, &next_char_pos);
        
        // Check if there is data to read from the ESP-01
        while (uart_is_readable(UART_ID)) {
            // Read one character and print it to the USB serial monitor
            char ch = uart_getc(UART_ID);
            putchar(ch);
        }
    }

    return 0;
}
