// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Класс SpriteBatch разбит на части для более легкого восприятия кода

#pragma once

#include "../core/object.h"
#include "graphics.h"
#include "../graphics_api/index_buffer.h"
#include "../graphics_api/shader_variation.h"
#include "../graphics_api/texture_2d.h"
#include "../graphics_api/vertex_buffer.h"

namespace dviglo
{

class DV_API SpriteBatchBase : public Object
{
    DV_OBJECT(SpriteBatchBase, Object);

    // ============================ Пакетный рендеринг треугольников ============================

private:

    // Максимальное число треугольников в порции
    inline static constexpr i32 max_triangles_in_portion_ = 600;

    // Число вершин в треугольнике
    inline static constexpr i32 vertices_per_triangle_ = 3;

    // Атрибуты вершин треугольников
    struct TVertex
    {
        Vector3 position;
        u32 color; // Цвет в формате 0xAABBGGRR
    };

    // Текущая порция треугольников
    TVertex t_vertices_[max_triangles_in_portion_ * vertices_per_triangle_];

    // Число вершин в массиве t_vertices_
    i32 t_num_vertices_ = 0;

    // Шейдеры для рендеринга треугольников. Инициализируются в конструкторе
    ShaderVariation* t_vertex_shader_;
    ShaderVariation* tPixelShader_;

    // Вершинный буфер для треугольников (индексный буфер не используется)
    SharedPtr<VertexBuffer> tVertexBuffer_;

protected:

    // Данные для функции AddTriangle().
    // Заполняем заранее выделенную память, вместо передачи кучи аргументов в функцию
    struct
    {
        TVertex v0, v1, v2;
    } triangle_;

    // Добавляет 3 вершины в массив t_vertices_. Вызывает flush(), если массив полон.
    // Перед вызовом этой функции необходимо заполнить структуру triangle_
    void AddTriangle();

public:

    /// Указывает цвет для следующих треугольников (в формате 0xAABBGGRR)
    void SetShapeColor(u32 color);

    /// Указывает цвет для следующих треугольников
    void SetShapeColor(const Color& color);

    // ============================ Пакетный рендеринг четырехугольников ============================

private:

    // Максимальное число четырёхугольников в порции
    inline static constexpr i32 max_quads_in_portion_ = 500;

    // Четырёхугольник состоит из двух треугольников, а значит у него 6 вершин.
    // То есть каждый четырёхугольник занимает 6 элементов в индексном буфере
    inline static constexpr i32 indices_per_quad_ = 6;

    // Две вершины четырёхугольника идентичны для обоих треугольников, поэтому
    // в вершинном буфере каждый четырёхугольник занимает 4 элемента
    inline static constexpr i32 vertices_per_quad_ = 4;

    // Атрибуты вершин четырёхугольников
    struct QVertex
    {
        Vector3 position;
        u32 color; // Цвет в формате 0xAABBGGRR
        Vector2 uv;
    };

    // Текущая порция четырёхугольников
    QVertex q_vertices_[max_quads_in_portion_ * vertices_per_quad_];

    // Число вершин в массиве q_vertices_
    i32 q_num_vertices_ = 0;

    // Текущая текстура
    Texture2D* q_current_texture_ = nullptr;

    // Текущие шейдеры
    ShaderVariation* q_current_vs_ = nullptr;
    ShaderVariation* q_current_ps_ = nullptr;

    // Буферы
    SharedPtr<IndexBuffer> q_index_buffer_;
    SharedPtr<VertexBuffer> q_vertex_buffer_;

protected:

    // Данные для функции AddQuad().
    // Заполняем заранее выделенную память, вместо передачи кучи аргументов в функцию
    struct
    {
        Texture2D* texture;
        ShaderVariation* vs;
        ShaderVariation* ps;
        QVertex v0, v1, v2, v3;
    } quad_;

    // Добавляет 4 вершины в массив quads_.
    // Если массив полон или требуемые шейдеры или текстура отличаются от текущих, то автоматически
    // происходит вызов функции flush() (то есть начинается новая порция).
    // Перед вызовом этой функции необходимо заполнить структуру quad_
    void AddQuad();

    // ============================ Общее ============================

private:

    void UpdateViewProjMatrix();
    IntRect GetViewportRect();

public:

    // Режим наложения
    BlendMode blend_mode_ = BLEND_ALPHA;

    // Если использовать CMP_LESSEQUAL, то будет учитываться содержимое буфера глубины
    // (но сам SpriteBatch ничего не пишет в буфер глубины).
    // При CMP_ALWAYS каждый выводимый спрайт будет перекрывать прежде отрисованные пиксели
    CompareMode compareMode_ = CMP_ALWAYS;

    // Если определена камера, то SpriteBatch рисует в мировых координатах
    Camera* camera_ = nullptr;

    // Размеры виртуального экрана. Если одна из координат <= 0, то используются реальные размеры экрана
    IntVector2 virtual_screen_size_ = IntVector2(0, 0);

    bool VirtualScreenUsed() const { return virtual_screen_size_.x > 0 && virtual_screen_size_.y > 0; }

    // Конструктор
    SpriteBatchBase();

    // Рендерит накопленную геометрию (то есть текущую порцию)
    void flush();

    // Переводит реальные координаты в виртуальные. Используется для курсора мыши
    Vector2 to_virtual_pos(const Vector2& real_pos);
};

} // namespace dviglo
