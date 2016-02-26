#pragma once

struct GuiContext;
class Renderer;

void calcTextSize(int ret[2], void *void_rend, const char *text);
void drawWidgets(GuiContext &ctx, Renderer &rend);