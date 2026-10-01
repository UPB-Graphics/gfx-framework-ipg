#pragma once

#include "imgui.h"


/*
 *  Hand-drawn interface components, for scenes that paint their UI straight onto the
 *  ImGui draw lists (see GUIScene) instead of using ImGui windows. Every control is
 *  immediate: it is drawn and hit-tested in the same call, every frame.
 *
 *  Controls that are dragged (sliders, scrubs) take an `id`, unique within the scene, and
 *  a `grab` the scene keeps between frames: the id of the control being dragged, or
 *  GRAB_NONE. It is what lets a drag continue after the cursor leaves the control.
 */
namespace ui
{
    // ---- palette ---------------------------------------------------------------------------
    constexpr ImU32 PANEL_BG         = IM_COL32(20, 20, 24, 240);
    constexpr ImU32 PANEL_BORDER     = IM_COL32(255, 255, 255, 38);
    constexpr ImU32 BTN_BG           = IM_COL32(48, 48, 56, 255);
    constexpr ImU32 BTN_HOVER        = IM_COL32(72, 72, 84, 255);
    constexpr ImU32 BTN_ON           = IM_COL32(255, 214, 0, 255);
    constexpr ImU32 BTN_DANGER       = IM_COL32(150, 48, 48, 255);
    constexpr ImU32 BTN_DANGER_HOVER = IM_COL32(196, 64, 64, 255);
    constexpr ImU32 TEXT_MAIN        = IM_COL32(236, 236, 240, 255);
    constexpr ImU32 TEXT_DIM         = IM_COL32(155, 155, 165, 255);
    constexpr ImU32 TEXT_ON          = IM_COL32(24, 24, 24, 255);
    constexpr ImU32 TEXT_WARN        = IM_COL32(255, 150, 80, 255);
    constexpr ImU32 TRACK_BG         = IM_COL32(58, 58, 68, 255);
    constexpr ImU32 TRACK_FILL       = IM_COL32(120, 170, 255, 255);

    constexpr int GRAB_NONE = 0;

    // Small glyphs drawn with lines and triangles, in place of an icon font
    enum class Icon { None, ArrowLeft, ArrowRight, Grid, Crosshair };

    bool PointIn(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b);

    void TextAt(ImDrawList *dl, float x, float y, ImU32 col, const char *text);
    void TextCentered(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, ImU32 col, const char *text);
    void DrawIcon(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, ImU32 col, Icon icon);

    // A panel background with its border
    void Panel(ImDrawList *dl, const ImVec2 &min, const ImVec2 &max);

    // A button showing `label`, or `icon` if there is one. `on` draws it highlighted, for
    // toggles and selections. True the frame the mouse is released over it.
    bool Button(ImDrawList *dl, const ImVec2 &pos, const ImVec2 &size, const char *label,
                bool on = false, Icon icon = Icon::None,
                ImU32 baseCol = BTN_BG, ImU32 hoverCol = BTN_HOVER);

    // A horizontal slider over a value in [0, 1]. True when the value changed.
    bool Slider(ImDrawList *dl, const ImVec2 &pos, float width, float &value01,
                int id, int &grab, ImU32 fill = TRACK_FILL);

    // A number box changed by dragging horizontally, `speed` per pixel, like ImGui::DragFloat.
    // `format` is a printf format for the value. True when the value changed.
    bool Scrub(ImDrawList *dl, const ImVec2 &pos, const ImVec2 &size, float &value,
               float speed, int id, int &grab, const char *format);

    // The tab on the left edge of the window, halfway down, that shows and hides a scene's
    // panels. Flips `open` when clicked, unless `busy` (a drag is going on). True while the
    // mouse is over it.
    bool SideTab(ImDrawList *dl, const ImVec2 &origin, const ImVec2 &size, bool &open, bool busy);
}   // namespace ui
