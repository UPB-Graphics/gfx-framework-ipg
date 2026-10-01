#pragma once

#include <string>
#include <vector>
#include <list>
#include <functional>

#include "utils/gl_utils.h"

#include <glm/glm.hpp>


#define MAX_2D_TEXTURES        (16)
#define INVALID_LOC            (-1)


class Shader
{
 public:
    Shader(const std::string &name);
    ~Shader();

    const char *GetName() const;
    GLuint GetProgramID() const;

    void Use() const;
    unsigned int Reload();

    void AddShader(const std::string &shaderFile, GLenum shaderType);
    void AddShaderCode(const std::string &shaderCode, GLenum shaderType);
    void ClearShaders();
    unsigned int CreateAndLink();

    // The directory `#include "file"` is resolved against, for every shader added with
    // AddShader. By default, it is the directory of the file that is being compiled.
    void SetIncludeDirectory(const std::string &directory);

    void BindTexturesUnits();
    GLint GetUniformLocation(const char * uniformName) const;

    void SetUniform(const char *uniformName, int value) const;
    void SetUniform(const char *uniformName, unsigned int value) const;
    void SetUniform(const char *uniformName, float value) const;
    void SetUniform(const char *uniformName, const glm::vec2 &value) const;
    void SetUniform(const char *uniformName, const glm::vec3 &value) const;
    void SetUniform(const char *uniformName, const glm::vec4 &value) const;
    void SetUniform(const char *uniformName, const glm::ivec2 &value) const;
    void SetUniform(const char *uniformName, const glm::mat3 &value) const;
    void SetUniform(const char *uniformName, const glm::mat4 &value) const;

    void OnLoad(std::function<void()> onLoad);

 private:
    void GetUniforms();
    static unsigned int CreateShader(const std::string &shaderFile, GLenum shaderType, const std::string &includeDirectory);
    static unsigned int CompileShader(const std::string shaderCode, GLenum shaderType);
    static unsigned int CreateProgram(const std::vector<unsigned int> &shaderObjects);

 public:
    GLuint program;

    // Textures
    GLint loc_textures[MAX_2D_TEXTURES];

    // MVP
    GLint loc_model_matrix;
    GLint loc_view_matrix;
    GLint loc_projection_matrix;

    // Shadow
    GLint loc_light_pos;
    GLint loc_light_color;
    GLint loc_light_radius;
    GLint loc_light_direction;

    // Camera
    GLint loc_eye_pos;
    GLint loc_eye_forward;
    GLint loc_z_far;
    GLint loc_z_near;

    // General
    GLint loc_resolution;
        
    // Text
    GLint text_color;

 private:
    struct ShaderFile
    {
        std::string file;
        GLenum type;
    };

    std::string shaderName;
    std::string includeDirectory;
    std::vector<ShaderFile> shaderFiles;
    std::vector<ShaderFile> shaderCodes;
    std::list<std::function<void()>> loadObservers;
};
