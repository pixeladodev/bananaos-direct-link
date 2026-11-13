/* Direct Link: the two panels in the menu's look (pixel.c).
   Top: the mode, a drawing of how the two consoles are linked in it, and what is happening.
   Bottom: a list as Settings and Bshare draw theirs (Mode, Role, Status, the two addresses), keys along the bottom.
   Only the Wi-Fi mode links today; Adapter and USB-C cable are there to show where the app is going. */
#include "view.h"

#include <stdio.h>
#include <string.h>

Hit g_hits[MAX_HITS];
int g_nhits;

/* Spanish: neutral, and the words BananaOS's own translation uses (Conectar, Desconectar, Unirse, Consola...) */
const PxTr view_tr[] = {
    { "Mode", "Modo" },
    { "Role", "Rol" },
    { "Status", "Estado" },
    { "This console", "Esta consola" },
    { "Other console", "Otra consola" },
    { "Adapter", "Adaptador" },
    { "USB-C cable", "Cable USB-C" },
    { "Host", "Anfitrión" },
    { "Join", "Unirse" },
    { "Guest", "Invitado" },
    { "Searching", "Buscando" },
    { "Host lost", "Sin anfitrión" },
    { "The host stopped answering. Looking for it again",
      "El anfitrión dejó de responder. Buscándolo de nuevo" },
    { "If it doesn't come back in a minute, the link closes and home Wi-Fi returns.",
      "Si no vuelve en un minuto, el enlace se cierra y vuelve el Wi-Fi de casa." },
    { "If it doesn't come back in a minute, the link closes.",
      "Si no vuelve en un minuto, el enlace se cierra." },
    { "The host closed the link.", "El anfitrión cerró el enlace." },
    { "Home Wi-Fi is back.", "Volvió el Wi-Fi de casa." },
    { "About", "Acerca de" },
    { "Made by", "Hecha por" },
    { "Source code", "Código fuente" },
    { "Credits", "Créditos" },
    { "Two consoles play together over their own Wi-Fi, no router needed: RetroArch, PPSSPP and BananaDS netplay on BananaOS.",
      "Dos consolas juegan juntas por su propio Wi-Fi, sin router: el juego en red de RetroArch, PPSSPP y BananaDS en BananaOS." },
    { "BananaOS and BananaLinux by JorreFog and the Banana contributors (MIT); wifi-link is adapted from BananaLinux. Pixelify Sans (OFL).",
      "BananaOS y BananaLinux, de JorreFog y los colaboradores de Banana (MIT); wifi-link está adaptado de BananaLinux. Pixelify Sans (OFL)." },
    { "An independent app, not part of BananaOS. Built and tested on the RG DS and the RG DS Plus. Free software under the MIT license.",
      "Una app independiente, no es parte de BananaOS. Hecha y probada en la RG DS y la RG DS Plus. Software libre bajo la licencia MIT." },
    { "Off", "Apagado" },
    { "Exit", "Salir" },
    { "Connect", "Conectar" },
    { "Disconnect", "Desconectar" },
    { "Uninstall", "Desinstalar" },
    { "Cancel", "Cancelar" },
    { "Back", "Atrás" },
    { "Move it", "Moverla" },
    { "Waiting", "Esperando" },
    { "Coming soon", "Próximamente" },
    { "Needs BananaOS support", "Requiere soporte de BananaOS" },
    { "Starting", "Iniciando" },
    { "Joining", "Uniéndose" },
    { "Closing", "Cerrando" },
    { "Linked", "Enlazado" },
    { "No Wi-Fi radio.", "No hay radio Wi-Fi." },
    { "Don't change Wi-Fi in Settings.", "No cambies el Wi-Fi en Ajustes." },
    { "Connect from RetroArch, PPSSPP, or BananaDS.", "Conéctate desde RetroArch, PPSSPP o BananaDS." },
    { "Home Wi-Fi will disconnect.", "El Wi-Fi de casa se desconectará." },
    { "Starting the link", "Iniciando el enlace" },
    { "Joining the link", "Uniéndose al enlace" },
    { "Restoring Wi-Fi", "Restaurando el Wi-Fi" },
    { "Closing the link", "Cerrando el enlace" },
    { "Two consoles on their own Wi-Fi, no router. Host on one, Guest on the other.",
      "Dos consolas en su propio Wi-Fi, sin router. Anfitrión en una, Invitado en la otra." },
    { "Coming soon: a USB-C Ethernet adapter on each console, joined by a network cable.",
      "Próximamente: un adaptador USB-C a Ethernet en cada consola, unidos por un cable de red." },
    { "Needs a BananaOS with USB link: the OTG port of one console to the DC/USB port of the other.",
      "Requiere un BananaOS con enlace USB: el puerto OTG de una consola al puerto DC/USB de la otra." },
    { "The console on the OTG side powers the other.", "La consola del lado OTG le da energía a la otra." },
    { "network cable", "cable de red" },
    { "Disconnect?", "¿Desconectar?" },
    { "Home Wi-Fi comes back.", "Vuelve el Wi-Fi de casa." },
    { "Delete this app?", "¿Borrar esta app?" },
    { "This deletes Direct Link.", "Esto borra Direct Link." },
    { "One folder too deep.", "Una carpeta de más." },
    { "A moves it up one folder.", "A la sube una carpeta." },
    { "Moved.", "Movida." },
    { "Open Direct Link again.", "Abre Direct Link de nuevo." },
    { NULL, NULL }
};

#define T(s) px_tr(s)

static const char *const MODE_NAMES[MODE_COUNT] = { "Wi-Fi", "Adapter", "USB-C cable" };

int view_mode_works(int mode)
{
    return mode == MODE_WIFI;
}

int view_can_connect(const Model *m)
{
    return m->view == V_MAIN && !m->linked && m->job == JOB_NONE && m->radio && view_mode_works(m->mode);
}

static void add_hit(const PxCanvas *c, float x, float y, float w, float h, int id, int arg)
{
    if (g_nhits >= MAX_HITS) return;
    g_hits[g_nhits].r.x = px_u(c, x);
    g_hits[g_nhits].r.y = px_u(c, y);
    g_hits[g_nhits].r.w = px_u(c, x + w) - g_hits[g_nhits].r.x;
    g_hits[g_nhits].r.h = px_u(c, y + h) - g_hits[g_nhits].r.y;
    g_hits[g_nhits].id = id;
    g_hits[g_nhits].arg = arg;
    g_nhits++;
}

static void add_face_hit(PxRect r, int id)
{
    if (g_nhits >= MAX_HITS) return;
    g_hits[g_nhits].r = r;
    g_hits[g_nhits].id = id;
    g_hits[g_nhits].arg = 0;
    g_nhits++;
}

static int ndots(const Model *m)
{
    return m->dots + 1;
}

/* the status, in a word or two; returns the dots it ends in (-1: none) */
static int status_word(const Model *m, char *buf, size_t n)
{
    if (m->job == JOB_HOST) { snprintf(buf, n, "%s", T("Starting")); return ndots(m); }
    if (m->job == JOB_JOIN) { snprintf(buf, n, "%s", T("Joining")); return ndots(m); }
    if (m->job == JOB_DOWN) { snprintf(buf, n, "%s", T("Closing")); return ndots(m); }
    if (!m->linked) { snprintf(buf, n, "%s", T("Off")); return -1; }
    if (m->lost) { snprintf(buf, n, "%s", T("Searching")); return ndots(m); }
    snprintf(buf, n, "%s", !strcmp(m->role, "join") ? T("Guest") : T("Host"));
    return -1;
}

/* ---- the drawings ---------------------------------------------------------------------------------------- */

/* a dual-screen handheld, open: x, y its top left, 120 x 104 */
static void console(PxCanvas *c, float x, float y, int lit)
{
    uint32_t body = px.light ? 0xe9e3d6 : 0x2e2b33, rim = px.edge;
    uint32_t glass = lit ? px_mix(px.accent.dark, px.accent.base, 0.35f) : (px.light ? 0xc9c0ae : 0x100f14);
    px_round(c, x - 3, y - 3, 126, 52, 8, rim, 255);
    px_round(c, x, y, 120, 46, 6, body, 255);
    px_rect(c, x + 12, y + 7, 96, 32, glass);
    if (lit) px_rect(c, x + 12, y + 7, 96, 6, px.accent.base);
    px_rect(c, x + 8, y + 49, 104, 4, rim);                     /* the hinge */
    px_round(c, x - 3, y + 53, 126, 54, 8, rim, 255);
    px_round(c, x, y + 56, 120, 48, 6, body, 255);
    px_rect(c, x + 30, y + 62, 60, 30, glass);
    /* the d-pad and the face buttons */
    px_rect(c, x + 10, y + 72, 12, 4, px.muted);
    px_rect(c, x + 14, y + 68, 4, 12, px.muted);
    px_rect(c, x + 100, y + 68, 5, 5, px.muted);
    px_rect(c, x + 106, y + 74, 5, 5, px.muted);
    px_rect(c, x + 94, y + 74, 5, 5, px.muted);
    px_rect(c, x + 100, y + 80, 5, 5, px.muted);
}

/* a short dashed line between two points on a row */
static void dashed(PxCanvas *c, float x0, float x1, float y, uint32_t rgb)
{
    float x;
    for (x = x0; x < x1; x += 12) px_rect(c, x, y, x + 7 > x1 ? x1 - x : 7, 3, rgb);
}

/* Wi-Fi: three arcs from each console, drawn as pixel steps */
static void waves(PxCanvas *c, float x, float y, int dir, uint32_t rgb)
{
    int k, i;
    for (k = 0; k < 3; k++) {
        float r = 10 + k * 10;
        for (i = -3; i <= 3; i++) {
            float dy = i * r / 4.0f, dx = r - (i * i) * r / 24.0f;
            px_rect(c, x + dir * dx - 2, y + dy - 2, 4, 4, rgb);
        }
    }
}

static void tag(PxCanvas *c, float cx, float y, const char *s, int active)
{
    float w = px_text_w(c, PX_MEDIUM, 13, s, PX_UPPER | PX_TRACK) + 18;
    px_chip(c, cx - w / 2, y, w, 24, active);
    px_text(c, PX_MEDIUM, 13, cx - w / 2 + 9, y + 5, s, active ? px_ink_on(px.accent.base) : px.ink,
            PX_UPPER | PX_TRACK);
}

static void diagram(PxCanvas *c, const Model *m)
{
    const float y = 168, lx = 90, rx = 430;
    int live = m->linked;
    uint32_t wire = view_mode_works(m->mode) ? px.accent.base : px.muted;
    console(c, lx, y, live && !(m->lost && !strcmp(m->role, "join")));
    console(c, rx, y, live);
    if (m->mode == MODE_WIFI) {
        waves(c, lx + 132, y + 50, 1, wire);
        waves(c, rx - 12, y + 50, -1, wire);
        tag(c, lx + 60, y + 118, "10.42.0.1", live && !m->lost && strcmp(m->role, "join"));
        tag(c, rx + 60, y + 118, "10.42.0.2-9", live && !strcmp(m->role, "join"));
    } else if (m->mode == MODE_ADAPTER) {
        /* each console: a USB-C to Ethernet adapter; a network cable between them */
        px_rect(c, lx + 123, y + 76, 14, 6, px.muted);
        px_round(c, lx + 137, y + 66, 30, 26, 4, px.muted, 255);
        px_rect(c, lx + 143, y + 72, 18, 14, px.screen_bg);
        px_rect(c, rx - 17, y + 76, 14, 6, px.muted);
        px_round(c, rx - 47, y + 66, 30, 26, 4, px.muted, 255);
        px_rect(c, rx - 41, y + 72, 18, 14, px.screen_bg);
        dashed(c, lx + 170, rx - 50, y + 78, wire);
        px_text_fit(c, PX_REGULAR, 13, 320, y + 92, T("network cable"), px.muted, PX_UPPER | PX_TRACK, 200, PX_CENTER);
        tag(c, lx + 60, y + 118, "USB-C > LAN", 0);
        tag(c, rx + 60, y + 118, "USB-C > LAN", 0);
    } else {
        /* the OTG port of one console to the DC/USB port of the other; the OTG side gives the power */
        px_rect(c, lx + 123, y + 76, 10, 6, px.muted);
        px_rect(c, rx - 13, y + 76, 10, 6, px.muted);
        dashed(c, lx + 136, rx - 16, y + 78, wire);
        /* a bolt over the cable: the power goes from left to right */
        px_rect(c, 314, y + 56, 8, 6, px.accent.base);
        px_rect(c, 310, y + 62, 14, 4, px.accent.base);
        px_rect(c, 316, y + 66, 6, 8, px.accent.base);
        px_triangle(c, rx - 30, y + 64, 10, 1, px.muted);
        tag(c, lx + 60, y + 118, "OTG", 0);
        tag(c, rx + 60, y + 118, "DC/USB", 0);
    }
}

void view_top(PxCanvas *c, const Model *m)
{
    char buf[160];
    const char *lines[4];
    uint32_t rgb[4];
    int n = 0, i;
    float hw, cw, y;
    px_screen(c);
    px_status(c, "Direct Link", m->battery, m->charging, m->clock);
    px_sheet(c, 18, 60, 604, 406);
    px_label(c, 44, 84, T("Mode"), px.ink);
    hw = px_text(c, PX_MEDIUM, 40, 42, 102, T(MODE_NAMES[m->mode]), px.ink, 0);
    if (view_mode_works(m->mode)) {
        /* the state, big: green linked, grey off, the accent on its way (or looking for a lost host) */
        int d = status_word(m, buf, sizeof buf);
        int kind = m->job != JOB_NONE || m->lost ? PX_BADGE_BUSY : m->linked ? PX_BADGE_ON : PX_BADGE_OFF;
        float bh = 46;
        if (42 + hw + 20 + px_badge_w(c, bh, buf) + (d >= 0 ? px_text_w(c, PX_MEDIUM, bh / 2, "...", PX_TRACK) : 0) > 598)
            bh = 38;
        px_badge_dots(c, 42 + hw + 20, 104 + (46 - bh) / 2, bh, buf, d, kind);
    } else {
        snprintf(buf, sizeof buf, "%s", m->mode == MODE_ADAPTER ? T("Coming soon") : T("Needs BananaOS support"));
        cw = px_text_w(c, PX_MEDIUM, 15, buf, PX_UPPER | PX_TRACK) + 24;
        if (42 + hw + 20 + cw > 598) cw = 598 - (42 + hw + 20);
        px_chip(c, 42 + hw + 20, 112, cw, 30, 0);
        px_text_fit(c, PX_MEDIUM, 15, 42 + hw + 32, 117, buf, px.muted, PX_UPPER | PX_TRACK, cw - 22, PX_LEFT);
    }
    px_line_h(c, 42, 156, 556, 2, px.edge, 70);
    diagram(c, m);
    px_line_h(c, 42, 326, 556, 2, px.edge, 70);

    /* what is happening, as the app has always said it */
    if (m->mode == MODE_ADAPTER) {
        lines[n] = T("Coming soon: a USB-C Ethernet adapter on each console, joined by a network cable.");
        rgb[n++] = px.ink;
    } else if (m->mode == MODE_USB) {
        lines[n] = T("Needs a BananaOS with USB link: the OTG port of one console to the DC/USB port of the other.");
        rgb[n++] = px.ink;
        lines[n] = T("The console on the OTG side powers the other.");
        rgb[n++] = px.muted;
    } else if (m->job == JOB_HOST || m->job == JOB_JOIN) {
        if (!strcmp(m->home, "on")) { lines[n] = T("Home Wi-Fi will disconnect."); rgb[n++] = px.muted; }
        snprintf(buf, sizeof buf, "%s%.*s", m->job == JOB_HOST ? T("Starting the link") : T("Joining the link"), ndots(m), "...");
        lines[n] = buf;
        rgb[n++] = px.ink;
    } else if (m->job == JOB_DOWN) {
        snprintf(buf, sizeof buf, "%s%.*s", !strcmp(m->home, "on") ? T("Restoring Wi-Fi") : T("Closing the link"), ndots(m), "...");
        lines[n] = buf;
        rgb[n++] = px.ink;
    } else if (m->linked && m->lost) {
        snprintf(buf, sizeof buf, "%s%.*s", T("The host stopped answering. Looking for it again"), ndots(m), "...");
        lines[n] = buf; rgb[n++] = px.ink;
        lines[n] = !strcmp(m->home, "on") ? T("If it doesn't come back in a minute, the link closes and home Wi-Fi returns.")
                                          : T("If it doesn't come back in a minute, the link closes.");
        rgb[n++] = px.muted;
    } else if (m->linked) {
        lines[n] = T("Don't change Wi-Fi in Settings."); rgb[n++] = px.ink;
        lines[n] = T("Connect from RetroArch, PPSSPP, or BananaDS."); rgb[n++] = px.muted;
    } else if (!m->radio) {
        lines[n] = T("No Wi-Fi radio."); rgb[n++] = px.warn;
    } else {
        lines[n] = T("Two consoles on their own Wi-Fi, no router. Host on one, Guest on the other.");
        rgb[n++] = px.ink;
    }
    if (m->err[0] && m->job == JOB_NONE && m->mode == MODE_WIFI && n < 4) { lines[n] = m->err; rgb[n++] = m->linked ? px.warn : px.ink; }
    y = 338;
    for (i = 0; i < n && y < 440; i++)
        y += 24 * px_text_wrap(c, PX_REGULAR, 18, 42, y, lines[i], rgb[i], 556, 2, 24) + 4;
}

/* Mode's three choices, as chips: the one shown lit, the ones that don't work yet greyed */
static void mode_chips(PxCanvas *c, const Model *m, float y, int sel)
{
    float w[MODE_COUNT], total = 0, x;
    int i, locked = m->linked || m->job != JOB_NONE;
    for (i = 0; i < MODE_COUNT; i++) {
        w[i] = px_text_w(c, PX_MEDIUM, 16, T(MODE_NAMES[i]), 0) + 20;
        total += w[i] + (i ? 6 : 0);
    }
    x = 570 - total;
    if (!locked) {
        px_triangle(c, x - 12, y + 27, 8, -1, sel ? px.on_pick : px.muted);
        px_triangle(c, 582, y + 27, 8, 1, sel ? px.on_pick : px.muted);
    }
    for (i = 0; i < MODE_COUNT; i++) {
        int on = m->mode == i, works = view_mode_works(i);
        uint32_t ink = on ? px_ink_on(px.accent.base) : works ? px.ink : px.dim;
        if (on && !works) {
            /* shown, but greyed: it can't link */
            px_chip(c, x, y + 12, w[i], 30, 0);
            ink = px.muted;
            px_rect(c, x + 6, y + 37, w[i] - 12, 2, px.accent.base);
        } else px_chip(c, x, y + 12, w[i], 30, on);
        px_text(c, PX_MEDIUM, 16, x + 10, y + 17, T(MODE_NAMES[i]), ink, 0);
        add_hit(c, x, y + 8, w[i], 38, HIT_MODE, i);
        x += w[i] + 6;
    }
}

static void row(PxCanvas *c, const Model *m, int i, float y)
{
    int sel = m->cursor == i && m->view == V_MAIN;
    uint32_t ink = sel ? px.on_pick : px.ink, sub = sel ? px.on_pick_sub : px.muted;
    char buf[96];
    static const char *const titles[ROW_COUNT] = { "Mode", "Role", "Status", "This console", "Other console" };
    px_card(c, 18, y, 588, 54, sel, 0);
    add_hit(c, 18, y, 588, 54, HIT_ROW, i);
    px_text_fit(c, PX_MEDIUM, 20, 38, y + 15, T(titles[i]), ink, 0, i == ROW_MODE ? 120 : 260, PX_LEFT);
    switch (i) {
    case ROW_MODE:
        mode_chips(c, m, y, sel);
        break;
    case ROW_ROLE: {
        int locked = m->linked || m->job != JOB_NONE;
        int join = m->linked ? !strcmp(m->role, "join") : m->role_sel;
        const char *v = join ? T("Guest") : T("Host");
        px_value_arrows(c, 584, y + 15, 20, v, view_mode_works(m->mode) ? ink : sub, !locked, !locked);
        break;
    }
    case ROW_STATUS: {
        int d = status_word(m, buf, sizeof buf);
        if (d >= 0) px_text_dots(c, PX_MEDIUM, 20, 584, y + 15, buf, d, ink, 0, PX_RIGHT);
        else px_text_fit(c, PX_MEDIUM, 20, 584, y + 15, buf, m->linked ? ink : sub, 0, 300, PX_RIGHT);
        break;
    }
    case ROW_IP:
        px_text_fit(c, PX_MEDIUM, 20, 584, y + 15, m->linked && m->ip[0] ? m->ip : "-", m->linked ? ink : sub, 0,
                    300, PX_RIGHT);
        break;
    default:
        if (!m->linked) snprintf(buf, sizeof buf, "-");
        else if (m->lost) snprintf(buf, sizeof buf, "%s", T("Host lost"));
        else if (m->peer[0]) snprintf(buf, sizeof buf, "%s", m->peer);
        else snprintf(buf, sizeof buf, "%s%s", T("Waiting"), "...");
        px_text_fit(c, PX_MEDIUM, 20, 584, y + 15, buf, m->linked && m->peer[0] && !m->lost ? ink : sub, 0, 300,
                    PX_RIGHT);
        break;
    }
}

static void dialog(PxCanvas *c, const char *title, const char *sub, const char *mono)
{
    px_rect_a(c, 0, 0, 640, 480, 0x000000, px.light ? 70 : 150);
    px_card(c, 50, 120, 540, 200, 0, 0);
    px_text_wrap(c, PX_MEDIUM, 26, 76, 146, title, px.ink, 488, 2, 32);
    if (mono) px_text_fit(c, PX_REGULAR, 18, 76, 222, mono, px.muted, 0, 488, PX_LEFT);
    if (sub && sub[0]) px_text_wrap(c, PX_REGULAR, 18, 76, mono ? 252 : 222, sub, px.muted, 488, 3, 25);
}

/* SELECT: who made it, where its source is, what it is built on */
static void view_about(PxCanvas *c, const Model *m)
{
    char buf[96];
    PxRect r;
    float y;
    g_nhits = 0;
    px_screen(c);
    px_bubble(c, 18, 14, 604, 54, 0);
    px_text_fit(c, PX_MEDIUM, 26, 38, 25, T("About"), px.ink, 0, 440, PX_LEFT);
    px_card(c, 18, 82, 604, 326, 0, 0);
    px_text(c, PX_MEDIUM, 28, 40, 98, "Direct Link", px.ink, 0);
    snprintf(buf, sizeof buf, "v%s", m->version[0] ? m->version : "?");
    px_text_fit(c, PX_MEDIUM, 20, 600, 104, buf, px.muted, 0, 160, PX_RIGHT);
    y = 138;
    y += 25 * px_text_wrap(c, PX_REGULAR, 18, 40, y,
                           T("Two consoles play together over their own Wi-Fi, no router needed: RetroArch, PPSSPP and BananaDS netplay on BananaOS."),
                           px.ink, 560, 3, 25) + 10;
    px_label(c, 40, y + 4, T("Made by"), px.muted);
    px_text(c, PX_MEDIUM, 18, 200, y, "Pixelado Dev", px.ink, 0);
    y += 26;
    px_label(c, 40, y + 4, T("Source code"), px.muted);
    px_text_fit(c, PX_MEDIUM, 18, 200, y, "github.com/pixeladodev/bananaos-direct-link", px.ink, 0, 400, PX_LEFT);
    y += 32;
    px_label(c, 40, y, T("Credits"), px.muted);
    y += 20;
    y += 23 * px_text_wrap(c, PX_REGULAR, 16, 40, y,
                           T("BananaOS and BananaLinux by JorreFog and the Banana contributors (MIT); wifi-link is adapted from BananaLinux. Pixelify Sans (OFL)."),
                           px.muted, 560, 3, 23) + 6;
    px_text_wrap(c, PX_REGULAR, 16, 40, y,
                 T("An independent app, not part of BananaOS. Built and tested on the RG DS and the RG DS Plus. Free software under the MIT license."),
                 px.muted, 560, 3, 23);
    px_face(c, 18, 422, PX_LEFT, "B", T("Back"), 0, &r);
    add_face_hit(r, HIT_BACK);
}

void view_bottom(PxCanvas *c, const Model *m)
{
    char count[16];
    PxRect r;
    int i, busy = m->job != JOB_NONE;
    g_nhits = 0;
    px_screen(c);
    px_bubble(c, 18, 14, 604, 54, 0);
    px_text_fit(c, PX_MEDIUM, 26, 38, 25, "Direct Link", px.ink, 0, 440, PX_LEFT);
    snprintf(count, sizeof count, "%02d/%02d", m->cursor + 1, ROW_COUNT);
    px_text(c, PX_MEDIUM, 19, 602 - px_text_w(c, PX_MEDIUM, 19, count, 0), 30, count, px.muted, 0);
    for (i = 0; i < ROW_COUNT; i++) row(c, m, i, 80 + i * 64);
    px_rail(c, 615, 80, 310, m->cursor, 1, ROW_COUNT);

    if (m->view == V_MAIN) {
        PxFace f[4];
        PxRect hr[4];
        int ids[4], n = 0, i;
        f[n].key = "B"; f[n].label = T("Exit"); f[n].dis = busy; ids[n++] = HIT_BACK;
        f[n].key = "SELECT"; f[n].label = T("About"); f[n].dis = 0; ids[n++] = HIT_SELECT;
        if (m->linked) {
            f[n].key = "Y"; f[n].label = T("Disconnect"); f[n].dis = busy; ids[n++] = HIT_Y;
        } else {
            f[n].key = "Y"; f[n].label = T("Uninstall"); f[n].dis = busy; ids[n++] = HIT_Y;
            /* a guest joins; a host connects */
            f[n].key = "A"; f[n].label = m->role_sel ? T("Join") : T("Connect"); f[n].dis = !view_can_connect(m);
            ids[n++] = HIT_OK;
        }
        px_faces(c, 422, n, 2, f, hr);
        for (i = 0; i < n; i++) add_face_hit(hr[i], ids[i]);
        return;
    }
    if (m->view == V_ABOUT) {
        view_about(c, m);
        return;
    }
    g_nhits = 0;
    if (m->view == V_DISC) {
        dialog(c, T("Disconnect?"), !strcmp(m->home, "on") ? T("Home Wi-Fi comes back.") : "", NULL);
        px_face(c, 18, 422, PX_LEFT, "B", T("Cancel"), 0, &r);
        add_face_hit(r, HIT_BACK);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Disconnect"), 0, &r);
        add_face_hit(r, HIT_OK);
    } else if (m->view == V_UNINSTALL) {
        dialog(c, T("Delete this app?"), m->err[0] ? m->err : T("This deletes Direct Link."), NULL);
        px_face(c, 18, 422, PX_LEFT, "B", T("Cancel"), 0, &r);
        add_face_hit(r, HIT_BACK);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Uninstall"), 0, &r);
        add_face_hit(r, HIT_OK);
    } else if (m->view == V_NEST) {
        dialog(c, T("One folder too deep."), m->err[0] ? m->err : T("A moves it up one folder."),
               "direct-link/direct-link");
        px_face(c, 18, 422, PX_LEFT, "B", T("Back"), 0, &r);
        add_face_hit(r, HIT_BACK);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Move it"), 0, &r);
        add_face_hit(r, HIT_OK);
    } else {
        dialog(c, T("Moved."), T("Open Direct Link again."), NULL);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Exit"), 0, &r);
        add_face_hit(r, HIT_OK);
    }
}
