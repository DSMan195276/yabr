
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "list.h"
#include "render.h"
#include "config.h"

#include "alsa.h"
#include "battery.h"
#include "datetime.h"
#include "mail.h"
#include "mpd.h"
#include "tasks.h"
#include "wireless.h"

#define COLOR_BACK    (0xFF1D1F21)   /* Default background */
#define COLOR_FORE    (0xFFC5C8C6)   /* Default foreground */
//#define COLOR_HEAD    (0xFFB5BD68)   /* Background for first element */
#define COLOR_SEC_B1  (0xFF282A2E)   /* Background for section 1 */
#define COLOR_SEC_B2  (0xFF353A3F)   /* Background for section 2 */
#define COLOR_SEC_B3  (0xFF40474E)   /* Background for section 3 */
#define COLOR_URGENT  (0xFFCE935F)   /* Background color for mail alert */
#define COLOR_DISABLE (0xFF1D1F21)   /* Foreground for disable elements */
//#define COLOR_WSP     (0xFF8C9440)   /* Background for selected workspace */

#define COLOR_HEAD 0xFFBD6871
#define COLOR_WSP  0xFF944040

#define BAR_COLOR_WSP_FORE COLOR_WSP
#define BAR_COLOR_WSP_BACK COLOR_HEAD

#define BAR_COLOR_WSP_FOCUSED_FORE COLOR_BACK
#define BAR_COLOR_WSP_FOCUSED_BACK BAR_COLOR_WSP_FORE

#define BAR_COLOR_WSP_UNFOCUSED_FORE COLOR_DISABLE
#define BAR_COLOR_WSP_UNFOCUSED_BACK BAR_COLOR_WSP_BACK

#define BAR_COLOR_WSP_URGENT_FORE COLOR_BACK
#define BAR_COLOR_WSP_URGENT_BACK COLOR_URGENT

#define BAR_COLOR_TITLE_FORE COLOR_FORE
#define BAR_COLOR_TITLE_BACK COLOR_SEC_B1

#define BAR_COLOR_STATUS_LAST_FORE COLOR_BACK
#define BAR_COLOR_STATUS_LAST_BACK COLOR_HEAD

#define BAR_COLOR_MODE_FORE COLOR_FORE
#define BAR_COLOR_MODE_BACK COLOR_SEC_B2

#define BAR_COLOR_STATUS_URGENT_FORE COLOR_BACK
#define BAR_COLOR_STATUS_URGENT_BACK COLOR_URGENT

#define BAR_COLOR_CENTERED_FORE COLOR_BACK
#define BAR_COLOR_CENTERED_BACK COLOR_HEAD

#define BAR_DEFAULT_COLOR_BACK (0xFF1D1F21)
#define BAR_DEFAULT_COLOR_FORE (0xFFC5C8C6)

#define BAR_USE_SEP 1

#define BAR_SEP_RIGHTSIDE ""
#define BAR_SEP_LEFTSIDE ""

#define BAR_SEP_RIGHTSIDE_SAME ""
#define BAR_SEP_LEFTSIDE_SAME ""

#define LEMONBAR_FONT "-*-terminesspowerline-medium-*-*-*-12-*-*-*-*-*-*-*"

struct yabr_config yabr_config = {
    .debug = NULL,

    .colors = {
        .wsp_focused = { BAR_COLOR_WSP_FOCUSED_FORE, BAR_COLOR_WSP_FOCUSED_BACK },
        .wsp_unfocused = { BAR_COLOR_WSP_UNFOCUSED_FORE, BAR_COLOR_WSP_UNFOCUSED_BACK },
        .wsp_urgent = { BAR_COLOR_WSP_URGENT_FORE, BAR_COLOR_WSP_URGENT_BACK },
        .title = { BAR_COLOR_TITLE_FORE, BAR_COLOR_TITLE_BACK },
        .status_last = { BAR_COLOR_STATUS_LAST_FORE, BAR_COLOR_STATUS_LAST_BACK },
        .mode = { BAR_COLOR_MODE_FORE, BAR_COLOR_MODE_BACK },
        .status_urgent = { BAR_COLOR_STATUS_URGENT_FORE, BAR_COLOR_STATUS_URGENT_BACK },
        .centered = { BAR_COLOR_CENTERED_FORE, BAR_COLOR_CENTERED_BACK },
        .background = { BAR_DEFAULT_COLOR_FORE, BAR_DEFAULT_COLOR_BACK },
        .section_count = 3,
        .section_cols = (struct bar_color[]) {
            { .fore = COLOR_FORE, .back = COLOR_SEC_B1 },
            { .fore = COLOR_FORE, .back = COLOR_SEC_B2 },
            { .fore = COLOR_FORE, .back = COLOR_SEC_B3 },
        },
    },

    .use_separator = BAR_USE_SEP,

    .sep_rightside = BAR_SEP_RIGHTSIDE,
    .sep_rightside_same = BAR_SEP_RIGHTSIDE_SAME,

    .sep_leftside = BAR_SEP_LEFTSIDE,
    .sep_leftside_same = BAR_SEP_LEFTSIDE_SAME,

    .lemonbar_font = LEMONBAR_FONT,
    .lemonbar_font_size = 12,
    .lemonbar_on_bottom = 0,

    .state = BAR_STATE_INIT(yabr_config.state),
    .descs = {
        [DESC_ALSA] = &alsa_status_description,
        [DESC_BATTERY] = &battery_status_description,
        [DESC_DATETIME] = &datetime_status_description,
        [DESC_MAIL] = &mail_status_description,
        [DESC_MPD] = &mpdmon_status_description,
        [DESC_TASKS] = &tasks_status_description,
        [DESC_WIRELESS] = &wireless_status_description,
    },
};

