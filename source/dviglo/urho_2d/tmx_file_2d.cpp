// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/graphics.h"
#include "../graphics_api/texture_2d.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../io/path.h"
#include "../math/area_allocator.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"
#include "sprite_2d.h"
#include "tmx_file_2d.h"

#include "../common/debug_new.h"


namespace dviglo
{

TmxLayer2D::TmxLayer2D(TmxFile2D* tmxFile, TileMapLayerType2D type) :
    tmxFile_(tmxFile),
    type_(type)
{
}

TmxFile2D* TmxLayer2D::GetTmxFile() const
{
    return tmxFile_;
}

bool TmxLayer2D::HasProperty(const String& name) const
{
    if (!propertySet_)
        return false;
    return propertySet_->HasProperty(name);
}

const String& TmxLayer2D::GetProperty(const String& name) const
{
    if (!propertySet_)
        return String::EMPTY;
    return propertySet_->GetProperty(name);
}

void TmxLayer2D::LoadInfo(const XmlElement& element)
{
    name_ = element.GetAttribute("name");
    width_ = element.GetI32("width");
    height_ = element.GetI32("height");
    if (element.HasAttribute("visible"))
        visible_ = element.GetI32("visible") != 0;
    else
        visible_ = true;
}

void TmxLayer2D::LoadPropertySet(const XmlElement& element)
{
    propertySet_ = new PropertySet2D();
    propertySet_->Load(element);
}

TmxTileLayer2D::TmxTileLayer2D(TmxFile2D* tmxFile) :
    TmxLayer2D(tmxFile, LT_TILE_LAYER)
{
}

enum LayerEncoding {
    XML,
    CSV,
    Base64,
};

bool TmxTileLayer2D::Load(const XmlElement& element, const TileMapInfo2D& info)
{
    LoadInfo(element);

    XmlElement dataElem = element.GetChild("data");
    if (!dataElem)
    {
        DV_LOGERROR("Could not find data in layer");
        return false;
    }

    LayerEncoding encoding;
    if (dataElem.HasAttribute("compression"))
    {
        DV_LOGERROR("Compression not supported now");
        return false;
    }

    if (dataElem.HasAttribute("encoding"))
    {
        String encodingAttribute = dataElem.GetAttribute("encoding");
        if (encodingAttribute == "xml")
            encoding = XML;
        else if (encodingAttribute == "csv")
            encoding = CSV;
        else if (encodingAttribute == "base64")
            encoding = Base64;
        else
        {
            DV_LOGERROR("Invalid encoding: " + encodingAttribute);
            return false;
        }
    }
    else
        encoding = XML;

    tiles_.Resize((unsigned)(width_ * height_));
    if (encoding == XML)
    {
        XmlElement tileElem = dataElem.GetChild("tile");

        for (int y = 0; y < height_; ++y)
        {
            for (int x = 0; x < width_; ++x)
            {
                if (!tileElem)
                    return false;

                unsigned gid = tileElem.GetU32("gid");
                if (gid > 0)
                {
                    SharedPtr<Tile2D> tile(new Tile2D());
                    tile->gid_ = gid;
                    tile->sprite_ = tmxFile_->GetTileSprite(gid & ~FLIP_ALL);
                    tile->propertySet_ = tmxFile_->GetTilePropertySet(gid & ~FLIP_ALL);
                    tiles_[y * width_ + x] = tile;
                }

                tileElem = tileElem.GetNext("tile");
            }
        }
    }
    else if (encoding == CSV)
    {
        String dataValue = dataElem.GetValue();
        Vector<String> gidVector = dataValue.Split(',');
        int currentIndex = 0;
        for (int y = 0; y < height_; ++y)
        {
            for (int x = 0; x < width_; ++x)
            {
                gidVector[currentIndex].Replace("\n", "");
                unsigned gid = ToU32(gidVector[currentIndex]);
                if (gid > 0)
                {
                    SharedPtr<Tile2D> tile(new Tile2D());
                    tile->gid_ = gid;
                    tile->sprite_ = tmxFile_->GetTileSprite(gid & ~FLIP_ALL);
                    tile->propertySet_ = tmxFile_->GetTilePropertySet(gid & ~FLIP_ALL);
                    tiles_[y * width_ + x] = tile;
                }
                ++currentIndex;
            }
        }
    }
    else if (encoding == Base64)
    {
        String dataValue = dataElem.GetValue();
        int startPosition = 0;
        while (!IsAlpha(dataValue[startPosition]) && !IsDigit(dataValue[startPosition])
              && dataValue[startPosition] != '+' && dataValue[startPosition] != '/') ++startPosition;
        dataValue = dataValue.Substring(startPosition);
        Vector<unsigned char> buffer = DecodeBase64(dataValue);
        int currentIndex = 0;
        for (int y = 0; y < height_; ++y)
        {
            for (int x = 0; x < width_; ++x)
            {
                // buffer contains 32-bit integers in little-endian format
                unsigned gid = ((unsigned)buffer[currentIndex+3] << 24u)
                             | ((unsigned)buffer[currentIndex+2] << 16u)
                             | ((unsigned)buffer[currentIndex+1] << 8u)
                             | (unsigned)buffer[currentIndex];
                if (gid > 0)
                {
                    SharedPtr<Tile2D> tile(new Tile2D());
                    tile->gid_ = gid;
                    tile->sprite_ = tmxFile_->GetTileSprite(gid & ~FLIP_ALL);
                    tile->propertySet_ = tmxFile_->GetTilePropertySet(gid & ~FLIP_ALL);
                    tiles_[y * width_ + x] = tile;
                }
                currentIndex += 4;
            }
        }
    }

    if (element.HasChild("properties"))
        LoadPropertySet(element.GetChild("properties"));

    return true;
}

Tile2D* TmxTileLayer2D::GetTile(int x, int y) const
{
    if (x < 0 || x >= width_ || y < 0 || y >= height_)
        return nullptr;

    return tiles_[y * width_ + x];
}

TmxObjectGroup2D::TmxObjectGroup2D(TmxFile2D* tmxFile) :
    TmxLayer2D(tmxFile, LT_OBJECT_GROUP)
{
}

bool TmxObjectGroup2D::Load(const XmlElement& element, const TileMapInfo2D& info)
{
    LoadInfo(element);

    for (XmlElement objectElem = element.GetChild("object"); objectElem; objectElem = objectElem.GetNext("object"))
    {
        SharedPtr<TileMapObject2D> object(new TileMapObject2D());
        StoreObject(objectElem, object, info);
        objects_.Push(object);
    }

    if (element.HasChild("properties"))
        LoadPropertySet(element.GetChild("properties"));

    return true;
}

void TmxObjectGroup2D::StoreObject(const XmlElement& objectElem, const SharedPtr<TileMapObject2D>& object, const TileMapInfo2D& info, bool isTile)
{
        if (objectElem.HasAttribute("name"))
            object->name_ = objectElem.GetAttribute("name");
        if (objectElem.HasAttribute("type"))
            object->type_ = objectElem.GetAttribute("type");

        if (objectElem.HasAttribute("gid"))
            object->objectType_ = OT_TILE;
        else if (objectElem.HasChild("polygon"))
            object->objectType_ = OT_POLYGON;
        else if (objectElem.HasChild("polyline"))
            object->objectType_ = OT_POLYLINE;
        else if (objectElem.HasChild("ellipse"))
            object->objectType_ = OT_ELLIPSE;
        else
            object->objectType_ = OT_RECTANGLE;

        const Vector2 position(objectElem.GetFloat("x"), objectElem.GetFloat("y"));
        const Vector2 size(objectElem.GetFloat("width"), objectElem.GetFloat("height"));

        switch (object->objectType_)
        {
        case OT_RECTANGLE:
        case OT_ELLIPSE:
            object->position_ = info.ConvertPosition(Vector2(position.x_, position.y_ + size.y_));
            object->size_ = Vector2(size.x_ * PIXEL_SIZE, size.y_ * PIXEL_SIZE);
            break;

        case OT_TILE:
            object->position_ = info.ConvertPosition(position);
            object->gid_ = objectElem.GetU32("gid");
            object->sprite_ = tmxFile_->GetTileSprite(object->gid_ & ~FLIP_ALL);

            if (objectElem.HasAttribute("width") || objectElem.HasAttribute("height"))
            {
                object->size_ = Vector2(size.x_ * PIXEL_SIZE, size.y_ * PIXEL_SIZE);
            }
            else if (object->sprite_)
            {
                IntVector2 spriteSize = object->sprite_->GetRectangle().Size();
                object->size_ = Vector2(spriteSize.x_, spriteSize.y_);
            }
            break;

        case OT_POLYGON:
        case OT_POLYLINE:
            {
                Vector<String> points;

                const char* name = object->objectType_ == OT_POLYGON ? "polygon" : "polyline";
                XmlElement polygonElem = objectElem.GetChild(name);
                points = polygonElem.GetAttribute("points").Split(' ');

                if (points.Size() <= 1)
                    return;

                object->points_.Resize(points.Size());

                for (i32 i = 0; i < points.Size(); ++i)
                {
                    points[i].Replace(',', ' ');
                    Vector2 point = position + ToVector2(points[i]);
                    object->points_[i] = info.ConvertPosition(point);
                }
            }
            break;

        default: break;
        }

        if (objectElem.HasChild("properties"))
        {
            object->propertySet_ = new PropertySet2D();
            object->propertySet_->Load(objectElem.GetChild("properties"));
        }
}

TileMapObject2D* TmxObjectGroup2D::GetObject(unsigned index) const
{
    if (index >= objects_.Size())
        return nullptr;
    return objects_[index];
}


TmxImageLayer2D::TmxImageLayer2D(TmxFile2D* tmxFile) :
    TmxLayer2D(tmxFile, LT_IMAGE_LAYER)
{
}

bool TmxImageLayer2D::Load(const XmlElement& element, const TileMapInfo2D& info)
{
    LoadInfo(element);

    XmlElement imageElem = element.GetChild("image");
    if (!imageElem)
        return false;

    position_ = Vector2(0.0f, info.GetMapHeight());
    source_ = imageElem.GetAttribute("source");
    String textureFilePath = get_parent(tmxFile_->GetName()) + source_;
    SharedPtr<Texture2D> texture(DV_RES_CACHE.GetResource<Texture2D>(textureFilePath));
    if (!texture)
    {
        DV_LOGERROR("Could not load texture " + textureFilePath);
        return false;
    }

    sprite_ = new Sprite2D();
    sprite_->SetTexture(texture);
    sprite_->SetRectangle(IntRect(0, 0, texture->GetWidth(), texture->GetHeight()));
    // Set image hot spot at left top
    sprite_->SetHotSpot(Vector2(0.0f, 1.0f));

    if (element.HasChild("properties"))
        LoadPropertySet(element.GetChild("properties"));

    return true;
}

Sprite2D* TmxImageLayer2D::GetSprite() const
{
    return sprite_;
}

TmxFile2D::TmxFile2D() :
    edge_offset_(0.f)
{
}

TmxFile2D::~TmxFile2D()
{
    for (const TmxLayer2D* layer : layers_)
        delete layer;
}

void TmxFile2D::register_object()
{
    DV_CONTEXT.RegisterFactory<TmxFile2D>();
}

bool TmxFile2D::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    loadXMLFile_ = new XmlFile();
    if (!loadXMLFile_->Load(source))
    {
        DV_LOGERROR("Load XML failed " + source.GetName());
        loadXMLFile_.Reset();
        return false;
    }

    XmlElement rootElem = loadXMLFile_->GetRoot("map");
    if (!rootElem)
    {
        DV_LOGERROR("Invalid tmx file " + source.GetName());
        loadXMLFile_.Reset();
        return false;
    }

    // If we're async loading, request the texture now. Finish during EndLoad().
    if (GetAsyncLoadState() == ASYNC_LOADING)
    {
        for (XmlElement tileSetElem = rootElem.GetChild("tileset"); tileSetElem; tileSetElem = tileSetElem.GetNext("tileset"))
        {
            // Tile set defined in TSX file
            if (tileSetElem.HasAttribute("source"))
            {
                String source = tileSetElem.GetAttribute("source");
                SharedPtr<XmlFile> tsxXMLFile = LoadTSXFile(source);
                if (!tsxXMLFile)
                    return false;

                tsxXMLFiles_[source] = tsxXMLFile;

                String textureFilePath =
                    get_parent(GetName()) + tsxXMLFile->GetRoot("tileset").GetChild("image").GetAttribute("source");
                DV_RES_CACHE.BackgroundLoadResource<Texture2D>(textureFilePath, true, this);
            }
            else
            {
                String textureFilePath = get_parent(GetName()) + tileSetElem.GetChild("image").GetAttribute("source");
                DV_RES_CACHE.BackgroundLoadResource<Texture2D>(textureFilePath, true, this);
            }
        }

        for (XmlElement imageLayerElem = rootElem.GetChild("imagelayer"); imageLayerElem;
             imageLayerElem = imageLayerElem.GetNext("imagelayer"))
        {
            String textureFilePath = get_parent(GetName()) + imageLayerElem.GetChild("image").GetAttribute("source");
            DV_RES_CACHE.BackgroundLoadResource<Texture2D>(textureFilePath, true, this);
        }
    }

    return true;
}

bool TmxFile2D::EndLoad()
{
    if (!loadXMLFile_)
        return false;

    XmlElement rootElem = loadXMLFile_->GetRoot("map");
    String version = rootElem.GetAttribute("version");
    if (!version.StartsWith("1."))
    {
        DV_LOGERRORF("Invalid TMX version: %s", version.c_str());
        return false;
    }

    String orientation = rootElem.GetAttribute("orientation");
    if (orientation == "orthogonal")
        info_.orientation_ = O_ORTHOGONAL;
    else if (orientation == "isometric")
        info_.orientation_ = O_ISOMETRIC;
    else if (orientation == "staggered")
        info_.orientation_ = O_STAGGERED;
    else if (orientation == "hexagonal")
        info_.orientation_ = O_HEXAGONAL;
    else
    {
        DV_LOGERROR("Unsupported orientation type " + orientation);
        return false;
    }

    info_.width_ = rootElem.GetI32("width");
    info_.height_ = rootElem.GetI32("height");
    info_.tileWidth_ = rootElem.GetFloat("tilewidth") * PIXEL_SIZE;
    info_.tileHeight_ = rootElem.GetFloat("tileheight") * PIXEL_SIZE;

    for (const TmxLayer2D* layer : layers_)
        delete layer;
    layers_.Clear();

    for (XmlElement childElement = rootElem.GetChild(); childElement; childElement = childElement.GetNext())
    {
        bool ret = true;
        String name = childElement.GetName();
        if (name == "tileset")
            ret = LoadTileSet(childElement);
        else if (name == "layer")
        {
            auto* tileLayer = new TmxTileLayer2D(this);
            ret = tileLayer->Load(childElement, info_);

            layers_.Push(tileLayer);
        }
        else if (name == "objectgroup")
        {
            auto* objectGroup = new TmxObjectGroup2D(this);
            ret = objectGroup->Load(childElement, info_);

            layers_.Push(objectGroup);

        }
        else if (name == "imagelayer")
        {
            auto* imageLayer = new TmxImageLayer2D(this);
            ret = imageLayer->Load(childElement, info_);

            layers_.Push(imageLayer);
        }

        if (!ret)
        {
            loadXMLFile_.Reset();
            tsxXMLFiles_.Clear();
            return false;
        }
    }

    loadXMLFile_.Reset();
    tsxXMLFiles_.Clear();
    return true;
}

bool TmxFile2D::SetInfo(Orientation2D orientation, int width, int height, float tileWidth, float tileHeight)
{
    if (layers_.Size() > 0)
        return false;
    info_.orientation_ = orientation;
    info_.width_ = width;
    info_.height_ = height;
    info_.tileWidth_ = tileWidth * PIXEL_SIZE;
    info_.tileHeight_ = tileHeight * PIXEL_SIZE;
    return true;
}

void TmxFile2D::AddLayer(unsigned index, TmxLayer2D *layer)
{
    if (index > layers_.Size())
        layers_.Push(layer);
    else // index <= layers_.size()
        layers_.Insert(index, layer);
}

void TmxFile2D::AddLayer(TmxLayer2D *layer)
{
    layers_.Push(layer);
}

Sprite2D* TmxFile2D::GetTileSprite(unsigned gid) const
{
    HashMap<unsigned, SharedPtr<Sprite2D>>::ConstIterator i = gidToSpriteMapping_.Find(gid);
    if (i == gidToSpriteMapping_.End())
        return nullptr;

    return i->second_;
}

Vector<SharedPtr<TileMapObject2D>> TmxFile2D::GetTileCollisionShapes(unsigned gid) const
{
    Vector<SharedPtr<TileMapObject2D>> tileShapes;
    HashMap<unsigned, Vector<SharedPtr<TileMapObject2D>>>::ConstIterator i = gidToCollisionShapeMapping_.Find(gid);
    if (i == gidToCollisionShapeMapping_.End())
        return tileShapes;

    return i->second_;
}

PropertySet2D* TmxFile2D::GetTilePropertySet(unsigned gid) const
{
    HashMap<unsigned, SharedPtr<PropertySet2D>>::ConstIterator i = gidToPropertySetMapping_.Find(gid);
    if (i == gidToPropertySetMapping_.End())
        return nullptr;
    return i->second_;
}

const TmxLayer2D* TmxFile2D::GetLayer(unsigned index) const
{
    if (index >= layers_.Size())
        return nullptr;

    return layers_[index];
}

void TmxFile2D::SetSpriteTextureEdgeOffset(float offset)
{
    edge_offset_ = offset;
    for (auto& i : gidToSpriteMapping_)
        i.second_->SetTextureEdgeOffset(offset);
}

SharedPtr<XmlFile> TmxFile2D::LoadTSXFile(const String& source)
{
    String tsxFilePath = get_parent(GetName()) + source;
    SharedPtr<File> tsxFile = DV_RES_CACHE.GetFile(tsxFilePath);
    SharedPtr<XmlFile> tsxXMLFile(new XmlFile());
    if (!tsxFile || !tsxXMLFile->Load(*tsxFile))
    {
        DV_LOGERROR("Load TSX file failed " + tsxFilePath);
        return SharedPtr<XmlFile>();
    }

    return tsxXMLFile;
}

struct TileImageInfo {
    Image* image;
    unsigned tileGid;
    int imageWidth;
    int imageHeight;
    int x;
    int y;
};

bool TmxFile2D::LoadTileSet(const XmlElement& element)
{
    unsigned firstgid = element.GetU32("firstgid");

    XmlElement tileSetElem;
    if (element.HasAttribute("source"))
    {
        String source = element.GetAttribute("source");
        HashMap<String, SharedPtr<XmlFile>>::Iterator i = tsxXMLFiles_.Find(source);
        if (i == tsxXMLFiles_.End())
        {
            SharedPtr<XmlFile> tsxXMLFile = LoadTSXFile(source);
            if (!tsxXMLFile)
                return false;

            // Add to mapping to avoid release
            tsxXMLFiles_[source] = tsxXMLFile;

            tileSetElem = tsxXMLFile->GetRoot("tileset");
        }
        else
            tileSetElem = i->second_->GetRoot("tileset");
    }
    else
        tileSetElem = element;

    int tileWidth = tileSetElem.GetI32("tilewidth");
    int tileHeight = tileSetElem.GetI32("tileheight");
    int spacing = tileSetElem.GetI32("spacing");
    int margin = tileSetElem.GetI32("margin");
    int imageWidth;
    int imageHeight;
    bool isSingleTileSet = false;

    ResourceCache& cache = DV_RES_CACHE;

    {
        XmlElement imageElem = tileSetElem.GetChild("image");
        // Tileset based on single tileset image
        if (imageElem.NotNull()) {
            isSingleTileSet = true;
            String textureFilePath = get_parent(GetName()) + imageElem.GetAttribute("source");
            SharedPtr<Texture2D> texture(cache.GetResource<Texture2D>(textureFilePath));
            if (!texture)
            {
                DV_LOGERROR("Could not load texture " + textureFilePath);
                return false;
            }

            // Set hot spot at left bottom
            Vector2 hotSpot(0.0f, 0.0f);
            if (tileSetElem.HasChild("tileoffset"))
            {
                XmlElement offsetElem = tileSetElem.GetChild("tileoffset");
                hotSpot.x_ += offsetElem.GetFloat("x") / (float)tileWidth;
                hotSpot.y_ += offsetElem.GetFloat("y") / (float)tileHeight;
            }

            imageWidth = imageElem.GetI32("width");
            imageHeight = imageElem.GetI32("height");

            unsigned gid = firstgid;
            for (int y = margin; y + tileHeight <= imageHeight - margin; y += tileHeight + spacing)
            {
                for (int x = margin; x + tileWidth <= imageWidth - margin; x += tileWidth + spacing)
                {
                    SharedPtr<Sprite2D> sprite(new Sprite2D());
                    sprite->SetTexture(texture);
                    sprite->SetRectangle(IntRect(x, y, x + tileWidth, y + tileHeight));
                    sprite->SetHotSpot(hotSpot);

                    gidToSpriteMapping_[gid++] = sprite;
                }
            }
        }
    }

    Vector<TileImageInfo> tileImageInfos;
    for (XmlElement tileElem = tileSetElem.GetChild("tile"); tileElem; tileElem = tileElem.GetNext("tile"))
    {
        unsigned gid = firstgid + tileElem.GetU32("id");
        // Tileset based on collection of images
        if (!isSingleTileSet)
        {
            XmlElement imageElem = tileElem.GetChild("image");
            if (imageElem.NotNull()) {
                String textureFilePath = get_parent(GetName()) + imageElem.GetAttribute("source");
                SharedPtr<Image> image(cache.GetResource<Image>(textureFilePath));
                if (!image)
                {
                    DV_LOGERROR("Could not load image " + textureFilePath);
                    return false;
                }
                tileWidth = imageWidth = imageElem.GetI32("width");
                tileHeight = imageHeight = imageElem.GetI32("height");
                TileImageInfo info = {image, gid, imageWidth, imageHeight, 0, 0};
                tileImageInfos.Push(info);
            }
        }
        // Tile collision shape(s)
        TmxObjectGroup2D objectGroup(this);
        for (XmlElement collisionElem = tileElem.GetChild("objectgroup"); collisionElem; collisionElem = collisionElem.GetNext("objectgroup"))
        {
            Vector<SharedPtr<TileMapObject2D>> objects;
            for (XmlElement objectElem = collisionElem.GetChild("object"); objectElem; objectElem = objectElem.GetNext("object"))
            {
                SharedPtr<TileMapObject2D> object(new TileMapObject2D());

                // Convert Tiled local position (left top) to Urho3D local position (left bottom)
                objectElem.SetAttribute("y", String(info_.GetMapHeight() / PIXEL_SIZE - (tileHeight - objectElem.GetFloat("y"))));

                objectGroup.StoreObject(objectElem, object, info_, true);
                objects.Push(object);
            }
            gidToCollisionShapeMapping_[gid] = objects;
        }
        if (tileElem.HasChild("properties"))
        {
            SharedPtr<PropertySet2D> propertySet(new PropertySet2D());
            propertySet->Load(tileElem.GetChild("properties"));
            gidToPropertySetMapping_[gid] = propertySet;
        }
    }

    if (!isSingleTileSet)
    {
        if (tileImageInfos.Empty())
            return false;

        AreaAllocator allocator(128, 128, 2048, 2048);

        for (int i = 0; i < tileImageInfos.Size(); ++i)
        {
            TileImageInfo& info = tileImageInfos[i];
            if (!allocator.Allocate(info.imageWidth + 1, info.imageHeight + 1, info.x, info.y))
            {
                DV_LOGERROR("Could not allocate area");
                return false;
            }
        }

        SharedPtr<Texture2D> texture(new Texture2D());
        texture->SetMipsToSkip(QUALITY_LOW, 0);
        texture->SetNumLevels(1);
        texture->SetSize(allocator.GetWidth(), allocator.GetHeight(), Graphics::GetRGBAFormat());

        auto textureDataSize = (unsigned)allocator.GetWidth() * allocator.GetHeight() * 4;
        SharedArrayPtr<unsigned char> textureData(new unsigned char[textureDataSize]);
        memset(textureData.Get(), 0, textureDataSize);

        for (int i = 0; i < tileImageInfos.Size(); ++i)
        {
            TileImageInfo& info = tileImageInfos[i];
            SharedPtr<Image> image = info.image->ConvertToRGBA();

            for (int y = 0; y < image->GetHeight(); ++y)
            {
                memcpy(textureData.Get() + ((info.y + y) * allocator.GetWidth() + info.x) * 4,
                    image->GetData() + y * image->GetWidth() * 4, (size_t)image->GetWidth() * 4);
            }

            SharedPtr<Sprite2D> sprite(new Sprite2D());
            sprite->SetTexture(texture);
            sprite->SetRectangle(IntRect(info.x, info.y, info.x + info.imageWidth, info.y + info.imageHeight));
            sprite->SetHotSpot(Vector2::ZERO);
            gidToSpriteMapping_[info.tileGid] = sprite;
        }
        texture->SetData(0, 0, 0, allocator.GetWidth(), allocator.GetHeight(), textureData.Get());
    }

    return true;
}

}
