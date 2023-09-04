int rain_seed = 0x152fe104;

// i = 16k
#define CACHED_SEEDS 10000
int seeds_cache[CACHED_SEEDS];

int get_random_number(int i) {
    assert(i >= 0);
    assert(i/16 < CACHED_SEEDS);

    srand(seeds_cache[i/16]);
    for (int _c = 0; _c + 1 < i % 16; ++_c) {
        rand();
    }
    return rand();
}

s64 min_y() {
    return -field.h2 + points_per_pixel() * 20;
}

s64 max_size() {
    return screen_field.w2 / 20 * points_per_pixel();
}

s64 get_delta_y_from_rand(int r) {
    return points_per_pixel() / 60 * (((r & 0xF000) >> 12)*4 + 100) * 2; // от 100 до 610 пикселей в секунду
}

s64 longest_bullet_lifetime() {
    Linear_Start start_longest;
    start_longest.p0.x = 0;
    start_longest.p0.y = min_y();
    start_longest.delta.x = 0;
    start_longest.delta.y = get_delta_y_from_rand(0);
    return linear_bullet_lifetime(&start_longest, max_size(), &field);
}

s64 get_size_from_rand(int) {
    return max_size();// * (float) (r % 256 + 1)/256;
}

Interval_Ids random_rain_get_bullets_interval_ids(s64 t) {
    Interval_Ids i;
    s64 t_max = longest_bullet_lifetime();
    i.min = 0;//(int)(((t - t_max) - 10 + 5) / 6);
    i.max = 10000;//(int)((t - 10 + 5) / 6);

    if (i.min < 0) i.min = 0;
    if (i.max < 0) i.max = 0;
    return i;
}

bool random_rain_get_bullet(s64 t, int i, Point *p, s64 *size) {
    assert(p);
    Linear_Start start;
    //start.t0 = 10 + 6 * i; // пять пуль в секунду
    start.t0 = 10 + i; // 60 пуль в секунду
    if (t < start.t0) return false;

    int r = get_random_number(i);

    //start.p0.x = -field.w2 + field.w2 * 2 / 16 / 2 + (r & 15) * (field.w2 * 2 / 16);
    start.p0.x = -field.w2 + (float) (r & 0xFFF) / (RAND_MAX & 0xFFF) * field.w2 * 2;
    start.p0.y = min_y();
    start.delta.x = 0;
    start.delta.y = get_delta_y_from_rand(r); // от 100 до 610 пикселей в секунду
    //if (IN_INIT) SDL_Log("delta = %lld", start.delta.y);
    //if (IN_INIT) SDL_Log("start.p0 = %lld %lld", start.p0.x/points_per_pixel(), start.p0.y/points_per_pixel());

    *size = get_size_from_rand(r);
    start.t1 = start.t0 + linear_bullet_lifetime(&start, *size, &field);
    //if (IN_INIT) SDL_Log("t0 = %lld", start.t0);
    //if (IN_INIT) SDL_Log("t1 = %lld", start.t1);

    if (t > start.t1) return false;

    p->x = start.p0.x + (t - start.t0) * start.delta.x;
    p->y = start.p0.y + (t - start.t0) * start.delta.y;
    return true;
}

#define LVL random_rain
#define FUNC(Name) LVL##_##name

void init_lvl_random_rain() {
    get_bullets_interval_ids = random_rain_get_bullets_interval_ids;
    get_bullet               = random_rain_get_bullet;
    hero.positions[0].x = 0;
    hero.positions[0].y = -field.h2;//field.h2*3/4;

    srand(rain_seed);
    for (int i = 0; i < CACHED_SEEDS; ++i) {
        seeds_cache[i] = rand();rand();rand();rand();rand();rand();rand();rand();
                         rand();rand();rand();rand();rand();rand();rand();rand();
    }
}
