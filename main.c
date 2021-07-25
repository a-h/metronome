#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "numbers.h"

// https://github.com/matiasilva/pico-examples/tree/oled-i2c/i2c/oled_i2c
// Commands (see SSD1306 datasheet)
#define OLED_SET_CONTRAST _u(0x81)
#define OLED_SET_ENTIRE_ON _u(0xA4)
#define OLED_SET_NORM_INV _u(0xA6)
#define OLED_SET_DISP _u(0xAE)
#define OLED_SET_MEM_ADDR _u(0x20)
#define OLED_SET_COL_ADDR _u(0x21)
#define OLED_SET_PAGE_ADDR _u(0x22)
#define OLED_SET_DISP_START_LINE _u(0x40)
#define OLED_SET_SEG_REMAP _u(0xA0)
#define OLED_SET_MUX_RATIO _u(0xA8)
#define OLED_SET_COM_OUT_DIR _u(0xC0)
#define OLED_SET_DISP_OFFSET _u(0xD3)
#define OLED_SET_COM_PIN_CFG _u(0xDA)
#define OLED_SET_DISP_CLK_DIV _u(0xD5)
#define OLED_SET_PRECHARGE _u(0xD9)
#define OLED_SET_VCOM_DESEL _u(0xDB)
#define OLED_SET_CHARGE_PUMP _u(0x8D)
#define OLED_SET_HORIZ_SCROLL _u(0x26)
#define OLED_SET_SCROLL _u(0x2E)

#define OLED_ADDR _u(0x3C)
#define OLED_HEIGHT _u(32)
#define OLED_WIDTH _u(128)
#define OLED_PAGE_HEIGHT _u(8)
#define OLED_NUM_PAGES OLED_HEIGHT / OLED_PAGE_HEIGHT
#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_WIDTH)

#define OLED_WRITE_MODE _u(0xFE)
#define OLED_READ_MODE _u(0xFF)

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};

void calc_render_area_buflen(struct render_area* area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void oled_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command

    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = { 0x80, cmd };
    i2c_write_blocking(i2c_default, (OLED_ADDR & OLED_WRITE_MODE), buf, 2, false);
}

void oled_send_buf(uint8_t buf[], int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    // TODO find a more memory-efficient way to do this..
    // maybe break the data transfer into pages?
    uint8_t temp_buf[buflen + 1];
    for (int i = 1; i < buflen + 1; i++) {
        temp_buf[i] = buf[i - 1];
    }
    // Co = 0, D/C = 1 => the driver expects data to be written to RAM
    temp_buf[0] = 0x40;
    i2c_write_blocking(i2c_default, (OLED_ADDR & OLED_WRITE_MODE), temp_buf, buflen + 1, false);
}

void oled_init() {
    oled_send_cmd(OLED_SET_DISP | 0x00); // set display off

    /* memory mapping */
    oled_send_cmd(OLED_SET_MEM_ADDR); // set memory address mode
    oled_send_cmd(0x00); // horizontal addressing mode

    /* resolution and layout */
    oled_send_cmd(OLED_SET_DISP_START_LINE); // set display start line to 0

    oled_send_cmd(OLED_SET_SEG_REMAP | 0x01); // set segment re-map
    // column address 127 is mapped to SEG0

    oled_send_cmd(OLED_SET_MUX_RATIO); // set multiplex ratio
    oled_send_cmd(OLED_HEIGHT - 1); // our display is only 32 pixels high

    oled_send_cmd(OLED_SET_COM_OUT_DIR | 0x08); // set COM (common) output scan direction
    // scan from bottom up, COM[N-1] to COM0

    oled_send_cmd(OLED_SET_DISP_OFFSET); // set display offset
    oled_send_cmd(0x00); // no offset

    oled_send_cmd(OLED_SET_COM_PIN_CFG); // set COM (common) pins hardware configuration
    oled_send_cmd(0x02); // manufacturer magic number

    /* timing and driving scheme */
    oled_send_cmd(OLED_SET_DISP_CLK_DIV); // set display clock divide ratio
    oled_send_cmd(0x80); // div ratio of 1, standard freq

    oled_send_cmd(OLED_SET_PRECHARGE); // set pre-charge period
    oled_send_cmd(0xF1); // Vcc internally generated on our board

    oled_send_cmd(OLED_SET_VCOM_DESEL); // set VCOMH deselect level
    oled_send_cmd(0x30); // 0.83xVcc

    /* display */
    oled_send_cmd(OLED_SET_CONTRAST); // set contrast control
    oled_send_cmd(0xFF);

    oled_send_cmd(OLED_SET_ENTIRE_ON); // set entire display on to follow RAM content

    oled_send_cmd(OLED_SET_NORM_INV); // set normal (not inverted) display

    oled_send_cmd(OLED_SET_CHARGE_PUMP); // set charge pump
    oled_send_cmd(0x14); // Vcc internally generated on our board

    oled_send_cmd(OLED_SET_SCROLL | 0x00); // deactivate horizontal scrolling if set
    // this is necessary as memory writes will corrupt if scrolling was enabled

    oled_send_cmd(OLED_SET_DISP | 0x01); // turn display on
}

void render(uint8_t buf[], struct render_area* area) {
    // update a portion of the display with a render area
    oled_send_cmd(OLED_SET_COL_ADDR);
    oled_send_cmd(area->start_col);
    oled_send_cmd(area->end_col);

    oled_send_cmd(OLED_SET_PAGE_ADDR);
    oled_send_cmd(area->start_page);
    oled_send_cmd(area->end_page);

    oled_send_buf(buf, area->buflen);
}

void clear_oled() {
    // initialize render area for entire frame (128 pixels by 4 pages)
    struct render_area frame_area = { start_col: 0, end_col : OLED_WIDTH - 1, start_page : 0, end_page : OLED_NUM_PAGES - 1 };
    calc_render_area_buflen(&frame_area);

    // zero the entire display
    uint8_t buf[OLED_BUF_LEN];
    for (int i = 0; i < OLED_BUF_LEN; i++) {
        buf[i] = 0x00;
    }
    render(buf, &frame_area);
}

// Code for the rotary encoder.
// Adapted from https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
struct rotary_encoder_state {
    // Pins.
    uint clk;
    uint dt;
    uint sw;

    // Data.
    int counter;
    int currentStateCLK;
    int lastStateCLK;
    int currentDir;
    unsigned long lastButtonPress;
};

struct rotary_encoder_state rotary_init(uint clk, uint dt, uint sw) {
    gpio_set_function(clk, GPIO_IN);
    gpio_set_function(dt, GPIO_IN);
    gpio_pull_up(sw);
    struct rotary_encoder_state state = {
        .clk = clk,
        .dt = dt,
        .sw = sw,
        .lastStateCLK = gpio_get(clk),
    };
    return state;
}

bool rotary_update(struct rotary_encoder_state* state) {
    // Read the current state of CLK.
    state->currentStateCLK = gpio_get(state->clk);

    // If last and current state of CLK are different, then a pulse occurred.
    // React to only 1 state change to avoid double count.
    if (state->currentStateCLK != state->lastStateCLK && state->currentStateCLK == 1){
            // If the DT state is different than the CLK state then
            // the encoder is rotating CW so increment.
            if (gpio_get(state->dt) != state->currentStateCLK) {
                    state->counter ++;
                    state->currentDir = +1;
            } else {
                    // Encoder is rotating CCW so decrement.
                    state->counter --;
                    state->currentDir = -1;
            }
    }

    // Remember last CLK state.
    state->lastStateCLK = state->currentStateCLK;

    // Read the button state.
    bool pressed = false;

    // If we detect LOW signal, button is pressed.
    if (gpio_get(state->sw) == 0) {
            //if 50ms have passed since last LOW pulse, it means that the
            //button has been pressed, released and pressed again
            long now = time_us_64();
            pressed = (now - state->lastButtonPress) > 50000;

            // Remember last button press event.
            state->lastButtonPress = now;
    }

    // Put in a slight delay to help debounce the reading
    sleep_ms(1);
    return pressed;
}

int main() {
    stdio_init_all();

    // I2C is "open drain", pull ups to keep signal high when no data is being
    // sent
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    oled_init();
    clear_oled();

    // Draw numbers.
    for(int i = 0; i < 10; i ++) {
        int start = (i * FONT_WIDTH) + i;
        int end = start + FONT_WIDTH - 1;
        struct render_area font_tile_area = { .start_col = start, .end_col = end, .start_page = 0, .end_page = 0 };
        calc_render_area_buflen(&font_tile_area);
        render(numbers[i], &font_tile_area);
    }

    // Initiate the rotary encoder.
    struct rotary_encoder_state encoder_state = rotary_init(13, 12, 11); // GP13, GP12, GP11. Physical 17, 16, 15.
    
    long lastCounter;
    while(true) {
        bool pressed = rotary_update(&encoder_state);
        if(pressed) {
             printf("Pressed!\n");
        }
        if(encoder_state.counter != lastCounter) {
             printf("Counter updated: %d\n", encoder_state.counter);
        }
        lastCounter = encoder_state.counter;
    }

    return 0;
}
