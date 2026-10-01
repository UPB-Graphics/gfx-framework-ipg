#include "lab/lab02/lab02.h"

#include "core/ui_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

// The editor interface for Lab02: the on-canvas handles, the toolbar, the vertex popup,
// and the mesh upload those edits drive. The scene's GPU work is in lab02.cpp.
//
// The whole editor is painted straight onto the viewport draw lists and hit-tested by hand -
// there is not one ImGui::Begin in this file. Everything is placed relative to the edges of
// the window, so there is no panel chrome to dock or move.
//
// The canvas is y-up (like OpenGL), while the screen is y-down, so canvas <-> screen
// conversions flip y.

using namespace lab;
using namespace ui;

namespace
{
    // Vertices placed on the canvas get a random colour and depth, so the colour and depth
    // interpolation show up on every new triangle without having to edit it first
    float Random01()
    {
        static std::mt19937 generator { std::random_device {}() };
        static std::uniform_real_distribution<float> distribution { 0.f, 1.f };
        return distribution(generator);
    }

    // Handle sizes, in screen pixels. The hit radius is generous so a handle is easy to grab;
    // the drawn radius is smaller so the picture underneath stays visible.
    constexpr float HANDLE_RADIUS = 6.f;
    constexpr float HANDLE_HIT_RADIUS = 12.f;

    // A dragged handle is kept within this margin of the canvas, so a vertex can be nudged just
    // off-screen (the shader still bounds-checks per pixel) but never dragged out of reach.
    constexpr float HANDLE_MARGIN = 256.f;

    constexpr ImU32 COL_EDGE        = IM_COL32(255, 255, 255, 90);
    constexpr ImU32 COL_FILL        = IM_COL32(255, 255, 255, 18);
    constexpr ImU32 COL_HANDLE_RING = IM_COL32(255, 255, 255, 220);
    constexpr ImU32 COL_HANDLE_DARK = IM_COL32(0, 0, 0, 200);
    constexpr ImU32 COL_HOT         = IM_COL32(255, 255, 255, 140);
    constexpr ImU32 COL_SELECTED    = IM_COL32(255, 214, 0, 255);
    constexpr ImU32 COL_PENDING     = IM_COL32(120, 220, 255, 255);
    constexpr ImU32 COL_INTERSECT   = IM_COL32(255, 90, 210, 255);

    // Tokens for `grab` - which hand-drawn control is dragging (see ui_utils.h)
    enum { GrabNone = GRAB_NONE, GrabPosX, GrabPosY, GrabDepth, GrabColR, GrabColG, GrabColB, GrabSegments };

    // The 3D segment along which triangles A and B actually intersect - their vertices carry
    // depth in z, so each triangle is a plane in (x, y, depth) space and two that overlap in the
    // projection still only meet if their depth ranges cross. Returns false when they miss, when
    // their planes are parallel, or when the overlap collapses to a point. Moller's method:
    // clip the line where the two planes meet to each triangle, then intersect the intervals.
    bool TriTriIntersection(const glm::vec3 A[3], const glm::vec3 B[3], glm::vec3& outP, glm::vec3& outQ)
    {
        const glm::vec3 nA = glm::cross(A[1] - A[0], A[2] - A[0]);
        const glm::vec3 nB = glm::cross(B[1] - B[0], B[2] - B[0]);
        if (glm::dot(nA, nA) < 1e-8f || glm::dot(nB, nB) < 1e-8f)
            return false;                                   // a degenerate triangle has no plane

        const float offA = -glm::dot(nA, A[0]);
        float db[3] = { glm::dot(nA, B[0]) + offA, glm::dot(nA, B[1]) + offA, glm::dot(nA, B[2]) + offA };
        if ((db[0] > 0.f && db[1] > 0.f && db[2] > 0.f) || (db[0] < 0.f && db[1] < 0.f && db[2] < 0.f))
            return false;                                   // B is wholly on one side of A

        const float offB = -glm::dot(nB, B[0]);
        float da[3] = { glm::dot(nB, A[0]) + offB, glm::dot(nB, A[1]) + offB, glm::dot(nB, A[2]) + offB };
        if ((da[0] > 0.f && da[1] > 0.f && da[2] > 0.f) || (da[0] < 0.f && da[1] < 0.f && da[2] < 0.f))
            return false;

        const glm::vec3 dir = glm::cross(nA, nB);
        if (glm::dot(dir, dir) < 1e-8f)
            return false;                                   // parallel or coplanar planes

        // Interval [t0, t1] (with its 3D endpoints) that a triangle cuts out of the shared line.
        const auto interval = [&](const glm::vec3 V[3], const float d[3],
                                  float& t0, float& t1, glm::vec3& e0, glm::vec3& e1) -> bool
        {
            int i0, i1, i2;
            if (d[0] * d[1] > 0.f)                      { i0 = 2; i1 = 0; i2 = 1; }
            else if (d[0] * d[2] > 0.f)                 { i0 = 1; i1 = 0; i2 = 2; }
            else if (d[1] * d[2] > 0.f || d[0] != 0.f)  { i0 = 0; i1 = 1; i2 = 2; }
            else if (d[1] != 0.f)                       { i0 = 1; i1 = 0; i2 = 2; }
            else if (d[2] != 0.f)                       { i0 = 2; i1 = 0; i2 = 1; }
            else return false;                             // triangle lies in the other plane

            const float f0 = d[i0] / (d[i0] - d[i1]);
            const float f1 = d[i0] / (d[i0] - d[i2]);
            e0 = V[i0] + (V[i1] - V[i0]) * f0;
            e1 = V[i0] + (V[i2] - V[i0]) * f1;
            t0 = glm::dot(dir, e0);
            t1 = glm::dot(dir, e1);
            if (t0 > t1) { std::swap(t0, t1); std::swap(e0, e1); }
            return true;
        };

        float at0, at1, bt0, bt1;
        glm::vec3 ae0, ae1, be0, be1;
        if (!interval(A, da, at0, at1, ae0, ae1) || !interval(B, db, bt0, bt1, be0, be1))
            return false;

        const float start = std::max(at0, bt0);
        const float end   = std::min(at1, bt1);
        if (start >= end)
            return false;                                   // planes cross, triangles do not

        const float span = at1 - at0;
        const auto pointAt = [&](float t) {
            const float s = span != 0.f ? (t - at0) / span : 0.f;
            return ae0 + (ae1 - ae0) * s;
        };
        outP = pointAt(start);
        outQ = pointAt(end);
        return true;
    }

    // Winding-agnostic point-in-triangle, used to pick a triangle for right-click deletion.
    bool PointInTriangle(const glm::vec2& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        const float d1 = (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
        const float d2 = (p.x - c.x) * (b.y - c.y) - (b.x - c.x) * (p.y - c.y);
        const float d3 = (p.x - a.x) * (c.y - a.y) - (c.x - a.x) * (p.y - a.y);

        const bool hasNeg = d1 < 0.f || d2 < 0.f || d3 < 0.f;
        const bool hasPos = d1 > 0.f || d2 > 0.f || d3 > 0.f;

        return !(hasNeg && hasPos);
    }
}

void Lab02::UploadMesh()
{
    UpdateMesh("triangles", vertices, triangles);
    UpdateMesh("circle", circle_vertices, circle_triangles);
}

void Lab02::DrawUserInterface()
{
    const ImGuiIO& io = ImGui::GetIO();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 origin = viewport->Pos;
    const ImVec2 size   = viewport->Size;
    if (size.x <= 0.f || size.y <= 0.f)
        return;

    // Both draw lists belong to the main viewport, not to any window: bg sits under the (absent)
    // windows, fg on top of everything. Mesh and handles go on bg, the editor chrome on fg.
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    ImDrawList* fg = ImGui::GetForegroundDrawList();

    const ImVec2 mouse = io.MousePos;
    const bool lclick  = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool ldouble = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    const bool rclick  = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

    // A vertex drag or a slider drag owns the mouse; chrome buttons ignore the release that ends it.
    const bool uiBusy = dragging_vertex >= 0 || grab != GrabNone;

    // ---- edge tab: toggles the whole editor -------------------------------------------------
    const bool overTab = SideTab(fg, origin, size, editor_open, uiBusy);

    if (!editor_open)
        return;

    // The scene blits colorImage across the whole window, so canvas space (CANVAS_SIZE) maps to
    // screen space by one per-axis scale, plus a flip, since the canvas is y-up. Every position
    // on screen goes through these two.
    const ImVec2 scale { size.x / static_cast<float>(CANVAS_SIZE.x),
                         size.y / static_cast<float>(CANVAS_SIZE.y) };
    const auto toScreen = [&](const glm::vec3& p) -> ImVec2 {
        return { origin.x + p.x * scale.x, origin.y + (static_cast<float>(CANVAS_SIZE.y) - p.y) * scale.y };
    };
    const auto toCanvas = [&](const ImVec2& s) -> glm::vec2 {
        return { (s.x - origin.x) / scale.x, static_cast<float>(CANVAS_SIZE.y) - (s.y - origin.y) / scale.y };
    };

    const auto vertexCount = static_cast<int>(vertices.size());
    const auto triangleCount = static_cast<int>(triangles.size());

    // The circle is generated by BuildCircle, not authored on the canvas, so while it is the
    // active mesh the editor draws no wireframe and no handles and the canvas ignores the mouse.
    // The toolbar stays, because that is where the mesh is switched back.
    const bool editable = !show_circle;

    int deleteVertex = -1;
    int deleteTriangle = -1;

    // ---- mesh: faint fill plus edges, drawn under the handles -----------------------------
    if (editable) {
        for (const Triangle& t : triangles) {
            if (t.x >= vertices.size() || t.y >= vertices.size() || t.z >= vertices.size())
                continue;

            const ImVec2 a = toScreen(vertices[t.x].position);
            const ImVec2 b = toScreen(vertices[t.y].position);
            const ImVec2 c = toScreen(vertices[t.z].position);

            bg->AddTriangleFilled(a, b, c, COL_FILL);
            bg->AddTriangle(a, b, c, COL_EDGE, 1.5f);
        }
    }

    // ---- highlight the 3D segment where two triangles actually intersect ---------------------
    if (editable && triangleCount >= 2) {
        const auto triOk = [&](const Triangle& t) {
            return t.x < vertices.size() && t.y < vertices.size() && t.z < vertices.size();
        };
        const auto sharedVerts = [](const Triangle& p, const Triangle& q) {
            int n = 0;
            for (int i = 0; i < 3; ++i)
                if (p[i] == q.x || p[i] == q.y || p[i] == q.z)
                    ++n;
            return n;
        };

        for (int i = 0; i < triangleCount; ++i) {
            if (!triOk(triangles[i]))
                continue;

            const glm::vec3 A[3] { vertices[triangles[i].x].position,
                                   vertices[triangles[i].y].position,
                                   vertices[triangles[i].z].position };

            for (int j = i + 1; j < triangleCount; ++j) {
                if (!triOk(triangles[j]) || sharedVerts(triangles[i], triangles[j]) >= 2)
                    continue;   // a shared edge is contact, not a penetration line

                const glm::vec3 B[3] { vertices[triangles[j].x].position,
                                       vertices[triangles[j].y].position,
                                       vertices[triangles[j].z].position };

                glm::vec3 p, q;
                if (!TriTriIntersection(A, B, p, q))
                    continue;

                const ImVec2 sp = toScreen(p);
                const ImVec2 sq = toScreen(q);
                bg->AddLine(sp, sq, IM_COL32(255, 90, 210, 90), 6.f);   // glow
                bg->AddLine(sp, sq, COL_INTERSECT, 2.5f);
                bg->AddCircleFilled(sp, 3.f, COL_INTERSECT);
                bg->AddCircleFilled(sq, 3.f, COL_INTERSECT);
            }
        }
    }

    if (editable && tool == Tool::AddTriangle && !pending_corners.empty()) {
        ImVec2 pts[3];
        int n = 0;
        for (const glm::uint c : pending_corners)
            if (c < vertices.size())
                pts[n++] = toScreen(vertices[c].position);

        for (int i = 0; i < n; ++i)
            bg->AddLine(pts[i], i + 1 < n ? pts[i + 1] : mouse, COL_PENDING, 1.5f);
        if (n >= 1)
            bg->AddLine(pts[0], mouse, IM_COL32(120, 220, 255, 90), 1.5f);
    }

    // ================= toolbar: top-left, just past the tab ================================
    bool overToolbar = false;
    {
        const float X = origin.x + 34.f, Y = origin.y + 14.f;
        constexpr float W = 300.f, P = 10.f, LH = 18.f, ROW = 22.f;
        const int hints = tool == Tool::Select ? 4 : 3;
        const int warns = (vertexCount >= MAX_VERTICES ? 1 : 0) + (triangleCount >= MAX_TRIANGLES ? 1 : 0);
        const float H = show_circle
            ? P + ROW + 10.f + LH + ROW + 8.f + LH + LH + P
            : P + ROW + 6.f + ROW + 10.f + hints * LH + 8.f + LH + warns * LH + P;

        const ImVec2 pmin { X, Y };
        const ImVec2 pmax { X + W, Y + H };
        overToolbar = PointIn(mouse, pmin, pmax);

        Panel(fg, pmin, pmax);

        const float cx = X + P;
        float cy = Y + P;
        const float iw = W - P * 2.f;
        const float bw = (iw - 6.f) * 0.5f;

        // ---- which mesh is rasterized: the hand-edited triangles, or the generated circle ----
        if (Button(fg, { cx, cy }, { bw, ROW }, "Triangles", !show_circle) && !uiBusy)
            show_circle = false;
        if (Button(fg, { cx + bw + 6.f, cy }, { bw, ROW }, "Circle", show_circle) && !uiBusy)
            show_circle = true;
        cy += ROW;

        char buf[96];
        if (show_circle) {
            // The editor has nothing to edit here: this mesh comes out of BuildCircle, not out of
            // the canvas. The one parameter exposed is how many triangles it is cut into.
            cy += 10.f;
            TextAt(fg, cx, cy, TEXT_DIM, "Segments");
            cy += LH;
            if (Scrub(fg, { cx, cy }, { iw, ROW }, circle_segments, 0.25f, GrabSegments, grab, "%.0f")) {
                circle_segments = std::clamp(circle_segments,
                                            static_cast<float>(CIRCLE_MIN_SEGMENTS),
                                            static_cast<float>(CIRCLE_MAX_SEGMENTS));
                circle_dirty = true;
            }
            cy += ROW + 8.f;

            std::snprintf(buf, sizeof(buf), "%d verts     %d tris",
                          static_cast<int>(circle_vertices.size()),
                          static_cast<int>(circle_triangles.size()));
            TextAt(fg, cx, cy, circle_triangles.empty() ? TEXT_WARN : TEXT_MAIN, buf);
            cy += LH;
            if (circle_triangles.empty())
                TextAt(fg, cx, cy, TEXT_DIM, "BuildCircle produced nothing yet.");
        } else {
            cy += 6.f;
            if (Button(fg, { cx, cy }, { bw, ROW }, "Select", tool == Tool::Select) && !uiBusy)
                tool = Tool::Select;
            if (Button(fg, { cx + bw + 6.f, cy }, { bw, ROW }, "+ Triangle", tool == Tool::AddTriangle) && !uiBusy)
                tool = Tool::AddTriangle;
            cy += ROW + 10.f;

            // Leaving AddTriangle abandons a half-placed triangle rather than carrying it into Select.
            if (tool != Tool::AddTriangle && !pending_corners.empty())
                pending_corners.clear();

            if (tool == Tool::Select) {
                TextAt(fg, cx, cy, TEXT_DIM, "Drag a handle to move it.");                       cy += LH;
                TextAt(fg, cx, cy, TEXT_DIM, "Double-click a handle: edit x / y, depth, colour."); cy += LH;
                TextAt(fg, cx, cy, TEXT_DIM, "Right-click a handle: delete it.");                 cy += LH;
                TextAt(fg, cx, cy, TEXT_DIM, "Right-click inside a triangle: remove it.");        cy += LH;
            } else {
                std::snprintf(buf, sizeof(buf), "Click the canvas to place a corner   (%d / 3).",
                              static_cast<int>(pending_corners.size()));
                TextAt(fg, cx, cy, TEXT_DIM, buf);                                        cy += LH;
                TextAt(fg, cx, cy, TEXT_DIM, "Click a handle to reuse that vertex.");     cy += LH;
                TextAt(fg, cx, cy, TEXT_DIM, "Right-click to cancel the corner.");        cy += LH;
            }
            cy += 8.f;

            std::snprintf(buf, sizeof(buf), "%d verts     %d tris", vertexCount, triangleCount);
            TextAt(fg, cx, cy, TEXT_MAIN, buf);                                          cy += LH;
            if (vertexCount >= MAX_VERTICES)    { TextAt(fg, cx, cy, TEXT_WARN, "vertex buffer full");    cy += LH; }
            if (triangleCount >= MAX_TRIANGLES) { TextAt(fg, cx, cy, TEXT_WARN, "triangle buffer full");  cy += LH; }
        }
    }

    // ================= vertex editor: floating panel next to the handle ===================
    bool overEditor = false;
    if (editable && popup_vertex >= 0 && popup_vertex < vertexCount) {
        constexpr float P = 10.f, W = 236.f, LH = 15.f, SL = 12.f, ROW = 20.f;
        const float H = P + 18.f + 4.f + LH + ROW + 8.f + LH + SL + 8.f
                      + LH + (SL + 5.f) * 2.f + SL + 8.f + 22.f + P;

        ImVec2 p = toScreen(vertices[popup_vertex].position);
        p.x += 16.f;
        p.y += 8.f;
        p.x = std::clamp(p.x, origin.x + 4.f, origin.x + size.x - W - 4.f);
        p.y = std::clamp(p.y, origin.y + 4.f, origin.y + size.y - H - 4.f);

        const ImVec2 pmin = p;
        const ImVec2 pmax { p.x + W, p.y + H };
        overEditor = PointIn(mouse, pmin, pmax);

        Panel(fg, pmin, pmax);

        Vertex& v = vertices[popup_vertex];
        const float cx = p.x + P;
        const float iw = W - P * 2.f;
        float cy = p.y + P;

        const auto users = std::ranges::count_if(triangles, [&](const Triangle& t) {
            const auto vi = static_cast<glm::uint>(popup_vertex);
            return t.x == vi || t.y == vi || t.z == vi;
        });

        char buf[64];
        std::snprintf(buf, sizeof(buf), "Vertex %d", popup_vertex);
        TextAt(fg, cx, cy, TEXT_MAIN, buf);
        std::snprintf(buf, sizeof(buf), users == 1 ? "%lld triangle" : "%lld triangles",
                      static_cast<long long>(users));
        {
            const ImVec2 ts = ImGui::CalcTextSize(buf);
            TextAt(fg, pmax.x - P - ts.x, cy, TEXT_DIM, buf);
        }
        cy += 18.f + 4.f;

        TextAt(fg, cx, cy, TEXT_DIM, "Position");
        cy += LH;
        {
            const float half = (iw - 6.f) * 0.5f;
            // Scrub speed is canvas units per screen pixel, so the number tracks the cursor 1:1
            // over the canvas. Positions stay unclamped here - only a drag of the handle is bounded.
            mesh_dirty |= Scrub(fg, { cx, cy }, { half, ROW }, v.position.x,
                               static_cast<float>(CANVAS_SIZE.x) / size.x, GrabPosX, grab, "x %.0f");
            mesh_dirty |= Scrub(fg, { cx + half + 6.f, cy }, { half, ROW }, v.position.y,
                               static_cast<float>(CANVAS_SIZE.y) / size.y, GrabPosY, grab, "y %.0f");
        }
        cy += ROW + 8.f;

        std::snprintf(buf, sizeof(buf), "Depth   %.3f", v.position.z);
        TextAt(fg, cx, cy, TEXT_DIM, buf);
        cy += LH;
        // Depth is clamped to [0, 1]: the depth image clears to 1.0 and the test is a plain less-than.
        mesh_dirty |= Slider(fg, { cx, cy }, iw, v.position.z, GrabDepth, grab);
        cy += SL + 8.f;

        TextAt(fg, cx, cy, TEXT_DIM, "Colour");
        fg->AddRectFilled({ pmax.x - P - 22.f, cy - 1.f }, { pmax.x - P, cy + 13.f },
                          ImGui::GetColorU32(ImVec4 { v.color.r, v.color.g, v.color.b, 1.f }), 3.f);
        cy += LH;
        mesh_dirty |= Slider(fg, { cx, cy }, iw, v.color.r, GrabColR, grab, IM_COL32(220, 70, 70, 255));  cy += SL + 5.f;
        mesh_dirty |= Slider(fg, { cx, cy }, iw, v.color.g, GrabColG, grab, IM_COL32(70, 200, 90, 255));   cy += SL + 5.f;
        mesh_dirty |= Slider(fg, { cx, cy }, iw, v.color.b, GrabColB, grab, IM_COL32(90, 130, 245, 255));  cy += SL + 8.f;

        if (Button(fg, { cx, cy }, { iw, 22.f }, "Delete vertex", false, Icon::None, BTN_DANGER, BTN_DANGER_HOVER)
            && grab == GrabNone && !uiBusy) {
            deleteVertex = popup_vertex;
            popup_vertex = -1;
        }
    } else {
        popup_vertex = -1;
    }

    const bool overChrome = overTab || overToolbar || overEditor;

    // A click anywhere off the editor panel closes it (the click still falls through to the canvas).
    if (popup_vertex >= 0 && lclick && !overEditor && grab == GrabNone)
        popup_vertex = -1;

    // ---- canvas interaction -------------------------------------------------------------
    int hoveredVertex = -1;

    if (!editable) {
        dragging_vertex = -1;
    } else if (dragging_vertex >= 0) {
        if (dragging_vertex >= vertexCount || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            dragging_vertex = -1;
        } else {
            if (io.MouseDelta.x != 0.f || io.MouseDelta.y != 0.f) {
                // Screen y grows downwards, canvas y upwards
                glm::vec3& pos = vertices[dragging_vertex].position;
                pos.x = std::clamp(pos.x + io.MouseDelta.x / scale.x,
                                   -HANDLE_MARGIN, static_cast<float>(CANVAS_SIZE.x) + HANDLE_MARGIN);
                pos.y = std::clamp(pos.y - io.MouseDelta.y / scale.y,
                                   -HANDLE_MARGIN, static_cast<float>(CANVAS_SIZE.y) + HANDLE_MARGIN);
                mesh_dirty = true;
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }
    } else if (!overChrome && grab == GrabNone) {
        float best = HANDLE_HIT_RADIUS * HANDLE_HIT_RADIUS;
        for (int i = 0; i < vertexCount; ++i) {
            const ImVec2 h = toScreen(vertices[i].position);
            const float dx = h.x - mouse.x;
            const float dy = h.y - mouse.y;
            if (dx * dx + dy * dy <= best) {
                best = dx * dx + dy * dy;
                hoveredVertex = i;
            }
        }

        if (hoveredVertex >= 0)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        if (hoveredVertex >= 0 && ldouble) {
            popup_vertex = hoveredVertex;
            selected_vertex = hoveredVertex;
            dragging_vertex = -1;
        } else if (lclick && !ldouble) {
            if (tool == Tool::Select) {
                selected_vertex = hoveredVertex;   // -1 on an empty click, which deselects
                dragging_vertex = hoveredVertex;
            } else { // Tool::AddTriangle
                glm::uint corner = 0;
                bool haveCorner = false;

                if (hoveredVertex >= 0) {
                    corner = static_cast<glm::uint>(hoveredVertex);
                    haveCorner = true;
                } else if (vertexCount < MAX_VERTICES) {
                    const glm::vec2 c = toCanvas(mouse);
                    vertices.push_back({ { c.x, c.y, Random01() }, { Random01(), Random01(), Random01() } });
                    corner = static_cast<glm::uint>(vertices.size() - 1);
                    haveCorner = true;
                    mesh_dirty = true;
                }

                if (haveCorner) {
                    pending_corners.push_back(corner);
                    selected_vertex = static_cast<int>(corner);

                    if (pending_corners.size() == 3) {
                        if (static_cast<int>(triangles.size()) < MAX_TRIANGLES) {
                            triangles.push_back({ pending_corners[0], pending_corners[1], pending_corners[2] });
                            mesh_dirty = true;
                        }
                        pending_corners.clear();
                    }
                }
            }
        } else if (rclick) {
            if (hoveredVertex >= 0) {
                deleteVertex = hoveredVertex;
            } else if (tool == Tool::AddTriangle && !pending_corners.empty()) {
                pending_corners.clear();
            } else {
                const glm::vec2 m = toCanvas(mouse);
                for (int i = static_cast<int>(triangles.size()) - 1; i >= 0; --i) {
                    const Triangle& t = triangles[i];
                    if (t.x >= vertices.size() || t.y >= vertices.size() || t.z >= vertices.size())
                        continue;
                    if (PointInTriangle(m, vertices[t.x].position, vertices[t.y].position, vertices[t.z].position)) {
                        deleteTriangle = i;
                        break;
                    }
                }
            }
        }
    }

    // ---- handles (on top of the wireframe) --------------------------------------------
    for (int i = 0; editable && i < vertexCount; ++i) {
        const ImVec2 h = toScreen(vertices[i].position);
        const glm::vec3& col = vertices[i].color;
        const ImU32 fill = ImGui::GetColorU32(ImVec4 { col.r, col.g, col.b, 1.f });

        const bool isPending = std::ranges::find(pending_corners, static_cast<glm::uint>(i)) != pending_corners.end();
        const bool isSelected = i == selected_vertex;
        const bool isHot = i == hoveredVertex || i == dragging_vertex || i == popup_vertex;

        bg->AddCircleFilled(h, HANDLE_RADIUS, fill);
        bg->AddCircle(h, HANDLE_RADIUS, COL_HANDLE_DARK, 0, 1.5f);
        bg->AddCircle(h, HANDLE_RADIUS + 1.5f, COL_HANDLE_RING, 0, 1.5f);

        if (isHot)
            bg->AddCircle(h, HANDLE_HIT_RADIUS, COL_HOT, 0, 1.5f);
        if (isSelected)
            bg->AddCircle(h, HANDLE_RADIUS + 4.f, COL_SELECTED, 0, 2.f);
        if (isPending)
            bg->AddCircle(h, HANDLE_RADIUS + 6.f, COL_PENDING, 0, 2.f);
    }

    // ---- deferred structural edits ---------------------------------------------------
    if (deleteVertex >= 0) {
        // Every triangle that used this vertex loses a corner and cannot survive it, so those
        // go too; the rest have their higher indices pulled down to match the shortened array.
        const auto removed = static_cast<glm::uint>(deleteVertex);

        std::erase_if(triangles, [&](const Triangle& t) {
            return t.x == removed || t.y == removed || t.z == removed;
        });

        for (Triangle& t : triangles)
            for (int corner = 0; corner < 3; ++corner)
                if (t[corner] > removed)
                    --t[corner];

        vertices.erase(vertices.begin() + deleteVertex);

        // Indices have shifted under it, so a staged triangle can no longer be trusted.
        pending_corners.clear();
        dragging_vertex = -1;
        popup_vertex = -1;
        grab = GrabNone;

        if (selected_vertex == deleteVertex)
            selected_vertex = -1;
        else if (selected_vertex > deleteVertex)
            --selected_vertex;

        mesh_dirty = true;
    } else if (deleteTriangle >= 0) {
        triangles.erase(triangles.begin() + deleteTriangle);
        mesh_dirty = true;
    }
}
