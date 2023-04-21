#include <math.h>

#include <mes.h>
#include <gpu.h>
#include <timer.h>
#include <input.h>
#include <rng.h>

#include "graphics.h"
#include "maths.h"
#include "strings.h"

#define LEVEL 0 // this is the level already completed by the user (0 = no level completed)
#define HS 123456
// todo: replace this with real data once vmes implements the data header

#define PLAYERX_MAX 74
#define PLAYERY 80

#define BACKGROUND 0b111

bool run = true;
uint8_t exit_code = CODE_EXIT;

uint8_t x(uint8_t factor) { return 8+factor*6; }
uint8_t y(uint8_t factor) { return 8+factor*8; }
uint8_t w(uint8_t factor) { return factor*6; }
uint8_t h(uint8_t factor) { return factor*8; }

uint32_t start_level(uint8_t level) {
    // init
    rng_init();

    gpu_blank(BACK_BUFFER, 0);
    gpu_blank(FRONT_BUFFER, 0);

    gpu_print_text(FRONT_BUFFER, x(0), y(0), 0b001, 0b000, "LEVEL");
    gpu_print_text(FRONT_BUFFER, x(6), y(0), 0b001, 0b000, INLINE_DECIMAL2(level));
    gpu_print_text(FRONT_BUFFER, x(0), y(2), 0b001, 0b000, "HI-SCORE");
    gpu_print_text(FRONT_BUFFER, x(0), y(3), 0b001, 0b000, INLINE_DECIMAL8(HS));
    gpu_print_text(FRONT_BUFFER, x(0), y(5), 0b001, 0b000, "SCORE");
    gpu_print_text(FRONT_BUFFER, x(0), y(8), 0b001, 0b000, "WAVE  /");
    gpu_print_text(FRONT_BUFFER, x(7), y(8), 0b001, 0b000, INLINE_DECIMAL1(ceil((double)level/8)));

    Surface space = surf_create(w(14), h(13));
    Surface singer = surf_create(10, 10);
    Surface guitarist = surf_create(10, 10);
    Surface pianist = surf_create(10, 10);
    Surface drummer = surf_create(10, 10);
    Surface bassist = surf_create(10, 10);
    Surface bass = surf_create(w(2), h(3));
    Surface* enemies[4] = {&singer, &guitarist, &pianist, &drummer};

    surf_fill(&singer, 0b001);
    surf_fill(&guitarist, 0b010);
    surf_fill(&pianist, 0b011);
    surf_fill(&drummer, 0b100);
    surf_fill(&space, BACKGROUND);
    surf_fill(&bassist, 0b001);
    surf_fill(&bass, 0b011);

    uint8_t playerx = 0;
    uint32_t score = 0;
    uint8_t wave = 1; // current wave, maximum number of waves is level/8
//    uint8_t left = level*4; // enemies left, max 256 at level 64
    uint8_t lives = (64-level)/16; // lives LEFT, game over when 0 and player dies

    List px = list_create(0); // projectiles x coordinates
    List py = list_create(0); // projectiles y coordinates
    uint8_t cooldown = 0;

    List ex = list_create(0);
    List ey = list_create(0);
    List et = list_create(0);

    for (int i = 0; i < level*4; i++) {
        list_append(&et, rng_u32()%4); // type
        list_append(&ex, (w(14)/(level*4) * i) + (w(14)/(level*4))/2); // x position
        list_append(&ey, 20); // yt position
    }

    while (true) {
        // handle input
        if (input_get_button(0, BUTTON_LEFT)) if (playerx > 0) playerx--;
        if (input_get_button(0, BUTTON_RIGHT)) if (playerx < PLAYERX_MAX) playerx++;
        if (input_get_button(0, BUTTON_A) && cooldown == 0) {
            list_append(&px, playerx+5);
            list_append(&py, PLAYERY);
            cooldown += 15;
        }
        if (cooldown > 0) cooldown--;
        for (int i = 0; i < px.size; i++) {
            if (py.data[i] == 0) {
                list_remove(&px, i);
                list_remove(&py, i);
                i--;
            } else {
                py.data[i] -= 1;
            }
        }

        // render HUD
        gpu_print_text(FRONT_BUFFER, x(0), y(6), 0b001, 0b000, INLINE_DECIMAL8(score));
        gpu_print_text(FRONT_BUFFER, x(5), y(8), 0b001, 0b000, INLINE_DECIMAL1(wave));
        if (lives != 0) {
            for (int i = 0; i < lives; i++) {
                gpu_send_buf(FRONT_BUFFER, bass.w, bass.h, x(0) + w(2)*i, y(10), bass.d);
            }
        }
        gpu_send_buf(FRONT_BUFFER, bass.w, bass.h, x(8), y(10), bass.d);

        // render player
        surf_fill(&space, BACKGROUND);
        surf_draw_surf(&space, &bassist, playerx, PLAYERY);
        surf_draw_line(&space, playerx, PLAYERY+5, 0, 100, 0b001);

        // check enemy collision and draw projectiles
        for (int i = 0; i < et.size; i++) {
            surf_draw_filled_rectangle(&space, ex.data[i]-5, ey.data[i]-5, 10, 10, 0b101); // hitboxes in yellow
        }
        for (int proj = 0; proj < px.size; proj++) {
            uint8_t projx = px.data[proj];
            uint8_t projy = py.data[proj];
            if (surf_get_pixel(&space, projx, projy) == 0b101) {
                // collision
                for (int enemy = 0; enemy < et.size; enemy++) {
                    if (difference(ex.data[enemy], projx) <= 5 && difference(ey.data[enemy], projy) <= 5) {
                        surf_draw_filled_rectangle(&space, ex.data[enemy]-5, ey.data[enemy]-5, 10, 10, BACKGROUND);
                        list_remove(&et, enemy);
                        list_remove(&ex, enemy);
                        list_remove(&ey, enemy);
                        break;
                    }
                }
                list_remove(&px, proj);
                list_remove(&py, proj);
                proj--;
            } else {
                // no collision
                surf_set_pixel(&space, projx, projy, 0b001);
            }

        }

        // render enemies
        for (int i = 0; i < et.size; i++) {
            surf_draw_surf(&space, enemies[et.data[i]], ex.data[i]-5, ey.data[i]-5);
        }

        // send all to gpu
        gpu_send_buf(FRONT_BUFFER, space.w, space.h, x(10), y(0), space.d);

        timer_block_ms(20); // 50 fps
    }

    // cleanup
    surf_destroy(&space);
    surf_destroy(&singer);
    surf_destroy(&guitarist);
    surf_destroy(&pianist);
    surf_destroy(&drummer);
    surf_destroy(&bassist);

    return score;
}

void render_menu(uint8_t level) {
    gpu_print_text(FRONT_BUFFER, x(1), y(3), 0b001, 0b000, FONT_ARROWLEFT " PAGE x " FONT_ARROWRIGHT);
    gpu_print_text(FRONT_BUFFER, x(8), y(3), 0b001, 0b000, INLINE_DECIMAL1(level/8+1));
    for (int i = 0; i < 8; i ++) {
        gpu_print_text(FRONT_BUFFER, x(1), y(5)+8*i, 0b001, 0b000, "LEVEL");
        gpu_print_text(FRONT_BUFFER, x(7), y(5)+8*i, 0b001, 0b000, INLINE_DECIMAL2(((level/8)*8)+i+1));
        gpu_print_text(FRONT_BUFFER, x(10), y(5+i), 0b001, 0b000, " ");
    }
    gpu_print_text(FRONT_BUFFER, x(10), y(5+level%8), 0b001, 0b000, "<");
}


uint8_t menu() {
    gpu_blank(FRONT_BUFFER, 0);

    Surface title = surf_create(w(22), h(2));
    surf_fill(&title, 0b001);
    gpu_send_buf(FRONT_BUFFER, w(22), h(2), x(1), y(0), title.data);
    gpu_print_text(FRONT_BUFFER, x(12), y(11), 0b001, 0b000, "DIFFICULTY:");
    gpu_print_text(FRONT_BUFFER, x(12), y(8), 0b001, 0b000, "HIGHSCORE:");
    gpu_print_text(FRONT_BUFFER, x(12), y(5), 0b001, 0b000, "STATUS:");

    render_menu(0);
    uint8_t level, last_level, last_up, last_down, last_left, last_right, up, down, left, right;
    level = last_level = last_up = last_down = last_left = last_right = up = down = left = right = 0;
    bool play = false;
    while (!play) {
        if (input_get_button(0, BUTTON_START)) play = true;
        up = input_get_button(0, BUTTON_UP);
        down = input_get_button(0, BUTTON_DOWN);
        left = input_get_button(0, BUTTON_LEFT);
        right = input_get_button(0, BUTTON_RIGHT);

        if (!last_up && up) level = (uint8_t)(level-1)%64;
        if (!last_down && down) level = (uint8_t)(level+1)%64;
        if (!last_left && left) level = (uint8_t)(level-8)%64;
        if (!last_right && right) level = (uint8_t)(level+8)%64;
        if (level != last_level) render_menu(level);

        last_level = level;
        last_up = up; last_down = down; last_left = left; last_right = right;
        timer_block_ms(16);
    }
    surf_destroy(&title);
    return level+1;
}

uint8_t start(void) {
    //startup sequence
    gpu_blank(FRONT_BUFFER, 0);
    timer_block_ms(1000);
    if (!input_get_button(0, BUTTON_A)) { //skip
        gpu_print_text(FRONT_BUFFER, 25, 30, 0b001, 0b000, "YOUR PLAY THE BASS.");
        timer_block_ms(1000);
        gpu_print_text(FRONT_BUFFER, 33, 50, 0b001, 0b000, "THE OTHERS NEVER");
        gpu_print_text(FRONT_BUFFER, 30, 60, 0b001, 0b000, "LET YOU PLAY YOUR");
        gpu_print_text(FRONT_BUFFER, 36, 70, 0b001, 0b000, "EPIC BASS SOLO.");
        timer_block_ms(3000);
        gpu_blank(FRONT_BUFFER, 0b000);
        timer_block_ms(1000);
        gpu_print_text(FRONT_BUFFER, 35, 50, 0b001, 0b000, "DEFEND YOURSELF!");
        timer_block_ms(1000);
        gpu_blank(FRONT_BUFFER, 0b000);
        timer_block_ms(1000);
    }

    // logic loop
    while(run) {start_level(menu());}
    return exit_code;
}
