/*

MIT License

Copyright (c) 2022 Oliver Schmidt (https://a2retro.de/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include "pico/printf.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "bus.pio.h"

void board();

#ifdef TRACE
void uart_printf(uart_inst_t *uart, const char *format, ...) {
    static char buffer[0x100];

    va_list va;
    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);

    buffer[0xFF] = '\0';
    uart_puts(uart, buffer);
}
#endif

int main() {
    stdio_init_all();
    stdio_set_translate_crlf(&stdio_usb, false);

#ifdef TRACE
    uart_init(uart0, 115200);
    uart_set_translate_crlf(uart0, true);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
#endif

    for (uint gpio = gpio_addr; gpio < gpio_addr + size_addr; gpio++) {
        gpio_init(gpio);
        gpio_set_pulls(gpio, false, false);  // floating
    }

    for (uint gpio = gpio_data; gpio < gpio_data + size_data; gpio++) {
        pio_gpio_init(pio0, gpio);
        gpio_set_pulls(gpio, false, false);  // floating
    }

    gpio_init(gpio_enbl);
    gpio_set_pulls(gpio_enbl, false, false); // floating

    gpio_init(gpio_irq);
    gpio_pull_up(gpio_irq);

    gpio_init(gpio_rdy);
    gpio_pull_up(gpio_rdy);

    gpio_init(gpio_led);
    gpio_set_dir(gpio_led, GPIO_OUT);

    uint offset;

    offset = pio_add_program(pio0, &enbl_program);
    enbl_program_init(offset);

    offset = pio_add_program(pio0, &write_program);
    write_program_init(offset);

    offset = pio_add_program(pio0, &read_program);
    read_program_init(offset);

    multicore_launch_core1(board);

    while (true) {
        if (multicore_fifo_rvalid()) {
            uint32_t data = multicore_fifo_pop_blocking();
            putchar(data);
#ifdef TRACE
            uart_printf(uart0, "> %02X\n", data);
#endif
        }

        if (multicore_fifo_wready()) {
            int data = getchar_timeout_us(0);
            if (data != PICO_ERROR_TIMEOUT) {
                multicore_fifo_push_blocking(data);
#ifdef TRACE
                uart_printf(uart0, "< %02X\n", data);
#endif
            }
        }
    }

    return 0;
}