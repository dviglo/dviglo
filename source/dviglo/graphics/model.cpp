// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "model.h"

#include "../common/utils.h"
#include "../core/context.h"
#include "../core/profiler.h"
#include "../graphics_api/index_buffer.h"
#include "../graphics_api/vertex_buffer.h"
#include "../io/file.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"
#include "geometry.h"
#include "graphics.h"

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

unsigned LookupVertexBuffer(VertexBuffer* buffer, const Vector<shared_ptr<VertexBuffer>>& buffers)
{
    for (unsigned i = 0; i < buffers.Size(); ++i)
    {
        if (buffers[i].get() == buffer)
            return i;
    }
    return 0;
}

unsigned LookupIndexBuffer(IndexBuffer* buffer, const Vector<shared_ptr<IndexBuffer>>& buffers)
{
    for (unsigned i = 0; i < buffers.Size(); ++i)
    {
        if (buffers[i].get() == buffer)
            return i;
    }

    return 0;
}

Model::Model()
{
}

Model::~Model() = default;

void Model::register_object()
{
    DV_CONTEXT.RegisterFactory<Model>();
}

bool Model::begin_load(Deserializer& source)
{
    // Check ID

    String fileID = source.ReadFileID();

    if (fileID != "UMDL" && fileID != "UMD2")
    {
        DV_LOGERROR(source.GetName() + " is not a valid model file");
        return false;
    }

    bool hasVertexDeclarations = (fileID == "UMD2");

    geometries_.Clear();
    geometryBoneMappings_.Clear();
    geometryCenters_.Clear();
    morphs_.Clear();
    vertexBuffers_.Clear();
    indexBuffers_.Clear();

    unsigned memoryUse = sizeof(Model);
    bool async = GetAsyncLoadState() == ASYNC_LOADING;

    // Read vertex buffers
    unsigned numVertexBuffers = source.ReadU32();
    vertexBuffers_.Reserve(numVertexBuffers);
    morphRangeStarts_.Resize(numVertexBuffers);
    morphRangeCounts_.Resize(numVertexBuffers);
    loadVBData_.Resize(numVertexBuffers);
    for (unsigned i = 0; i < numVertexBuffers; ++i)
    {
        VertexBufferDesc& desc = loadVBData_[i];

        desc.vertexCount_ = source.ReadU32();
        if (!hasVertexDeclarations)
        {
            VertexElements elementMask{source.ReadU32()};
            desc.vertexElements_ = VertexBuffer::GetElements(elementMask);
        }
        else
        {
            desc.vertexElements_.Clear();
            unsigned numElements = source.ReadU32();
            for (unsigned j = 0; j < numElements; ++j)
            {
                unsigned elementDesc = source.ReadU32();
                auto type = (VertexElementType)(elementDesc & 0xffu);
                auto semantic = (VertexElementSemantic)((elementDesc >> 8u) & 0xffu);
                auto index = (unsigned char)((elementDesc >> 16u) & 0xffu);
                desc.vertexElements_.Push(VertexElement(type, semantic, index));
            }
        }

        morphRangeStarts_[i] = source.ReadU32();
        morphRangeCounts_[i] = source.ReadU32();

        shared_ptr<VertexBuffer> buffer = make_shared<VertexBuffer>();
        unsigned vertexSize = VertexBuffer::GetVertexSize(desc.vertexElements_);
        desc.dataSize_ = desc.vertexCount_ * vertexSize;

        // Prepare vertex buffer data to be uploaded during end_load()
        if (async)
        {
            desc.data_ = make_unique<byte[]>(desc.dataSize_);
            source.Read(desc.data_.get(), desc.dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            desc.data_.reset(); // Make sure no previous data
            buffer->SetShadowed(true);
            buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
            void* dest = buffer->Lock(0, desc.vertexCount_);
            source.Read(dest, desc.vertexCount_ * vertexSize);
            buffer->Unlock();
        }

        memoryUse += sizeof(VertexBuffer) + desc.vertexCount_ * vertexSize;
        vertexBuffers_.Push(buffer);
    }

    // Read index buffers

    unsigned numIndexBuffers = source.ReadU32();
    indexBuffers_.Reserve(numIndexBuffers);
    loadIBData_.Resize(numIndexBuffers);

    for (unsigned i = 0; i < numIndexBuffers; ++i)
    {
        unsigned indexCount = source.ReadU32();
        unsigned indexSize = source.ReadU32();

        shared_ptr<IndexBuffer> buffer = make_shared<IndexBuffer>();

        // Prepare index buffer data to be uploaded during end_load()
        if (async)
        {
            loadIBData_[i].indexCount_ = indexCount;
            loadIBData_[i].indexSize_ = indexSize;
            loadIBData_[i].dataSize_ = indexCount * indexSize;
            loadIBData_[i].data_ = new byte[loadIBData_[i].dataSize_];
            source.Read(loadIBData_[i].data_.Get(), loadIBData_[i].dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            loadIBData_[i].data_.Reset(); // Make sure no previous data
            buffer->SetShadowed(true);
            buffer->SetSize(indexCount, indexSize > sizeof(unsigned short));
            void* dest = buffer->Lock(0, indexCount);
            source.Read(dest, indexCount * indexSize);
            buffer->Unlock();
        }

        memoryUse += sizeof(IndexBuffer) + indexCount * indexSize;
        indexBuffers_.Push(buffer);
    }

    // Read geometries
    unsigned numGeometries = source.ReadU32();
    geometries_.Reserve(numGeometries);
    geometryBoneMappings_.Reserve(numGeometries);
    geometryCenters_.Reserve(numGeometries);
    loadGeometries_.Resize(numGeometries);
    for (unsigned i = 0; i < numGeometries; ++i)
    {
        // Read bone mappings
        unsigned boneMappingCount = source.ReadU32();
        Vector<i32> boneMapping(boneMappingCount);
        for (unsigned j = 0; j < boneMappingCount; ++j)
            boneMapping[j] = source.ReadU32();
        geometryBoneMappings_.Push(boneMapping);

        unsigned numLodLevels = source.ReadU32();
        Vector<shared_ptr<Geometry>> geometryLodLevels;
        geometryLodLevels.Reserve(numLodLevels);
        loadGeometries_[i].Resize(numLodLevels);

        for (unsigned j = 0; j < numLodLevels; ++j)
        {
            float distance = source.ReadFloat();
            auto type = (PrimitiveType)source.ReadU32();

            unsigned vbRef = source.ReadU32();
            unsigned ibRef = source.ReadU32();
            unsigned indexStart = source.ReadU32();
            unsigned indexCount = source.ReadU32();

            if (vbRef >= vertexBuffers_.Size())
            {
                DV_LOGERROR("Vertex buffer index out of bounds");
                loadVBData_.Clear();
                loadIBData_.Clear();
                loadGeometries_.Clear();
                return false;
            }
            if (ibRef >= indexBuffers_.Size())
            {
                DV_LOGERROR("Index buffer index out of bounds");
                loadVBData_.Clear();
                loadIBData_.Clear();
                loadGeometries_.Clear();
                return false;
            }

            shared_ptr<Geometry> geometry = make_shared<Geometry>();
            geometry->SetLodDistance(distance);

            // Prepare geometry to be defined during end_load()
            loadGeometries_[i][j].type_ = type;
            loadGeometries_[i][j].vbRef_ = vbRef;
            loadGeometries_[i][j].ibRef_ = ibRef;
            loadGeometries_[i][j].indexStart_ = indexStart;
            loadGeometries_[i][j].indexCount_ = indexCount;

            geometryLodLevels.Push(geometry);
            memoryUse += sizeof(Geometry);
        }

        geometries_.Push(geometryLodLevels);
    }

    // Read morphs
    unsigned numMorphs = source.ReadU32();
    morphs_.Reserve(numMorphs);
    for (unsigned i = 0; i < numMorphs; ++i)
    {
        ModelMorph newMorph;

        newMorph.name_ = source.ReadString();
        newMorph.nameHash_ = newMorph.name_;
        newMorph.weight_ = 0.0f;
        unsigned numBuffers = source.ReadU32();

        for (unsigned j = 0; j < numBuffers; ++j)
        {
            VertexBufferMorph newBuffer;
            unsigned bufferIndex = source.ReadU32();

            newBuffer.elementMask_ = VertexElements{source.ReadU32()};
            newBuffer.vertexCount_ = source.ReadU32();

            // Base size: size of each vertex index
            unsigned vertexSize = sizeof(unsigned);
            // Add size of individual elements
            if (!!(newBuffer.elementMask_ & VertexElements::Position))
                vertexSize += sizeof(Vector3);
            if (!!(newBuffer.elementMask_ & VertexElements::Normal))
                vertexSize += sizeof(Vector3);
            if (!!(newBuffer.elementMask_ & VertexElements::Tangent))
                vertexSize += sizeof(Vector3);
            newBuffer.dataSize_ = newBuffer.vertexCount_ * vertexSize;
            newBuffer.morphData_ = make_shared<byte[]>(newBuffer.dataSize_);

            source.Read(newBuffer.morphData_.get(), newBuffer.vertexCount_ * vertexSize);

            newMorph.buffers_[bufferIndex] = newBuffer;
            memoryUse += sizeof(VertexBufferMorph) + newBuffer.vertexCount_ * vertexSize;
        }

        morphs_.Push(newMorph);
        memoryUse += sizeof(ModelMorph);
    }

    // Read skeleton
    skeleton_.Load(source);
    memoryUse += skeleton_.GetNumBones() * sizeof(Bone);

    // Read bounding box
    boundingBox_ = source.ReadBoundingBox();

    // Read geometry centers
    for (unsigned i = 0; i < geometries_.Size() && !source.IsEof(); ++i)
        geometryCenters_.Push(source.ReadVector3());
    while (geometryCenters_.Size() < geometries_.Size())
        geometryCenters_.Push(Vector3::ZERO);
    memoryUse += sizeof(Vector3) * geometries_.Size();

    // Read metadata
    String xmlName = replace_extension(GetName(), ".xml");
    SharedPtr<XmlFile> file(DV_RES_CACHE.GetTempResource<XmlFile>(xmlName, false));
    if (file)
        load_metadata_from_xml(file->GetRoot());

    SetMemoryUse(memoryUse);
    return true;
}

bool Model::end_load()
{
    // Upload vertex buffer data
    for (unsigned i = 0; i < vertexBuffers_.Size(); ++i)
    {
        VertexBuffer* buffer = vertexBuffers_[i].get();
        VertexBufferDesc& desc = loadVBData_[i];
        if (desc.data_)
        {
            buffer->SetShadowed(true);
            buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
            buffer->SetData(desc.data_.get());
        }
    }

    // Upload index buffer data
    for (unsigned i = 0; i < indexBuffers_.Size(); ++i)
    {
        IndexBuffer* buffer = indexBuffers_[i].get();
        IndexBufferDesc& desc = loadIBData_[i];
        if (desc.data_)
        {
            buffer->SetShadowed(true);
            buffer->SetSize(desc.indexCount_, desc.indexSize_ > sizeof(unsigned short));
            buffer->SetData(desc.data_.Get());
        }
    }

    // Set up geometries
    for (unsigned i = 0; i < geometries_.Size(); ++i)
    {
        for (unsigned j = 0; j < geometries_[i].Size(); ++j)
        {
            Geometry* geometry = geometries_[i][j].get();
            GeometryDesc& desc = loadGeometries_[i][j];
            geometry->SetVertexBuffer(0, vertexBuffers_[desc.vbRef_]);
            geometry->SetIndexBuffer(indexBuffers_[desc.ibRef_]);
            geometry->SetDrawRange(desc.type_, desc.indexStart_, desc.indexCount_);
        }
    }

    loadVBData_.Clear();
    loadIBData_.Clear();
    loadGeometries_.Clear();
    return true;
}

bool Model::Save(Serializer& dest) const
{
    // Write ID
    if (!dest.WriteFileID("UMD2"))
        return false;

    // Write vertex buffers
    dest.WriteU32(vertexBuffers_.Size());
    for (unsigned i = 0; i < vertexBuffers_.Size(); ++i)
    {
        VertexBuffer* buffer = vertexBuffers_[i].get();
        dest.WriteU32(buffer->GetVertexCount());
        const Vector<VertexElement>& elements = buffer->GetElements();
        dest.WriteU32(elements.Size());
        for (unsigned j = 0; j < elements.Size(); ++j)
        {
            unsigned elementDesc = ((unsigned)elements[j].type_) |
                (((unsigned)elements[j].semantic_) << 8u) |
                (((unsigned)elements[j].index_) << 16u);
            dest.WriteU32(elementDesc);
        }
        dest.WriteU32(morphRangeStarts_[i]);
        dest.WriteU32(morphRangeCounts_[i]);
        dest.Write(buffer->GetShadowData(), buffer->GetVertexCount() * buffer->GetVertexSize());
    }
    // Write index buffers
    dest.WriteU32(indexBuffers_.Size());
    for (unsigned i = 0; i < indexBuffers_.Size(); ++i)
    {
        IndexBuffer* buffer = indexBuffers_[i].get();
        dest.WriteU32(buffer->GetIndexCount());
        dest.WriteU32(buffer->GetIndexSize());
        dest.Write(buffer->GetShadowData(), buffer->GetIndexCount() * buffer->GetIndexSize());
    }
    // Write geometries
    dest.WriteU32(geometries_.Size());
    for (unsigned i = 0; i < geometries_.Size(); ++i)
    {
        // Write bone mappings
        dest.WriteU32(geometryBoneMappings_[i].Size());
        for (unsigned j = 0; j < geometryBoneMappings_[i].Size(); ++j)
            dest.WriteU32(geometryBoneMappings_[i][j]);

        // Write the LOD levels
        dest.WriteU32(geometries_[i].Size());
        for (unsigned j = 0; j < geometries_[i].Size(); ++j)
        {
            Geometry* geometry = geometries_[i][j].get();
            dest.WriteFloat(geometry->GetLodDistance());
            dest.WriteU32(geometry->GetPrimitiveType());
            dest.WriteU32(LookupVertexBuffer(geometry->GetVertexBuffer(0).get(), vertexBuffers_));
            dest.WriteU32(LookupIndexBuffer(geometry->GetIndexBuffer().get(), indexBuffers_));
            dest.WriteU32(geometry->GetIndexStart());
            dest.WriteU32(geometry->GetIndexCount());
        }
    }

    // Write morphs
    dest.WriteU32(morphs_.Size());
    for (unsigned i = 0; i < morphs_.Size(); ++i)
    {
        dest.WriteString(morphs_[i].name_);
        dest.WriteU32(morphs_[i].buffers_.Size());

        // Write morph vertex buffers
        for (HashMap<i32, VertexBufferMorph>::ConstIterator j = morphs_[i].buffers_.Begin();
             j != morphs_[i].buffers_.End(); ++j)
        {
            dest.WriteU32(j->first_);
            dest.WriteU32(to_u32(j->second_.elementMask_));
            dest.WriteU32(j->second_.vertexCount_);

            // Base size: size of each vertex index
            unsigned vertexSize = sizeof(unsigned);
            // Add size of individual elements
            if (!!(j->second_.elementMask_ & VertexElements::Position))
                vertexSize += sizeof(Vector3);
            if (!!(j->second_.elementMask_ & VertexElements::Normal))
                vertexSize += sizeof(Vector3);
            if (!!(j->second_.elementMask_ & VertexElements::Tangent))
                vertexSize += sizeof(Vector3);

            dest.Write(j->second_.morphData_.get(), vertexSize * j->second_.vertexCount_);
        }
    }

    // Write skeleton
    skeleton_.Save(dest);

    // Write bounding box
    dest.WriteBoundingBox(boundingBox_);

    // Write geometry centers
    for (unsigned i = 0; i < geometryCenters_.Size(); ++i)
        dest.WriteVector3(geometryCenters_[i]);

    // Write metadata
    if (HasMetadata())
    {
        auto* destFile = dynamic_cast<File*>(&dest);
        if (destFile)
        {
            String xmlName = replace_extension(destFile->GetName(), ".xml");

            SharedPtr<XmlFile> xml(new XmlFile());
            XmlElement rootElem = xml->CreateRoot("model");
            save_metadata_to_xml(rootElem);

            File xmlFile(xmlName, FILE_WRITE);
            xml->Save(xmlFile);
        }
        else
            DV_LOGWARNING("Can not save model metadata when not saving into a file");
    }

    return true;
}

void Model::SetBoundingBox(const BoundingBox& box)
{
    boundingBox_ = box;
}

bool Model::SetVertexBuffers(const Vector<shared_ptr<VertexBuffer>>& buffers, const Vector<i32>& morphRangeStarts,
    const Vector<i32>& morphRangeCounts)
{
    for (unsigned i = 0; i < buffers.Size(); ++i)
    {
        if (!buffers[i])
        {
            DV_LOGERROR("Null model vertex buffers specified");
            return false;
        }
        if (!buffers[i]->IsShadowed())
        {
            DV_LOGERROR("Model vertex buffers must be shadowed");
            return false;
        }
    }

    vertexBuffers_ = buffers;
    morphRangeStarts_.Resize(buffers.Size());
    morphRangeCounts_.Resize(buffers.Size());

    // If morph ranges are not specified for buffers, assume to be zero
    for (unsigned i = 0; i < buffers.Size(); ++i)
    {
        morphRangeStarts_[i] = i < morphRangeStarts.Size() ? morphRangeStarts[i] : 0;
        morphRangeCounts_[i] = i < morphRangeCounts.Size() ? morphRangeCounts[i] : 0;
    }

    return true;
}

bool Model::SetIndexBuffers(const Vector<shared_ptr<IndexBuffer>>& buffers)
{
    for (unsigned i = 0; i < buffers.Size(); ++i)
    {
        if (!buffers[i])
        {
            DV_LOGERROR("Null model index buffers specified");
            return false;
        }
        if (!buffers[i]->IsShadowed())
        {
            DV_LOGERROR("Model index buffers must be shadowed");
            return false;
        }
    }

    indexBuffers_ = buffers;
    return true;
}

void Model::SetNumGeometries(i32 num)
{
    assert(num >= 0);

    geometries_.Resize(num);
    geometryBoneMappings_.Resize(num);
    geometryCenters_.Resize(num);

    // For easier creation of from-scratch geometry, ensure that all geometries start with at least 1 LOD level (0 makes no sense)
    for (unsigned i = 0; i < geometries_.Size(); ++i)
    {
        if (geometries_[i].Empty())
            geometries_[i].Resize(1);
    }
}

bool Model::SetNumGeometryLodLevels(i32 index, i32 num)
{
    assert(index >= 0);
    assert(num >= 0);

    if (index >= geometries_.Size())
    {
        DV_LOGERROR("Geometry index out of bounds");
        return false;
    }
    if (!num)
    {
        DV_LOGERROR("Zero LOD levels not allowed");
        return false;
    }

    geometries_[index].Resize(num);
    return true;
}

bool Model::SetGeometry(i32 index, i32 lodLevel, shared_ptr<Geometry>& geometry)
{
    assert(index >= 0);
    assert(lodLevel >= 0);

    if (index >= geometries_.Size())
    {
        DV_LOGERROR("Geometry index out of bounds");
        return false;
    }
    if (lodLevel >= geometries_[index].Size())
    {
        DV_LOGERROR("LOD level index out of bounds");
        return false;
    }

    geometries_[index][lodLevel] = geometry;
    return true;
}

bool Model::SetGeometryCenter(i32 index, const Vector3& center)
{
    assert(index >= 0);

    if (index >= geometryCenters_.Size())
    {
        DV_LOGERROR("Geometry index out of bounds");
        return false;
    }

    geometryCenters_[index] = center;
    return true;
}

void Model::SetSkeleton(const Skeleton& skeleton)
{
    skeleton_ = skeleton;
}

void Model::SetGeometryBoneMappings(const Vector<Vector<i32>>& geometryBoneMappings)
{
    geometryBoneMappings_ = geometryBoneMappings;
}

void Model::SetMorphs(const Vector<ModelMorph>& morphs)
{
    morphs_ = morphs;
}

SharedPtr<Model> Model::Clone(const String& cloneName) const
{
    SharedPtr<Model> ret(new Model());

    ret->SetName(cloneName);
    ret->boundingBox_ = boundingBox_;
    ret->skeleton_ = skeleton_;
    ret->geometryBoneMappings_ = geometryBoneMappings_;
    ret->geometryCenters_ = geometryCenters_;
    ret->morphs_ = morphs_;
    ret->morphRangeStarts_ = morphRangeStarts_;
    ret->morphRangeCounts_ = morphRangeCounts_;

    // Deep copy vertex/index buffers
    HashMap<VertexBuffer*, shared_ptr<VertexBuffer>> vbMapping;
    for (Vector<shared_ptr<VertexBuffer>>::ConstIterator i = vertexBuffers_.Begin(); i != vertexBuffers_.End(); ++i)
    {
        VertexBuffer* origBuffer = i->get();
        shared_ptr<VertexBuffer> cloneBuffer;

        if (origBuffer)
        {
            cloneBuffer = make_shared<VertexBuffer>();
            cloneBuffer->SetSize(origBuffer->GetVertexCount(), origBuffer->GetElementMask(), origBuffer->IsDynamic());
            cloneBuffer->SetShadowed(origBuffer->IsShadowed());
            if (origBuffer->IsShadowed())
                cloneBuffer->SetData(origBuffer->GetShadowData());
            else
            {
                void* origData = origBuffer->Lock(0, origBuffer->GetVertexCount());
                if (origData)
                    cloneBuffer->SetData(origData);
                else
                    DV_LOGERROR("Failed to lock original vertex buffer for copying");
            }
            vbMapping[origBuffer] = cloneBuffer;
        }

        ret->vertexBuffers_.Push(cloneBuffer);
    }

    HashMap<IndexBuffer*, shared_ptr<IndexBuffer>> ibMapping;
    for (Vector<shared_ptr<IndexBuffer>>::ConstIterator i = indexBuffers_.Begin(); i != indexBuffers_.End(); ++i)
    {
        IndexBuffer* origBuffer = i->get();
        shared_ptr<IndexBuffer> cloneBuffer;

        if (origBuffer)
        {
            cloneBuffer = make_shared<IndexBuffer>();
            cloneBuffer->SetSize(origBuffer->GetIndexCount(), origBuffer->GetIndexSize() == sizeof(unsigned),
                origBuffer->IsDynamic());
            cloneBuffer->SetShadowed(origBuffer->IsShadowed());
            if (origBuffer->IsShadowed())
                cloneBuffer->SetData(origBuffer->GetShadowData());
            else
            {
                void* origData = origBuffer->Lock(0, origBuffer->GetIndexCount());
                if (origData)
                    cloneBuffer->SetData(origData);
                else
                    DV_LOGERROR("Failed to lock original index buffer for copying");
            }
            ibMapping[origBuffer] = cloneBuffer;
        }

        ret->indexBuffers_.Push(cloneBuffer);
    }

    // Deep copy all the geometry LOD levels and refer to the copied vertex/index buffers
    ret->geometries_.Resize(geometries_.Size());
    for (unsigned i = 0; i < geometries_.Size(); ++i)
    {
        ret->geometries_[i].Resize(geometries_[i].Size());
        for (unsigned j = 0; j < geometries_[i].Size(); ++j)
        {
            shared_ptr<Geometry> cloneGeometry;
            Geometry* origGeometry = geometries_[i][j].get();

            if (origGeometry)
            {
                cloneGeometry = make_shared<Geometry>();
                cloneGeometry->SetIndexBuffer(ibMapping[origGeometry->GetIndexBuffer().get()]);
                unsigned numVbs = origGeometry->GetNumVertexBuffers();
                for (unsigned k = 0; k < numVbs; ++k)
                {
                    cloneGeometry->SetVertexBuffer(k, vbMapping[origGeometry->GetVertexBuffer(k).get()]);
                }
                cloneGeometry->SetDrawRange(origGeometry->GetPrimitiveType(), origGeometry->GetIndexStart(),
                    origGeometry->GetIndexCount(), origGeometry->GetVertexStart(), origGeometry->GetVertexCount(), false);
                cloneGeometry->SetLodDistance(origGeometry->GetLodDistance());
            }

            ret->geometries_[i][j] = cloneGeometry;
        }
    }

    // Deep copy the morph data (if any) to allow modifying it
    for (Vector<ModelMorph>::Iterator i = ret->morphs_.Begin(); i != ret->morphs_.End(); ++i)
    {
        ModelMorph& morph = *i;
        for (HashMap<i32, VertexBufferMorph>::Iterator j = morph.buffers_.Begin(); j != morph.buffers_.End(); ++j)
        {
            VertexBufferMorph& vbMorph = j->second_;
            if (vbMorph.dataSize_)
            {
                shared_ptr<byte[]> cloneData = make_shared<byte[]>(vbMorph.dataSize_);
                memcpy(cloneData.get(), vbMorph.morphData_.get(), vbMorph.dataSize_);
                vbMorph.morphData_ = cloneData;
            }
        }
    }

    ret->SetMemoryUse(GetMemoryUse());

    return ret;
}

i32 Model::GetNumGeometryLodLevels(i32 index) const
{
    assert(index >= 0);
    return index < geometries_.Size() ? geometries_[index].Size() : 0;
}

Geometry* Model::GetGeometry(i32 index, i32 lodLevel) const
{
    assert(index >= 0);
    assert(lodLevel >= 0);

    if (index >= geometries_.Size() || geometries_[index].Empty())
        return nullptr;

    if (lodLevel >= geometries_[index].Size())
        lodLevel = geometries_[index].Size() - 1;

    return geometries_[index][lodLevel].get(); // Возвращать shared_ptr<Geometry>& ?
}

const ModelMorph* Model::GetMorph(i32 index) const
{
    assert(index >= 0);
    return index < morphs_.Size() ? &morphs_[index] : nullptr;
}

const ModelMorph* Model::GetMorph(const String& name) const
{
    return GetMorph(StringHash(name));
}

const ModelMorph* Model::GetMorph(StringHash nameHash) const
{
    for (Vector<ModelMorph>::ConstIterator i = morphs_.Begin(); i != morphs_.End(); ++i)
    {
        if (i->nameHash_ == nameHash)
            return &(*i);
    }

    return nullptr;
}

i32 Model::GetMorphRangeStart(i32 bufferIndex) const
{
    assert(bufferIndex >= 0);
    return bufferIndex < vertexBuffers_.Size() ? morphRangeStarts_[bufferIndex] : 0;
}

i32 Model::GetMorphRangeCount(i32 bufferIndex) const
{
    assert(bufferIndex >= 0);
    return bufferIndex < vertexBuffers_.Size() ? morphRangeCounts_[bufferIndex] : 0;
}

} // namespace dviglo
