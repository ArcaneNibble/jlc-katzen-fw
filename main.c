#include <string.h>
#include "hardware/clocks.h"
#include "hardware/gpio.h"
// #include "hardware/pio.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/unique_id.h"

#include "nec_transmit.h"
#include "nec_receive.h"

#define NUM_LEDS 16
char led_buf[12 + 4 * NUM_LEDS];

void set_led(int i, int level, int r, int g, int b) {
    led_buf[4 + i * 4 + 0] = (0xe0 | level) ^ 0xff;
    led_buf[4 + i * 4 + 1] = b ^ 0xff;
    led_buf[4 + i * 4 + 2] = g ^ 0xff;
    led_buf[4 + i * 4 + 3] = r ^ 0xff;
}

int main() {
    pico_unique_board_id_t uid;
    pico_get_unique_board_id(&uid);
    uint8_t addr = 0;
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
        addr ^= uid.id[i];

    spi_init(spi0, 1 * 1000 * 1000);
    spi_set_format(spi0, 8, true, false, SPI_MSB_FIRST);
    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);

    int ir_tx_sm = nec_tx_init(pio0, 10);           // uses two state machines, 16 instructions and one IRQ
    int ir_rx_sm = nec_rx_init(pio0, 9);            // uses one state machine and 9 instructions

    // int div_for_ir = clock_get_hz(clk_peri) / 38000;

    // // gpio_set_function(10, GPIO_FUNC_PWM);
    // uint ir_pwm_slice = pwm_gpio_to_slice_num(10);
    // pwm_set_clkdiv(ir_pwm_slice, 1);
    // pwm_set_wrap(ir_pwm_slice, div_for_ir);
    // pwm_set_chan_level(ir_pwm_slice, PWM_CHAN_A, 0);
    // pwm_set_enabled(ir_pwm_slice, true);

    // // pwm_set_chan_level(ir_pwm_slice, PWM_CHAN_A, div_for_ir / 2);

    // gpio_init(9);
    // gpio_set_dir(9, GPIO_IN);
    gpio_pull_up(9);

    /*memset(led_buf, 0xff, sizeof(led_buf));
    set_led(0, 9, 255, 0, 0);
    set_led(1, 9, 0, 255, 0);
    set_led(2, 9, 0, 0, 255);
    set_led(3, 9, 255, 0, 0);
    set_led(4, 9, 0, 255, 0);
    set_led(5, 9, 0, 0, 255);
    set_led(6, 9, 255, 0, 0);
    set_led(7, 9, 0, 255, 0);
    set_led(8, 9, 0, 0, 255);
    set_led(9, 9, 255, 0, 0);
    set_led(10, 9, 0, 255, 0);
    set_led(11, 9, 0, 0, 255);
    set_led(12, 9, 255, 0, 0);
    set_led(13, 9, 0, 255, 0);
    set_led(14, 9, 0, 0, 255);
    set_led(15, 9, 255, 0, 0);
    spi_write_blocking(spi0, led_buf, sizeof(led_buf));*/

    memset(led_buf, 0xff, sizeof(led_buf));
    for (int led = 0; led < NUM_LEDS; led++)
        set_led(led, 0, 0, 0, 0);
    /*set_led(0, 9, 255, 0, 0);
    set_led(1, 9, 0, 255, 0);
    set_led(2, 9, 0, 0, 255);
    spi_write_blocking(spi0, led_buf, sizeof(led_buf));

    busy_wait_ms(1000);

    // memset(led_buf, 0xff, sizeof(led_buf));
    for (int led = 0; led < NUM_LEDS; led++)
        set_led(led, 0, 0, 0, 0);
    set_led(3, 9, 255, 0, 0);
    set_led(4, 9, 0, 255, 0);
    set_led(5, 9, 0, 0, 255);
    spi_write_blocking(spi0, led_buf, sizeof(led_buf));

    while (1) {}*/

    // int xxx_ir = 0;
    // int xxx_ir_loops = 0;
    int ir_loops = 0;
    int led_start_i = 0;
    while (1) {
        for (int led = 0; led < NUM_LEDS; led++)
            set_led(led, 0, 0, 0, 0);
        set_led((led_start_i + 0) % NUM_LEDS, 1, 255, 0, 0);
        set_led((led_start_i + 1) % NUM_LEDS, 1, 0, 255, 0);
        set_led((led_start_i + 2) % NUM_LEDS, 1, 0, 0, 255);
        spi_write_blocking(spi0, led_buf, sizeof(led_buf));

        if (ir_loops == 10) {
            uint32_t tx_frame = nec_encode_frame(addr, led_start_i);
            pio_sm_put(pio0, ir_tx_sm, tx_frame);
            ir_loops = 0;
        } else {
            ir_loops++;
        }

        // gpio_put(10, xxx_ir);
        /*if (xxx_ir)
            pwm_set_chan_level(ir_pwm_slice, PWM_CHAN_A, div_for_ir / 2);
        else
            pwm_set_chan_level(ir_pwm_slice, PWM_CHAN_A, 0);*/

        /*if (xxx_ir_loops == 20) {
            xxx_ir = !xxx_ir;
            xxx_ir_loops = 0;
        } else {
            xxx_ir_loops++;
        }

        if (gpio_get(9)) {
            led_start_i = (led_start_i + 1) % NUM_LEDS;
        }*/

        led_start_i = (led_start_i + 1) % NUM_LEDS;

        while (!pio_sm_is_rx_fifo_empty(pio0, ir_rx_sm)) {
            uint32_t rx_frame = pio_sm_get(pio0, ir_rx_sm);
            uint8_t rx_address, rx_data;

            if (nec_decode_frame(rx_frame, &rx_address, &rx_data)) {
                if (rx_address != addr) {
                    // got something, not from self
                    led_start_i = ((led_start_i + rx_data) / 2) % NUM_LEDS;
                }
            }
        }

        busy_wait_ms(100);

        // if (xxx_ir)
        //     pwm_set_chan_level(ir_pwm_slice, PWM_CHAN_A, div_for_ir / 2);
        // else
        //     pwm_set_chan_level(ir_pwm_slice, PWM_CHAN_A, 0);
        
        // xxx_ir = !xxx_ir;
        // busy_wait_us(600);
    }

    return 0;
}
