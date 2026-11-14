/* Direct Link's two panels, drawn from a model (main.c fills it on the handheld, preview.c on a computer). */
#ifndef VIEW_H
#define VIEW_H

#include "pixel.h"
#include "updater.h"

enum { V_MAIN, V_SETTINGS, V_UNINSTALL, V_NEST, V_NEST_OK, V_DISC, V_ABOUT, V_UPDATE, V_UPDATED };
enum { ROW_MODE, ROW_ROLE, ROW_STATUS, ROW_COUNT };
enum { SET_UPDATES, SET_ABOUT, SET_COUNT };
/* how the two consoles are linked: only Wi-Fi works today */
enum { MODE_WIFI, MODE_ADAPTER, MODE_USB, MODE_COUNT };
enum { JOB_NONE, JOB_HOST, JOB_JOIN, JOB_DOWN };
/* what a touch on the bottom panel does */
enum { HIT_NONE, HIT_ROW, HIT_MODE, HIT_ROLE, HIT_BACK, HIT_OK, HIT_Y, HIT_MENU };

typedef struct {
    char state[32], role[16], ip[32], home[8], version[32];
    char phase[16];              /* wifi-link's phase: up, joining, starting, lost (a guest that can't hear the host) */
    char peer[48];               /* another console on the link, when it is known */
    int peers;                   /* consoles seen on the link (host may see several) */
    int lost;                    /* a guest whose host stopped answering (wifi-link looks for it again) */
    int radio;                   /* wlan0 is there */
    int linked;
    int job, dots;               /* the script running, and its "..." (0-2) */
    int mode, role_sel;          /* the mode shown; 0 host, 1 guest */
    int view, cursor;
    PxUpdate upd;                /* the Updates row: update.py's last word */
    char err[240];
    int battery, charging;
    char clock[16];
} Model;

typedef struct { PxRect r; int id, arg; } Hit;

#define MAX_HITS 16
extern Hit g_hits[MAX_HITS];
extern int g_nhits;

extern const PxTr view_tr[];
void view_top(PxCanvas *c, const Model *m);
void view_bottom(PxCanvas *c, const Model *m);
int view_mode_works(int mode);
int view_can_connect(const Model *m);

#endif
