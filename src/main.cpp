// нужно определить свой, просто не хочется чтобы этот путь зависел от того, в какой папке ты запустил бинарник
#define DATA_PATH "../data/"
//#define DATA_PATH "data/"

#include "puzzmaku.cpp"
//#include "lvl_pristine_danmaku_hell.cpp"
//#include "lvl_tetris.cpp"
#include "lvl_random_rain.cpp"

#include <chrono> // sleep
#include <thread> // sleep

void update_input() {
    // съедаем все инпут ивенты
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            close_sdl();
            exit(0);
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
            case SDLK_UP:    key_pressed(&keys.up);     break;
            case SDLK_DOWN:  key_pressed(&keys.down);   break;
            case SDLK_LEFT:  key_pressed(&keys.left);   break;
            case SDLK_RIGHT: key_pressed(&keys.right);  break;
            case SDLK_z:     key_pressed(&keys.rewind); break;
            default: break;
            }
        } else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
            case SDLK_UP:    key_released(&keys.up);     break;
            case SDLK_DOWN:  key_released(&keys.down);   break;
            case SDLK_LEFT:  key_released(&keys.left);   break;
            case SDLK_RIGHT: key_released(&keys.right);  break;
            case SDLK_z:     key_released(&keys.rewind); break;
            case SDLK_1:        hero.step_x = (hero.step_y -= 1);    if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_2:        hero.step_x = (hero.step_y += 1);    if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_3:        hero.step_x = (hero.step_y -= 10);   if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_4:        hero.step_x = (hero.step_y += 10);   if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_5:        hero.step_x = (hero.step_y -= 100);  if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_6:        hero.step_x = (hero.step_y += 100);  if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_PAGEUP:   hero.step_x = (hero.step_y += 1000); if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_PAGEDOWN: hero.step_x = (hero.step_y -= 1000); if (hero.step_y < 0) hero.step_x = hero.step_y = 0; break;
            case SDLK_x: step_period *= 1.1; break;
            case SDLK_c: step_period /= 1.1; break;
            // default: pause(); break;
            }
        }
    }
}


// TODO решить как лучше читать инпут перед кадром или после, и посчитать задержку между вводом и результатом, мб замедленно попробовать покадрово поиграть
// TODO отделить симуляцию от фактических рисуемых кадров
void main_loop() {
    double accumulator = 0.0;
    for (;;) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // рисуем кадр из состояния
        if (accumulator >= step_period) {
        SDL_SetRenderDrawColor(renderer, 125, 125, 125, 255);
        SDL_RenderClear(renderer);

/*
        for (int i = 0; i < linear_bullets.n_starts; ++i) {
            Point *p = linear_pos(current_time, i);
            if (!p) continue;

            Bullet b;
            b.p = *p;
            b.size = linear_bullets.size / points_per_pixel();
            //SDL_Log("draw i = %d, pos = {%d, %d}", i, b.p.x, b.p.y);
            draw_bullet(&b);
        }
*/
        Interval_Ids ids = get_bullets_interval_ids(current_time);
        draw_hero();
        for (int i = ids.min; i <= ids.max; ++i) {
            Point p;
            s64 size;

            if (!get_bullet(current_time, i, &p, &size)) continue;

            Bullet b;
            b.p = p;
            b.size = (int)(size / points_per_pixel());
            //SDL_Log("draw i = %d, pos = {%d, %d}", i, b.p.x, b.p.y);
            draw_bullet(&b);
        }

        {
            SDL_Color c = s32_to_color(0);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            int x = point_to_pixel_x(field.cx - field.w2);
            int r = point_to_pixel_x(field.cx + field.w2);
            int y = point_to_pixel_y(field.cy - field.h2);
            int b = point_to_pixel_y(field.cy + field.h2);
            SDL_RenderDrawLine(renderer, x, 0, x, win_h);
            SDL_RenderDrawLine(renderer, r, 0, r, win_h);

            SDL_RenderDrawLine(renderer, 0, y, win_w, y);
            SDL_RenderDrawLine(renderer, 0, b, win_w, b);
        }

        // обновляем стейт на основе инпута
        update_input();
        /*
        for (int i = 0; i < linear_bullets.n_starts; ++i) {

            // Point* p = linear_pos(current_time, i);
            if (!p) continue;

            dead = collision_circle_circle(*p, linear_bullets.size, hero.positions[current_time], hero.hitbox);
            if (dead) break;
        }
        */
            for (int i = ids.min; i <= ids.max; ++i) {
                Point p;
                s64 size;
                // Point* p = linear_pos(current_time, i);
                if (!get_bullet(current_time, i, &p, &size)) continue;

                dead  = collision_circle_circle(p, size, hero.positions[current_time], hero.hitbox);
                if (dead) break;
            }

        if (current_time > 600 && hero.positions[current_time].y == -field.h2) {
            draw_text("CONGRATULATIONS!!!", screen_field.cx - 150, screen_field.cy);
        }
#if 0
            Vector collision_directions[50];
            s64    collision_sizes[50];
            int    n_collisions = 0;
            for (int i = ids.min; i <= ids.max; ++i) {
                Point p;
                s64 size;
                // Point* p = linear_pos(current_time, i);
                if (!lvl_random_rain_get_bullet(current_time, i, &p, &size)) continue;

                // dead =
                bool collision = collision_circle_circle(p, size, hero.positions[current_time], hero.hitbox);
                if (collision) {
                    collision_directions[n_collisions].x = p.x - hero.positions[current_time].x;
                    collision_directions[n_collisions].y = p.y - hero.positions[current_time].y;
                    collision_sizes[n_collisions]        = size;
                    ++n_collisions;
                }
                // if (dead) break;
            }

            Vector diagonals[10] = {{4, 0}, { 4, 1}, { 3, 2}, { 2, 3}, { 1, 3},
                                    {0, 3}, {-1, 3}, {-2, 3}, {-3, 2}, {-4, 1}};
            if (n_collisions >= 3) {
                bool squished = true;
                for (int diag = 0; diag < 10; ++diag) {
                    s64 x1 = diagonals[diag].x;
                    s64 y1 = diagonals[diag].y;

                    bool all_on_the_one_side = true;
                    int side = 0;
                    for (int dir = 0; dir < n_collisions; ++dir) {
                        s64 x2 = collision_directions[dir].x;
                        s64 y2 = collision_directions[dir].y;
                        if (side == 0) {
                            side = (x1*y2 - x2*y1) >= 0 ? 1 : -1;

                            continue;
                        }

                        if ((x1*y2 - x2*y1) * side < 0) {
                            all_on_the_one_side = false;
                            break;
                        }
                    }

                    if (all_on_the_one_side) {
                        squished = false;
                        break;
                    }
                }

                if (squished) dead = true;
            }

            if (!dead) {
                if (n_collisions > 0) {
                    // АТМТА получше придумай алгоритм
                    for (int i = 0; i < n_collisions; ++i) {
                        s64 dx = -collision_directions[i].x;
                        s64 dy = -collision_directions[i].y;
                        double l = sqrt(dx*dx + dy*dy);
                        dx = dx / l * (collision_sizes[i] - l + hero.hitbox);
                        dy = dy / l * (collision_sizes[i] - l + hero.hitbox);
                        hero.positions[current_time].x += dx;
                        hero.positions[current_time].y += dy;
                        // почему при касании движется странно?

                        // Point c =
                        // collision_circle_circle(p, size, hero.positions[current_time], hero.hitbox);
                    }
                }
            }
#endif
            step();

            //SDL_Log("time = %.2lf ms, fps = %.2lf, bullets = %d, step = %llu", diff.count() * 1000, 1.0/diff.count(), linear_bullets.n_starts, hero.step_x);
            accumulator = 0.0;
            SDL_RenderPresent(renderer);
        }

        // TODO почему всё тормозит если проставить SDL_RenderPresent(renderer); после часов?
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - frame_start;
        accumulator += diff.count();


        //if (current_time >= 20) return;
    }
}

SDL_Texture *import_bmp_file_to_texture(char const* name) {
    SDL_Texture *ret;
    SDL_Surface *tmp = SDL_LoadBMP(name);
    ret = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_SetTextureBlendMode(ret, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(tmp);
    return ret;
}

int main(int n_args, char ** args) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {SDL_Log("Could not initialize SDL: %s.\n", SDL_GetError()); return -1;}
    defer {SDL_Quit();};

    init_sdl();

    //SDL_Log("field is %d bytes", sizeof(Field));
    //return 0;

    {
        hero.image_of_hitbox = import_bmp_file_to_texture(DATA_PATH "Hero_Hitbox.bmp");
        hero.image           = import_bmp_file_to_texture(DATA_PATH "Marisa.bmp");
        hero.still_animation[0] = import_bmp_file_to_texture(DATA_PATH "back1.bmp");
        hero.still_animation[1] = import_bmp_file_to_texture(DATA_PATH "back2.bmp");
        hero.still_animation[2] = import_bmp_file_to_texture(DATA_PATH "back3.bmp");
        image_of_circle         = import_bmp_file_to_texture(DATA_PATH "circle2.bmp");
    }

    // init_lvl_pristine_danmaku_hell();
    init_lvl_random_rain();
    main_loop();
}
