/* Preview: Direct Link's panels as PNG files, on a computer (build/preview.sh). Both panels in one picture, top
   over bottom, for each panel size (RG DS 640x480, RG DS Plus 1024x768) and theme. Nothing here runs on a handheld. */
#include "pixel_png.h"
#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Model base(void)
{
    Model m;
    memset(&m, 0, sizeof m);
    snprintf(m.state, sizeof m.state, "down");
    snprintf(m.home, sizeof m.home, "on");
    snprintf(m.version, sizeof m.version, "0.1.0");
    m.radio = 1;
    m.battery = 64;
    snprintf(m.clock, sizeof m.clock, "14:30");
    m.upd.state = UPD_UPTODATE;
    snprintf(m.upd.current, sizeof m.upd.current, "0.1.0");
    snprintf(m.upd.latest, sizeof m.upd.latest, "0.1.0");
    return m;
}

static Model linked(void)
{
    Model m = base();
    snprintf(m.state, sizeof m.state, "up");
    snprintf(m.role, sizeof m.role, "host");
    snprintf(m.ip, sizeof m.ip, "10.42.0.1");
    snprintf(m.peer, sizeof m.peer, "10.42.0.2");
    m.linked = 1;
    m.cursor = ROW_STATUS;
    return m;
}

static int shot(const char *dir, const char *name, int w, int h, const char *theme, const char *accent,
                const char *lang, const Model *m)
{
    PxSettings st;
    PxCanvas top, bot;
    int gap = h / 24, ok, i;
    uint32_t *buf = calloc((size_t)w * (size_t)(h * 2 + gap), sizeof *buf);
    char path[512];
    if (!buf) return 0;
    px_settings_read("/nonexistent", &st);
    snprintf(st.theme, sizeof st.theme, "%s", theme);
    snprintf(st.accent, sizeof st.accent, "%s", accent);
    px_pal_make(&px, &st);
    px_lang_set(lang, view_tr);
    px_lang_add(upd_tr);
    px_canvas(&top, buf, w, h);
    px_canvas(&bot, buf + (size_t)w * (size_t)(h + gap), w, h);
    for (i = 0; i < w * gap; i++) buf[(size_t)w * (size_t)h + (size_t)i] = 0x000000;
    view_top(&top, m);
    view_bottom(&bot, m);
    {
        const char *stamp = getenv("PREVIEW_STAMP");
        if (stamp && stamp[0])
            snprintf(path, sizeof path, "%s/%s-direct-link-%s-%dx%d-%s%s%s.png", dir, stamp, name, w, h,
                     strncmp(theme, "user:", 5) ? theme : theme + 5,
                     strcmp(lang, "en") ? "-" : "", strcmp(lang, "en") ? lang : "");
        else
            snprintf(path, sizeof path, "%s/direct-link-%s-%dx%d-%s%s%s.png", dir, name, w, h,
                     strncmp(theme, "user:", 5) ? theme : theme + 5,
                     strcmp(lang, "en") ? "-" : "", strcmp(lang, "en") ? lang : "");
    }
    ok = px_png_write(path, buf, w, h * 2 + gap);
    if (ok) printf("%s\n", path);
    free(buf);
    return ok;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    static const int sizes[2][2] = { { 640, 480 }, { 1024, 768 } };
    static const char *const themes[2] = { "dark", "light" };
    Model m;
    int s, t, ok = 1;
    if (!px_fonts_load(NULL)) { fprintf(stderr, "preview: set PIXEL_UI_FONTS to a folder with Pixelify Sans\n"); return 1; }
    for (s = 0; s < 2; s++)
        for (t = 0; t < 2; t++) {
            m = base();
            ok &= shot(dir, "wifi", sizes[s][0], sizes[s][1], themes[t], "yellow", "en", &m);
            m = linked();
            ok &= shot(dir, "linked", sizes[s][0], sizes[s][1], themes[t], "yellow", "en", &m);
        }
    m = base();
    m.mode = MODE_ADAPTER;
    ok &= shot(dir, "adapter", 640, 480, "dark", "yellow", "en", &m);
    m = base();
    m.mode = MODE_USB;
    ok &= shot(dir, "usb-cable", 640, 480, "dark", "yellow", "en", &m);
    ok &= shot(dir, "usb-cable", 1024, 768, "light", "blue", "es", &m);
    m = base();
    m.job = JOB_JOIN;
    m.role_sel = 1;
    m.dots = 2;
    m.cursor = ROW_ROLE;
    ok &= shot(dir, "joining", 640, 480, "dark", "green", "es", &m);
    m = linked();
    m.view = V_DISC;
    ok &= shot(dir, "disconnect", 640, 480, "dark", "yellow", "es", &m);
    /* a guest, linked; then its host gone quiet (wifi-link looking for it again) */
    m = linked();
    snprintf(m.role, sizeof m.role, "join");
    snprintf(m.ip, sizeof m.ip, "10.42.0.2");
    snprintf(m.peer, sizeof m.peer, "10.42.0.1");
    m.role_sel = 1;
    m.cursor = ROW_STATUS;
    m.peers = 1;
    ok &= shot(dir, "guest", 640, 480, "dark", "yellow", "es", &m);
    m.lost = 1;
    m.peer[0] = 0;
    m.dots = 1;
    ok &= shot(dir, "host-lost", 640, 480, "dark", "yellow", "es", &m);
    ok &= shot(dir, "host-lost", 1024, 768, "light", "yellow", "en", &m);
    /* the link closed by wifi-link after that */
    m = base();
    m.role_sel = 1;
    snprintf(m.err, sizeof m.err, "El anfitrión cerró el enlace. Volvió el Wi-Fi de casa.");
    ok &= shot(dir, "host-left", 640, 480, "dark", "yellow", "es", &m);
    m = base();
    m.view = V_ABOUT;
    ok &= shot(dir, "about", 640, 480, "dark", "yellow", "es", &m);
    ok &= shot(dir, "about", 1024, 768, "light", "yellow", "en", &m);
    /* BananaOS's own themes */
    m = linked();
    ok &= shot(dir, "linked", 640, 480, "user:sakura", "yellow", "es", &m);
    ok &= shot(dir, "linked", 640, 480, "user:paper-notebook", "yellow", "en", &m);
    m = base();
    ok &= shot(dir, "wifi", 640, 480, "user:comic-panel", "yellow", "en", &m);
    ok &= shot(dir, "wifi", 640, 480, "user:neon-arcade", "yellow", "es", &m);
    /* the updater */
    m = base();
    m.view = V_SETTINGS;
    m.cursor = SET_UPDATES;
    ok &= shot(dir, "update-uptodate", 640, 480, "dark", "yellow", "es", &m);
    m.upd.state = UPD_AVAILABLE;
    snprintf(m.upd.latest, sizeof m.upd.latest, "0.2.0");
    ok &= shot(dir, "update-available", 640, 480, "light", "yellow", "en", &m);
    ok &= shot(dir, "update-available", 1024, 768, "dark", "yellow", "es", &m);
    m.view = V_UPDATE;
    ok &= shot(dir, "update-confirm", 640, 480, "dark", "yellow", "es", &m);
    m.view = V_MAIN;
    m.upd.state = UPD_DOWNLOADING;
    m.upd.installing = 1;
    m.upd.pid = 1;
    m.upd.pct = 73;
    ok &= shot(dir, "update-downloading", 640, 480, "dark", "yellow", "es", &m);
    m.upd.pid = 0;
    m.upd.state = UPD_DONE;
    snprintf(m.upd.current, sizeof m.upd.current, "0.2.0");
    m.view = V_UPDATED;
    ok &= shot(dir, "update-done", 1024, 768, "light", "yellow", "en", &m);
    m = linked();
    m.view = V_SETTINGS;
    m.cursor = SET_UPDATES;
    m.peers = 2;
    m.upd.state = UPD_OFFLINE;
    ok &= shot(dir, "update-linked", 640, 480, "dark", "yellow", "es", &m);
    m = linked();
    ok &= shot(dir, "readme", 1024, 768, "dark", "yellow", "en", &m);
    ok &= shot(dir, "readme", 1024, 768, "light", "yellow", "es", &m);
    px_fonts_free();
    return ok ? 0 : 1;
}
