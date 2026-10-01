#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "components/simple_scene.h"
#include "core/gpu/typed_buffer.h"
#include "components/rasterizer_types.h"

namespace gfxc
{
    // The base of the labs that rasterize triangles in a compute shader (labs 2, 3 and 4).
    //
    // It owns the color and depth images the shader draws into, the framebuffer that shows
    // them in the window, and the shader itself. A lab declares which shader it uses, creates
    // its meshes, and says how a frame is drawn: which uniforms are set and which meshes are
    // rasterized with them.
    class RasterizerScene : public gfxc::SimpleScene
    {
     public:
        RasterizerScene();

        void Init() override;

     protected:
        // The lab's compute shader (in assets/shaders/rasterizer), as a path relative to the
        // folder of the executable. Its `#include`s are resolved against that folder too, which
        // holds both assets/ and src/, so the shader can include the shared stages from
        // assets/ and the students' functions from each lab's gpu_programs/.
        virtual std::string GetShaderPath() const = 0;

        // Creates the lab's scene. The images and the shader exist by then.
        virtual void Initialize() = 0;

        // Draws a frame: sets the shader's uniforms and rasterizes meshes with them. The
        // images are cleared, and the shader is in use with the images bound.
        virtual void Draw(float deltaTimeSeconds) = 0;

        // Called after the images were recreated at the size of the window
        virtual void OnTargetResize() {}

        // A compute shader, stored in `shaders` under `name`, resolved like GetShaderPath
        Shader *CreateComputeShader(const std::string &name, const std::string &path);
        Shader *GetRasterizer() const;

        // A mesh is a vertex and an index buffer. They are allocated for `maxVertices` and
        // `maxTriangles` (at least the sizes given now), so UpdateMesh can grow the mesh.
        void CreateMesh(const std::string &name,
                        const std::vector<Vertex> &vertices,
                        const std::vector<Triangle> &triangles,
                        unsigned int maxVertices = 0,
                        unsigned int maxTriangles = 0);
        void UpdateMesh(const std::string &name,
                        const std::vector<Vertex> &vertices,
                        const std::vector<Triangle> &triangles);

        // Binds the mesh's buffers where the shaders read them (see InputAssembly.glsl)
        void BindMesh(const std::string &name);

        // Runs the rasterizer over every triangle of the mesh, one dispatch per triangle, so
        // the depth test sees them one after another
        void RasterizeMesh(const std::string &name);

     protected:
        // The size of the images. They follow the size of the window unless
        // `followWindowSize` is turned off, in which case they are stretched over it.
        glm::ivec2 targetSize { 0 };
        bool followWindowSize = true;

     private:
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;
        void OnWindowResize(int width, int height) override;

        void CreateTargets(glm::ivec2 size);

     private:
        static constexpr unsigned int WORKGROUP_SIZE = 8;

        struct MeshInfo {
            unsigned int triangleCount;
            unsigned int maxVertices;
            unsigned int maxTriangles;
        };
        std::unordered_map<std::string, MeshInfo> meshInfo;
    };
}   // namespace gfxc
