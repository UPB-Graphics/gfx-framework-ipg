#include "lab/lab03/lab03.h"

#include "core/ui_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>

// The editor interface for Lab03: the grid overlay (the lines that describe each viewport's logic
// space, and the cursor readout), and the toolbar that switches between the squares and the star
// and drives the grid. The scene is in lab03.cpp.
//
// Two things this file is careful about, both because the rest of the lab is the exercise:
//
// It never touches Viewport::matrix. transform2D::Viewport, Translate and the rest are what the
// student writes, and a grid built on them would be blank - or wrong in exactly the same way the
// student's code is wrong - precisely when it is most useful. So the mapping is redone here from
// the viewport and logic spaces each viewport was described with, which leaves the grid a fixed
// reference the rendered squares can be checked against.
//
// And there is not one ImGui::Begin in it. Everything is painted onto the viewport draw lists and
// hit-tested by hand, so the overlay cannot be docked or moved away from the picture it annotates.
//
// Logic space and the images are y-up, the screen is y-down: the logic origin is the bottom-left
// corner of a viewport, and every conversion to the screen flips y.

using namespace lab;
using namespace ui;

namespace
{
    // The cell sizes the - / + buttons step through, in logic units.
    constexpr float CELL_SIZES[] = { 0.25f, 0.5f, 1.f, 2.f, 4.f };
    constexpr int CELL_COUNT = static_cast<int>(std::size(CELL_SIZES));

    // Every Nth line away from the origin is drawn heavier, so a coordinate can be counted off in
    // groups rather than one line at a time.
    constexpr int MAJOR_EVERY = 5;

    // Below this many pixels apart the lines stop reading as a grid and become a wash of colour.
    constexpr float MIN_SPACING = 4.f;

    // ...and below this, a label on every major line runs into its neighbour.
    constexpr float MIN_LABEL_SPACING = 24.f;

    constexpr ImU32 COL_MINOR  = IM_COL32(255, 255, 255, 38);
    constexpr ImU32 COL_MAJOR  = IM_COL32(255, 255, 255, 90);
    constexpr ImU32 COL_AXIS   = IM_COL32(255, 170, 70, 220);
    constexpr ImU32 COL_BORDER = IM_COL32(255, 255, 255, 130);
    constexpr ImU32 COL_LABEL  = IM_COL32(255, 255, 255, 150);
    constexpr ImU32 COL_CELL   = IM_COL32(120, 200, 255, 40);
    constexpr ImU32 COL_CROSS  = IM_COL32(120, 200, 255, 200);

    // Tokens for `grab` - which hand-drawn control is dragging (see ui_utils.h)
    enum { GrabNone = GRAB_NONE, GrabRays };
}

void Lab03::DrawUserInterface()
{
    const ImGuiIO& io = ImGui::GetIO();

    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    const ImVec2 origin = mainViewport->Pos;
    const ImVec2 size = mainViewport->Size;
    if (size.x <= 0.f || size.y <= 0.f || targetSize.x <= 0 || targetSize.y <= 0)
        return;

    // The scene blits colorImage across the whole window, so an image pixel maps to a screen
    // pixel by one per-axis scale. Both are 1 while the images follow the window.
    const ImVec2 scale { size.x / static_cast<float>(targetSize.x),
                         size.y / static_cast<float>(targetSize.y) };

    // Both lists belong to the main viewport rather than to any window: bg sits under the (absent)
    // windows, fg on top of everything. The grid annotates the picture, so it goes on bg; the toolbar
    // and the readout sit above both.
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    ImDrawList* fg = ImGui::GetForegroundDrawList();

    const ImVec2 mouse = io.MousePos;

    if (ImGui::IsKeyPressed(ImGuiKey_G, false) && !io.WantCaptureKeyboard)
        grid_enabled = !grid_enabled;

    // A drag owns the mouse; buttons ignore the release that ends it
    const bool uiBusy = grab != GrabNone;

    // ---- edge tab: toggles the whole editor -------------------------------------------------
    SideTab(fg, origin, size, editor_open, uiBusy);

    if (!editor_open)
        return;

    const float cell = CELL_SIZES[std::clamp(grid_cell, 0, CELL_COUNT - 1)];
    const float textHeight = ImGui::GetTextLineHeight();

    // Whether the cursor is over a viewport, and where in its logic space: filled in while the
    // viewports are drawn, reported next to the cursor afterwards.
    bool hovered = false;
    glm::vec2 hoveredLogic { 0.f };

    // ---- the grid itself --------------------------------------------------------------------
    if (grid_enabled) {
        for (const Viewport& viewport : viewports) {
            const transform2D::ViewportSpace& v = viewport.space;
            const transform2D::LogicSpace& l = logic_space;

            // A viewport with no area, or describing no logic space, has no grid to draw and
            // would divide by zero working one out.
            if (v.width <= 0 || v.height <= 0 || l.width <= 0.f || l.height <= 0.f)
                continue;

            // The viewport on screen. The image is y-up, so its bottom edge is at v.y.
            const ImVec2 min { origin.x + v.x * scale.x,
                               origin.y + (targetSize.y - (v.y + v.height)) * scale.y };
            const ImVec2 max { min.x + v.width * scale.x,
                               min.y + v.height * scale.y };

            // Screen pixels per logic unit, per axis. Non-square cells are a real possibility:
            // a 16:9 logic space shown in a viewport of another aspect stretches one of them.
            const ImVec2 perUnit { (max.x - min.x) / l.width, (max.y - min.y) / l.height };

            const auto toScreen = [&](const glm::vec2& logic) -> ImVec2 {
                return { min.x + (logic.x - l.x) * perUnit.x,
                         max.y - (logic.y - l.y) * perUnit.y };
            };
            const auto toLogic = [&](const ImVec2& p) -> glm::vec2 {
                return { l.x + (p.x - min.x) / perUnit.x,
                         l.y + (max.y - p.y) / perUnit.y };
            };

            // The border goes down first so the axes, which sit on it whenever the logic origin
            // is the viewport's own corner, are drawn over it rather than under it. The clip is
            // a pixel generous for the same reason: a 2px line centred on the edge would
            // otherwise lose its outer half.
            bg->AddRect(min, max, COL_BORDER);
            bg->PushClipRect({ min.x - 1.f, min.y - 1.f }, { max.x + 1.f, max.y + 1.f }, true);

            // The cell under the cursor, filled before the lines so they stay on top of it.
            if (PointIn(mouse, min, max)) {
                hovered = true;
                hoveredLogic = toLogic(mouse);

                const glm::vec2 corner { std::floor(hoveredLogic.x / cell) * cell,
                                         std::floor(hoveredLogic.y / cell) * cell };
                const ImVec2 a = toScreen(corner);
                const ImVec2 b = toScreen(corner + cell);
                bg->AddRectFilled({ std::min(a.x, b.x), std::min(a.y, b.y) },
                                  { std::max(a.x, b.x), std::max(a.y, b.y) }, COL_CELL);
            }

            for (int axis = 0; axis < 2; ++axis) {
                const float spacing = (axis == 0 ? perUnit.x : perUnit.y) * cell;
                if (spacing < MIN_SPACING)
                    continue;

                const float from = axis == 0 ? l.x : l.y;
                const float to = from + (axis == 0 ? l.width : l.height);

                const int first = static_cast<int>(std::ceil(from / cell));
                const int last = static_cast<int>(std::floor(to / cell));

                for (int i = first; i <= last; ++i) {
                    const float logic = i * cell;
                    const ImU32 color = i == 0 ? COL_AXIS
                                      : i % MAJOR_EVERY == 0 ? COL_MAJOR
                                                             : COL_MINOR;

                    const ImVec2 at = axis == 0 ? toScreen({ logic, l.y }) : toScreen({ l.x, logic });
                    bg->AddLine(axis == 0 ? ImVec2 { at.x, min.y } : ImVec2 { min.x, at.y },
                                axis == 0 ? ImVec2 { at.x, max.y } : ImVec2 { max.x, at.y },
                                color, i == 0 ? 2.f : 1.f);

                    // Labels ride the two edges the logic origin sits at (the bottom and the
                    // left), and only the heavier lines carry one - a number on every cell is
                    // unreadable at any useful density.
                    if (!grid_labels || i % MAJOR_EVERY != 0 || spacing < MIN_LABEL_SPACING)
                        continue;

                    char text[16];
                    std::snprintf(text, sizeof(text), "%g", logic);
                    bg->AddText(axis == 0 ? ImVec2 { at.x + 3.f, max.y - textHeight - 2.f }
                                          : ImVec2 { min.x + 3.f, at.y - textHeight - 2.f },
                                COL_LABEL, text);
                }
            }

            bg->PopClipRect();
        }
    }

    // ---- cursor readout ---------------------------------------------------------------------
    // The one thing the grid cannot show by itself: where a fractional coordinate actually is.
    if (grid_enabled && grid_readout && hovered) {
        bg->AddLine({ mouse.x - 7.f, mouse.y }, { mouse.x + 7.f, mouse.y }, COL_CROSS);
        bg->AddLine({ mouse.x, mouse.y - 7.f }, { mouse.x, mouse.y + 7.f }, COL_CROSS);

        char text[48];
        std::snprintf(text, sizeof(text), "%.2f, %.2f", hoveredLogic.x, hoveredLogic.y);

        const ImVec2 ts = ImGui::CalcTextSize(text);
        const ImVec2 a { mouse.x + 14.f, mouse.y + 14.f };
        const ImVec2 b { a.x + ts.x + 10.f, a.y + ts.y + 6.f };

        fg->AddRectFilled(a, b, PANEL_BG, 3.f);
        fg->AddRect(a, b, PANEL_BORDER, 3.f);
        fg->AddText({ a.x + 5.f, a.y + 3.f }, TEXT_MAIN, text);
    }

    // ================= toolbar: top-left, just past the tab ================================
    // The logic origin is a viewport's bottom-left corner, which is where the grid labels come
    // out of, so the toolbar keeps away from it. The contents are drawn first, on the second
    // channel, so the panel behind them can be sized to fit once they are all laid out.
    {
        const float X = origin.x + 34.f, Y = origin.y + 14.f;
        constexpr float W = 300.f, P = 10.f, LH = 18.f, ROW = 22.f, BTN = 24.f, GAP = 4.f;
        const float cx = X + P;
        const float iw = W - P * 2.f;
        const float bw = (iw - 6.f) * 0.5f;
        float cy = Y + P;
        char buf[96];

        fg->ChannelsSplit(2);
        fg->ChannelsSetCurrent(1);

        // ---- which mesh is drawn: the squares, or the star ----
        if (Button(fg, { cx, cy }, { bw, ROW }, "Squares", !show_star) && !uiBusy)
            show_star = false;
        if (Button(fg, { cx + bw + 6.f, cy }, { bw, ROW }, "Star", show_star) && !uiBusy)
            show_star = true;
        cy += ROW;

        if (show_star) {
            // The star is one triangle, BuildStarRay's, drawn once per transform of
            // BuildStarTransforms. The one parameter exposed is how many rays it has.
            cy += 10.f;
            TextAt(fg, cx, cy, TEXT_DIM, "Rays");
            cy += LH;
            if (Scrub(fg, { cx, cy }, { iw, ROW }, star_rays, 0.1f, GrabRays, grab, "%.0f")) {
                star_rays = std::clamp(star_rays, static_cast<float>(STAR_MIN_RAYS), static_cast<float>(STAR_MAX_RAYS));
                star_dirty = true;
            }
            cy += ROW + 8.f;

            std::snprintf(buf, sizeof(buf), "%d verts     %d tris     %d transforms",
                          static_cast<int>(star_vertices.size()),
                          static_cast<int>(star_triangles.size()),
                          static_cast<int>(star_transforms.size()));
            TextAt(fg, cx, cy, star_triangles.empty() || star_transforms.empty() ? TEXT_WARN : TEXT_MAIN, buf);
            cy += LH;
            if (star_triangles.empty()) {
                TextAt(fg, cx, cy, TEXT_DIM, "BuildStarRay produced nothing yet.");
                cy += LH;
            }
            if (star_transforms.empty()) {
                TextAt(fg, cx, cy, TEXT_DIM, "BuildStarTransforms produced nothing yet.");
                cy += LH;
            }
        }
        cy += 10.f;

        // ---- the grid ----
        TextAt(fg, cx, cy, TEXT_DIM, "Grid (G)");
        cy += LH;
        {
            char cellText[16];
            std::snprintf(cellText, sizeof(cellText), "%g", cell);
            const float cellWidth = std::max(ImGui::CalcTextSize(cellText).x + 12.f, 34.f);

            ImVec2 at { cx, cy };
            const auto advance = [&](float width) { at.x += width + GAP; };

            if (Button(fg, at, { BTN, BTN }, "", grid_enabled, Icon::Grid) && !uiBusy)
                grid_enabled = !grid_enabled;
            advance(BTN);

            if (Button(fg, at, { BTN, BTN }, "-") && !uiBusy && grid_cell > 0)
                --grid_cell;
            advance(BTN);

            // Not a button: the cell size in logic units, sat between the two controls that change it.
            TextCentered(fg, at, { at.x + cellWidth, at.y + BTN }, grid_enabled ? TEXT_MAIN : TEXT_DIM, cellText);
            advance(cellWidth);

            if (Button(fg, at, { BTN, BTN }, "+") && !uiBusy && grid_cell < CELL_COUNT - 1)
                ++grid_cell;
            advance(BTN);

            if (Button(fg, at, { BTN, BTN }, "12", grid_labels) && !uiBusy)
                grid_labels = !grid_labels;
            advance(BTN);

            if (Button(fg, at, { BTN, BTN }, "", grid_readout, Icon::Crosshair) && !uiBusy)
                grid_readout = !grid_readout;
        }
        cy += BTN + 8.f;

        TextAt(fg, cx, cy, TEXT_DIM, "Arrow keys: move the logic space.");
        cy += LH;

        fg->ChannelsSetCurrent(0);
        Panel(fg, { X, Y }, { X + W, cy + P - 4.f });
        fg->ChannelsMerge();
    }
}
