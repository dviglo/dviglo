// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../containers/ref_counted.h"
#include "../../containers/vector.h"
#include "../../graphics_api/graphics_defs.h"

namespace dviglo
{

class Graphics;
class ShaderVariation;
class VertexBuffer;

/// Vertex declaration.
class DV_API VertexDeclaration_D3D11 : public RefCounted
{
public:
    /// Construct with vertex buffers and element masks to base declaration on.
    VertexDeclaration_D3D11(Graphics* graphics, ShaderVariation* vertexShader, VertexBuffer** buffers);
    /// Destruct.
    virtual ~VertexDeclaration_D3D11() override;

    /// Return input layout object corresponding to the declaration.
    void* GetInputLayout() const { return inputLayout_; }

private:
    /// Input layout object.
    void* inputLayout_;
};

}
