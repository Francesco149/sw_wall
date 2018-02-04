/*
    basic doom-style 3D demo in pure software rendering in < 400 LOC.
    sdl is only used to blit the finished frame to a window and handle input

    heavily inspired by
    [bisqwit's qbasic demo](https://youtu.be/HQYsFshbkYw?t=42s)

    ![](https://media.giphy.com/media/xUOwGeOoHG9u6DX6W4/giphy.gif)

    # rationale
    most 3D tutorials jump right into transformation matrices, OpenGL etc.
    starting from how it was done in the very first 3D games is much simpler

    # usage
    ```gcc -O3 wall.c -lSDL2 -lm -o wall && ./wall```

    controls: WASD, mouse (horizontal only)

    # license
    this is free and unencumbered software released into the public domain.
    refer to the attached UNLICENSE or http://unlicense.org/
*/

#define VERSION_STR "sw_wall-1.0.0"

#define m_abs(x) (((x) < 0) ? -(x) : (x))
#define m_sign(x) (((x) > 0) - ((x) < 0))
#define m_ptinrect(x, y, l, t, r, b) (x >= l && x < r && y >= t && y < b)
#define m_min(a, b) ((a) < (b) ? (a) : (b))
#define m_max(a, b) ((a) > (b) ? (a) : (b))
#define m_clamp(x, a, b) m_max(a, m_min(x, b))

#define W 320
#define H 200
#define AREA_STACK_SIZE 8

struct r_rect { int l, t, r, b; } area = { 0, 0, W, H };
#define r_ptinarea(x, y) m_ptinrect(x, y, area.l, area.t, area.r, area.b)

/* NOTE: there is no stack bounds checking! */
int area_stack_top = 0;
struct r_rect area_stack[AREA_STACK_SIZE];

void r_push_area() { area_stack[area_stack_top++] = area; }
void r_pop_area() { area = area_stack[--area_stack_top]; }

#define area_w (area.r-area.l)
#define area_h (area.b-area.t)

void r_area(int l, int t, int r, int b) {
    area.l = l, area.t = t, area.r = r, area.b = b;
}

void r_clear(int* pix, int color)
{
    int i;
    for (i = 0; i < W * H; ++i) pix[i] = color;
}

void r_vline(int* pix, int color, int x, int y0, int y1)
{
    int dy, dir;
    if (x < area.l || x >= area.r) return;
    y0 = m_clamp(y0, area.t, area.b-1);
    y1 = m_clamp(y1, area.t, area.b-1);
    dy = y1 - y0;
    dir = m_sign(dy) | 1;         /* | 1 prevents an infinite loop if dy is 0 */
    for (; y0 != y1 + dir; y0 += dir) pix[y0 * W + x] = color;
}

void r_hline(int* pix, int color, int x0, int x1, int y)
{
    int dx, dir;
    if (y < area.t || y >= area.b) return;
    x0 = m_clamp(x0, area.l, area.r-1);
    x1 = m_clamp(x1, area.l, area.r-1);
    dx = x1 - x0;
    dir = m_sign(dx) | 1;
    for (; x0 != x1 + dir; x0 += dir) pix[y * W + x0] = color;
}

/* clip line to area using liang-barsky */
int r_clip_line(float* x0, float* y0, float* x1, float* y1)
{
    float t0, t1;
    float p, q, r;
    float dx, dy;

    t0 = 0;
    t1 = 1;
    dx = *x1 - *x0;
    dy = *y1 - *y0;

#define edge(pval, qval) \
    p = (pval), q = (qval); \
    if (!p && q < 0) return 0; \
    r = q / p; \
    \
    if (p < 0) { \
        if (r > t1) return 0; \
        else if (r > t0) t0 = r; \
    } \
    else { \
        if (r < t0) return 0; \
        else if (r < t1) t1 = r; \
    }

    edge(-dx, *x0-area.l)
    edge(dx, area.r-1-*x0)
    edge(-dy, *y0-area.t)
    edge(dy, area.b-1-*y0)

#undef edge

    *x1 = *x0 + t1 * dx;
    *y1 = *y0 + t1 * dy;
    *x0 += t0 * dx;
    *y0 += t0 * dy;

    return 1;
}

void r_line(int* pix, int color, float x0, float y0, float x1, float y1)
{
    float dx, dy;
    float errstep, err;
    int xdir, ydir;

    x0 = (int)x0 + area.l;
    x1 = (int)x1 + area.l;
    y0 = (int)y0 + area.t;
    y1 = (int)y1 + area.t;

    if (x0 == x1) return r_vline(pix, color, x0, y0, y1);
    if (y0 == y1) return r_hline(pix, color, x0, x1, y0);

    if (!r_clip_line(&x0, &y0, &x1, &y1)) return;

    x0 = (int)(x0+0.5f);                /* center float coordinates on pixels */
    x1 = (int)(x1+0.5f);
    y0 = (int)(y0+0.5f);
    y1 = (int)(y1+0.5f);

    dx = x1-x0;                            /* bad implementation of bresenham */
    dy = y1-y0;
    errstep = m_abs(dy / dx);
    xdir = m_sign(dx);
    ydir = m_sign(dy);
    err = 0;

    for (; (int)x0 != (int)x1 + xdir; x0 += xdir)
    {
        pix[(int)y0 * W + (int)x0] = color;
        err += errstep;

        while (err >= 0.5f)
        {
            if ((int)y0 == (int)y1 + ydir) return;
            pix[(int)y0 * W + (int)x0] = color;
            y0 += ydir, err -= 1;
        }
    }
}

void r_rect(int* pix, int color, int l, int t, int r, int b)
{
    r_line(pix, color, l, t, r, t);
    r_line(pix, color, r, t, r, b);
    r_line(pix, color, l, b, r, b);
    r_line(pix, color, l, t, l, b);
}

/* -------------------------------------------------------------------------- */

#include <math.h>

/* movement axes */
int wishx = 0, wishz = 0;
int wishlook = 0;

/* player state */
float px = 50, pz = 50;
float pangle = 0;

/* wall */
int const wx0 = 40, wz0 = 30;
int const wx1 = 60, wz1 = 30;

#define cross(x0, y0, x1, y1) ((x0) * (y1) - (y0) * (x1))

/* intersection point between two line segments (black magic) */
void intersect(float rx0, float ry0, float rx1, float ry1, float sx0,
    float sy0, float sx1, float sy1, float* ix, float* iy)
{
    float x, y, denom;

    x = cross(rx0, ry0, rx1, ry1);
    y = cross(sx0, sy0, sx1, sy1);
    denom = cross(rx0-rx1, ry0-ry1, sx0-sx1, sy0-sy1);
    *ix = cross(x, rx0-rx1, y, sx0-sx1) / denom;
    *iy = cross(x, ry0-ry1, y, sy0-sy1) / denom;
}

void update(int* pix, float tdelta)
{
    float psin, pcos;
    float tx0, ty0t, ty0b, tz0, tx1, ty1t, ty1b, tz1;
    int midx, midy;
    float ix0, iz0, ix1, iz1;

    r_clear(pix, 0);

    pangle += M_PI * wishlook * tdelta;
    while (pangle < 0) pangle += 2*M_PI;      /* convert angle to 0-360 range */
    while (pangle >= 2*M_PI) pangle -= 2*M_PI;

    psin = sinf(pangle);
    pcos = cosf(pangle);

    /* flip z movement because forward must go towards the top of window */
    px += 20 * tdelta * (wishx * pcos + wishz * psin);
    pz -= 20 * tdelta * (wishx * (-psin) + wishz * pcos);

    r_push_area();

    /* 2D static view ------------------------------------------------------- */
    r_area(5, 50, 105, 150);
    r_line(pix, 0xFFFF00, wx0, wz0, wx1, wz0);                        /* wall */
    r_line(pix, 0x333333, px, pz, px+10*psin, pz-10*pcos);      /* player dir */
    r_line(pix, 0xFFFFFF, px, pz, px, pz);                          /* player */
    r_rect(pix, 0xFF0000, 0, 0, area_w-1, area_h-1);

    /* 2D top-down view ----------------------------------------------------- */
    r_area(110, 50, 210, 150);
    tx0 = (wx0-px) * pcos + (wz0-pz) * psin;      /* wall coords relative to- */
    tx1 = (wx1-px) * pcos + (wz1-pz) * psin;      /* the player's view        */
    tz0 = (wx0-px) * (-psin) + (wz0-pz) * pcos;
    tz1 = (wx1-px) * (-psin) + (wz1-pz) * pcos;
    midx = area_w / 2;           /* draw relative to the middle of the screen */
    midy = area_h / 2;
    r_line(pix, 0xFFFF00, tx0+midx, tz0+midy, tx1+midx, tz1+midy);    /* wall */
    r_line(pix, 0x333333, midx, midy, midx, midy-10);           /* player dir */
    r_line(pix, 0xFFFFFF, midx, midy, midx, midy);                  /* player */
    r_rect(pix, 0x00FF00, 0, 0, area_w-1, area_h-1);

    /* 3D first person view ------------------------------------------------- */
    r_area(215, 50, 315, 150);

    if (tz0 > 0 && tz1 > 0) {
        /* the wall is completely behind the player (remember the flipped z) */
        goto wall_done;
    }

    /* clip geometry to where it intersects the field of view */
    intersect(tx0, tz0, tx1, tz1, -0.0001f, 0.0001f, -50, 5, &ix0, &iz0);
    intersect(tx0, tz0, tx1, tz1, +0.0001f, 0.0001f, +50, 5, &ix1, &iz1);

    if (tz0 > 0) {
        if (iz0 <= 0) tx0 = ix0, tz0 = iz0;
        else          tx0 = ix1, tz0 = iz1;
    }

    if (tz1 > 0) {
        if (iz0 <= 0) tx1 = ix0, tz1 = iz0;
        else          tx1 = ix1, tz1 = iz1;
    }

    if (tz0 >= 0 || tz1 >= 0) {
        goto wall_done;   /* just in case intersect returns both negative z's */
    }

    midx = area_w / 2;           /* draw relative to the middle of the screen */
    midy = area_h / 2;
    tx0 = (-tx0 * 50) / tz0 + midx;             /* scale horizontal fov a bit */
    tx1 = (-tx1 * 50) / tz1 + midx;
    ty0t = -50 / tz0 + midy;                          /* hardcode wall height */
    ty1t = -50 / tz1 + midy;
    ty0b = 50 / tz0 + midy;
    ty1b = 50 / tz1 + midy;
    r_line(pix, 0xFFFF00, tx0, ty0t, tx1, ty1t);
    r_line(pix, 0xFFFF00, tx1, ty1t, tx1, ty1b);
    r_line(pix, 0xFFFF00, tx0, ty0b, tx1, ty1b);
    r_line(pix, 0xFFFF00, tx0, ty0t, tx0, ty0b);
wall_done:
    r_rect(pix, 0x0000FF, 0, 0, area_w-1, area_h-1);

    r_pop_area();
}

/* -------------------------------------------------------------------------- */

#include <SDL2/SDL.h>
#include <stdio.h>

#define SCALE 2
#define WINDOW_W (W * SCALE)
#define WINDOW_H (H * SCALE)
#define BPP 32
#define DEFPOS SDL_WINDOWPOS_UNDEFINED

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* screen;

int running = 1;
float tlast;
int frames = 0;
float tsecond = 1;

float sdl_time() { return SDL_GetTicks() / 1000.0f; }

void sdl_init()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    window = SDL_CreateWindow("persp", DEFPOS, DEFPOS, WINDOW_W, WINDOW_H, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    SDL_SetWindowTitle(window, "persp");
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, W, H);

    tlast = sdl_time();
}

void sdl_event(SDL_Event* e)
{
    int down;

    switch (e->type)
    {
    case SDL_QUIT: running = 0; break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        down = e->type == SDL_KEYDOWN;
        switch (e->key.keysym.sym)
        {
        case SDLK_a: wishx = down * -1; break;
        case SDLK_d: wishx = down * +1; break;
        case SDLK_w: wishz = down * +1; break;
        case SDLK_s: wishz = down * -1; break;
        case SDLK_ESCAPE: running = 0; break;
        }
        break;
    }
}

void sdl_tick()
{
    float tnow, tdelta;
    int* pix;
    int pitch;

    /* cap fps to timer accuracy (1000fps with sdl) */
    do { tnow = sdl_time(); } while (tnow == tlast);
    tdelta = tnow-tlast;
    tlast = tnow;

    ++frames;
    tsecond -= tdelta;
    if (tsecond <= 0)
    {
        fprintf(stderr, "%d fps\n", frames);
        tsecond += 1;
        frames = 0;
    }

    SDL_GetRelativeMouseState(&wishlook, 0);
    SDL_LockTexture(screen, 0, (void**)&pix, &pitch);

    update(pix, tdelta);

    SDL_UnlockTexture(screen);
    SDL_RenderCopyEx(renderer, screen, 0, 0, 0, 0, SDL_FLIP_NONE);
    SDL_RenderPresent(renderer);
}

int main()
{
    fprintf(stderr, "%s\n", VERSION_STR);
    fprintf(stderr, "%s\n", "SDL backend");

    sdl_init();

    while (running)
    {
        SDL_Event e;

        while (SDL_PollEvent(&e))
            sdl_event(&e);

        sdl_tick();
    }

    SDL_Quit();

    return 0;
}
