/* Direct Link: host or join a Wi-Fi link between two handhelds.
   Colours are BananaOS's palette. The typeface is Pixelify Sans. libdrm, both panels. */
#include "stb_truetype.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define C_BG    0x191820
#define C_PANEL 0x211f26
#define C_BLUE  0xf0b429
#define C_LIGHT 0xf1ede2
#define C_GREY  0xa39d8e
#define C_ONSEL 0x17161c
#define C_RED   0xff8a96
#define C_DIM   0x6b6559
#define C_GREEN 0x56bf6b

static volatile sig_atomic_t g_stop;

static void on_signal(int sig) { (void)sig; g_stop = 1; }

typedef struct {
    uint32_t conn, crtc, fb, handle, pitch, size;
    uint8_t *map;
    drmModeModeInfo mode;
} Panel;

static int g_fd = -1, g_w, g_h, g_shown;
static Panel g_p[2];
static uint32_t *g_shadow;

static unsigned char *g_ttf;
static unsigned char g_atlas[512 * 512], g_atlas_lg[512 * 512];
static stbtt_bakedchar g_glyph[96], g_glyph_lg[96];

typedef struct {
    unsigned char *atlas;
    stbtt_bakedchar *glyph;
    int px;
    int ok;
} Face;

static Face g_body, g_title;

static char g_app[512];

static void px(int x, int y, uint32_t rgb)
{
    if (!g_shadow || x < 0 || y < 0 || x >= g_w || y >= g_h) return;
    g_shadow[(size_t)y * (size_t)g_w + (size_t)x] = rgb;
}

static void blend(int x, int y, uint32_t rgb, int a)
{
    uint32_t d;
    int dr, dg, db, sr, sg, sb;
    size_t i;
    if (a <= 0 || !g_shadow || x < 0 || y < 0 || x >= g_w || y >= g_h) return;
    i = (size_t)y * (size_t)g_w + (size_t)x;
    if (a >= 255) { g_shadow[i] = rgb; return; }
    d = g_shadow[i];
    dr = (d >> 16) & 255; dg = (d >> 8) & 255; db = d & 255;
    sr = (rgb >> 16) & 255; sg = (rgb >> 8) & 255; sb = rgb & 255;
    g_shadow[i] = ((dr + (sr - dr) * a / 255) << 16) | ((dg + (sg - dg) * a / 255) << 8) | (db + (sb - db) * a / 255);
}

static void fill(int x0, int y0, int x1, int y1, uint32_t rgb)
{
    int x, y;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g_w) x1 = g_w;
    if (y1 > g_h) y1 = g_h;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++)
            px(x, y, rgb);
}

static void clear_shadow(uint32_t rgb)
{
    size_t i, n;
    if (!g_shadow) return;
    n = (size_t)g_w * (size_t)g_h;
    for (i = 0; i < n; i++) g_shadow[i] = rgb;
}

static void blit(Panel *p)
{
    int y;
    if (!g_shadow || !p->map) return;
    for (y = 0; y < g_h; y++)
        memcpy(p->map + (size_t)y * p->pitch, g_shadow + (size_t)y * (size_t)g_w, (size_t)g_w * 4);
}

static int bake(Face *f, unsigned char *atlas, stbtt_bakedchar *glyph, int px)
{
    f->atlas = atlas;
    f->glyph = glyph;
    f->px = px;
    f->ok = 0;
    if (px < 8) return 0;
    if (stbtt_BakeFontBitmap(g_ttf, 0, (float)px, atlas, 512, 512, 32, 96, glyph) <= 0) return 0;
    f->ok = 1;
    return 1;
}

static int load_font(void)
{
    static const char *const paths[] = {
        "/usr/share/banana/rnds/fonts/PixelifySans-Medium.ttf",
        "/usr/share/banana/rnds/fonts/PixelifySans-Regular.ttf",
        NULL
    };
    FILE *f;
    long n;
    int i, px, title;
    for (i = 0; paths[i]; i++) {
        f = fopen(paths[i], "rb");
        if (!f) continue;
        if (fseek(f, 0, SEEK_END) != 0) { fclose(f); continue; }
        n = ftell(f);
        if (n < 1 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); continue; }
        g_ttf = malloc((size_t)n);
        if (!g_ttf || fread(g_ttf, 1, (size_t)n, f) != (size_t)n) {
            free(g_ttf); g_ttf = NULL; fclose(f); continue;
        }
        fclose(f);
        px = g_h / 22;
        if (px < 16) px = 16;
        if (px > 42) px = 42;
        if (!bake(&g_body, g_atlas, g_glyph, px)) { free(g_ttf); g_ttf = NULL; continue; }
        title = 26 * g_h / 480;
        if (title < 16) title = 16;
        if (!bake(&g_title, g_atlas_lg, g_glyph_lg, title)) g_title = g_body;
        return 1;
    }
    return 0;
}

static int text_w(const Face *f, const char *s)
{
    float x = 0, y = 0;
    stbtt_aligned_quad q;
    if (!f->ok) return f->px > 0 ? f->px * (int)strlen(s) / 2 : 0;
    for (; *s; s++) {
        if ((unsigned char)*s < 32 || (unsigned char)*s > 126) { x += f->px / 2.0f; continue; }
        stbtt_GetBakedQuad(f->glyph, 512, 512, (unsigned char)*s - 32, &x, &y, &q, 1);
    }
    return (int)(x + 0.5f);
}

static void text_face(const Face *f, int x, int y, const char *s, uint32_t rgb)
{
    float xp = (float)x, yp = (float)(y + f->px);
    stbtt_aligned_quad q;
    int ix, iy;
    if (!f->ok) return;
    for (; *s; s++) {
        int sx, sy, ax, ay;
        if ((unsigned char)*s < 32 || (unsigned char)*s > 126) { xp += f->px / 2.0f; continue; }
        stbtt_GetBakedQuad(f->glyph, 512, 512, (unsigned char)*s - 32, &xp, &yp, &q, 1);
        sx = (int)(q.x1 - q.x0); sy = (int)(q.y1 - q.y0);
        if (sx < 1 || sy < 1) continue;
        for (iy = 0; iy < sy; iy++) {
            int ty = (int)((q.t0 * 512) + (q.t1 - q.t0) * 512 * iy / sy);
            if (ty < 0) ty = 0;
            if (ty > 511) ty = 511;
            for (ix = 0; ix < sx; ix++) {
                int tx = (int)((q.s0 * 512) + (q.s1 - q.s0) * 512 * ix / sx);
                if (tx < 0) tx = 0;
                if (tx > 511) tx = 511;
                ax = (int)q.x0 + ix;
                ay = (int)q.y0 + iy;
                blend(ax, ay, rgb, f->atlas[ty * 512 + tx]);
            }
        }
    }
}

/* Word wrap. Returns the height used. */
static int text_wrap(const Face *f, int x, int y, const char *s, uint32_t rgb, int max_w, int max_lines)
{
    char line[160], word[64], trial[240];
    int lines = 0, gap;
    if (!f->ok || max_lines < 1) return 0;
    gap = f->px / 4;
    if (gap < 4) gap = 4;
    line[0] = 0;
    while (*s && lines < max_lines) {
        int wn = 0;
        while (*s == ' ') s++;
        if (!*s) break;
        while (*s && *s != ' ' && wn < 60) word[wn++] = *s++;
        word[wn] = 0;
        if (line[0]) snprintf(trial, sizeof trial, "%s %s", line, word);
        else snprintf(trial, sizeof trial, "%s", word);
        if (text_w(f, trial) <= max_w || !line[0]) {
            size_t k = 0;
            while (trial[k] && k + 1 < sizeof line) { line[k] = trial[k]; k++; }
            line[k] = 0;
            continue;
        }
        text_face(f, x, y, line, rgb);
        y += f->px + gap;
        lines++;
        {
            size_t k = 0;
            while (word[k] && k + 1 < sizeof line) { line[k] = word[k]; k++; }
            line[k] = 0;
        }
    }
    if (line[0] && lines < max_lines) {
        text_face(f, x, y, line, rgb);
        lines++;
    }
    if (!lines) return 0;
    return lines * f->px + (lines - 1) * gap;
}

static void planes_upright(void)
{
    drmModePlaneRes *pr;
    uint32_t j;
    drmSetClientCap(g_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    pr = drmModeGetPlaneResources(g_fd);
    if (!pr) return;
    for (j = 0; j < pr->count_planes; j++) {
        drmModeObjectProperties *pp = drmModeObjectGetProperties(g_fd, pr->planes[j], DRM_MODE_OBJECT_PLANE);
        uint32_t i;
        if (!pp) continue;
        for (i = 0; i < pp->count_props; i++) {
            drmModePropertyRes *prop = drmModeGetProperty(g_fd, pp->props[i]);
            if (!prop) continue;
            if (!strcmp(prop->name, "rotation") && pp->prop_values[i] != DRM_MODE_ROTATE_0)
                drmModeObjectSetProperty(g_fd, pr->planes[j], DRM_MODE_OBJECT_PLANE, prop->prop_id, DRM_MODE_ROTATE_0);
            drmModeFreeProperty(prop);
        }
        drmModeFreeObjectProperties(pp);
    }
    drmModeFreePlaneResources(pr);
}

static void panel_free(Panel *p)
{
    if (p->map && p->map != MAP_FAILED) munmap(p->map, p->size);
    if (p->fb) drmModeRmFB(g_fd, p->fb);
    if (p->handle) {
        struct drm_mode_destroy_dumb d;
        memset(&d, 0, sizeof d);
        d.handle = p->handle;
        drmIoctl(g_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &d);
    }
    memset(p, 0, sizeof *p);
}

static void display_close(void)
{
    int s;
    if (g_fd < 0) return;
    drmDropMaster(g_fd);
    for (s = 0; s < 2; s++) panel_free(&g_p[s]);
    close(g_fd);
    g_fd = -1;
}

static int make_buf(Panel *p)
{
    struct drm_mode_create_dumb c;
    struct drm_mode_map_dumb m;
    uint32_t handles[4], pitches[4], offsets[4];
    memset(&c, 0, sizeof c);
    c.width = (uint32_t)g_w;
    c.height = (uint32_t)g_h;
    c.bpp = 32;
    if (drmIoctl(g_fd, DRM_IOCTL_MODE_CREATE_DUMB, &c)) return 0;
    p->handle = c.handle;
    p->pitch = c.pitch;
    p->size = c.size;
    handles[0] = p->handle; pitches[0] = p->pitch; offsets[0] = 0;
    handles[1] = handles[2] = handles[3] = 0;
    pitches[1] = pitches[2] = pitches[3] = 0;
    offsets[1] = offsets[2] = offsets[3] = 0;
    if (drmModeAddFB2(g_fd, (uint32_t)g_w, (uint32_t)g_h, DRM_FORMAT_XRGB8888, handles, pitches, offsets, &p->fb, 0))
        return 0;
    memset(&m, 0, sizeof m);
    m.handle = p->handle;
    if (drmIoctl(g_fd, DRM_IOCTL_MODE_MAP_DUMB, &m)) return 0;
    p->map = mmap(NULL, p->size, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, (off_t)m.offset);
    if (p->map == MAP_FAILED) return 0;
    memset(p->map, 0, p->size);
    return 1;
}

static int display_open(void)
{
    const char *top = getenv("BANANA_TOP");
        char path[288];
    DIR *d;
    struct dirent *e;
    int t;
    if (!top || !top[0]) top = "DSI-2";
    d = opendir("/dev/dri");
    if (!d) { fprintf(stderr, "direct-link: no /dev/dri (%s)\n", strerror(errno)); return 0; }
    while ((e = readdir(d))) {
        drmModeRes *res;
        typedef struct { uint32_t conn, crtc; drmModeModeInfo mode; int istop; } Found;
        Found found[2], tmp;
        int nfound = 0;
        uint32_t used[2];
        int i;
        if (strncmp(e->d_name, "card", 4)) continue;
        snprintf(path, sizeof path, "/dev/dri/%s", e->d_name);
        g_fd = open(path, O_RDWR | O_CLOEXEC);
        if (g_fd < 0) continue;
        res = drmModeGetResources(g_fd);
        if (!res) { close(g_fd); g_fd = -1; continue; }
        for (i = 0; i < res->count_connectors && nfound < 2; i++) {
            drmModeConnector *c = drmModeGetConnector(g_fd, res->connectors[i]);
            const char *tn;
            char name[32];
            drmModeModeInfo mode;
            uint32_t crtc = 0;
            int m, en;
            if (!c) continue;
            tn = drmModeGetConnectorTypeName(c->connector_type);
            if (!tn || strcmp(tn, "DSI") || c->connection != DRM_MODE_CONNECTED || c->count_modes < 1) {
                drmModeFreeConnector(c);
                continue;
            }
            mode = c->modes[0];
            for (m = 0; m < c->count_modes; m++)
                if (c->modes[m].type & DRM_MODE_TYPE_PREFERRED) { mode = c->modes[m]; break; }
            for (en = 0; en < c->count_encoders && !crtc; en++) {
                drmModeEncoder *enc = drmModeGetEncoder(g_fd, c->encoders[en]);
                int k;
                if (!enc) continue;
                for (k = 0; k < res->count_crtcs && !crtc; k++) {
                    int u, taken = 0;
                    if (!(enc->possible_crtcs & (1u << k))) continue;
                    for (u = 0; u < nfound; u++) if (used[u] == res->crtcs[k]) taken = 1;
                    if (!taken) crtc = res->crtcs[k];
                }
                drmModeFreeEncoder(enc);
            }
            if (crtc) {
                snprintf(name, sizeof name, "%s-%u", tn, c->connector_type_id);
                used[nfound] = crtc;
                found[nfound].conn = c->connector_id;
                found[nfound].crtc = crtc;
                found[nfound].mode = mode;
                found[nfound].istop = !strcmp(name, top);
                nfound++;
            }
            drmModeFreeConnector(c);
        }
        drmModeFreeResources(res);
        if (nfound < 2) { close(g_fd); g_fd = -1; continue; }
        if (!found[0].istop && !found[1].istop) {
            tmp = found[0];
            found[0] = found[1];
            found[1] = tmp;
        }
        if (found[1].istop) {
            tmp = found[0];
            found[0] = found[1];
            found[1] = tmp;
        }
        if (found[0].mode.hdisplay != found[1].mode.hdisplay || found[0].mode.vdisplay != found[1].mode.vdisplay) {
            fprintf(stderr, "direct-link: the two panels differ in size\n");
            close(g_fd); g_fd = -1; closedir(d); return 0;
        }
        for (t = 0; t < 50; t++) { if (drmSetMaster(g_fd) == 0) break; usleep(20000); }
        g_w = found[0].mode.hdisplay;
        g_h = found[0].mode.vdisplay;
        for (i = 0; i < 2; i++) {
            memset(&g_p[i], 0, sizeof g_p[i]);
            g_p[i].conn = found[i].conn;
            g_p[i].crtc = found[i].crtc;
            g_p[i].mode = found[i].mode;
            if (!make_buf(&g_p[i])) {
                fprintf(stderr, "direct-link: no scanout buffer (%s)\n", strerror(errno));
                display_close();
                closedir(d);
                return 0;
            }
        }
        planes_upright();
        closedir(d);
        return 1;
    }
    closedir(d);
    fprintf(stderr, "direct-link: no two DSI panels\n");
    return 0;
}

static void present(void)
{
    int s;
    if (g_shown) return;
    for (s = 0; s < 2; s++)
        drmModeSetCrtc(g_fd, g_p[s].crtc, g_p[s].fb, 0, 0, &g_p[s].conn, 1, &g_p[s].mode);
    g_shown = 1;
}

enum {
    ACT_NONE, ACT_LEFT, ACT_RIGHT, ACT_OK, ACT_BACK, ACT_Y,
    ACT_HOST, ACT_JOIN, ACT_DISC
};
enum { VIEW_MAIN, VIEW_UNINSTALL, VIEW_NEST, VIEW_NEST_OK, VIEW_DISC };
enum { JOB_NONE, JOB_HOST, JOB_JOIN, JOB_DOWN };
enum { ST_IDLE, ST_ON, ST_RED, ST_DIM };

typedef struct {
    char state[32];
    char role[16];
    char ip[32];
    char home[8];
} Link;

static int g_view, g_focus, g_job;
static pid_t g_child;
static int g_pipe = -1;
static char g_acc[240];
static int g_acc_n;
static char g_job_line[240];

static struct { int x0, y0, x1, y1, id; } g_hit[4];
static int g_nhit;

static void copy_str(char *dst, size_t n, const char *src)
{
    size_t i = 0;
    if (n == 0) return;
    while (i + 1 < n && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void kv(char *line, const char *key, char *dst, int n)
{
    size_t k = strlen(key);
    if (n > 0 && strncmp(line, key, k) == 0 && line[k] == '=')
        copy_str(dst, (size_t)n, line + k + 1);
}

static void load_link(Link *lk)
{
    char cmd[640], line[512];
    FILE *f;
    memset(lk, 0, sizeof *lk);
    snprintf(lk->state, sizeof lk->state, "down");
    snprintf(cmd, sizeof cmd, "sh '%s/status.sh' 2>/dev/null", g_app);
    f = popen(cmd, "r");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        kv(line, "state", lk->state, (int)sizeof lk->state);
        kv(line, "role", lk->role, (int)sizeof lk->role);
        kv(line, "ip", lk->ip, (int)sizeof lk->ip);
        kv(line, "home_wifi", lk->home, (int)sizeof lk->home);
    }
    pclose(f);
}

static int run_sh(const char *script, char *err, int errn)
{
    char cmd[640], line[512];
    FILE *f;
    int st;
    err[0] = 0;
    snprintf(cmd, sizeof cmd, "sh '%s/%s' 2>&1", g_app, script);
    f = popen(cmd, "r");
    if (!f) { snprintf(err, (size_t)errn, "could not run %s", script); return 0; }
    if (fgets(line, sizeof line, f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        copy_str(err, (size_t)errn, line);
    }
    st = pclose(f);
    if (st == 0) err[0] = 0;
    return st == 0;
}

static int radio_ok(void)
{
    return access("/sys/class/net/wlan0", F_OK) == 0;
}

static int linked(const Link *lk)
{
    if (!strcmp(lk->state, "up") && (!strcmp(lk->role, "host") || !strcmp(lk->role, "join")))
        return 1;
    if (g_job == JOB_DOWN && strcmp(lk->state, "down") &&
        (!strcmp(lk->role, "host") || !strcmp(lk->role, "join")))
        return 1;
    return 0;
}

static const char *mode_word(const Link *lk)
{
    if (!linked(lk)) return "Off";
    if (!strcmp(lk->role, "join")) return "Guest";
    return "Host";
}

static int ticks(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static const char *ellipsis(void)
{
    int n = (ticks() / 400) % 3;
    if (n == 0) return ".";
    if (n == 1) return "..";
    return "...";
}

static void add_hit(int x0, int y0, int x1, int y1, int id)
{
    if (g_nhit >= 4) return;
    g_hit[g_nhit].x0 = x0;
    g_hit[g_nhit].y0 = y0;
    g_hit[g_nhit].x1 = x1;
    g_hit[g_nhit].y1 = y1;
    g_hit[g_nhit].id = id;
    g_nhit++;
}

static void square(int x, int y, int w, int h, const char *label, int st, int id, int live)
{
    const Face *f = &g_body;
    uint32_t bg = C_PANEL, fg = C_LIGHT;
    int tw, tx, ty;
    if (st == ST_ON) { bg = C_BLUE; fg = C_ONSEL; }
    else if (st == ST_RED) { bg = C_RED; fg = C_ONSEL; }
    else if (st == ST_DIM) { bg = C_PANEL; fg = C_DIM; }
    fill(x, y, x + w, y + h, bg);
    tw = text_w(f, label);
    tx = x + (w - tw) / 2;
    ty = y + (h - f->px) / 2;
    if (ty < y) ty = y;
    text_face(f, tx, ty, label, fg);
    if (live) add_hit(x, y, x + w, y + h, id);
}

static void legend(const char *a, const char *b, const char *y)
{
    const Face *f = &g_body;
    int margin = g_w / 18;
    int y0 = g_h - f->px * 2;
    int x = margin;
    char buf[64];
    if (y0 < 0) y0 = 0;
    if (a && a[0]) {
        snprintf(buf, sizeof buf, "A  %s", a);
        text_face(f, x, y0, buf, C_GREY);
        x += text_w(f, buf) + f->px;
    }
    if (b && b[0]) {
        snprintf(buf, sizeof buf, "B  %s", b);
        text_face(f, x, y0, buf, C_GREY);
    }
    if (y && y[0]) {
        int tw;
        snprintf(buf, sizeof buf, "Y  %s", y);
        tw = text_w(f, buf);
        text_face(f, g_w - margin - tw, y0, buf, C_GREY);
    }
}

static int line(const Face *f, int x, int y, const char *s, uint32_t rgb, int max_w)
{
    int h = text_wrap(f, x, y, s, rgb, max_w, 2);
    return h > 0 ? h + f->px / 3 : 0;
}

static void draw_top(const Link *lk, const char *err)
{
    const Face *title = g_title.ok ? &g_title : &g_body;
    const Face *body = &g_body;
    const char *word = mode_word(lk);
    int margin = g_w / 18;
    int max_w = g_w - margin * 2;
    int y, tw;
    char buf[160];
    int on = linked(lk);
    int home = !strcmp(lk->home, "on");
    clear_shadow(C_BG);
    if (!title->ok) return;
    tw = text_w(title, word);
    text_face(title, (g_w - tw) / 2, g_h / 8, word, C_GREEN);
    y = g_h / 8 + title->px + body->px;
    if (!radio_ok() && !on && g_job == JOB_NONE)
        y += line(body, margin, y, "No Wi-Fi radio.", C_RED, max_w);
    if (on && g_job != JOB_HOST && g_job != JOB_JOIN) {
        y += line(body, margin, y, "Don't change Wi-Fi in Settings.", C_LIGHT, max_w);
        y += line(body, margin, y, "Connect from RetroArch, PPSSPP, or BananaDS.", C_GREY, max_w);
        if (lk->ip[0]) {
            snprintf(buf, sizeof buf, "This handheld  %s", lk->ip);
            y += line(body, margin, y, buf, C_DIM, max_w);
        }
    }
    if (g_job == JOB_HOST || g_job == JOB_JOIN) {
        if (home) y += line(body, margin, y, "Home Wi-Fi will disconnect.", C_GREY, max_w);
        snprintf(buf, sizeof buf, "%s%s", g_job == JOB_HOST ? "Starting the link" : "Joining the link", ellipsis());
        y += line(body, margin, y, buf, C_LIGHT, max_w);
    } else if (g_job == JOB_DOWN) {
        snprintf(buf, sizeof buf, "%s%s", home ? "Restoring Wi-Fi" : "Closing the link", ellipsis());
        y += line(body, margin, y, buf, C_LIGHT, max_w);
    }
    if (err && err[0] && g_job == JOB_NONE)
        line(body, margin, y, err, C_RED, max_w);
}

static void draw_bottom(const Link *lk)
{
    const Face *body = &g_body;
    int margin = g_w / 18;
    int max_w = g_w - margin * 2;
    int box_h = body->px * 3;
    int box_w, gap, yb, x0, live;
    int on = linked(lk);
    int busy = g_job != JOB_NONE;
    if (box_h < 64) box_h = 64;
    box_w = g_w * 34 / 100;
    if (box_w < body->px * 6) box_w = body->px * 6;
    gap = body->px;
    if (gap < 12) gap = 12;
    yb = (g_h - box_h) / 2;
    live = !busy;
    clear_shadow(C_BG);
    g_nhit = 0;
    if (g_view == VIEW_NEST || g_view == VIEW_NEST_OK) {
        if (g_view == VIEW_NEST) {
            text_wrap(body, margin, g_h / 6, "One folder too deep.", C_LIGHT, max_w, 2);
            text_wrap(body, margin, g_h / 6 + body->px * 2, "direct-link/direct-link", C_GREY, max_w, 1);
            text_wrap(body, margin, g_h / 6 + body->px * 4, "A moves it up one folder.", C_GREY, max_w, 2);
            legend("move it up", "back", NULL);
        } else {
            text_wrap(body, margin, g_h / 5, "Moved.", C_LIGHT, max_w, 1);
            text_wrap(body, margin, g_h / 5 + body->px * 2, "Open Direct Link again.", C_GREY, max_w, 2);
            legend("", "back", NULL);
        }
        return;
    }
    if (g_view == VIEW_UNINSTALL || g_view == VIEW_DISC) {
        const char *q = g_view == VIEW_DISC ? "Disconnect?" : "Delete this app?";
        const char *sub = g_view == VIEW_DISC ? "" : "This deletes Direct Link.";
        const char *yes = g_view == VIEW_DISC ? "Disconnect" : "Uninstall";
        int total = box_w * 2 + gap;
        x0 = (g_w - total) / 2;
        text_wrap(body, margin, g_h / 6, q, C_LIGHT, max_w, 2);
        if (sub[0]) text_wrap(body, margin, g_h / 6 + body->px * 2, sub, C_GREY, max_w, 2);
        square(x0, yb, box_w, box_h, yes, g_view == VIEW_DISC ? ST_RED : ST_ON, ACT_OK, 1);
        square(x0 + box_w + gap, yb, box_w, box_h, "Cancel", ST_IDLE, ACT_BACK, 1);
        legend(g_view == VIEW_DISC ? "disconnect" : "uninstall", "cancel", NULL);
        return;
    }
    if (on) {
        x0 = (g_w - box_w) / 2;
        square(x0, yb, box_w, box_h, "Disconnect", busy ? ST_DIM : ST_RED, ACT_DISC, live);
        if (!busy) legend("disconnect", "back", NULL);
        return;
    }
    if (!radio_ok()) {
        text_wrap(body, margin, g_h / 3, "No Wi-Fi radio.", C_RED, max_w, 2);
        if (!busy) legend(NULL, "back", "uninstall");
        return;
    }
    {
        int total = box_w * 2 + gap;
        int st0 = busy ? ST_DIM : (g_focus == 0 ? ST_ON : ST_IDLE);
        int st1 = busy ? ST_DIM : (g_focus == 1 ? ST_ON : ST_IDLE);
        x0 = (g_w - total) / 2;
        square(x0, yb, box_w, box_h, "Host", st0, ACT_HOST, live);
        square(x0 + box_w + gap, yb, box_w, box_h, "Join", st1, ACT_JOIN, live);
        if (!busy) legend(g_focus == 0 ? "host" : "join", "back", "uninstall");
    }
}

static void draw(const Link *lk, const char *err)
{
    int s;
    g_nhit = 0;
    if (g_view == VIEW_NEST || g_view == VIEW_NEST_OK) {
        for (s = 0; s < 2; s++) {
            draw_bottom(lk);
            blit(&g_p[s]);
        }
    } else {
        draw_top(lk, err);
        blit(&g_p[0]);
        draw_bottom(lk);
        blit(&g_p[1]);
    }
    present();
}

static void note_out(char c)
{
    if (c == '\n' || c == '\r') {
        if (g_acc_n > 0) {
            g_acc[g_acc_n] = 0;
            copy_str(g_job_line, sizeof g_job_line, g_acc);
            g_acc_n = 0;
        }
        return;
    }
    if (g_acc_n + 1 < (int)sizeof g_acc) g_acc[g_acc_n++] = c;
}

static void drain_job(void)
{
    char b[128];
    ssize_t n;
    int i;
    if (g_pipe < 0) return;
    while ((n = read(g_pipe, b, sizeof b)) > 0)
        for (i = 0; i < (int)n; i++) note_out(b[i]);
}

static void start_job(int job)
{
    int p[2];
    pid_t pid;
    char path[640];
    const char *script;
    if (g_child > 0) return;
    script = job == JOB_HOST ? "host.sh" : job == JOB_JOIN ? "join.sh" : "down.sh";
    if (pipe(p) < 0) return;
    pid = fork();
    if (pid < 0) { close(p[0]); close(p[1]); return; }
    if (pid == 0) {
        close(p[0]);
        dup2(p[1], 1);
        snprintf(path, sizeof path, "%s/%s", g_app, script);
        execl("/bin/sh", "sh", path, (char *)NULL);
        _exit(127);
    }
    close(p[1]);
    fcntl(p[0], F_SETFL, fcntl(p[0], F_GETFL, 0) | O_NONBLOCK);
    g_pipe = p[0];
    g_child = pid;
    g_job = job;
    g_acc_n = 0;
    g_job_line[0] = 0;
}

static int reap_job(char *err, int errn)
{
    int st, kind;
    pid_t p;
    const char *m;
    if (g_child <= 0) return 0;
    p = waitpid(g_child, &st, WNOHANG);
    if (p == 0 || p < 0) return 0;
    drain_job();
    note_out('\n');
    close(g_pipe);
    g_pipe = -1;
    g_child = 0;
    kind = g_job;
    g_job = JOB_NONE;
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
        m = g_job_line;
        if (!strncmp(m, "error ", 6)) m += 6;
        if (!m[0]) m = kind == JOB_DOWN ? "could not disconnect" : "the link did not start";
        copy_str(err, (size_t)errn, m);
    } else err[0] = 0;
    return 1;
}

static int bit_on(int fd, int ev, int code)
{
    unsigned long b[KEY_MAX / (8 * (int)sizeof(long)) + 1];
    memset(b, 0, sizeof b);
    if (ioctl(fd, EVIOCGBIT(ev, sizeof b), b) < 0) return 0;
    return (b[code / (8 * (int)sizeof(long))] >> (code % (8 * (int)sizeof(long)))) & 1;
}

typedef struct {
    int fd;
    int touch;
    int minx, maxx, miny, maxy;
    int x, y, down, was;
} Dev;

static int key_action(int code, int value)
{
    if (value != 1) return ACT_NONE;
    switch (code) {
    case KEY_LEFT: case BTN_DPAD_LEFT: case KEY_UP: case BTN_DPAD_UP: return ACT_LEFT;
    case KEY_RIGHT: case BTN_DPAD_RIGHT: case KEY_DOWN: case BTN_DPAD_DOWN: return ACT_RIGHT;
    case BTN_EAST: case KEY_ENTER: return ACT_OK;
    case BTN_WEST: return ACT_Y;
    case BTN_SOUTH: case KEY_BACKSPACE: case KEY_ESC: return ACT_BACK;
    default: return ACT_NONE;
    }
}

static int touch_px(int v, int lo, int hi, int span)
{
    if (hi <= lo || span <= 0) return 0;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return (int)((long)(v - lo) * span / (hi - lo + 1));
}

static int app_is_nested(void)
{
    const char *tail = "/direct-link/direct-link";
    size_t n = strlen(g_app), t = strlen(tail);
    if (n < t) return 0;
    return strcmp(g_app + n - t, tail) == 0;
}

static int app_dir(void)
{
    char exe[512];
    ssize_t n;
    char *slash;
    n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n < 0) return 0;
    exe[n] = 0;
    slash = strrchr(exe, '/');
    if (!slash) return 0;
    *slash = 0;
    slash = strrchr(exe, '/');
    if (!slash) return 0;
    *slash = 0;
    snprintf(g_app, sizeof g_app, "%s", exe);
    return 1;
}

static void handle(int act, Link *lk, char *err, int errn)
{
    if (act == ACT_NONE || g_child > 0) return;
    if (g_view == VIEW_NEST) {
        if (act == ACT_OK) {
            if (run_sh("fix-nest.sh", err, errn)) g_view = VIEW_NEST_OK;
        } else if (act == ACT_BACK) g_stop = 1;
        return;
    }
    if (g_view == VIEW_NEST_OK) { g_stop = 1; return; }
    if (g_view == VIEW_UNINSTALL) {
        if (act == ACT_OK) {
            if (run_sh("uninstall.sh", err, errn)) g_stop = 1;
            else g_view = VIEW_MAIN;
        } else if (act == ACT_BACK) { g_view = VIEW_MAIN; err[0] = 0; }
        return;
    }
    if (g_view == VIEW_DISC) {
        if (act == ACT_OK) { g_view = VIEW_MAIN; err[0] = 0; start_job(JOB_DOWN); }
        else if (act == ACT_BACK) { g_view = VIEW_MAIN; err[0] = 0; }
        return;
    }
    if (act == ACT_BACK) { g_stop = 1; return; }
    if (linked(lk)) {
        if (act == ACT_OK || act == ACT_DISC) g_view = VIEW_DISC;
        return;
    }
    if (!radio_ok()) {
        if (act == ACT_Y) g_view = VIEW_UNINSTALL;
        return;
    }
    if (act == ACT_LEFT) g_focus = 0;
    else if (act == ACT_RIGHT) g_focus = 1;
    else if (act == ACT_HOST) { g_focus = 0; err[0] = 0; start_job(JOB_HOST); }
    else if (act == ACT_JOIN) { g_focus = 1; err[0] = 0; start_job(JOB_JOIN); }
    else if (act == ACT_OK) { err[0] = 0; start_job(g_focus == 0 ? JOB_HOST : JOB_JOIN); }
    else if (act == ACT_Y) g_view = VIEW_UNINSTALL;
}

int main(void)
{
    Link lk;
    char err[240];
    Dev dev[32];
    int ndev = 0, i, dirty = 1, dots = -1;
    DIR *d;
    struct dirent *e;
    err[0] = 0;
    memset(&lk, 0, sizeof lk);
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    signal(SIGCHLD, SIG_DFL);
    if (!app_dir()) { fprintf(stderr, "direct-link: cannot find the app directory\n"); return 1; }
    if (app_is_nested()) g_view = VIEW_NEST;
    if (!display_open()) return 1;
    g_shadow = calloc((size_t)g_w * (size_t)g_h, sizeof *g_shadow);
    if (!g_shadow) { display_close(); return 1; }
    if (!load_font()) fprintf(stderr, "direct-link: Pixelify Sans was not found\n");
    d = opendir("/dev/input");
    if (d) {
        while ((e = readdir(d)) && ndev < 32) {
            char path[320], sys[320], real[PATH_MAX];
            int fd, bottom = 0;
            if (strncmp(e->d_name, "event", 5)) continue;
            snprintf(path, sizeof path, "/dev/input/%s", e->d_name);
            fd = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
            if (fd < 0) continue;
            snprintf(sys, sizeof sys, "/sys/class/input/%s/device", e->d_name);
            if (realpath(sys, real) && strstr(real, "fe5e0000.i2c")) bottom = 1;
            memset(&dev[ndev], 0, sizeof dev[ndev]);
            dev[ndev].fd = fd;
            if (bottom && (bit_on(fd, EV_ABS, ABS_MT_POSITION_X) || bit_on(fd, EV_ABS, ABS_X))) {
                struct input_absinfo ax, ay;
                int mx = bit_on(fd, EV_ABS, ABS_MT_POSITION_X) ? ABS_MT_POSITION_X : ABS_X;
                int my = bit_on(fd, EV_ABS, ABS_MT_POSITION_Y) ? ABS_MT_POSITION_Y : ABS_Y;
                memset(&ax, 0, sizeof ax);
                memset(&ay, 0, sizeof ay);
                ioctl(fd, EVIOCGABS(mx), &ax);
                ioctl(fd, EVIOCGABS(my), &ay);
                dev[ndev].touch = 1;
                dev[ndev].minx = ax.minimum;
                dev[ndev].maxx = ax.maximum;
                dev[ndev].miny = ay.minimum;
                dev[ndev].maxy = ay.maximum;
                dev[ndev].x = ax.value;
                dev[ndev].y = ay.value;
            }
            ndev++;
        }
        closedir(d);
    }
    load_link(&lk);
    while (!g_stop) {
        struct pollfd pf[33];
        int nf = 0, r, act = ACT_NONE, now_dots;
        for (i = 0; i < ndev; i++) {
            pf[nf].fd = dev[i].fd;
            pf[nf].events = POLLIN;
            nf++;
        }
        if (g_pipe >= 0) { pf[nf].fd = g_pipe; pf[nf].events = POLLIN; nf++; }
        if (dirty) draw(&lk, err);
        dirty = 0;
        r = poll(pf, (nfds_t)nf, g_child > 0 ? 400 : 1000);
        if (r < 0) { if (errno == EINTR) continue; break; }
        if (g_pipe >= 0 && nf > 0 && (pf[nf - 1].revents & POLLIN)) drain_job();
        for (i = 0; i < ndev; i++) {
            struct input_event ev;
            if (!(pf[i].revents & POLLIN)) continue;
            while (read(dev[i].fd, &ev, sizeof ev) == (ssize_t)sizeof ev) {
                if (dev[i].touch) {
                    if (ev.type == EV_ABS && (ev.code == ABS_MT_POSITION_X || ev.code == ABS_X))
                        dev[i].x = ev.value;
                    else if (ev.type == EV_ABS && (ev.code == ABS_MT_POSITION_Y || ev.code == ABS_Y))
                        dev[i].y = ev.value;
                    else if (ev.type == EV_ABS && ev.code == ABS_MT_TRACKING_ID)
                        dev[i].down = ev.value >= 0;
                    else if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
                        dev[i].down = ev.value != 0;
                    else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                        if (!dev[i].down && dev[i].was) {
                            int hx = touch_px(dev[i].x, dev[i].minx, dev[i].maxx, g_w);
                            int hy = touch_px(dev[i].y, dev[i].miny, dev[i].maxy, g_h);
                            int h, id = ACT_NONE;
                            for (h = 0; h < g_nhit; h++)
                                if (hx >= g_hit[h].x0 && hx < g_hit[h].x1 && hy >= g_hit[h].y0 && hy < g_hit[h].y1)
                                    id = g_hit[h].id;
                            if (id) act = id;
                        }
                        dev[i].was = dev[i].down;
                    }
                    continue;
                }
                if (ev.type == EV_KEY) {
                    int a = key_action(ev.code, ev.value);
                    if (a) act = a;
                }
            }
        }
        int was_busy = g_child > 0;
        if (reap_job(err, (int)sizeof err)) { load_link(&lk); dirty = 1; }
        if (!was_busy) handle(act, &lk, err, (int)sizeof err);
        if (act && !was_busy) dirty = 1;
        now_dots = g_child > 0 ? (ticks() / 400) % 3 : -1;
        if (g_child > 0 && now_dots != dots) {
            dots = now_dots;
            load_link(&lk);
            dirty = 1;
        }
        if (g_child <= 0 && r == 0) {
            Link n;
            load_link(&n);
            if (memcmp(&n, &lk, sizeof lk) != 0) { lk = n; dirty = 1; }
        }
    }
    if (g_pipe >= 0) close(g_pipe);
    for (i = 0; i < ndev; i++) close(dev[i].fd);
    display_close();
    free(g_shadow);
    free(g_ttf);
    return 0;
}
