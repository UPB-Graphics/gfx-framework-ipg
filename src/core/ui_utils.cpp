#include "core/ui_utils.h"

#include <algorithm>
#include <cstdio>


bool ui::PointIn(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b)
{
    return p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y;
}


void ui::TextAt(ImDrawList *dl, float x, float y, ImU32 col, const char *text)
{
    dl->AddText({ x, y }, col, text);
}


void ui::TextCentered(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, ImU32 col, const char *text)
{
    const ImVec2 ts = ImGui::CalcTextSize(text);
    dl->AddText({ a.x + (b.x - a.x - ts.x) * 0.5f, a.y + (b.y - a.y - ts.y) * 0.5f }, col, text);
}


void ui::DrawIcon(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, ImU32 col, Icon icon)
{
    const ImVec2 c { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
    constexpr float S = 6.f;

    switch (icon)
    {
    case Icon::ArrowLeft:
    case Icon::ArrowRight:
    {
        const float dir = icon == Icon::ArrowLeft ? -1.f : 1.f;
        dl->AddTriangleFilled({ c.x + dir * 4.f, c.y }, { c.x - dir * 4.f, c.y - 5.5f }, { c.x - dir * 4.f, c.y + 5.5f }, col);
        break;
    }
    case Icon::Grid:
        dl->AddRect({ c.x - S, c.y - S }, { c.x + S, c.y + S }, col, 1.f, 0, 1.5f);
        dl->AddLine({ c.x, c.y - S }, { c.x, c.y + S }, col, 1.5f);
        dl->AddLine({ c.x - S, c.y }, { c.x + S, c.y }, col, 1.5f);
        break;
    case Icon::Crosshair:
        dl->AddCircle(c, S - 1.5f, col, 0, 1.5f);
        dl->AddLine({ c.x, c.y - S - 1.f }, { c.x, c.y + S + 1.f }, col, 1.5f);
        dl->AddLine({ c.x - S - 1.f, c.y }, { c.x + S + 1.f, c.y }, col, 1.5f);
        break;
    case Icon::None:
        break;
    }
}


void ui::Panel(ImDrawList *dl, const ImVec2 &min, const ImVec2 &max)
{
    dl->AddRectFilled(min, max, PANEL_BG, 6.f);
    dl->AddRect(min, max, PANEL_BORDER, 6.f);
}


bool ui::Button(ImDrawList *dl, const ImVec2 &pos, const ImVec2 &size, const char *label,
                bool on, Icon icon, ImU32 baseCol, ImU32 hoverCol)
{
    const ImVec2 a = pos;
    const ImVec2 b { pos.x + size.x, pos.y + size.y };
    const bool hovered = PointIn(ImGui::GetIO().MousePos, a, b);
    const ImU32 textCol = on ? TEXT_ON : TEXT_MAIN;

    dl->AddRectFilled(a, b, on ? BTN_ON : (hovered ? hoverCol : baseCol), 4.f);
    if (icon == Icon::None)
        TextCentered(dl, a, b, textCol, label);
    else
        DrawIcon(dl, a, b, textCol, icon);

    if (hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    return hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
}


bool ui::Slider(ImDrawList *dl, const ImVec2 &pos, float width, float &value01,
                int id, int &grab, ImU32 fill)
{
    constexpr float H = 12.f;
    const ImGuiIO &io = ImGui::GetIO();
    const ImVec2 a = pos;
    const ImVec2 b { pos.x + width, pos.y + H };

    if (grab == GRAB_NONE
        && PointIn(io.MousePos, { a.x - 3.f, a.y - 5.f }, { b.x + 3.f, b.y + 5.f })
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        grab = id;

    bool changed = false;
    if (grab == id)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            grab = GRAB_NONE;
        else
        {
            const float nt = std::clamp((io.MousePos.x - a.x) / width, 0.f, 1.f);
            if (nt != value01) { value01 = nt; changed = true; }
        }
    }

    dl->AddRectFilled(a, b, TRACK_BG, 3.f);
    dl->AddRectFilled(a, { a.x + width * value01, b.y }, fill, 3.f);
    dl->AddCircleFilled({ a.x + width * value01, (a.y + b.y) * 0.5f }, 6.f, IM_COL32(255, 255, 255, 255));
    return changed;
}


bool ui::Scrub(ImDrawList *dl, const ImVec2 &pos, const ImVec2 &size, float &value,
               float speed, int id, int &grab, const char *format)
{
    const ImGuiIO &io = ImGui::GetIO();
    const ImVec2 a = pos;
    const ImVec2 b { pos.x + size.x, pos.y + size.y };
    const bool hovered = PointIn(io.MousePos, a, b);

    if (grab == GRAB_NONE && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        grab = id;

    bool changed = false;
    if (grab == id)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            grab = GRAB_NONE;
        else if (io.MouseDelta.x != 0.f)
        {
            value += io.MouseDelta.x * speed;
            changed = true;
        }
    }
    if (grab == id || hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    dl->AddRectFilled(a, b, grab == id || hovered ? BTN_HOVER : BTN_BG, 3.f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), format, value);
    TextCentered(dl, a, b, TEXT_MAIN, buf);
    return changed;
}


bool ui::SideTab(ImDrawList *dl, const ImVec2 &origin, const ImVec2 &size, bool &open, bool busy)
{
    constexpr float TAB_W = 22.f, TAB_H = 48.f;
    const ImVec2 tabMin { origin.x, origin.y + size.y * 0.5f - TAB_H * 0.5f };
    const ImVec2 tabMax { origin.x + TAB_W, tabMin.y + TAB_H };
    const bool hovered = PointIn(ImGui::GetIO().MousePos, tabMin, tabMax);

    dl->AddRectFilled(tabMin, tabMax, hovered ? BTN_HOVER : PANEL_BG, 0.f);
    dl->AddLine({ tabMax.x, tabMin.y }, { tabMax.x, tabMax.y }, PANEL_BORDER, 1.f);
    DrawIcon(dl, tabMin, tabMax, TEXT_MAIN, open ? Icon::ArrowLeft : Icon::ArrowRight);

    if (hovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (!busy && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            open = !open;
    }

    return hovered;
}
