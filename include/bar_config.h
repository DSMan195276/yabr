#ifndef INCLUDE_BAR_CONFIG_H
#define INCLUDE_BAR_CONFIG_H

#define COLOR_BACK    (0xFF1D1F21)   /* Default background */
#define COLOR_FORE    (0xFFC5C8C6)   /* Default foreground */
//#define COLOR_HEAD    (0xFFB5BD68)   /* Background for first element */
#define COLOR_SEC_B1  (0xFF282A2E)   /* Background for section 1 */
#define COLOR_SEC_B2  (0xFF454A4F)   /* Background for section 2 */
#define COLOR_SEC_B3  (0xFF60676E)   /* Background for section 3 */
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

#define LEMONBAR_FONT "-xos4-terminesspowerline-medium-r-normal--12-120-72-72-c-60-iso10646-1"
//#define LEMONBAR_GEOMETRY "x15+1600+0"

#define BAR_COLOR_DATE_FORE COLOR_FORE
#define BAR_COLOR_DATE_BACK COLOR_SEC_B1
#define BAR_COLOR_TIME_FORE COLOR_BACK
#define BAR_COLOR_TIME_BACK COLOR_HEAD

#define DATE_FORMAT "%m-%d-%Y"
#define TIME_FORMAT "%H:%M"
#define TIME_TIMEOUT 30 /* Seconds */
#define DATE_TIMEOUT (5 * 60) /* Seconds */
#define DATETIME_IS_SPLIT F(DATETIME_SPLIT)

#define BATTERY_USE "BAT0"
#define BATTERY_TIMEOUT (30) /* Seconds */

#define ALSA_MIX "Master"
#define ALSA_CARD "default"

#define MPD_SERVER "127.0.0.1"
#define MPD_PORT 6600
#define MPD_TIMEOUT 5 /* Seconds */

#define WIRELESS_IFACE "wlp3s0"

#define TASKS_TIMEOUT (5 * 60) /* Seconds */

#define MAIL_NAME "Gmail"
#define MAIL_DIR "/mnt/data/mail/Inbox"
#define MAIL_TIMEOUT (5 * 60) /* Seconds */

#define BAR_OUTPUT "VGA1"

#define BAR_MAX_OUTPUTS 6

#endif
