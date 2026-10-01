#include "lab/lab04/lab04.h"

#include "core/ui_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// The editor interface for Lab04: the toolbar with the mesh, model, camera, pipeline and overlay
// settings, the projected triangles (outline, winding, face) drawn over the image, and the
// vertex inspector following one vertex through every stage. The scene is in lab04.cpp.
//
// Like the other labs' editors, it is painted straight onto the viewport draw lists and
// hit-tested by hand - there is not one ImGui::Begin in this file.
//
// The overlay shows what the stages of the shader produce: Lab04::Inspect runs them over the
// vertices and triangles of the mesh (assets/shaders/rasterizer/Lab04Inspect.comp.glsl) and
// reads the results back.

using namespace lab;
using namespace ui;

namespace
{
    constexpr ImU32 COL_FRONT       = IM_COL32(80, 190, 255, 230);
    constexpr ImU32 COL_BACK        = IM_COL32(255, 150, 60, 230);
    constexpr ImU32 COL_UNKNOWN     = IM_COL32(200, 200, 200, 230);
    constexpr ImU32 COL_CULLED      = IM_COL32(255, 255, 255, 60);
    constexpr ImU32 COL_HANDLE_RING = IM_COL32(255, 255, 255, 220);
    constexpr ImU32 COL_HANDLE_DARK = IM_COL32(0, 0, 0, 200);
    constexpr ImU32 COL_HOT         = IM_COL32(255, 255, 255, 140);
    constexpr ImU32 COL_SELECTED    = IM_COL32(255, 214, 0, 255);
    constexpr ImU32 COL_LABEL       = IM_COL32(255, 255, 255, 200);

    constexpr float HANDLE_RADIUS = 5.f;
    constexpr float HANDLE_HIT_RADIUS = 10.f;

    // The faces written by Lab04Inspect.comp.glsl (the constants of gpu_programs/lab04.glsl)
    constexpr int FACE_BEHIND_CAMERA = -1;
    constexpr int FACE_BACK = 0;
    constexpr int FACE_FRONT = 1;

    // Tokens for `grab` - which hand-drawn control is dragging (see ui_utils.h)
    enum {
        GrabNone = GRAB_NONE,
        GrabPosition, GrabRotation = GrabPosition + 3, GrabScale = GrabRotation + 3,
        GrabFov = GrabScale + 3, GrabNear, GrabFar
    };

    // The inspector panel's columns: the name of the space, then x, y, z and w
    constexpr float INSPECTOR_LABEL_W = 64.f;
    constexpr float INSPECTOR_VALUE_W = 62.f;

    // An arrowhead in the middle of the edge a -> b, pointing from a to b. It is pulled a little
    // towards `center`, the middle of its triangle, so the arrows of two triangles sharing the
    // edge (which point in opposite directions) sit side by side instead of on top of each other.
    void EdgeArrow(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, const ImVec2 &center, ImU32 col)
    {
        const glm::vec2 d { b.x - a.x, b.y - a.y };
        const float length = glm::length(d);
        if (length < 24.f)
            return;

        constexpr float S = 6.f;
        const glm::vec2 dir = d / length;
        const glm::vec2 normal { -dir.y, dir.x };
        const glm::vec2 middle = glm::vec2(a.x, a.y) + d * 0.5f;
        const glm::vec2 inward = glm::vec2(center.x, center.y) - middle;
        const glm::vec2 offset = glm::dot(inward, normal) > 0.f ? normal * S : -normal * S;
        const glm::vec2 tip = middle + offset + dir * S;
        const glm::vec2 left = tip - dir * S * 2.f + normal * S;
        const glm::vec2 right = tip - dir * S * 2.f - normal * S;
        dl->AddTriangleFilled({ tip.x, tip.y }, { left.x, left.y }, { right.x, right.y }, col);
    }

    // A row of three scrubs for the components of `value`, after a label. True when one changed.
    bool Scrub3(ImDrawList *dl, float x, float y, float width, const char *label, glm::vec3 &value,
                float speed, int firstId, int &grab, const char *format)
    {
        constexpr float LABEL_W = 62.f, ROW = 22.f, GAP = 4.f;
        TextAt(dl, x, y + (ROW - ImGui::GetTextLineHeight()) * 0.5f, TEXT_DIM, label);

        const float w = (width - LABEL_W - GAP * 2.f) / 3.f;
        bool changed = false;
        for (int i = 0; i < 3; ++i) {
            changed |= Scrub(dl, { x + LABEL_W + i * (w + GAP), y }, { w, ROW }, value[i], speed, firstId + i, grab, format);
        }
        return changed;
    }

    // One row of the inspector: the name of the space and up to four components
    void InspectorRow(ImDrawList *dl, float x, float y, const char *label, const glm::vec4 &value, int components)
    {
        TextAt(dl, x, y, TEXT_DIM, label);

        for (int i = 0; i < components; ++i) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.3f", value[i]);
            const float right = x + INSPECTOR_LABEL_W + (i + 1) * INSPECTOR_VALUE_W;
            TextAt(dl, right - ImGui::CalcTextSize(buf).x, y, TEXT_MAIN, buf);
        }
    }
}

void Lab04::DrawUserInterface()
{
    const ImGuiIO &io = ImGui::GetIO();

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 origin = viewport->Pos;
    const ImVec2 size = viewport->Size;
    if (size.x <= 0.f || size.y <= 0.f || targetSize.x <= 0 || targetSize.y <= 0)
        return;

    // Both draw lists belong to the main viewport, not to any window: bg sits under the (absent)
    // windows, fg on top of everything. The overlay goes on bg, the editor chrome on fg.
    ImDrawList *bg = ImGui::GetBackgroundDrawList();
    ImDrawList *fg = ImGui::GetForegroundDrawList();

    const ImVec2 mouse = io.MousePos;
    const bool uiBusy = grab != GrabNone;

    // ---- edge tab: toggles the whole editor -------------------------------------------------
    const bool overTab = SideTab(fg, origin, size, editor_open, uiBusy);

    if (!editor_open) {
        hovered_vertex = -1;
        return;
    }

    // The mesh on screen: the cube, or the tetrahedron of the bonus
    const std::vector<Vertex> &vertices = ShownVertices();
    const std::vector<Triangle> &triangles = ShownTriangles();

    const bool inspected = inspected_vertices.size() == vertices.size() && inspected_faces.size() == triangles.size();

    // The image is blitted over the whole window, and it is y up while the screen is y down
    const ImVec2 scale { size.x / static_cast<float>(targetSize.x), size.y / static_cast<float>(targetSize.y) };
    const auto toScreen = [&](const glm::vec4 &pixel) -> ImVec2 {
        return { origin.x + pixel.x * scale.x, origin.y + (static_cast<float>(targetSize.y) - pixel.y) * scale.y };
    };

    // A vertex has a place on screen unless it is behind the camera, which is not drawn
    const auto onScreen = [&](int i) { return inspected && (!perspective_divide || inspected_vertices[i].clip.w > 0.f); };

    // ---- the triangles: outline, winding, face ---------------------------------------------
    int behindCamera = 0, front = 0, back = 0, unknown = 0, culled = 0;

    for (size_t i = 0; inspected && i < triangles.size(); ++i) {
        const Triangle &t = triangles[i];
        const int face = inspected_faces[i];

        if (face == FACE_BEHIND_CAMERA) {
            ++behindCamera;
            continue;
        }

        const bool isCulled = cull_face_option == BOTH_FACES
                           || (face == FACE_FRONT && cull_face_option == FRONT_FACES)
                           || (face == FACE_BACK && cull_face_option == BACK_FACES);

        if (face == FACE_FRONT) {
            ++front;
        } else if (face == FACE_BACK) {
            ++back;
        } else {
            ++unknown;
        }
        if (isCulled) {
            ++culled;
        }

        if (!show_wireframe)
            continue;

        const ImVec2 a = toScreen(inspected_vertices[t.x].screen);
        const ImVec2 b = toScreen(inspected_vertices[t.y].screen);
        const ImVec2 c = toScreen(inspected_vertices[t.z].screen);
        const ImU32 col = isCulled ? COL_CULLED
                        : face == FACE_FRONT ? COL_FRONT
                        : face == FACE_BACK ? COL_BACK
                                            : COL_UNKNOWN;

        bg->AddTriangle(a, b, c, col, isCulled ? 1.f : 1.5f);
        if (show_winding && !isCulled) {
            const ImVec2 center { (a.x + b.x + c.x) / 3.f, (a.y + b.y + c.y) / 3.f };
            EdgeArrow(bg, a, b, center, col);
            EdgeArrow(bg, b, c, center, col);
            EdgeArrow(bg, c, a, center, col);
        }
    }

    // ================= toolbar: top-left, just past the tab ================================
    // The contents are drawn first, on the second channel, so the panel behind them can be
    // sized to fit once they are all laid out.
    bool overToolbar = false;
    {
        const float X = origin.x + 34.f, Y = origin.y + 14.f;
        constexpr float W = 320.f, P = 10.f, LH = 18.f, ROW = 22.f, GAP = 6.f;
        const float cx = X + P;
        const float iw = W - P * 2.f;
        float cy = Y + P;
        char buf[96];

        fg->ChannelsSplit(2);
        fg->ChannelsSetCurrent(1);

        const auto section = [&](const char *title) {
            TextAt(fg, cx, cy, TEXT_MAIN, title);
            cy += LH + 2.f;
        };

        // ---- which mesh is drawn: the cube, or the tetrahedron ----
        {
            const float w = (iw - GAP) * 0.5f;
            if (Button(fg, { cx, cy }, { w, ROW }, "Cube", !show_tetrahedron) && !uiBusy && show_tetrahedron) {
                show_tetrahedron = false;
                selected_vertex = -1;
            }
            if (Button(fg, { cx + w + GAP, cy }, { w, ROW }, "Tetrahedron", show_tetrahedron) && !uiBusy && !show_tetrahedron) {
                show_tetrahedron = true;
                selected_vertex = -1;
            }
            cy += ROW + 6.f;
        }
        if (show_tetrahedron) {
            std::snprintf(buf, sizeof(buf), "%d verts     %d tris",
                          static_cast<int>(vertices.size()), static_cast<int>(triangles.size()));
            TextAt(fg, cx, cy, triangles.empty() ? TEXT_WARN : TEXT_MAIN, buf);
            cy += LH;
            if (triangles.empty()) {
                TextAt(fg, cx, cy, TEXT_DIM, "BuildTetrahedron produced nothing yet.");
                cy += LH;
            }
        }
        cy += 6.f;

        // ---- the model transformation ----
        const float modelY = cy;
        section("Model");
        Scrub3(fg, cx, cy, iw, "position", mesh_position, 0.02f, GrabPosition, grab, "%.1f");
        mesh_position = glm::clamp(mesh_position, glm::vec3(-5.f), glm::vec3(5.f));
        cy += ROW + 4.f;
        if (Scrub3(fg, cx, cy, iw, "rotation", mesh_rotation, 0.5f, GrabRotation, grab, "%.0f")) {
            mesh_rotation = glm::mod(mesh_rotation + 180.f, glm::vec3(360.f)) - 180.f;
        }
        cy += ROW + 4.f;
        Scrub3(fg, cx, cy, iw, "scale", mesh_scale, 0.01f, GrabScale, grab, "%.2f");
        mesh_scale = glm::clamp(mesh_scale, glm::vec3(0.1f), glm::vec3(5.f));
        cy += ROW + 4.f;
        if (Button(fg, { cx, cy }, { iw, ROW }, "Spin", spin) && !uiBusy) {
            spin = !spin;
        }
        cy += ROW + 10.f;

        // ---- the camera ----
        section("Camera");
        {
            const float w = (iw - 8.f) / 3.f;
            Scrub(fg, { cx, cy }, { w, ROW }, fov, 0.2f, GrabFov, grab, "fov %.0f");
            Scrub(fg, { cx + w + 4.f, cy }, { w, ROW }, z_near, 0.005f, GrabNear, grab, "near %.2f");
            Scrub(fg, { cx + (w + 4.f) * 2.f, cy }, { w, ROW }, z_far, 0.2f, GrabFar, grab, "far %.0f");
            fov = std::clamp(fov, 20.f, 130.f);
            z_near = std::clamp(z_near, 0.01f, 2.f);
            z_far = std::clamp(z_far, 10.f, 100.f);
        }
        cy += ROW + 10.f;

        // ---- the stages that can be turned off, each one call in the shader's main ----
        section("Pipeline");
        {
            const float w = (iw - GAP) * 0.5f;
            if (Button(fg, { cx, cy }, { w, ROW }, "Perspective division", perspective_divide) && !uiBusy)
                perspective_divide = !perspective_divide;
            if (Button(fg, { cx + w + GAP, cy }, { w, ROW }, "Depth test", depth_test) && !uiBusy)
                depth_test = !depth_test;
            cy += ROW + 6.f;
        }
        if (!perspective_divide && depth_test) {
            TextAt(fg, cx, cy, TEXT_WARN, "Depths leave [0, 1]: turn off the depth test too.");
            cy += LH;
        }
        TextAt(fg, cx, cy, TEXT_DIM, "Face culling (F)");
        cy += LH;
        {
            static const char *names[] = { "Back", "Front", "None", "Both" };
            const float w = (iw - 3.f * 4.f) / 4.f;
            for (int i = 0; i < 4; ++i) {
                if (Button(fg, { cx + i * (w + 4.f), cy }, { w, ROW }, names[i], cull_face_option == i) && !uiBusy)
                    cull_face_option = static_cast<CULL_FACE_OPTION>(i);
            }
        }
        cy += ROW + 10.f;

        // ---- the overlay ----
        section("Overlay");
        {
            const float w = (iw - 8.f) / 3.f;
            if (Button(fg, { cx, cy }, { w, ROW }, "Wireframe", show_wireframe) && !uiBusy)
                show_wireframe = !show_wireframe;
            if (Button(fg, { cx + w + 4.f, cy }, { w, ROW }, "Winding", show_wireframe && show_winding) && !uiBusy)
                show_winding = !show_winding;
            if (Button(fg, { cx + (w + 4.f) * 2.f, cy }, { w, ROW }, "Vertices", show_vertices) && !uiBusy)
                show_vertices = !show_vertices;
        }
        cy += ROW + 8.f;

        // ---- what the overlay found ----
        {
            float x = cx;
            const auto count = [&](ImU32 col, const char *text) {
                TextAt(fg, x, cy, col, text);
                x += ImGui::CalcTextSize(text).x + 14.f;
            };
            std::snprintf(buf, sizeof(buf), "%d front", front);
            count(COL_FRONT, buf);
            std::snprintf(buf, sizeof(buf), "%d back", back);
            count(COL_BACK, buf);
            std::snprintf(buf, sizeof(buf), "%d culled", culled);
            count(TEXT_MAIN, buf);
            cy += LH;
        }
        if (unknown > 0) {
            std::snprintf(buf, sizeof(buf), "%d with no face determined", unknown);
            TextAt(fg, cx, cy, COL_UNKNOWN, buf);
            cy += LH;
        }
        if (behindCamera > 0) {
            std::snprintf(buf, sizeof(buf), "%d behind the camera, not drawn", behindCamera);
            TextAt(fg, cx, cy, TEXT_WARN, buf);
            cy += LH;
        }
        cy += 4.f;
        TextAt(fg, cx, cy, TEXT_DIM, "Right mouse + W A S D Q E: move the camera.");
        cy += LH;
        TextAt(fg, cx, cy, TEXT_DIM, "Click a vertex to pin it in the inspector.");
        cy += LH;

        std::snprintf(buf, sizeof(buf), "%.0f fps", io.Framerate);
        TextAt(fg, X + W - P - ImGui::CalcTextSize(buf).x, modelY, TEXT_DIM, buf);

        const ImVec2 pmin { X, Y };
        const ImVec2 pmax { X + W, cy + P - 4.f };
        overToolbar = PointIn(mouse, pmin, pmax);

        fg->ChannelsSetCurrent(0);
        Panel(fg, pmin, pmax);
        fg->ChannelsMerge();
    }

    // ================= vertex inspector: floating panel next to the vertex ==================
    constexpr float IP = 10.f, ILH = 17.f;
    const float inspectorW = IP * 2.f + INSPECTOR_LABEL_W + 4.f * INSPECTOR_VALUE_W;
    const float inspectorH = IP * 2.f + 20.f + ILH * 6.f;

    // Where the panel of vertex `i` goes: beside its handle, kept inside the window
    const auto inspectorAt = [&](int i) -> ImVec2 {
        ImVec2 p = toScreen(inspected_vertices[i].screen);
        p.x = std::clamp(p.x + 16.f, origin.x + 4.f, origin.x + size.x - inspectorW - 4.f);
        p.y = std::clamp(p.y + 8.f, origin.y + 4.f, origin.y + size.y - inspectorH - 4.f);
        return p;
    };

    // The pinned vertex's panel is where it was last frame too, so clicks on it are not picks
    bool overInspector = false;
    if (show_vertices && selected_vertex >= 0 && selected_vertex < static_cast<int>(vertices.size()) && onScreen(selected_vertex)) {
        const ImVec2 p = inspectorAt(selected_vertex);
        overInspector = PointIn(mouse, p, { p.x + inspectorW, p.y + inspectorH });
    }

    if (!show_vertices || !inspected) {
        hovered_vertex = -1;
        return;
    }

    // ---- the vertices: handles and picking --------------------------------------------------
    const bool overChrome = overTab || overToolbar || overInspector;

    hovered_vertex = -1;
    if (!overChrome && !uiBusy) {
        float best = HANDLE_HIT_RADIUS * HANDLE_HIT_RADIUS;
        for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
            if (!onScreen(i))
                continue;
            const ImVec2 h = toScreen(inspected_vertices[i].screen);
            const float dx = h.x - mouse.x, dy = h.y - mouse.y;
            if (dx * dx + dy * dy <= best) {
                best = dx * dx + dy * dy;
                hovered_vertex = i;
            }
        }

        // A click pins the vertex under the cursor, a click on nothing unpins it. The right
        // mouse button moves the camera, so only the left one picks.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selected_vertex = hovered_vertex;
        }
    }

    if (hovered_vertex >= 0)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        if (!onScreen(i))
            continue;

        const ImVec2 h = toScreen(inspected_vertices[i].screen);
        const glm::vec3 &color = vertices[i].color;

        bg->AddCircleFilled(h, HANDLE_RADIUS, ImGui::GetColorU32(ImVec4(color.r, color.g, color.b, 1.f)));
        bg->AddCircle(h, HANDLE_RADIUS, COL_HANDLE_DARK, 0, 1.5f);
        bg->AddCircle(h, HANDLE_RADIUS + 1.5f, COL_HANDLE_RING, 0, 1.5f);
        if (i == hovered_vertex)
            bg->AddCircle(h, HANDLE_HIT_RADIUS, COL_HOT, 0, 1.5f);
        if (i == selected_vertex)
            bg->AddCircle(h, HANDLE_RADIUS + 4.f, COL_SELECTED, 0, 2.f);

        char label[8];
        std::snprintf(label, sizeof(label), "%d", i);
        bg->AddText({ h.x + 8.f, h.y - 18.f }, COL_LABEL, label);
    }

    // ---- the inspector: the hovered vertex, or else the pinned one --------------------------
    const int vertex = hovered_vertex >= 0 ? hovered_vertex : selected_vertex;
    if (vertex < 0 || vertex >= static_cast<int>(vertices.size()) || !onScreen(vertex))
        return;

    const InspectedVertex &v = inspected_vertices[vertex];
    const ImVec2 pmin = inspectorAt(vertex);
    const ImVec2 pmax { pmin.x + inspectorW, pmin.y + inspectorH };
    Panel(fg, pmin, pmax);

    const float x = pmin.x + IP;
    float y = pmin.y + IP;
    char buf[64];

    std::snprintf(buf, sizeof(buf), "Vertex %d", vertex);
    TextAt(fg, x, y, TEXT_MAIN, buf);
    if (vertex == selected_vertex) {
        TextAt(fg, x + ImGui::CalcTextSize(buf).x + 8.f, y, TEXT_DIM, "pinned");
    }
    const glm::vec3 &color = vertices[vertex].color;
    fg->AddRectFilled({ pmax.x - IP - 22.f, y }, { pmax.x - IP, y + 14.f },
                      ImGui::GetColorU32(ImVec4(color.r, color.g, color.b, 1.f)), 3.f);
    y += 20.f;

    static const char *axes[] = { "x", "y", "z", "w" };
    for (int i = 0; i < 4; ++i) {
        const float right = x + INSPECTOR_LABEL_W + (i + 1) * INSPECTOR_VALUE_W;
        TextAt(fg, right - ImGui::CalcTextSize(axes[i]).x, y, TEXT_DIM, axes[i]);
    }
    y += ILH;

    InspectorRow(fg, x, y, "object", glm::vec4(vertices[vertex].position, 1.f), 4);   y += ILH;
    InspectorRow(fg, x, y, "world", v.world, 4);                                       y += ILH;
    InspectorRow(fg, x, y, "camera", v.camera, 4);                                     y += ILH;
    InspectorRow(fg, x, y, "clip", v.clip, 4);                                         y += ILH;
    InspectorRow(fg, x, y, perspective_divide ? "pixels" : "undivided", v.screen, 3);
}
