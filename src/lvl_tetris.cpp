struct Field {
    u8 a[10][20];
};

Field fields[10000];
int n_fields = 1;

// TODO u64
bool drop_tick(s64 tick) {
    assert(tick >= 0);
    const int period = 60;
    return (tick % period) == (period - 1);
}

