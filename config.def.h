#ifndef CONFIG_H
#define CONFIG_H
#define MOD Mod4Mask

const char* term[]    = {"st", 0};
const char* yazi[]    = {"kitty", "-e", "yazi", 0};
const char* browser[] = {"firefox-bin", 0};
const char* menu[]    = {"dmenu_run", 0};
const char* shot[]    = {"sh", "-c", "maim -s ~/Screenshots/$(date +%Y-%m-%d_%H-%M-%S).png", 0};

const char* volup[]   = {"sh", "-c",
    "wpctl set-volume @DEFAULT_AUDIO_SINK@ 1%+ && "
    "VOL=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{printf \"%d\", $2 * 100}') && "
    "notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VOL -t 1500 \"Volume\" \"$VOL%\" && "
    "kill -USR1 $(pidof kantbar)", 0};

const char* voldown[] = {"sh", "-c",
    "wpctl set-volume @DEFAULT_AUDIO_SINK@ 1%- && "
    "VOL=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{printf \"%d\", $2 * 100}') && "
    "notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VOL -t 1500 \"Volume\" \"$VOL%\" && "
    "kill -USR1 $(pidof kantbar)", 0};

const char* volmute[] = {"sh", "-c",
    "wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle && "
    "notify-send -h string:x-dunst-stack-tag:osd -t 1500 \"Volume\" \"Muted\" && "
    "kill -USR1 $(pidof kantbar)", 0};

const char* briup[]   = {"sh", "-c",
    "brightnessctl set 1%+ && "
    "BRI=$(brightnessctl get) && MAX=$(brightnessctl max) && "
    "VAL=$((BRI * 100 / MAX)) && "
    "notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VAL -t 1500 \"Brightness\" \"$VAL%\" && "
    "kill -USR1 $(pidof kantbar)", 0};

const char* bridown[] = {"sh", "-c",
    "brightnessctl set 1%- && "
    "BRI=$(brightnessctl get) && MAX=$(brightnessctl max) && "
    "VAL=$((BRI * 100 / MAX)) && "
    "notify-send -h string:x-dunst-stack-tag:osd -h int:value:$VAL -t 1500 \"Brightness\" \"$VAL%\" && "
    "kill -USR1 $(pidof kantbar)", 0};

static struct key keys[] = {
    // Programs
    {MOD,            XK_q,      run, {.com = term}},
    {MOD,            XK_e,      run, {.com = yazi}},
    {MOD,            XK_Return, run, {.com = browser}},
    {MOD,            XK_space,  run, {.com = menu}},
    {0,              XK_Print,  run, {.com = shot}},

    // Window management
    {MOD,            XK_w,      win_kill,   {0}},
    {MOD,            XK_f,      win_fs,     {0}},
    {MOD,            XK_c,      win_center, {0}},
    {Mod1Mask,            XK_Tab, win_next, {0}},
    {Mod1Mask|ShiftMask,  XK_Tab, win_prev, {0}},

    // Media / brightness
    {0, XF86XK_AudioRaiseVolume,  run, {.com = volup}},
    {0, XF86XK_AudioLowerVolume,  run, {.com = voldown}},
    {0, XF86XK_AudioMute,         run, {.com = volmute}},
    {0, XF86XK_MonBrightnessUp,   run, {.com = briup}},
    {0, XF86XK_MonBrightnessDown, run, {.com = bridown}},

    // Tag switching
    {MOD,            XK_1, ws_go,     {.i = 1}},
    {MOD,            XK_2, ws_go,     {.i = 2}},
    {MOD,            XK_3, ws_go,     {.i = 3}},
    {MOD,            XK_4, ws_go,     {.i = 4}},
    {MOD,            XK_5, ws_go,     {.i = 5}},
    {MOD,            XK_6, ws_go,     {.i = 6}},
    {MOD,            XK_7, ws_go,     {.i = 7}},
    {MOD,            XK_8, ws_go,     {.i = 8}},
    {MOD,            XK_9, ws_go,     {.i = 9}},

    // Move window to tag
    {MOD|ShiftMask,  XK_1, win_to_ws, {.i = 1}},
    {MOD|ShiftMask,  XK_2, win_to_ws, {.i = 2}},
    {MOD|ShiftMask,  XK_3, win_to_ws, {.i = 3}},
    {MOD|ShiftMask,  XK_4, win_to_ws, {.i = 4}},
    {MOD|ShiftMask,  XK_5, win_to_ws, {.i = 5}},
    {MOD|ShiftMask,  XK_6, win_to_ws, {.i = 6}},
    {MOD|ShiftMask,  XK_7, win_to_ws, {.i = 7}},
    {MOD|ShiftMask,  XK_8, win_to_ws, {.i = 8}},
    {MOD|ShiftMask,  XK_9, win_to_ws, {.i = 9}},
};
#endif
