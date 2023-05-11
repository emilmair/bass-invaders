#include <math.h>

#include <mes.h>
#include <gpu.h>
#include <timer.h>
#include <input.h>
#include <rng.h>

#include "graphics.h"
#include "maths.h"
#include "strings.h"

#include "guitarist.m3if.asset"
#include "singer.m3if.asset"
#include "pianist.m3if.asset"
#include "drummer.m3if.asset"
#include "bassist.m3if.asset"
#include "bass.m3if.asset"
#include "levels.m3if.asset"
#include "title.m3if.asset"

#define LEVEL 0 // this is the level already completed by the user (0 = no level completed)
#define HS 123456
// todo: replace this with real data once vmes implements the data header

#define PLAYERX_MAX 74
#define PLAYERY 94
#define BLOCKY h(8)
#define BLOCKX 4

// debug
#define FPS_OVERLAY false
#define SHOW_INTRO false

// palette
#define WHITE 0b000
#define BLACK 0b001
#define DARK_RED 0b010
#define MEDIUM_RED 0b011
#define LIGHT_RED 0b100
#define DARK_BLUE 0b101
#define MEDIUM_BLUE 0b110
#define LIGHT_BLUE 0b111

uint8_t exit_code = CODE_EXIT;

uint8_t x(uint8_t factor) { return 8+factor*6; }
uint8_t y(uint8_t factor) { return 8+factor*8; }
uint8_t w(uint8_t factor) { return factor*6; }
uint8_t h(uint8_t factor) { return factor*8; }

// palette testing
uint16_t palette[8] = {COLOR_TO_GPIO(0b111, 0b111, 0b111), \
                       COLOR_TO_GPIO(0b000, 0b000, 0b000), \
                       COLOR_TO_GPIO(0b110, 0b010, 0b010), \
                       COLOR_TO_GPIO(0b111, 0b011, 0b011), \
                       COLOR_TO_GPIO(0b111, 0b101, 0b101), \
                       COLOR_TO_GPIO(0b011, 0b011, 0b101), \
                       COLOR_TO_GPIO(0b100, 0b100, 0b110), \
                       COLOR_TO_GPIO(0b101, 0b101, 0b111)};

uint32_t start_level(uint8_t level) {
    gpu_blank(FRONT_BUFFER, WHITE);

    gpu_print_text(FRONT_BUFFER, x(0), y(0), BLACK, WHITE, "LEVEL");
    gpu_print_text(FRONT_BUFFER, x(6), y(0), BLACK, WHITE, INLINE_DECIMAL2(level));
    gpu_print_text(FRONT_BUFFER, x(0), y(2), BLACK, WHITE, "HI-SCORE");
    gpu_print_text(FRONT_BUFFER, x(0), y(3), BLACK, WHITE, INLINE_DECIMAL8(HS));
    gpu_print_text(FRONT_BUFFER, x(0), y(5), BLACK, WHITE, "SCORE");
    gpu_print_text(FRONT_BUFFER, x(0), y(8), BLACK, WHITE, "WAVE  /");
    gpu_print_text(FRONT_BUFFER, x(7), y(8), BLACK, WHITE, INLINE_DECIMAL1(ceil((double)level/8)));

    Surface space = surf_create(w(14), h(13));
    Surface singer = surf_create_from_memory(10, 10, ASSET_SINGER_M3IF);
    Surface guitarist = surf_create_from_memory(10, 10, ASSET_GUITARIST_M3IF);
    Surface pianist = surf_create_from_memory(10, 10, ASSET_PIANIST_M3IF);
    Surface drummer = surf_create_from_memory(10, 10, ASSET_DRUMMER_M3IF);
    Surface bassist = surf_create_from_memory(10, 10, ASSET_BASSIST_M3IF);
    Surface bass = surf_create_from_memory(w(2), h(3), ASSET_BASS_M3IF);
    Surface levels = surf_create_from_memory(10, 192, ASSET_LEVELS_M3IF);
    Surface* enemies[4] = {&singer, &guitarist, &pianist, &drummer};
    surf_fill(&space, WHITE);

    uint8_t playerx = 0;
    uint32_t score = 1000;
    uint8_t wave = 1; // current wave, maximum number of waves is level/8
//    uint8_t left = level*4; // enemies left, max 256 at level 64
    uint8_t lives = (64-level)/16; // lives LEFT, game over when 0 and player dies
    uint8_t last_lives = lives;
    gpu_send_buf(FRONT_BUFFER, bass.w, bass.h, x(8), y(10), bass.d);
    for (int i = 0; i < lives; i++) {
        gpu_send_buf(FRONT_BUFFER, bass.w, bass.h, x(0) + w(2)*i+i, y(10), bass.d);
    }

    List px = list_create(0); // projectiles x coordinates
    List py = list_create(0); // projectiles y coordinates
    uint8_t cooldown = 0;

    List epx = list_create(0); // enemy projectiles x coordinates
    List epy = list_create(0); // enemy projectiles y coordinates
    uint8_t ep_cooldown = 0;

    List ex = list_create(0); // enemy x coordinates
    List ey = list_create(0); // enemy y coordinates
    List et = list_create(0); // enemy type

    List bx = list_create(0); // block x coordinates
    List by = list_create(0); // block y coordinates

    for (int i = 0; i < level*4; i++) {
        list_append(&et, rng_u32()%4); // type
        list_append(&ex, (w(14)/(level*4) * i) + (w(14)/(level*4))/2); // x position
        list_append(&ey, 20); // yt position
    }

    uint8_t block_level = level;
    if (level > 32) block_level = level - 32;
    bool flip = level > 32; // simply invert the block arrangement after 32 levels
    for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 6; j++) {
            uint8_t posx = i;
            if (i > 9) posx = 18-i;
            uint8_t posy = j+(block_level-1)*6;
            if (flip) posy = 5-j+(block_level-1)*6;
            if (surf_get_pixel(&levels,posx,posy) == BLACK) {
                list_append(&bx, BLOCKX + i*4 + 2);
                list_append(&by, BLOCKY + j*4 + 2);
            }
        }
    }

    uint32_t start = 0;
    uint32_t stop = 0;
    uint32_t deltatime = 0;

    bool run = true;
    while (run) {
        // handle input
        if (input_get_button(0, BUTTON_LEFT)) if (playerx > 0) playerx--;
        if (input_get_button(0, BUTTON_RIGHT)) if (playerx < PLAYERX_MAX) playerx++;

        // shoot projectiles
        if (input_get_button(0, BUTTON_A) && cooldown == 0) {
            list_append(&px, playerx+5);
            list_append(&py, PLAYERY);
            cooldown += 10;
        }
        if (ep_cooldown == 0) {
            uint8_t e = rng_u32()%et.size;
            list_append(&epx, ex.data[e]);
            list_append(&epy, ey.data[e]+5);
            ep_cooldown = rng_u32()%(640/level);
        }

        // process player projectiles
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

        // process enemy projectiles
        if (ep_cooldown > 0) ep_cooldown--;
        for (int i = 0; i < epx.size; i++) {
            if (epy.data[i] == space.height) {
                list_remove(&epx, i);
                list_remove(&epy, i);
                i--;
            } else {
                epy.data[i] += 1;
            }
        }

        // clear canvas
        surf_fill(&space, WHITE);

        // draw hitboxes
        for (int i = 0; i < et.size; i++) {
            surf_draw_filled_rectangle(&space, ex.data[i]-5, ey.data[i]-5, 10, 10, DARK_RED); // enemy hitboxes in dark red
        }
        for (int i = 0; i < bx.size; i++) {
            surf_draw_filled_rectangle(&space, bx.data[i]-2, by.data[i]-2, 4, 4, MEDIUM_BLUE); // block hitboxes in medium red
        }
        surf_draw_filled_rectangle(&space, playerx, PLAYERY, 10, 10, LIGHT_RED); // player in light red

        // check collision and draw player projectiles
        for (int proj = 0; proj < px.size; proj++) {
            uint8_t projx = px.data[proj];
            uint8_t projy = py.data[proj];
            uint8_t pixel = surf_get_pixel(&space, projx, projy);
            if (pixel == DARK_RED) {
                // enemy collision
                for (int enemy = 0; enemy < et.size; enemy++) {
                    if (difference(ex.data[enemy], projx) <= 5 && difference(ey.data[enemy], projy) <= 5) {
                        surf_draw_filled_rectangle(&space, ex.data[enemy]-5, ey.data[enemy]-5, 10, 10, WHITE);
                        list_remove(&et, enemy);
                        list_remove(&ex, enemy);
                        list_remove(&ey, enemy);
                        score += 100;
                        if (et.size == 0) {
                            // victory
                            run = false;
                        }
                        break;
                    }
                }
                list_remove(&px, proj);
                list_remove(&py, proj);
                proj--;
            } else if (pixel == MEDIUM_BLUE) {
                // block collision
                for (int block = 0; block < bx.size; block++) {
                    int8_t diffx = projx - bx.data[block];
                    int8_t diffy = projy - by.data[block];
                    if (diffx >= -2 && diffx <= 1 && diffy >= -2 && diffy <= 1) {
                        surf_draw_filled_rectangle(&space, bx.data[block]-2, by.data[block]-2, 4, 4, WHITE);
                        list_remove(&bx, block);
                        list_remove(&by, block);
                        score -= 10;
                        break;
                    }
                }
                list_remove(&px, proj);
                list_remove(&py, proj);
                proj--;
            } else {
                // no collision
                surf_set_pixel(&space, projx, projy, BLACK);
            }
        }

        // check collision and draw enemy projectiles
        for (int proj = 0; proj < epx.size; proj++) {
            uint8_t projx = epx.data[proj];
            uint8_t projy = epy.data[proj];
            uint8_t pixel = surf_get_pixel(&space, projx, projy);
            if (pixel == LIGHT_RED) {
                // player collision
                if (lives == 0) {
                    // defeat
                    run = false;
                } else lives--;
                list_remove(&epx, proj);
                list_remove(&epy, proj);
                proj--;
            } else if (pixel == MEDIUM_BLUE) {
                // block collision
                for (int block = 0; block < bx.size; block++) {
                    int8_t diffx = projx - bx.data[block];
                    int8_t diffy = projy - by.data[block];
                    if (diffx >= -2 && diffx <= 1 && diffy >= -2 && diffy <= 1) {
                        surf_draw_filled_rectangle(&space, bx.data[block]-2, by.data[block]-2, 4, 4, WHITE);
                        list_remove(&bx, block);
                        list_remove(&by, block);
                        score -= 10;
                        break;
                    }
                }
                list_remove(&epx, proj);
                list_remove(&epy, proj);
                proj--;
            } else {
                // no collision
                surf_set_pixel(&space, projx, projy, BLACK);
            }
        }

        // render enemies
        for (int i = 0; i < et.size; i++) {
            surf_draw_surf_fast(&space, enemies[et.data[i]], ex.data[i]-5, ey.data[i]-5);
        }

        // render player and cable
        surf_draw_surf_fast(&space, &bassist, playerx, PLAYERY);
        surf_draw_line(&space, playerx, PLAYERY+2, 0, 101, BLACK);

        // timing
        stop = timer_get_ms();
        deltatime = stop - start;
        gpu_block_frame();
        start = timer_get_ms();

        // render HUD
        gpu_print_text(FRONT_BUFFER, x(0), y(6), BLACK, WHITE, INLINE_DECIMAL8(score));
        gpu_print_text(FRONT_BUFFER, x(5), y(8), BLACK, WHITE, INLINE_DECIMAL1(wave));
        if (lives != last_lives) {
            gpu_send_buf(FRONT_BUFFER, w(2), h(3), x(2*lives)+lives, y(10), space.d);
        }
        last_lives = lives;
        if (FPS_OVERLAY) {
            gpu_print_text(FRONT_BUFFER, 160-6*3, 0, WHITE, BLACK, INLINE_DECIMAL3(deltatime));
        }

        // render canvas
        gpu_send_buf(FRONT_BUFFER, space.w, space.h, x(10), y(0), space.d);

        // wait for everything to be sent
        gpu_block_ack();
    }

    // cleanup
    surf_destroy(&space);
    list_destroy(&px);
    list_destroy(&py);
    list_destroy(&epx);
    list_destroy(&epy);
    list_destroy(&ex);
    list_destroy(&ey);
    list_destroy(&et);
    list_destroy(&bx);
    list_destroy(&by);

    return score;
}

void render_menu(uint8_t level) {
    gpu_print_text(FRONT_BUFFER, x(1), y(3), BLACK, WHITE, FONT_ARROWLEFT " PAGE x " FONT_ARROWRIGHT);
    gpu_print_text(FRONT_BUFFER, x(8), y(3), BLACK, WHITE, INLINE_DECIMAL1(level/8+1));
    for (int i = 0; i < 8; i ++) {
        uint8_t color = BLACK;
        if (i == level % 8) color = DARK_RED;
        gpu_print_text(FRONT_BUFFER, x(1), y(5)+8*i, color, WHITE, "LEVEL");
        gpu_print_text(FRONT_BUFFER, x(7), y(5)+8*i, color, WHITE, INLINE_DECIMAL2(((level/8)*8)+i+1));
        gpu_print_text(FRONT_BUFFER, x(10), y(5+i), color, WHITE, " ");
    }
    gpu_print_text(FRONT_BUFFER, x(10), y(5+level%8), BLACK, WHITE, "<");
}


uint8_t menu() {
    gpu_blank(FRONT_BUFFER, 0);
    Surface title = surf_create_from_memory(w(22), h(2), ASSET_TITLE_M3IF);

    gpu_send_buf(FRONT_BUFFER, w(22), h(2), x(1), y(0), title.data);
    gpu_print_text(FRONT_BUFFER, x(12), y(11), BLACK, WHITE, "DIFFICULTY:");
    gpu_print_text(FRONT_BUFFER, x(12), y(8), BLACK, WHITE, "HIGHSCORE:");
    gpu_print_text(FRONT_BUFFER, x(12), y(5), BLACK, WHITE, "STATUS:");

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
    return level+1;
}

uint8_t start(void) {
    // init
    rng_init();
    gpu_update_palette(palette);
    gpu_blank(FRONT_BUFFER, 0);

    // intro sequence
    if (SHOW_INTRO) {
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
    }

    // logic loop
    while(true) {start_level(menu());}
    return exit_code;
}
