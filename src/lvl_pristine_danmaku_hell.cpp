void init_lvl_pristine_danmaku_hell() {
    hero.positions[0].x = 0;
    hero.positions[0].y = field.h2/2;

    int n = 10;
    int n_packs = 100;
    double time_between_packs = 0.5;
    linear_bullets.n_starts = n * n_packs;
    linear_bullets.size = 50 * points_per_pixel();
    // linear_bullets.starts[0].p0    = screen_to_coords({win_w/2, win_h/2});
    // linear_bullets.starts[0].delta = screen_to_coords({0, 1});
    // linear_bullets.starts[0].t0 = 60;
    // linear_bullets.starts[0].t1 = 60*6;
    Field_Rect* f = &field;
    SDL_Log("fl = %lli, ft = %lli, fr = %lli, fb = %lli", f->cx - f->w2, f->cy - f->h2, f->cx + f->w2, f->cy + f->h2);


    double delta_angle = M_PI/20.0;
    for (int pack = 0; pack < n_packs; ++pack){
        Point center = {0, -field.w2/2};// screen_to_coords({win_w/2, win_h/2});
        s64 radius   = 100 * points_per_pixel();
        s64 speed    = 3 * points_per_pixel();
        for (int i = 0; i < n; ++i) {
            double angle = (2 * M_PI * i) / n + delta_angle * pack;
            double dir_x = cos(angle);
            double dir_y = sin(angle);
            Linear_Start *start = &linear_bullets.starts[pack*n + i];
            start->p0    = center_radius_and_angle_to_point(center, radius, angle);
            start->delta = length_and_angle_to_vector(speed, angle);
            start->t0    = 10 + pack*(s32)(60*time_between_packs);
            start->t1    = start->t0 + linear_bullet_lifetime(start, linear_bullets.size, &field);
            if (start - linear_bullets.starts >= 10 && start - linear_bullets.starts < 20) {
                start->t0 += 12;
                start->t1 += 12;
            }
            log_linear_bullet_start(start);
            // SDL_Log("t = %d", start->t1 - start->t0);
        }
    }
}
