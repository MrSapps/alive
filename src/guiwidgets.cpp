#include "guiwidgets.hpp"
#include "renderer.hpp"
#include "gui.h"

void drawButton(void *void_rend, float x, float y, float w, float h, bool down, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos[0], 1.f*s->pos[1], 1.f*s->size[0], 1.f*s->size[1]);
    else
        rend->resetScissor();

    float cornerRadius = 4.0f;
    Color gradBegin = { 1.f, 1.f, 1.f, 64 / 255.f };
    Color gradEnd = { 0.f, 0.f, 0.f, 64 / 255.f };

    Color bgColor = { 0.3f, 0.3f, 0.3f, 0.5f };

    if (down)
    {
        Color begin = { 0.f, 0.f, 0.f, 64 / 255.f };
        Color end = { 0.3f, 0.3f, 0.3f, 64 / 255.f };
        gradBegin = begin;
        gradEnd = end;
    }

    RenderPaint overlay = rend->linearGradient(x, y, x, y + h,
        gradBegin,
        gradEnd);

    rend->beginPath();
    rend->roundedRect(x + 1, y + 1, w - 2, h - 2, cornerRadius - 1);

    rend->fillColor(bgColor);
    rend->fill();

    rend->fillPaint(overlay);
    rend->fill();

    Color outlineColor = Color{ 0, 0, 0, 0.3f };
    if (hover && !down)
    {
        outlineColor.r = 1.f;
        outlineColor.g = 1.f;
        outlineColor.b = 1.f;
    }
    rend->beginPath();
    rend->roundedRect(x + 0.5f, y + 0.5f, w - 1, h - 1, cornerRadius - 0.5f);
    rend->strokeColor(outlineColor);
    rend->stroke();

    rend->endLayer();
}

void drawCheckBox(void *void_rend, float x, float y, float w, bool checked, bool /*down*/, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos[0], 1.f*s->pos[1], 1.f*s->size[0], 1.f*s->size[1]);
    else
        rend->resetScissor();

    RenderPaint bg;

    rend->beginPath();

    if (checked)
        bg = rend->boxGradient(x + 2, y + 2, x + w - 2, x + w - 2, 3, 3, Color{ 0.5f, 1.f, 0.5f, 92 / 255.f }, Color{ 0.5f, 1.f, 0.5f, 30 / 255.f });
    else
        bg = rend->boxGradient(x + 2, y + 2, x + w - 2, x + w - 2, 3, 3, Color{ 0.f, 0.f, 0.f, 30 / 255.f }, Color{ 0.f, 0.f, 0.f, 92 / 255.f });
    rend->roundedRect(x + 1, y + 1, w - 2, w - 2, 3);
    rend->fillPaint(bg);
    rend->fill();

    rend->strokeWidth(1.0f);
    if (hover)
        rend->strokeColor(Color{ 1.f, 1.f, 1.f, 0.3f });
    else
        rend->strokeColor(Color{ 0.f, 0.f, 0.f, 0.3f });
    rend->stroke();

    rend->endLayer();
}

void drawRadioButton(void *void_rend, float x, float y, float w, bool checked, bool /*down*/, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos[0], 1.f*s->pos[1], 1.f*s->size[0], 1.f*s->size[1]);
    else
        rend->resetScissor();

    RenderPaint bg;

    rend->beginPath();

    if (checked)
        bg = rend->radialGradient(x + w / 2 + 1, y + w / 2 + 1, w / 2 - 2, w / 2, Color{ 0.5f, 1.f, 0.5f, 92 / 255.f }, Color{ 0.5f, 1.f, 0.5f, 30 / 255.f });
    else
        bg = rend->radialGradient(x + w / 2 + 1, y + w / 2 + 1, w / 2 - 2, w / 2, Color{ 0.f, 0.f, 0.f, 30 / 255.f }, Color{ 0.f, 0.f, 0.f, 92 / 255.f });
    rend->circle(x + w / 2, y + w / 2, w / 2);
    rend->fillPaint(bg);
    rend->fill();

    rend->strokeWidth(1.0f);
    if (hover)
        rend->strokeColor(Color{ 1.f, 1.f, 1.f, 0.3f });
    else
        rend->strokeColor(Color{ 0.f, 0.f, 0.f, 0.3f });
    rend->stroke();

    rend->endLayer();
}

void drawTextBox(void *void_rend, float x, float y, float w, float h, bool active, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos[0], 1.f*s->pos[1], 1.f*s->size[0], 1.f*s->size[1]);
    else
        rend->resetScissor();

    RenderPaint bg = rend->boxGradient(x + 1, y + 1 + 1.5f, w - 2, h - 2, 3, 4, Color{ 1.f, 1.f, 1.f, 32 / 255.f }, Color{ 32 / 255.f, 32 / 255.f, 32 / 255.f, 32 / 255.f });
    rend->beginPath();
    rend->roundedRect(x + 1, y + 1, w - 2, h - 2, 4 - 1);
    rend->fillPaint(bg);
    rend->fill();

    rend->beginPath();
    rend->roundedRect(x + 0.5f, y + 0.5f, w - 1, h - 1, 4 - 0.5f);
    if (hover || active)
        rend->strokeColor(Color{ 1.f, 1.f, 1.f, 0.3f });
    else
        rend->strokeColor(Color{ 0.f, 0.f, 0.f, 48 / 255.f });
    rend->stroke();

    rend->endLayer();
}

static const float g_gui_font_size = 16.f;

void drawText(void *void_rend, float x, float y, const char *text, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos[0], 1.f*s->pos[1], 1.f*s->size[0], 1.f*s->size[1]);
    else
        rend->resetScissor();

    rend->fontSize(g_gui_font_size);
    rend->textAlign(TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP);
    rend->fontBlur(0);
    rend->fillColor(Color{ 1.f, 1.f, 1.f, 160 / 255.f });
    rend->text(x, y, text);

    rend->endLayer();
}

void calcTextSize(float ret[2], void *void_rend, const char *text, int layer)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);

    rend->fontSize(g_gui_font_size);
    rend->textAlign(TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP);
    rend->fontBlur(0);
    float bounds[4];
    rend->textBounds(0, 0, text, bounds);
    ret[0] = bounds[2] - bounds[0];
    ret[1] = bounds[3] - bounds[1];

    rend->endLayer();
}

void drawWindow(void *void_rend, float x, float y, float w, float h, float titleBarHeight, const char *title, bool focus, int layer)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer); // Makes window reordering possible
    rend->resetScissor();

    float cornerRadius = 3.0f;
    RenderPaint shadowPaint;
    RenderPaint headerPaint;

    // Window
    rend->beginPath();
    rend->roundedRect(x, y, w, h, cornerRadius);
    rend->fillColor(Color{ 28 / 255.f, 30 / 255.f, 34 / 255.f, 220 / 255.f });
    //	nvgFillColor(vg, nvgRGBA(0,0,0,128));
    rend->fill();

    // Drop shadow
    shadowPaint = rend->boxGradient(x, y + 2, w, h, cornerRadius * 2, 10, Color{ 0.f, 0.f, 0.f, 128 / 255.f }, Color{ 0.f, 0.f, 0.f, 0.f });
    rend->beginPath();
    rend->rect(x - 10, y - 10, w + 20, h + 30);
    rend->roundedRect(x, y, w, h, cornerRadius);
    rend->solidPathWinding(false);
    rend->fillPaint(shadowPaint);
    rend->fill();
    rend->solidPathWinding(true);

    // Header
    if (focus)
        headerPaint = rend->linearGradient(x, y, x, y + 15, Color{ 1.0f, 1.f, 1.0f, 16 / 255.f }, Color{ 0.f, 0.0f, 0.f, 32 / 255.f });
    else
        headerPaint = rend->linearGradient(x, y, x, y + 15, Color{ 1.f, 1.f, 1.f, 16 / 255.f }, Color{ 1.f, 1.f, 1.f, 16 / 255.f });
    rend->beginPath();
    rend->roundedRect(x + 1, y, w - 2, titleBarHeight, cornerRadius - 1);
    rend->fillPaint(headerPaint);
    rend->fill();
    rend->beginPath();
    rend->moveTo(x + 0.5f, y - 0.5f + titleBarHeight);
    rend->lineTo(x + 0.5f + w - 1, y - 0.5f + titleBarHeight);
    rend->strokeColor(Color{ 0, 0, 0, 64 / 255.f });
    rend->stroke();

    rend->fontSize(18.0f);
    rend->textAlign(TEXT_ALIGN_CENTER | TEXT_ALIGN_MIDDLE);
    rend->fontBlur(3);
    rend->fillColor(Color{ 0.f, 0.f, 0.f, 160 / 255.f });
    rend->text(x + w / 2, y + 16 + 1, title);

    rend->fontBlur(0);
    rend->fillColor(Color{ 230 / 255.f, 230 / 255.f, 230 / 255.f, 200 / 255.f });
    rend->text(x + w / 2, y + 16, title);

    rend->endLayer();
}
