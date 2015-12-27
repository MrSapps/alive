#pragma once

struct GuiScissor;

void drawButton(void *void_rend, float x, float y, float w, float h, bool down, bool hover, int layer, GuiScissor *s);
void drawCheckBox(void *void_rend, float x, float y, float w, bool checked, bool /*down*/, bool hover, int layer, GuiScissor *s);
void drawRadioButton(void *void_rend, float x, float y, float w, bool checked, bool /*down*/, bool hover, int layer, GuiScissor *s);
void drawTextBox(void *void_rend, float x, float y, float w, float h, bool active, bool hover, int layer, GuiScissor *s);
void drawText(void *void_rend, float x, float y, const char *text, int layer, GuiScissor *s);
void calcTextSize(float ret[2], void *void_rend, const char *text, int layer);
void drawWindow(void *void_rend, float x, float y, float w, float h, float titleBarHeight, const char *title, bool focus, int layer);
