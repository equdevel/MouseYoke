#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#define XPLM200
#define XPLM210
#define XPLM300
#define IBM 1

#include <XPLM/XPLMUtilities.h>
#include <XPLM/XPLMDataAccess.h>
#include <XPLM/XPLMPlugin.h>
#include <XPLM/XPLMPlanes.h>
#include <XPLM/XPLMDisplay.h>
#include <XPLM/XPLMGraphics.h>
#include <XPLM/XPLMProcessing.h>
#include <GL/gl.h>

#define PLUGIN_NAME         "MouseYoke"
#define PLUGIN_SIG          "MouseYoke.Segment"
#define PLUGIN_DESCRIPTION  "The best X-Plane mouse yoke! )))"
#define PLUGIN_VERSION      "1.1"

#define BOX_SIZE 300
#define BOX_HALF_SIZE BOX_SIZE/2

int toggle_yoke_control_cb(XPLMCommandRef cmd, XPLMCommandPhase phase, void *ref);
int toggle_rudder_control_cb(XPLMCommandRef cmd, XPLMCommandPhase phase, void *ref);
int draw_cb(XPLMDrawingPhase phase, int before, void *ref);
float loop_cb(float last_call, float last_loop, int count, void *ref);

void get_cursor_pos(int *x, int *y);
void set_cursor_pos(int x, int y);
void set_cursor_from_yoke();
void set_cursor_from_rudder();

void draw_yoke_box();
void calc_screen_params();

#endif /*_PLUGIN_H_*/
