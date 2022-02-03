#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "plugin.h"

static XPLMCommandRef toggle_yoke_control, toggle_rudder_control;
static XPLMDataRef yoke_heading_ratio, yoke_roll_ratio, yoke_pitch_ratio;
static XPLMDataRef eq_pfc_yoke;
static XPLMFlightLoopID loop_id;

static HWND xp_hwnd;
static float white[] = {1.0f, 1.0f, 1.0f};

static int screen_width, screen_height;
static int screen_center_x, screen_center_y;
static int box_left, box_right, box_bottom, box_top;

static int yoke_control_enabled, rudder_control_enabled;


PLUGIN_API int XPluginStart(char *name, char *sig, char *desc) {
    /* SDK docs state buffers are at least 256 bytes. */
    sprintf(name, "%s %s", PLUGIN_NAME, PLUGIN_VERSION);
    strcpy(sig, PLUGIN_SIG);
    strcpy(desc, PLUGIN_DESCRIPTION);
    toggle_yoke_control = XPLMCreateCommand("MouseYoke/ToggleYokeControl", "Toggle mouse yoke control");
    toggle_rudder_control = XPLMCreateCommand("MouseYoke/ToggleRudderControl", "Toggle mouse rudder control");
    yoke_pitch_ratio = XPLMFindDataRef("sim/cockpit2/controls/yoke_pitch_ratio");
    yoke_roll_ratio = XPLMFindDataRef("sim/cockpit2/controls/yoke_roll_ratio");
    yoke_heading_ratio = XPLMFindDataRef("sim/cockpit2/controls/yoke_heading_ratio");
    eq_pfc_yoke = XPLMFindDataRef("sim/joystick/eq_pfc_yoke");
    XPLMDataRef has_joystick = XPLMFindDataRef("sim/joystick/has_joystick");
    if(XPLMGetDatai(has_joystick)) {
        //joystick detected, unloading plugin
        return 0;
    }
    xp_hwnd = FindWindowA("X-System", "X-System");
    if(!xp_hwnd) {
        //could not find X-Plane 11 window
        return 0;
    }
    return 1;
}

PLUGIN_API void XPluginStop(void) {

}

PLUGIN_API int XPluginEnable(void) {
    XPLMRegisterCommandHandler(toggle_yoke_control, toggle_yoke_control_cb, 0, NULL);
    XPLMRegisterCommandHandler(toggle_rudder_control, toggle_rudder_control_cb, 0, NULL);
    XPLMRegisterDrawCallback(draw_cb, xplm_Phase_Window, 0, NULL);
    XPLMCreateFlightLoop_t params = {
        .structSize = sizeof(XPLMCreateFlightLoop_t),
        .phase = xplm_FlightLoop_Phase_BeforeFlightModel,
        .refcon = NULL,
        .callbackFunc = loop_cb
    };
    loop_id = XPLMCreateFlightLoop(&params);
    return 1;
}

PLUGIN_API void XPluginDisable(void) {
    XPLMUnregisterCommandHandler(toggle_yoke_control, toggle_yoke_control_cb, 0, NULL);
    XPLMUnregisterCommandHandler(toggle_rudder_control, toggle_rudder_control_cb, 0, NULL);
    XPLMSetDatai(eq_pfc_yoke, 0);
    XPLMUnregisterDrawCallback(draw_cb, xplm_Phase_Window, 0, NULL);
    if(loop_id)
        XPLMDestroyFlightLoop(loop_id);
    loop_id = NULL;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, int msg, void *param) {
    if(from != XPLM_PLUGIN_XPLANE)
        return;
    if(msg == XPLM_MSG_PLANE_LOADED) {
        int index = (int)param;
        /* user's plane */
        if(index == XPLM_USER_AIRCRAFT) {
            /* This will hide the clickable yoke control box. */
            XPLMSetDatai(eq_pfc_yoke, 1);
        }
    }
}

int toggle_yoke_control_cb(XPLMCommandRef cmd, XPLMCommandPhase phase, void *ref) {
    if(phase != xplm_CommandBegin)
        return 1;
    rudder_control_enabled = 0;
    if(yoke_control_enabled) {
        yoke_control_enabled = 0;
    } else {
        yoke_control_enabled = 1;
        calc_screen_params();
        XPLMSetDataf(yoke_heading_ratio, 0);
        set_cursor_from_yoke();
        XPLMScheduleFlightLoop(loop_id, -1.0f, 0);
    }
    return 1;
}

int toggle_rudder_control_cb(XPLMCommandRef cmd, XPLMCommandPhase phase, void *ref) {
    if(phase != xplm_CommandBegin)
        return 1;
    yoke_control_enabled = 0;
    if(rudder_control_enabled) {
        rudder_control_enabled = 0;
    } else {
        rudder_control_enabled = 1;
        calc_screen_params();
        //XPLMSetDataf(yoke_heading_ratio, 0);
        //XPLMSetDataf(yoke_roll_ratio, 0);
        //XPLMSetDataf(yoke_pitch_ratio, 0);
        set_cursor_from_rudder();
        XPLMScheduleFlightLoop(loop_id, -1.0f, 0);
    }
    return 1;
}

int draw_cb(XPLMDrawingPhase phase, int before, void *ref) {
    if(yoke_control_enabled || rudder_control_enabled) {
        char str[256];
        int m_x, m_y;
        get_cursor_pos(&m_x, &m_y);
        draw_yoke_box();
        if(yoke_control_enabled)
            sprintf(str, "MOUSE CONTROL: YOKE");
        else if(rudder_control_enabled)
            sprintf(str, "MOUSE CONTROL: RUDDER");
        else
            sprintf(str, "MOUSE CONTROL: OFF");
        XPLMDrawString(white, 20, 150, str, NULL, xplmFont_Basic);
        sprintf(str, "Resolution: %d x %d", screen_width, screen_height);
        XPLMDrawString(white, 20, 120, str, NULL, xplmFont_Basic);
        sprintf(str, "Mouse X Y: %d %d", m_x, m_y);
        XPLMDrawString(white, 20, 90, str, NULL, xplmFont_Basic);
        sprintf(str, "Yoke heading ratio: %f", XPLMGetDataf(yoke_heading_ratio));
        XPLMDrawString(white, 20, 60, str, NULL, xplmFont_Basic);
        sprintf(str, "Yoke roll ratio: %f", XPLMGetDataf(yoke_roll_ratio));
        XPLMDrawString(white, 20, 40, str, NULL, xplmFont_Basic);
        sprintf(str, "Yoke pitch ratio: %f", XPLMGetDataf(yoke_pitch_ratio));
        XPLMDrawString(white, 20, 20, str, NULL, xplmFont_Basic);
    }
    return 1;
}

float loop_cb(float last_call, float last_loop, int count, void *ref) {
    int m_x, m_y;
    get_cursor_pos(&m_x, &m_y);

    if(yoke_control_enabled) {
        float roll_ratio = 1 - (box_right - m_x)/((float)BOX_HALF_SIZE);
        float pitch_ratio = (box_top - m_y)/((float)BOX_HALF_SIZE) - 1;
        if(roll_ratio < -1)
            roll_ratio = -1;
        else if(roll_ratio > 1)
            roll_ratio = 1;
        XPLMSetDataf(yoke_roll_ratio, roll_ratio);
        if(pitch_ratio < -1)
            pitch_ratio = -1;
        else if(pitch_ratio > 1)
            pitch_ratio = 1;
        XPLMSetDataf(yoke_pitch_ratio, pitch_ratio);
    } else if(rudder_control_enabled) {
        float heading_ratio = 1 - (box_right - m_x)/((float)BOX_HALF_SIZE);
        if(heading_ratio < -1)
            heading_ratio = -1;
        else if(heading_ratio > 1)
            heading_ratio = 1;
        XPLMSetDataf(yoke_heading_ratio, heading_ratio);
    } else {
        /* If user has disabled mouse control, suspend loop. */
        /* Don't call us anymore. */
        return 0;
    }
    /* Call us again next frame. */
    return -1.0f;
}

void get_cursor_pos(int *x, int *y) {
    XPLMGetMouseLocationGlobal(x, y);
}

void set_cursor_from_yoke() {
    set_cursor_pos(
        screen_center_x + lroundf(XPLMGetDataf(yoke_roll_ratio) * BOX_HALF_SIZE),
        screen_center_y - lroundf(XPLMGetDataf(yoke_pitch_ratio) * BOX_HALF_SIZE)
    );
}

void set_cursor_from_rudder() {
    set_cursor_pos(
        screen_center_x + lroundf(XPLMGetDataf(yoke_heading_ratio) * BOX_HALF_SIZE),
        screen_center_y
    );
}

void set_cursor_pos(int x, int y) {
    POINT pt = {
        .x = x,
        /* On windows (0,0) is the upper-left corner. */
        .y = screen_height - y
    };
    ClientToScreen(xp_hwnd, &pt);
    SetCursorPos(pt.x, pt.y);
}

void draw_yoke_box() {
    glBegin(GL_LINE_LOOP);
    glColor3fv(white);
    glVertex2i(box_left, box_bottom);
    glVertex2i(box_left, box_top);
    glVertex2i(box_right, box_top);
    glVertex2i(box_right, box_bottom);
    glEnd();

    glBegin(GL_LINES);
    glColor3fv(white);
    glVertex2i(box_left, screen_center_y);
    glVertex2i(box_right, screen_center_y);
    glVertex2i(screen_center_x, box_bottom);
    glVertex2i(screen_center_x, box_top);
    glEnd();
}

void calc_screen_params() {
    XPLMGetScreenSize(&screen_width, &screen_height);
    screen_center_x = screen_width/2;
    screen_center_y = screen_height/2;
    box_left = screen_center_x - BOX_HALF_SIZE;
    box_right = screen_center_x + BOX_HALF_SIZE;
    box_bottom = screen_center_y - BOX_HALF_SIZE;
    box_top = screen_center_y + BOX_HALF_SIZE;
}
