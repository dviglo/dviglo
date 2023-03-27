// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "sprite_batch_base.h"

#include "graphics.h"
#include "camera.h"

namespace dviglo
{

void SpriteBatchBase::AddTriangle()
{
    // Рендерили четырёхугольники, а теперь нужно рендерить треугольники
    if (qNumVertices_ > 0)
        flush();

    memcpy(tVertices_ + tNumVertices_, &triangle_, sizeof(triangle_));
    tNumVertices_ += vertices_per_triangle_;

    // Если после добавления вершин мы заполнили массив до предела, то рендерим порцию
    if (tNumVertices_ == max_triangles_in_portion_ * vertices_per_triangle_)
        flush();
}

void SpriteBatchBase::SetShapeColor(u32 color)
{
    triangle_.v0.color = color;
    triangle_.v1.color = color;
    triangle_.v2.color = color;
}

void SpriteBatchBase::SetShapeColor(const Color& color)
{
    SetShapeColor(color.ToU32());
}

void SpriteBatchBase::AddQuad()
{
    // Рендерили треугольники, а теперь нужно рендерить четырехугольники
    if (tNumVertices_ > 0)
        flush();

    if (quad_.texture != qCurrentTexture_ || quad_.vs != qCurrentVS_ || quad_.ps != qCurrentPS_)
    {
        flush();

        qCurrentVS_ = quad_.vs;
        qCurrentPS_ = quad_.ps;
        qCurrentTexture_ = quad_.texture;
    }

    memcpy(qVertices_ + qNumVertices_, &(quad_.v0), sizeof(QVertex) * vertices_per_quad_);
    qNumVertices_ += vertices_per_quad_;

    // Если после добавления вершин мы заполнили массив до предела, то рендерим порцию
    if (qNumVertices_ == max_quads_in_portion_ * vertices_per_quad_)
        flush();
}

IntRect SpriteBatchBase::GetViewportRect()
{
    Graphics& graphics = DV_GRAPHICS;

    if (!VirtualScreenUsed())
        return IntRect(0, 0, graphics.GetWidth(), graphics.GetHeight());

    float realAspect = (float)graphics.GetWidth() / graphics.GetHeight();
    float virtualAspect = (float)virtualScreenSize_.x / virtualScreenSize_.y;

    float virtualScreenScale;
    if (realAspect > virtualAspect)
    {
        // Окно шире, чем надо. Будут пустые полосы по бокам
        virtualScreenScale = (float)graphics.GetHeight() / virtualScreenSize_.y;
    }
    else
    {
        // Высота окна больше, чем надо. Будут пустые полосы сверху и снизу
        virtualScreenScale = (float)graphics.GetWidth() / virtualScreenSize_.x;
    }

    i32 viewportWidth = (i32)(virtualScreenSize_.x * virtualScreenScale);
    i32 viewportHeight = (i32)(virtualScreenSize_.y * virtualScreenScale);

    // Центрируем вьюпорт
    i32 viewportX = (graphics.GetWidth() - viewportWidth) / 2;
    i32 viewportY = (graphics.GetHeight() - viewportHeight) / 2;

    return IntRect(viewportX, viewportY, viewportWidth + viewportX, viewportHeight + viewportY);
}

Vector2 SpriteBatchBase::to_virtual_pos(const Vector2& real_pos)
{
    if (!VirtualScreenUsed())
        return real_pos;

    IntRect viewport_rect = GetViewportRect();
    float factor = (float)virtualScreenSize_.x / viewport_rect.Width();

    float virtual_x = (real_pos.x - viewport_rect.left_) * factor;
    float virtual_y = (real_pos.y - viewport_rect.top_) * factor;

    return Vector2(virtual_x, virtual_y);
}

void SpriteBatchBase::UpdateViewProjMatrix()
{
    Graphics& graphics = DV_GRAPHICS;

    if (camera_)
    {
        Matrix4 matrix = camera_->GetGPUProjection() * camera_->GetView();
        graphics.SetShaderParameter(VSP_VIEWPROJ, matrix);
        return;
    }

    i32 width;
    i32 height;

    if (VirtualScreenUsed())
    {
        width = virtualScreenSize_.x;
        height = virtualScreenSize_.y;
    }
    else
    {
        width = graphics.GetWidth();
        height = graphics.GetHeight();
    }

    float pixelWidth = 2.0f / width; // Двойка, так как длина отрезка [-1, 1] равна двум
    float pixelHeight = 2.0f / height;

    Matrix4 matrix = Matrix4(pixelWidth,  0.0f,         0.0f, -1.0f,
                             0.0f,       -pixelHeight,  0.0f,  1.0f,
                             0.0f,        0.0f,         1.0f,  0.0f,
                             0.0f,        0.0f,         0.0f,  1.0f);

    graphics.SetShaderParameter(VSP_VIEWPROJ, matrix);
}

using GpuIndex16 = u16;

SpriteBatchBase::SpriteBatchBase()
{
    qIndexBuffer_ = new IndexBuffer();
    qIndexBuffer_->SetShadowed(true);

    // Индексный буфер всегда содержит набор четырёхугольников, поэтому его можно сразу заполнить
    qIndexBuffer_->SetSize(max_quads_in_portion_ * indices_per_quad_, false);
    GpuIndex16* buffer = (GpuIndex16*)qIndexBuffer_->Lock(0, qIndexBuffer_->GetIndexCount());
    for (i32 i = 0; i < max_quads_in_portion_; i++)
    {
        // Первый треугольник четырёхугольника
        buffer[i * indices_per_quad_ + 0] = i * vertices_per_quad_ + 0;
        buffer[i * indices_per_quad_ + 1] = i * vertices_per_quad_ + 1;
        buffer[i * indices_per_quad_ + 2] = i * vertices_per_quad_ + 2;

        // Второй треугольник
        buffer[i * indices_per_quad_ + 3] = i * vertices_per_quad_ + 2;
        buffer[i * indices_per_quad_ + 4] = i * vertices_per_quad_ + 3;
        buffer[i * indices_per_quad_ + 5] = i * vertices_per_quad_ + 0;
    }
    qIndexBuffer_->Unlock();

    qVertexBuffer_ = new VertexBuffer();
    qVertexBuffer_->SetSize(max_quads_in_portion_ * vertices_per_quad_, VertexElements::Position | VertexElements::Color | VertexElements::TexCoord1, true);

    Graphics& graphics = DV_GRAPHICS;

    tVertexBuffer_ = new VertexBuffer();
    tVertexBuffer_->SetSize(max_triangles_in_portion_ * vertices_per_triangle_, VertexElements::Position | VertexElements::Color, true);
    tVertexShader_ = graphics.GetShader(VS, "TriangleBatch");
    tPixelShader_ = graphics.GetShader(PS, "TriangleBatch");
    SetShapeColor(Color::WHITE);
}

void SpriteBatchBase::flush()
{
    if (tNumVertices_ > 0)
    {
        Graphics& graphics = DV_GRAPHICS;

        graphics.ResetRenderTargets();
        graphics.ClearParameterSources();
        graphics.SetCullMode(CULL_NONE);
        graphics.SetDepthWrite(false);
        graphics.SetStencilTest(false);
        graphics.SetScissorTest(false);
        graphics.SetColorWrite(true);
        graphics.SetDepthTest(compareMode_);
        graphics.SetBlendMode(blend_mode_);
        graphics.SetViewport(GetViewportRect());

        graphics.SetIndexBuffer(nullptr);
        graphics.SetVertexBuffer(tVertexBuffer_);
        graphics.SetTexture(0, nullptr);

        // Параметры шейдеров нужно задавать после указания шейдеров
        graphics.SetShaders(tVertexShader_, tPixelShader_);
        graphics.SetShaderParameter(VSP_MODEL, Matrix3x4::IDENTITY);
        UpdateViewProjMatrix();

        // Копируем накопленную геометрию в память видеокарты
        TVertex* buffer = (TVertex*)tVertexBuffer_->Lock(0, tNumVertices_, true);
        memcpy(buffer, tVertices_, tNumVertices_ * sizeof(TVertex));
        tVertexBuffer_->Unlock();

        // И отрисовываем её
        graphics.Draw(TRIANGLE_LIST, 0, tNumVertices_);

        // Начинаем новую порцию
        tNumVertices_ = 0;
    }

    else if (qNumVertices_ > 0)
    {
        Graphics& graphics = DV_GRAPHICS;

        graphics.ResetRenderTargets();
        graphics.ClearParameterSources();
        graphics.SetCullMode(CULL_NONE);
        graphics.SetDepthWrite(false);
        graphics.SetStencilTest(false);
        graphics.SetScissorTest(false);
        graphics.SetColorWrite(true);
        graphics.SetDepthTest(compareMode_);
        graphics.SetBlendMode(blend_mode_);
        graphics.SetViewport(GetViewportRect());

        graphics.SetIndexBuffer(qIndexBuffer_);
        graphics.SetVertexBuffer(qVertexBuffer_);
        graphics.SetTexture(0, qCurrentTexture_);

        // Параметры шейдеров нужно задавать после указания шейдеров
        graphics.SetShaders(qCurrentVS_, qCurrentPS_);
        graphics.SetShaderParameter(VSP_MODEL, Matrix3x4::IDENTITY);
        UpdateViewProjMatrix();
        // Мы используем только цвета вершин. Но это значение требует шейдер Basic
        graphics.SetShaderParameter(PSP_MATDIFFCOLOR, Color::WHITE);

        // Копируем накопленную геометрию в память видеокарты
        QVertex* buffer = (QVertex*)qVertexBuffer_->Lock(0, qNumVertices_, true);
        memcpy(buffer, qVertices_, qNumVertices_ * sizeof(QVertex));
        qVertexBuffer_->Unlock();

        // И отрисовываем её
        i32 numQuads = qNumVertices_ / vertices_per_quad_;
        graphics.Draw(TRIANGLE_LIST, 0, numQuads * indices_per_quad_, 0, qNumVertices_);

        // Начинаем новую порцию
        qNumVertices_ = 0;
    }
}

}
