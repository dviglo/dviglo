// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "../../graphics/graphics.h"
#include "../graphics_impl.h"
#include "ogl_shader_program.h"
#include "../shader.h"
#include "../shader_variation.h"
#include "../../io/log.h"

#include "../../common/debug_new.h"

namespace dviglo
{

const char* ShaderVariation::elementSemanticNames_OGL[] =
{
    "POS",
    "NORMAL",
    "BINORMAL",
    "TANGENT",
    "TEXCOORD",
    "COLOR",
    "BLENDWEIGHT",
    "BLENDINDICES",
    "OBJECTINDEX"
};

void ShaderVariation::OnDeviceLost_OGL()
{
    if (gpu_object_name_ && !DV_GRAPHICS->IsDeviceLost())
        glDeleteShader(gpu_object_name_);

    GpuObject::OnDeviceLost();

    compilerOutput_.Clear();
}

void ShaderVariation::Release_OGL()
{
    if (gpu_object_name_)
    {
        if (GParams::is_headless())
            return;

        Graphics* graphics = DV_GRAPHICS;

        if (!graphics->IsDeviceLost())
        {
            if (type_ == VS)
            {
                if (graphics->GetVertexShader() == this)
                    graphics->SetShaders(nullptr, nullptr);
            }
            else
            {
                if (graphics->GetPixelShader() == this)
                    graphics->SetShaders(nullptr, nullptr);
            }

            glDeleteShader(gpu_object_name_);
        }

        gpu_object_name_ = 0;
        graphics->CleanupShaderPrograms_OGL(this);
    }

    compilerOutput_.Clear();
}

bool ShaderVariation::Create_OGL()
{
    Release_OGL();

    if (!owner_)
    {
        compilerOutput_ = "Owner shader has expired";
        return false;
    }

    gpu_object_name_ = glCreateShader(type_ == VS ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    if (!gpu_object_name_)
    {
        compilerOutput_ = "Could not create shader object";
        return false;
    }

    const String& originalShaderCode = owner_->GetSourceCode();

    // В начале файла должна находиться директива #version:
    // https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#Version
    String shaderCode = "#version 150\n";

    // После версии добавляем свои дефайны

#if defined(DESKTOP_GRAPHICS)
    shaderCode += "#define DESKTOP_GRAPHICS\n";
#elif defined(MOBILE_GRAPHICS)
    shaderCode += "#define MOBILE_GRAPHICS\n";
#endif

    // Distinguish between VS and FS compile in case the shader code wants to include/omit different things
    shaderCode += type_ == VS ? "#define COMPILEVS\n" : "#define COMPILEFS\n";

    // Add define for the maximum number of supported bones
    shaderCode += "#define MAXBONES " + String(Graphics::GetMaxBones()) + "\n";

    // Prepend the defines to the shader code
    Vector<String> defineVec = defines_.Split(' ');
    for (unsigned i = 0; i < defineVec.Size(); ++i)
    {
        // Add extra space for the checking code below
        String defineString = "#define " + defineVec[i].Replaced('=', ' ') + " \n";
        shaderCode += defineString;

        // In debug mode, check that all defines are referenced by the shader code
#ifdef _DEBUG
        String defineCheck = defineString.Substring(8, defineString.Find(' ', 8) - 8);
        if (originalShaderCode.Find(defineCheck) == String::NPOS)
            DV_LOGWARNING("Shader " + GetFullName() + " does not use the define " + defineCheck);
#endif
    }

    shaderCode += "#define GL3\n";

    shaderCode += originalShaderCode;

    const char* shaderCStr = shaderCode.c_str();
    glShaderSource(gpu_object_name_, 1, &shaderCStr, nullptr);
    glCompileShader(gpu_object_name_);

    GLint compiled;
    glGetShaderiv(gpu_object_name_, GL_COMPILE_STATUS, &compiled);

    // Компилятор может выдавать предупреждения, поэтому проверяем лог даже при успешной компиляции
    GLint log_buffer_size;
    glGetShaderiv(gpu_object_name_, GL_INFO_LOG_LENGTH, &log_buffer_size);

    if (!log_buffer_size)
    {
        compilerOutput_.Clear();
    }
    else
    {
        // glGetShaderiv возвращает длину лога вместе с конечным нулём
        compilerOutput_.Resize(log_buffer_size - 1);

        glGetShaderInfoLog(gpu_object_name_, log_buffer_size, nullptr, &compilerOutput_[0]);

        // Удаляем перевод строки в конце
        if (compilerOutput_.EndsWith("\n"))
            compilerOutput_.Resize(compilerOutput_.Length() - 1);
    }

    if (!compiled)
    {
        glDeleteShader(gpu_object_name_);
        gpu_object_name_ = 0;
    }

    return gpu_object_name_ != 0;
}

void ShaderVariation::SetDefines_OGL(const String& defines)
{
    defines_ = defines;
}

}
