
// глобальные переменные
// ...

#include "defer.h"

#include <assert.h>
#include "array.h"

#include "SDL.h"
#include "SDL_ttf.h"
#include <atomic>
#include <chrono>
#include <string>
#include <cmath>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u64atom = std::atomic_uint64_t;

static constexpr int DEFAULT_SAMPLES = 512;
static constexpr int RECORD_BUFFER_SIZE = DEFAULT_SAMPLES * 128;

struct Devices {
    char const* play_device   = NULL;
    char const* record_device = NULL;
};

// struct Rendering {
SDL_Window   *window;
int win_w, win_h;
SDL_Renderer *renderer;
TTF_Font     *font;
SDL_AudioSpec spec;

void set_default_spec(SDL_AudioSpec *spec);
bool init_sdl();
void close_sdl();

struct Field_Rect {
    s64 cx = 0, cy = 0;
    s64 w2 = (1LL << 19), h2 = 2*(1LL << 19);
};

struct Field_Screen_Rect {
    int cx = 750, cy = 475;
    int w2 = 225, h2 = 450;
};

struct Point {
    s64 x, y;
};

struct Vector {
    Vector() {}
    Vector(s64 x, s64 y) : x(x), y(y) {}
    s64 x, y;
};

struct Hero {
    s64 step_x = (1 << 12), step_y = (1 << 12);
    int size = 16;
    s64 hitbox = (1 << 13);
    SDL_Texture *image_of_hitbox = NULL;
    SDL_Texture *image = NULL;
    SDL_Texture *still_animation[3];
    int n_still_animation = 3;
    Point positions[60*60*60];
};


struct Bullet {
    Point p;
    int size;
};

struct Key_State {
    bool pressed       = false;
    bool just_pressed  = false;
    bool just_released = false;
};

struct Keys {
    Key_State up, down, left, right;
    Key_State rewind;
};

struct Linear_Start {
    Point  p0;
    Vector delta;
    s64 t0, t1;
};

#define MAX_BULLETS 10000
struct Linear_Bullets {
    Linear_Start starts[MAX_BULLETS];
    Point currents[MAX_BULLETS];
    int n_starts = 0;
    s64 size;
};

// struct Game_State {
SDL_Texture *image_of_circle = NULL;
Hero hero;
bool dead = false;
Keys keys;
s64 current_time = 0;
Linear_Bullets linear_bullets;
Field_Rect field;
Field_Screen_Rect screen_field;
double step_period = 1.0/60.0;

s64 points_per_pixel() {
    return field.w2 / screen_field.w2;
}

int point_to_pixel_x(s64 point) {
    return screen_field.cx + (int)(point/points_per_pixel());
}

int point_to_pixel_y(s64 point) {
    return screen_field.cy + (int)(point/points_per_pixel());
}

s64 pixel_to_point_x(int pixel) {
    s64 n = points_per_pixel();
    int x = pixel - screen_field.cx;
    return x * n;
}

s64 pixel_to_point_y(int pixel) {
    s64 n = points_per_pixel();
    int y = pixel - screen_field.cy;
    return y * n;
}

Point *linear_pos(s64 t, int i) {
    Linear_Start *start = &linear_bullets.starts[i];
    if (t < start->t0 || t > start->t1) return NULL;

    Point *current = &linear_bullets.currents[i];
    current->x = start->p0.x + (t - start->t0) * start->delta.x;
    current->y = start->p0.y + (t - start->t0) * start->delta.y;
    return current;
}

// Bullet bullets[MAX_BULLETS];
// int    n_bullets;
//
// void bullets_remove(Bullet *b) {
//     if ((b - bullets) >= n_bullets) return;
//     n_bullets -= 1;
//     *b = bullets[n_bullets];
// }

void key_pressed(Key_State* k) {
    if (!k->pressed) {
        k->pressed      = true;
        k->just_pressed = true;
    } else {
        k->just_pressed = false;
    }
}

void key_released(Key_State* k) {
    if (k->pressed) {
        k->pressed       = false;
        // хотим ли снимать just_pressed тут?
        k->just_released = true;
    } else {
        k->just_released = true;
    }
}

void pause() {
    SDL_Event event;
    for (;;) {
        if (SDL_PollEvent(&event)) {
            // TODO сделать версию с keydown
            if (event.type == SDL_KEYUP) {
                break;
            } else if (event.type == SDL_QUIT) {
                return;
            }
        }
    }
}

SDL_Color s32_to_color(s32 n) {
    SDL_Color c;
    u8 * n_bytes = (u8 *) &n;
    c.r = n_bytes[2];
    c.g = n_bytes[1];
    c.b = n_bytes[0];
    c.a = 255;
    return c;
}

struct Screen_Point {
    int x, y;
};

Screen_Point coords_to_screen(Point const& p) {
    return {point_to_pixel_x(p.x), point_to_pixel_y(p.y)};
}

Point screen_to_coords(Screen_Point p) {
    if (p.x < 0 || p.y < 0 || p.x >= win_w || p.y >= win_h) {
        SDL_Log("BAD p = %d %d", p.x, p.y);
        p.x = win_w / 2;
        p.y = win_h / 2;
    }
    return {pixel_to_point_x(p.x), pixel_to_point_y(p.y)};
}

int revers(int x, int middle) {
    if (x <= middle) return x;
    return middle + (middle - x);
}

void draw_hero() {
    Screen_Point screen_p = coords_to_screen(hero.positions[current_time]);
    SDL_Rect rect;
    rect.x = screen_p.x - hero.size;
    rect.y = screen_p.y - hero.size;
    rect.w = 2*hero.size;
    rect.h = 2*hero.size;
    SDL_Color c = s32_to_color(0x0FFFF0);
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
    SDL_RenderFillRect(renderer, &rect);

    if (!hero.image) {
    } else {
        SDL_Rect image_rect = {0, 0, 256, 300};
        SDL_Rect hero_rect = image_rect;
        hero_rect.w /= 2;
        hero_rect.h /= 2;
        hero_rect.x = screen_p.x - hero_rect.w/2;
        hero_rect.y = screen_p.y - hero_rect.h/2;
        SDL_RenderCopy(renderer, hero.still_animation[revers((current_time/2) % (hero.n_still_animation*2 - 1), hero.n_still_animation - 1)], &image_rect, &hero_rect);
    }


    SDL_Rect hitbox_rect;
    hitbox_rect.x = screen_p.x - (int)(hero.hitbox/points_per_pixel());
    hitbox_rect.y = screen_p.y - (int)(hero.hitbox/points_per_pixel());
    hitbox_rect.w = 2*(int)(hero.hitbox/points_per_pixel());
    hitbox_rect.h = 2*(int)(hero.hitbox/points_per_pixel());
    SDL_Rect image_rect = {0, 0, 16, 16};
    SDL_RenderCopy(renderer, hero.image_of_hitbox, &image_rect, &hitbox_rect);
}

void draw_bullet(Bullet *b) {
    Screen_Point screen_p = coords_to_screen(b->p);
//    SDL_Rect rect;
//    int thickness = 3;
//    rect.x = screen_p.x - (b->size + thickness);
//    rect.y = screen_p.y - (b->size + thickness);
//    rect.w = 2*(b->size + thickness);
//    rect.h = 2*(b->size + thickness);
//    // c = s32_to_color(0xFF0000);
//    SDL_SetRenderDrawColor(renderer, 194, 92, 85, 255);
//    // SDL_Log("rect %d %d %d %d", rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
//    SDL_RenderFillRect(renderer, &rect);
//    rect.x = screen_p.x - b->size;
//    rect.y = screen_p.y - b->size;
//    rect.w = 2*b->size;
//    rect.h = 2*b->size;
//    SDL_Color c;
//    c = s32_to_color(0xFFFFFF);
//    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
//    // SDL_Log("%d", b->size);
//    // SDL_Log("rect %d %d %d %d", rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
//    SDL_RenderFillRect(renderer, &rect);

    SDL_Rect hitbox_rect;
    hitbox_rect.x = screen_p.x - b->size;
    hitbox_rect.y = screen_p.y - b->size;
    hitbox_rect.w = 2*b->size;
    hitbox_rect.h = 2*b->size;
    SDL_Rect image_rect = {0, 0, 200, 200};
    SDL_RenderCopy(renderer, image_of_circle, &image_rect, &hitbox_rect);
//    {
//        SDL_Rect hitbox_rect;
//        hitbox_rect.x = screen_p.x - b->size;
//        hitbox_rect.y = screen_p.y - b->size;
//        hitbox_rect.w = 2*b->size;
//        hitbox_rect.h = 2*b->size;
//        SDL_Rect image_rect = {0, 0, 16, 16};
//        SDL_RenderCopy(renderer, hero.image_of_hitbox, &image_rect, &hitbox_rect);
//    }
}

bool bullet_out_of_screen(Bullet *b) {
    if (b->p.x > win_w*2/3 + b->size) return true;
    if (b->p.y > win_h*2/3 + b->size) return true;
    if (b->p.x + b->size < win_w*1/3) return true;
    if (b->p.y + b->size < win_h*1/3) return true;
    return false;
}

bool collision_circle_circle(Point c1, s64 rad1, Point c2, s64 rad2) {
    s64 dist2 = (c1.x - c2.x)*(c1.x - c2.x) + (c1.y - c2.y)*(c1.y - c2.y);
    bool res = dist2 < (rad1 + rad2)*(rad1 + rad2);
    /*
    if (res) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }

    SDL_RenderDrawLine(renderer, point_to_pixel_x(c1.x), point_to_pixel_y(c1.y), point_to_pixel_x(c2.x), point_to_pixel_y(c2.y));
    */
    return res;
}

void set_default_spec() {
    spec.freq     = 44100;
    spec.channels = 1;
    spec.samples  = DEFAULT_SAMPLES;
    spec.callback = NULL;
    spec.userdata = NULL;
    spec.format   = AUDIO_S16;
}

void print_record_devices(int n_record_devices) {
    SDL_Log("list of record devices:");
    for (int i = 0; i < n_record_devices; ++i) {
        SDL_Log("%d: %s", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
    }
}

bool get_record_device_from_cmd_args(char const **record_device, int n_args, char const* const* args) {
    if (n_args != 1 && n_args != 2) {SDL_Log("need 0 or 1 arguments: %s idx_record", args[0]); return false;}

    int record_device_idx = -1;
    int n_record_devices = SDL_GetNumAudioDevices(SDL_TRUE);
    if (n_args == 1) {
        record_device_idx = 0;
    }
    if (n_args == 3) {
        record_device_idx = SDL_atoi(args[1]);
        if ((record_device_idx == 0 && args[1][0] != '0') || record_device_idx >= n_record_devices) {SDL_Log("you should choose recording device number from 0 to %d, but you entered %s", n_record_devices - 1, args[1]); return false;}
    }
    if (record_device_idx == -1) {SDL_Log("Pass the correct number as a second argument but not '%s'", args[1]); print_record_devices(n_record_devices); return false;}

    assert(record_device_idx < n_record_devices);
    *record_device = SDL_GetAudioDeviceName(record_device_idx, SDL_TRUE);

    SDL_Log("You chose recording device %d. %s", record_device_idx, record_device);
    print_record_devices(n_record_devices);
    return true;
}

struct Record {
    u64atom current_writen_pos = 0;
    Array<u8> record_buffer;
};

struct Audio_Wav {
    Array<u8>     wav;
    SDL_AudioSpec spec;
    char const*   name = NULL;
};

bool init_sdl() {
    bool success = false;
    window   = SDL_CreateWindow("puzzmaku", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1500, 950, SDL_WINDOW_OPENGL); if (!window) {SDL_Log("Could not create window: %s\n", SDL_GetError());     return false;}
    SDL_GetWindowSize(window, &win_w, &win_h);
    defer {if (!success) SDL_DestroyWindow(window);};

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);                                                   if (!renderer) {SDL_Log("Could not create rendered: %s\n", SDL_GetError()); return false;}
    defer {if (!success) SDL_DestroyRenderer(renderer);};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    int ttf_init_result = TTF_Init();                                                                                                        if (ttf_init_result == -1) {SDL_Log("Could not init TTF: %s\n", SDL_GetError());     return false;}
    defer {if (!success) TTF_Quit();};

    char const *font_path = DATA_PATH "Inconsolata-Regular.ttf";
    font     = TTF_OpenFont(font_path, 36);                                                                                             if (!font) {SDL_Log("Can't open font %s: %s", font_path, SDL_GetError());    return false;}
    defer {if (!success) TTF_CloseFont(font);};

    success = true;
    return success;
}

void close_sdl() {
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void record_audio(Record *record, u8 *stream, int chunk_len) {
    u64 start_pos = record->current_writen_pos.load();
    if (start_pos + chunk_len > RECORD_BUFFER_SIZE) start_pos = 0;

    memcpy(&record->record_buffer[start_pos], stream, chunk_len);
    record->current_writen_pos.store(start_pos + chunk_len);
}

void draw_vertical_line(s32 color, double pos) {
    assert(pos >= 0);
    assert(pos <= 1);
    SDL_Color c = s32_to_color(color);
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    int screen_pos = (int) (pos * win_w);
    SDL_RenderDrawLine(renderer, screen_pos, 0, screen_pos, win_h);
}

void draw_text(char const* text, int x, int y) {
    SDL_Rect message_rect;
    message_rect.x = x + 2;
    message_rect.y = y + 2;
    TTF_SizeText(font, text, &message_rect.w, &message_rect.h);

    {
        SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, text, {128, 128, 128}); if (!surfaceMessage) {SDL_Log("draw_text error: %s", SDL_GetError()); return;}
        defer {SDL_FreeSurface(surfaceMessage);};

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);   if (!texture) {SDL_Log("SDL_CreateTextureFromSurface: %s", SDL_GetError()); return;}
        defer {SDL_DestroyTexture(texture);};

        SDL_RenderCopy(renderer, texture, NULL, &message_rect);
    }

    message_rect.x = x;
    message_rect.y = y;

    {
        SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, text, {255, 255, 255}); if (!surfaceMessage) {SDL_Log("draw_text error: %s", SDL_GetError()); return;}
        defer {SDL_FreeSurface(surfaceMessage);};

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);   if (!texture) {SDL_Log("SDL_CreateTextureFromSurface: %s", SDL_GetError()); return;}
        defer {SDL_DestroyTexture(texture);};

        SDL_RenderCopy(renderer, texture, NULL, &message_rect);
    }
}

void step() {
    // if (dead) SDL_Log("dead!");

    if (keys.rewind.pressed) {
        if (current_time > 0) {
            current_time -= 1;
            dead = false;
        }
    } else if (!dead) {
        current_time += 1;

        if (current_time == 0x7FFFFFFFFFFFFFFF) {
            SDL_Log("TOO LONG GAME");
            exit(0);
        }

        s64 d_x = 0, d_y = 0;
        if (keys.up.pressed)    d_y = 0ULL - hero.step_y;
        if (keys.down.pressed)  d_y = hero.step_y;
        if (keys.left.pressed)  d_x = 0ULL - hero.step_x;
        if (keys.right.pressed) d_x = hero.step_x;
        if (current_time == 0) {
            // hero.positions[0].x = 0;
            // hero.positions[0].y = -field.w2 - 1;
        } else {
            hero.positions[current_time].x = hero.positions[current_time - 1].x + d_x;
            hero.positions[current_time].y = hero.positions[current_time - 1].y + d_y;
            if (hero.positions[current_time].x <= -field.w2) hero.positions[current_time].x = -field.w2;
            if (hero.positions[current_time].x >=  field.w2) hero.positions[current_time].x =  field.w2;
            if (hero.positions[current_time].y <= -field.h2) hero.positions[current_time].y = -field.h2;
            if (hero.positions[current_time].y >=  field.h2) hero.positions[current_time].y =  field.h2;
        }
    }
}

Point operator+(Point const& l, Vector const& r) {
    return {l.x + r.x, l.y + r.y};
}

Point center_radius_and_angle_to_point(Point const& center, s64 radius, double angle) {
    Point ret;
    double dir_x = cos(angle);
    double dir_y = sin(angle);
    ret.x = (s64) (center.x + dir_x * radius);
    ret.y = (s64) (center.y + dir_y * radius);
    return ret;
}

Vector length_and_angle_to_vector(s64 length, double angle) {
    Vector ret;
    double dir_x = cos(angle);
    double dir_y = sin(angle);
    ret.x = (s64) (dir_x * length);
    ret.y = (s64) (dir_y * length);
    return ret;
}

s64 inverted_linear_function(s64 x, s64 s0, s64 delta) {
    // x = s0 + delta*t
    return (x - s0)/delta;
}

s64 linear_bullet_lifetime(Linear_Start *start, s64 size, Field_Rect *f) {
    // TODO бесконечное время жизни, стоячие пули
    s64 tx = 0x7FFFFFFFFFFFFFFF;
    if (start->delta.x > 0) {
        tx = inverted_linear_function(f->cx + f->w2 + size, start->p0.x, start->delta.x);
    } else if (start->delta.x < 0) {
        tx = inverted_linear_function(f->cx - f->w2 - size, start->p0.x, start->delta.x);
    }
    s64 ty = 0x7FFFFFFFFFFFFFFF;
    if (start->delta.y > 0) {
        ty = inverted_linear_function(f->cy + f->h2 + size, start->p0.y, start->delta.y);
    } else if (start->delta.y < 0) {
        ty = inverted_linear_function(f->cy - f->h2 - size, start->p0.y, start->delta.y);
    }
    // SDL_Log("tx = %lli, ty =  %lli, size = %lli", tx, ty, size);

    return (tx > ty) ? ty : tx;
}

void log_linear_bullet_start(Linear_Start *s) {
    SDL_Log("p0 = (%lli, %lli), d = (%lli, %lli), t0 = %lli, t1 = %lli", s->p0.x, s->p0.y, s->delta.x, s->delta.y, s->t0, s->t1);
}

struct Interval_Ids {
    int min;
    int max;
};

Interval_Ids (*get_bullets_interval_ids)(s64) = NULL;
bool (*get_bullet)(s64 t, int i, Point *p, s64 *size) = NULL;
