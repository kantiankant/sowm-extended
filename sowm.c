// sowm-extended: an itsy bitsy floating window manager - now with features!

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "sowm.h"

static client       *list = {0}, *ws_list[10] = {0}, *cur;
static int          ws = 1, sw, sh, wx, wy, numlock = 0;
static unsigned int ww, wh;

static Display      *d;
static XButtonEvent mouse;
static Window       root;

static Atom net_supported, net_wm_window_type, net_wm_window_type_dock,
            net_wm_strut, net_wm_strut_partial,
            net_number_of_desktops, net_current_desktop,
            net_supporting_wm_check, net_wm_name, ewmh_utf8_string;

static int strut[4] = {0, 0, 0, 0};

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [MappingNotify]    = mapping_notify,
    [DestroyNotify]    = notify_destroy,
    [EnterNotify]      = notify_enter,
    [MotionNotify]     = notify_motion
};

#include "config.h"

void win_focus(client *c) {
    cur = c;
    XSetInputFocus(d, cur->w, RevertToParent, CurrentTime);
}

void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);

    if (list) win_focus(list->prev);
}

void notify_enter(XEvent *e) {
    while(XCheckTypedEvent(d, EnterNotify, e));

    for win if (c->w == e->xcrossing.window) win_focus(c);
}

void notify_motion(XEvent *e) {
    if (!mouse.subwindow || cur->f) return;

    while(XCheckTypedEvent(d, MotionNotify, e));

    int xd = e->xbutton.x_root - mouse.x_root;
    int yd = e->xbutton.y_root - mouse.y_root;

    XMoveResizeWindow(d, mouse.subwindow,
        wx + (mouse.button == 1 ? xd : 0),
        wy + (mouse.button == 1 ? yd : 0),
        MAX(1, ww + (mouse.button == 3 ? xd : 0)),
        MAX(1, wh + (mouse.button == 3 ? yd : 0)));
}

void key_press(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(d, e->xkey.keycode, 0, 0);

    for (unsigned int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].keysym == keysym &&
            mod_clean(keys[i].mod) == mod_clean(e->xkey.state))
            keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
    if (!e->xbutton.subwindow) return;

    win_size(e->xbutton.subwindow, &wx, &wy, &ww, &wh);
    XRaiseWindow(d, e->xbutton.subwindow);
    mouse = e->xbutton;
}

void button_release(XEvent *e) {
    mouse.subwindow = 0;
}

void win_add(Window w) {
    client *c;

    if (!(c = (client *) calloc(1, sizeof(client))))
        exit(1);

    c->w = w;

    if (list) {
        list->prev->next = c;
        c->prev          = list->prev;
        list->prev       = c;
        c->next          = list;

    } else {
        list = c;
        list->prev = list->next = list;
    }

    ws_save(ws);
}

void win_del(Window w) {
    client *x = 0;

    for win if (c->w == w) x = c;

    if (!list || !x)  return;
    if (x->prev == x) list = 0;
    if (list == x)    list = x->next;
    if (x->next)      x->next->prev = x->prev;
    if (x->prev)      x->prev->next = x->next;

    free(x);
    ws_save(ws);
}

void win_kill(const Arg arg) {
    if (cur) XKillClient(d, cur->w);
}

void win_center(const Arg arg) {
    if (!cur) return;

    win_size(cur->w, &(int){0}, &(int){0}, &ww, &wh);
    XMoveWindow(d, cur->w, (sw - ww) / 2, (sh - wh) / 2);
}

void win_fs(const Arg arg) {
    if (!cur) return;

    if ((cur->f = cur->f ? 0 : 1)) {
        win_size(cur->w, &cur->wx, &cur->wy, &cur->ww, &cur->wh);
        XMoveResizeWindow(d, cur->w, 0, 0, sw, sh);

    } else {
        XMoveResizeWindow(d, cur->w, cur->wx, cur->wy, cur->ww, cur->wh);
    }
}

void win_to_ws(const Arg arg) {
    int tmp = ws;

    if (arg.i == tmp) return;
    if (!cur || arg.i == tmp) return;

    ws_sel(arg.i);
    win_add(cur->w);
    ws_save(arg.i);

    ws_sel(tmp);
    win_del(cur->w);
    XUnmapWindow(d, cur->w);
    ws_save(tmp);

    if (list) win_focus(list);
}

void win_prev(const Arg arg) {
    if (!cur) return;

    XRaiseWindow(d, cur->prev->w);
    win_focus(cur->prev);
}

void win_next(const Arg arg) {
    if (!cur) return;

    XRaiseWindow(d, cur->next->w);
    win_focus(cur->next);
}

void ws_go(const Arg arg) {
    int tmp = ws;

    if (arg.i == ws) return;

    ws_save(ws);
    ws_sel(arg.i);

    for win XMapWindow(d, c->w);

    ws_sel(tmp);

    for win XUnmapWindow(d, c->w);

    ws_sel(arg.i);

    if (list) win_focus(list); else cur = 0;

    long cur_ws = ws - 1;
    XChangeProperty(d, root, net_current_desktop, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&cur_ws, 1);
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;

    XConfigureWindow(d, ev->window, ev->value_mask, &(XWindowChanges) {
        .x          = ev->x,
        .y          = ev->y,
        .width      = ev->width,
        .height     = ev->height,
        .sibling    = ev->above,
        .stack_mode = ev->detail
    });
}

static int win_is_dock(Window w) {
    Atom type;
    int fmt;
    unsigned long n, extra;
    unsigned char *data = NULL;

    if (XGetWindowProperty(d, w, net_wm_window_type, 0, 1, False, XA_ATOM,
            &type, &fmt, &n, &extra, &data) == Success && data) {
        int dock = (*(Atom *)data == net_wm_window_type_dock);
        XFree(data);
        return dock;
    }
    return 0;
}

static void update_struts(Window w) {
    Atom type;
    int fmt;
    unsigned long n, extra;
    unsigned char *data = NULL;

    if (XGetWindowProperty(d, w, net_wm_strut_partial, 0, 12, False, XA_CARDINAL,
            &type, &fmt, &n, &extra, &data) == Success && data && n >= 4) {
        long *s = (long *)data;
        strut[0] = MAX(strut[0], (int)s[0]); // left
        strut[1] = MAX(strut[1], (int)s[1]); // right
        strut[2] = MAX(strut[2], (int)s[2]); // top
        strut[3] = MAX(strut[3], (int)s[3]); // bottom
        XFree(data);
        return;
    }
    if (data) XFree(data);

    if (XGetWindowProperty(d, w, net_wm_strut, 0, 4, False, XA_CARDINAL,
            &type, &fmt, &n, &extra, &data) == Success && data && n == 4) {
        long *s = (long *)data;
        strut[0] = MAX(strut[0], (int)s[0]);
        strut[1] = MAX(strut[1], (int)s[1]);
        strut[2] = MAX(strut[2], (int)s[2]);
        strut[3] = MAX(strut[3], (int)s[3]);
        XFree(data);
    }
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

    if (win_is_dock(w)) {
        update_struts(w);
        XMapWindow(d, w);
        return;
    }

    XSelectInput(d, w, StructureNotifyMask|EnterWindowMask);
    win_size(w, &wx, &wy, &ww, &wh);
    win_add(w);
    cur = list->prev;

    if (wx + wy == 0) {
        int cx, cy, dummy;
        unsigned int udummy;
        Window wdummy;
        XQueryPointer(d, root, &wdummy, &wdummy, &cx, &cy, &dummy, &dummy, &udummy);
        win_size(w, &dummy, &dummy, &ww, &wh);
        int ax = strut[0];
        int ay = strut[2];
        int aw = sw - strut[0] - strut[1];
        int ah = sh - strut[2] - strut[3];
        int nx = cx - (int)ww / 2;
        int ny = cy - (int)wh / 2;
        if (nx < ax)                 nx = ax;
        if (ny < ay)                 ny = ay;
        if (nx + (int)ww > ax + aw)  nx = ax + aw - (int)ww;
        if (ny + (int)wh > ay + ah)  ny = ay + ah - (int)wh;
        XMoveWindow(d, w, nx, ny);
    }

    XMapWindow(d, w);
    win_focus(list->prev);
}

void mapping_notify(XEvent *e) {
    XMappingEvent *ev = &e->xmapping;

    if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
        XRefreshKeyboardMapping(ev);
        input_grab(root);
    }
}

void run(const Arg arg) {
    if (fork()) return;
    if (d) close(ConnectionNumber(d));

    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

void input_grab(Window root) {
    unsigned int i, j, modifiers[] = {0, LockMask, numlock, numlock|LockMask};
    XModifierKeymap *modmap = XGetModifierMapping(d);
    KeyCode code;

    for (i = 0; i < 8; i++)
        for (int k = 0; k < modmap->max_keypermod; k++)
            if (modmap->modifiermap[i * modmap->max_keypermod + k]
                == XKeysymToKeycode(d, 0xff7f))
                numlock = (1 << i);

    XUngrabKey(d, AnyKey, AnyModifier, root);

    for (i = 0; i < sizeof(keys)/sizeof(*keys); i++)
        if ((code = XKeysymToKeycode(d, keys[i].keysym)))
            for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
                XGrabKey(d, code, keys[i].mod | modifiers[j], root,
                        True, GrabModeAsync, GrabModeAsync);

    for (i = 1; i < 4; i += 2)
        for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
            XGrabButton(d, i, MOD | modifiers[j], root, True,
                ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
                GrabModeAsync, GrabModeAsync, 0, 0);

    XFreeModifiermap(modmap);
}

int main(void) {
    XEvent ev;

    if (!(d = XOpenDisplay(0))) exit(1);

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    int s = DefaultScreen(d);
    root  = RootWindow(d, s);
    sw    = XDisplayWidth(d, s);
    sh    = XDisplayHeight(d, s);

    net_supporting_wm_check  = XInternAtom(d, "_NET_SUPPORTING_WM_CHECK",  False);
    net_wm_name              = XInternAtom(d, "_NET_WM_NAME",              False);
    ewmh_utf8_string         = XInternAtom(d, "UTF8_STRING",               False);
    net_supported            = XInternAtom(d, "_NET_SUPPORTED",            False);
    net_wm_window_type       = XInternAtom(d, "_NET_WM_WINDOW_TYPE",       False);
    net_wm_window_type_dock  = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK",  False);
    net_wm_strut             = XInternAtom(d, "_NET_WM_STRUT",             False);
    net_wm_strut_partial     = XInternAtom(d, "_NET_WM_STRUT_PARTIAL",     False);
    net_number_of_desktops   = XInternAtom(d, "_NET_NUMBER_OF_DESKTOPS",   False);
    net_current_desktop      = XInternAtom(d, "_NET_CURRENT_DESKTOP",      False);

    Window wmcheck = XCreateSimpleWindow(d, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(d, root,    net_supporting_wm_check, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wmcheck, 1);
    XChangeProperty(d, wmcheck, net_supporting_wm_check, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wmcheck, 1);
    XChangeProperty(d, wmcheck, net_wm_name, ewmh_utf8_string, 8,
        PropModeReplace, (unsigned char *)"sowm-extended", 13);

    Atom supported[] = {
        net_supporting_wm_check,
        net_wm_name,
        net_wm_window_type,
        net_wm_window_type_dock,
        net_wm_strut,
        net_wm_strut_partial,
        net_number_of_desktops,
        net_current_desktop,
    };
    XChangeProperty(d, root, net_supported, XA_ATOM, 32,
        PropModeReplace, (unsigned char *)supported,
        sizeof(supported) / sizeof(Atom));

    long num_ws = 9; 
    XChangeProperty(d, root, net_number_of_desktops, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&num_ws, 1);
    long cur_ws = 0;
    XChangeProperty(d, root, net_current_desktop, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&cur_ws, 1);

    XSelectInput(d,  root, SubstructureRedirectMask);
    XDefineCursor(d, root, XCreateFontCursor(d, 68));
    input_grab(root);

    while (1 && !XNextEvent(d, &ev)) 
        if (events[ev.type]) events[ev.type](&ev);
}
