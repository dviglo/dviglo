// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../io/log.h"
#include "xml_file.h"

#include <pugixml.hpp>

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

const XmlElement XmlElement::EMPTY;

XmlElement::XmlElement() :
    node_(nullptr),
    xpathResultSet_(nullptr),
    xpathNode_(nullptr),
    xpathResultIndex_(0)
{
}

XmlElement::XmlElement(XmlFile* file, pugi::xml_node_struct* node) :
    file_(file),
    node_(node),
    xpathResultSet_(nullptr),
    xpathNode_(nullptr),
    xpathResultIndex_(0)
{
}

XmlElement::XmlElement(XmlFile* file, const XPathResultSet* resultSet, const pugi::xpath_node* xpathNode,
    i32 xpathResultIndex) :
    file_(file),
    node_(nullptr),
    xpathResultSet_(resultSet),
    xpathNode_(resultSet ? xpathNode : (xpathNode ? new pugi::xpath_node(*xpathNode) : nullptr)),
    xpathResultIndex_(xpathResultIndex)
{
    assert(xpathResultIndex >= 0);
}

XmlElement::XmlElement(const XmlElement& rhs) :
    file_(rhs.file_),
    node_(rhs.node_),
    xpathResultSet_(rhs.xpathResultSet_),
    xpathNode_(rhs.xpathResultSet_ ? rhs.xpathNode_ : (rhs.xpathNode_ ? new pugi::xpath_node(*rhs.xpathNode_) : nullptr)),
    xpathResultIndex_(rhs.xpathResultIndex_)
{
}

XmlElement::~XmlElement()
{
    // XmlElement class takes the ownership of a single xpath_node object, so destruct it now
    if (!xpathResultSet_ && xpathNode_)
    {
        delete xpathNode_;
        xpathNode_ = nullptr;
    }
}

XmlElement& XmlElement::operator =(const XmlElement& rhs)
{
    file_ = rhs.file_;
    node_ = rhs.node_;
    xpathResultSet_ = rhs.xpathResultSet_;
    xpathNode_ = rhs.xpathResultSet_ ? rhs.xpathNode_ : (rhs.xpathNode_ ? new pugi::xpath_node(*rhs.xpathNode_) : nullptr);
    xpathResultIndex_ = rhs.xpathResultIndex_;
    return *this;
}

XmlElement XmlElement::create_child(const String& name)
{
    return create_child(name.c_str());
}

XmlElement XmlElement::create_child(const char* name)
{
    if (!file_ || (!node_ && !xpathNode_))
        return XmlElement();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    pugi::xml_node child = const_cast<pugi::xml_node&>(node).append_child(name);
    return XmlElement(file_, child.internal_object());
}

XmlElement XmlElement::GetOrCreateChild(const String& name)
{
    XmlElement child = GetChild(name);
    if (child.NotNull())
        return child;
    else
        return create_child(name);
}

XmlElement XmlElement::GetOrCreateChild(const char* name)
{
    XmlElement child = GetChild(name);
    if (child.NotNull())
        return child;
    else
        return create_child(name);
}

bool XmlElement::AppendChild(XmlElement element, bool asCopy)
{
    if (!element.file_ || (!element.node_ && !element.xpathNode_) || !file_ || (!node_ && !xpathNode_))
        return false;

    pugi::xml_node node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    const pugi::xml_node& child = element.xpathNode_ ? element.xpathNode_->node() : pugi::xml_node(element.node_);

    if (asCopy)
        node.append_copy(child);
    else
        node.append_move(child);
    return true;
}

bool XmlElement::Remove()
{
    return GetParent().RemoveChild(*this);
}

bool XmlElement::RemoveChild(const XmlElement& element)
{
    if (!element.file_ || (!element.node_ && !element.xpathNode_) || !file_ || (!node_ && !xpathNode_))
        return false;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    const pugi::xml_node& child = element.xpathNode_ ? element.xpathNode_->node() : pugi::xml_node(element.node_);
    return const_cast<pugi::xml_node&>(node).remove_child(child);
}

bool XmlElement::RemoveChild(const String& name)
{
    return RemoveChild(name.c_str());
}

bool XmlElement::RemoveChild(const char* name)
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return const_cast<pugi::xml_node&>(node).remove_child(name);
}

bool XmlElement::RemoveChildren(const String& name)
{
    return RemoveChildren(name.c_str());
}

bool XmlElement::RemoveChildren(const char* name)
{
    if ((!file_ || !node_) && !xpathNode_)
        return false;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    if (!String::CStringLength(name))
    {
        for (;;)
        {
            pugi::xml_node child = node.last_child();
            if (child.empty())
                break;
            const_cast<pugi::xml_node&>(node).remove_child(child);
        }
    }
    else
    {
        for (;;)
        {
            pugi::xml_node child = node.child(name);
            if (child.empty())
                break;
            const_cast<pugi::xml_node&>(node).remove_child(child);
        }
    }

    return true;
}

bool XmlElement::RemoveAttribute(const String& name)
{
    return RemoveAttribute(name.c_str());
}

bool XmlElement::RemoveAttribute(const char* name)
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    // If xpath_node contains just attribute, remove it regardless of the specified name
    if (xpathNode_ && xpathNode_->attribute())
        return xpathNode_->parent().remove_attribute(
            xpathNode_->attribute());  // In attribute context, xpath_node's parent is the parent node of the attribute itself

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return const_cast<pugi::xml_node&>(node).remove_attribute(node.attribute(name));
}

XmlElement XmlElement::SelectSingle(const String& query, pugi::xpath_variable_set* variables) const
{
    if (!file_ || (!node_ && !xpathNode_))
        return XmlElement();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    pugi::xpath_node result = node.select_node(query.c_str(), variables);
    return XmlElement(file_, nullptr, &result, 0);
}

XmlElement XmlElement::SelectSinglePrepared(const XPathQuery& query) const
{
    if (!file_ || (!node_ && !xpathNode_ && !query.GetXPathQuery()))
        return XmlElement();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    pugi::xpath_node result = node.select_node(*query.GetXPathQuery());
    return XmlElement(file_, nullptr, &result, 0);
}

XPathResultSet XmlElement::Select(const String& query, pugi::xpath_variable_set* variables) const
{
    if (!file_ || (!node_ && !xpathNode_))
        return XPathResultSet();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    pugi::xpath_node_set result = node.select_nodes(query.c_str(), variables);
    return XPathResultSet(file_, &result);
}

XPathResultSet XmlElement::SelectPrepared(const XPathQuery& query) const
{
    if (!file_ || (!node_ && !xpathNode_ && query.GetXPathQuery()))
        return XPathResultSet();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    pugi::xpath_node_set result = node.select_nodes(*query.GetXPathQuery());
    return XPathResultSet(file_, &result);
}

bool XmlElement::SetValue(const String& value)
{
    return SetValue(value.c_str());
}

bool XmlElement::SetValue(const char* value)
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);

    // Search for existing value first
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
    {
        if (child.type() == pugi::node_pcdata)
            return const_cast<pugi::xml_node&>(child).set_value(value);
    }

    // If no previous value found, append new
    return const_cast<pugi::xml_node&>(node).append_child(pugi::node_pcdata).set_value(value);
}

bool XmlElement::SetAttribute(const String& name, const String& value)
{
    return SetAttribute(name.c_str(), value.c_str());
}

bool XmlElement::SetAttribute(const char* name, const char* value)
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    // If xpath_node contains just attribute, set its value regardless of the specified name
    if (xpathNode_ && xpathNode_->attribute())
        return xpathNode_->attribute().set_value(value);

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    pugi::xml_attribute attr = node.attribute(name);
    if (attr.empty())
        attr = const_cast<pugi::xml_node&>(node).append_attribute(name);
    return attr.set_value(value);
}

bool XmlElement::SetAttribute(const String& value)
{
    return SetAttribute(value.c_str());
}

bool XmlElement::SetAttribute(const char* value)
{
    // If xpath_node contains just attribute, set its value
    return xpathNode_ && xpathNode_->attribute() && xpathNode_->attribute().set_value(value);
}

bool XmlElement::SetBool(const String& name, bool value)
{
    return SetAttribute(name, String(value));
}

bool XmlElement::SetBoundingBox(const BoundingBox& value)
{
    if (!SetVector3("min", value.min_))
        return false;
    return SetVector3("max", value.max_);
}

bool XmlElement::SetBuffer(const String& name, const void* data, i32 size)
{
    assert(size >= 0);
    String dataStr;
    BufferToString(dataStr, data, size);
    return SetAttribute(name, dataStr);
}

bool XmlElement::SetBuffer(const String& name, const Vector<unsigned char>& value)
{
    if (!value.Size())
        return SetAttribute(name, String::EMPTY);
    else
        return SetBuffer(name, &value[0], value.Size());
}

bool XmlElement::SetColor(const String& name, const Color& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetFloat(const String& name, float value)
{
    return SetAttribute(name, String(value));
}

bool XmlElement::SetDouble(const String& name, double value)
{
    return SetAttribute(name, String(value));
}

bool XmlElement::SetU32(const String& name, u32 value)
{
    return SetAttribute(name, String(value));
}

bool XmlElement::SetI32(const String& name, i32 value)
{
    return SetAttribute(name, String(value));
}

bool XmlElement::SetU64(const String& name, u64 value)
{
    return SetAttribute(name, String(value));
}

bool XmlElement::SetI64(const String& name, i64 value)
{
    return SetAttribute(name, String(value));
}

bool XmlElement::SetIntRect(const String& name, const IntRect& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetIntVector2(const String& name, const IntVector2& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetIntVector3(const String& name, const IntVector3& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetRect(const String& name, const Rect& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetQuaternion(const String& name, const Quaternion& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetString(const String& name, const String& value)
{
    return SetAttribute(name, value);
}

bool XmlElement::SetVariant(const Variant& value)
{
    if (!SetAttribute("type", value.GetTypeName()))
        return false;

    return SetVariantValue(value);
}

bool XmlElement::SetVariantValue(const Variant& value)
{
    switch (value.GetType())
    {
    case VAR_RESOURCEREF:
        return SetResourceRef(value.GetResourceRef());

    case VAR_RESOURCEREFLIST:
        return SetResourceRefList(value.GetResourceRefList());

    case VAR_VARIANTVECTOR:
        return SetVariantVector(value.GetVariantVector());

    case VAR_STRINGVECTOR:
        return SetStringVector(value.GetStringVector());

    case VAR_VARIANTMAP:
        return SetVariantMap(value.GetVariantMap());

    default:
        return SetAttribute("value", value.ToString().c_str());
    }
}

bool XmlElement::SetResourceRef(const ResourceRef& value)
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    return SetAttribute("value", String(DV_CONTEXT->GetTypeName(value.type_)) + ";" + value.name_);
}

bool XmlElement::SetResourceRefList(const ResourceRefList& value)
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    String str(DV_CONTEXT->GetTypeName(value.type_));
    for (const String& name : value.names_)
    {
        str += ";";
        str += name;
    }

    return SetAttribute("value", str.c_str());
}

bool XmlElement::SetVariantVector(const VariantVector& value)
{
    // Must remove all existing variant child elements (if they exist) to not cause confusion
    if (!RemoveChildren("variant"))
        return false;

    for (VariantVector::ConstIterator i = value.Begin(); i != value.End(); ++i)
    {
        XmlElement variantElem = create_child("variant");
        if (!variantElem)
            return false;
        variantElem.SetVariant(*i);
    }

    return true;
}

bool XmlElement::SetStringVector(const StringVector& value)
{
    if (!RemoveChildren("string"))
        return false;

    for (StringVector::ConstIterator i = value.Begin(); i != value.End(); ++i)
    {
        XmlElement stringElem = create_child("string");
        if (!stringElem)
            return false;
        stringElem.SetAttribute("value", *i);
    }

    return true;
}

bool XmlElement::SetVariantMap(const VariantMap& value)
{
    if (!RemoveChildren("variant"))
        return false;

    for (VariantMap::ConstIterator i = value.Begin(); i != value.End(); ++i)
    {
        XmlElement variantElem = create_child("variant");
        if (!variantElem)
            return false;
        variantElem.SetU32("hash", i->first_.Value());
        variantElem.SetVariant(i->second_);
    }

    return true;
}

bool XmlElement::SetVector2(const String& name, const Vector2& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetVector3(const String& name, const Vector3& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetVector4(const String& name, const Vector4& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetVectorVariant(const String& name, const Variant& value)
{
    VariantType type = value.GetType();
    if (type == VAR_FLOAT || type == VAR_VECTOR2 || type == VAR_VECTOR3 || type == VAR_VECTOR4 || type == VAR_MATRIX3 ||
        type == VAR_MATRIX3X4 || type == VAR_MATRIX4)
        return SetAttribute(name, value.ToString());
    else
        return false;
}

bool XmlElement::SetMatrix3(const String& name, const Matrix3& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetMatrix3x4(const String& name, const Matrix3x4& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::SetMatrix4(const String& name, const Matrix4& value)
{
    return SetAttribute(name, value.ToString());
}

bool XmlElement::IsNull() const
{
    return !NotNull();
}

bool XmlElement::NotNull() const
{
    return node_ || (xpathNode_ && !xpathNode_->operator !());
}

XmlElement::operator bool() const
{
    return NotNull();
}

String XmlElement::GetName() const
{
    if ((!file_ || !node_) && !xpathNode_)
        return String();

    // If xpath_node contains just attribute, return its name instead
    if (xpathNode_ && xpathNode_->attribute())
        return String(xpathNode_->attribute().name());

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return String(node.name());
}

bool XmlElement::HasChild(const String& name) const
{
    return HasChild(name.c_str());
}

bool XmlElement::HasChild(const char* name) const
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return !node.child(name).empty();
}

XmlElement XmlElement::GetChild(const String& name) const
{
    return GetChild(name.c_str());
}

XmlElement XmlElement::GetChild(const char* name) const
{
    if (!file_ || (!node_ && !xpathNode_))
        return XmlElement();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    if (!String::CStringLength(name))
        return XmlElement(file_, node.first_child().internal_object());
    else
        return XmlElement(file_, node.child(name).internal_object());
}

XmlElement XmlElement::GetNext(const String& name) const
{
    return GetNext(name.c_str());
}

XmlElement XmlElement::GetNext(const char* name) const
{
    if (!file_ || (!node_ && !xpathNode_))
        return XmlElement();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    if (!String::CStringLength(name))
        return XmlElement(file_, node.next_sibling().internal_object());
    else
        return XmlElement(file_, node.next_sibling(name).internal_object());
}

XmlElement XmlElement::GetParent() const
{
    if (!file_ || (!node_ && !xpathNode_))
        return XmlElement();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return XmlElement(file_, node.parent().internal_object());
}

i32 XmlElement::GetNumAttributes() const
{
    if (!file_ || (!node_ && !xpathNode_))
        return 0;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    i32 ret = 0;

    pugi::xml_attribute attr = node.first_attribute();
    while (!attr.empty())
    {
        ++ret;
        attr = attr.next_attribute();
    }

    return ret;
}

bool XmlElement::HasAttribute(const String& name) const
{
    return HasAttribute(name.c_str());
}

bool XmlElement::HasAttribute(const char* name) const
{
    if (!file_ || (!node_ && !xpathNode_))
        return false;

    // If xpath_node contains just attribute, check against it
    if (xpathNode_ && xpathNode_->attribute())
        return String(xpathNode_->attribute().name()) == name;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return !node.attribute(name).empty();
}

String XmlElement::GetValue() const
{
    if (!file_ || (!node_ && !xpathNode_))
        return String::EMPTY;

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return String(node.child_value());
}

String XmlElement::GetAttribute(const String& name) const
{
    return String(GetAttributeCString(name.c_str()));
}

String XmlElement::GetAttribute(const char* name) const
{
    return String(GetAttributeCString(name));
}

const char* XmlElement::GetAttributeCString(const char* name) const
{
    if (!file_ || (!node_ && !xpathNode_))
        return nullptr;

    // If xpath_node contains just attribute, return it regardless of the specified name
    if (xpathNode_ && xpathNode_->attribute())
        return xpathNode_->attribute().value();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    return node.attribute(name).value();
}

String XmlElement::GetAttributeLower(const String& name) const
{
    return GetAttribute(name).ToLower();
}

String XmlElement::GetAttributeLower(const char* name) const
{
    return String(GetAttribute(name)).ToLower();
}

String XmlElement::GetAttributeUpper(const String& name) const
{
    return GetAttribute(name).ToUpper();
}

String XmlElement::GetAttributeUpper(const char* name) const
{
    return String(GetAttribute(name)).ToUpper();
}

Vector<String> XmlElement::GetAttributeNames() const
{
    if (!file_ || (!node_ && !xpathNode_))
        return Vector<String>();

    const pugi::xml_node& node = xpathNode_ ? xpathNode_->node() : pugi::xml_node(node_);
    Vector<String> ret;

    pugi::xml_attribute attr = node.first_attribute();
    while (!attr.empty())
    {
        ret.Push(String(attr.name()));
        attr = attr.next_attribute();
    }

    return ret;
}

bool XmlElement::GetBool(const String& name) const
{
    return ToBool(GetAttribute(name));
}

BoundingBox XmlElement::GetBoundingBox() const
{
    BoundingBox ret;

    ret.min_ = GetVector3("min");
    ret.max_ = GetVector3("max");
    return ret;
}

Vector<byte> XmlElement::GetBuffer(const String& name) const
{
    Vector<byte> ret;
    StringToBuffer(ret, GetAttribute(name));
    return ret;
}

bool XmlElement::GetBuffer(const String& name, void* dest, i32 size) const
{
    Vector<String> bytes = GetAttribute(name).Split(' ');
    if (size < bytes.Size())
        return false;

    byte* destBytes = (byte*)dest;
    for (i32 i = 0; i < bytes.Size(); ++i)
        destBytes[i] = static_cast<byte>(ToI32(bytes[i]));
    return true;
}

Color XmlElement::GetColor(const String& name) const
{
    return ToColor(GetAttribute(name));
}

float XmlElement::GetFloat(const String& name) const
{
    return ToFloat(GetAttribute(name));
}

double XmlElement::GetDouble(const String& name) const
{
    return ToDouble(GetAttribute(name));
}

u32 XmlElement::GetU32(const String& name) const
{
    return ToU32(GetAttribute(name));
}

i32 XmlElement::GetI32(const String& name) const
{
    return ToI32(GetAttribute(name));
}

u64 XmlElement::GetU64(const String& name) const
{
    return ToU64(GetAttribute(name));
}

i64 XmlElement::GetI64(const String& name) const
{
    return ToI64(GetAttribute(name));
}

IntRect XmlElement::GetIntRect(const String& name) const
{
    return ToIntRect(GetAttribute(name));
}

IntVector2 XmlElement::GetIntVector2(const String& name) const
{
    return ToIntVector2(GetAttribute(name));
}

IntVector3 XmlElement::GetIntVector3(const String& name) const
{
    return ToIntVector3(GetAttribute(name));
}

Quaternion XmlElement::GetQuaternion(const String& name) const
{
    return ToQuaternion(GetAttribute(name));
}

Rect XmlElement::GetRect(const String& name) const
{
    return ToRect(GetAttribute(name));
}

Variant XmlElement::GetVariant() const
{
    VariantType type = Variant::GetTypeFromName(GetAttribute("type"));
    return GetVariantValue(type);
}

Variant XmlElement::GetVariantValue(VariantType type) const
{
    Variant ret;

    if (type == VAR_RESOURCEREF)
        ret = GetResourceRef();
    else if (type == VAR_RESOURCEREFLIST)
        ret = GetResourceRefList();
    else if (type == VAR_VARIANTVECTOR)
        ret = GetVariantVector();
    else if (type == VAR_STRINGVECTOR)
        ret = GetStringVector();
    else if (type == VAR_VARIANTMAP)
        ret = GetVariantMap();
    else
        ret.FromString(type, GetAttributeCString("value"));

    return ret;
}

ResourceRef XmlElement::GetResourceRef() const
{
    ResourceRef ret;

    Vector<String> values = GetAttribute("value").Split(';');
    if (values.Size() == 2)
    {
        ret.type_ = values[0];
        ret.name_ = values[1];
    }

    return ret;
}

ResourceRefList XmlElement::GetResourceRefList() const
{
    ResourceRefList ret;

    Vector<String> values = GetAttribute("value").Split(';', true);
    if (values.Size() >= 1)
    {
        ret.type_ = values[0];
        ret.names_.Resize(values.Size() - 1);
        for (i32 i = 1; i < values.Size(); ++i)
            ret.names_[i - 1] = values[i];
    }

    return ret;
}

VariantVector XmlElement::GetVariantVector() const
{
    VariantVector ret;

    XmlElement variantElem = GetChild("variant");
    while (variantElem)
    {
        ret.Push(variantElem.GetVariant());
        variantElem = variantElem.GetNext("variant");
    }

    return ret;
}

StringVector XmlElement::GetStringVector() const
{
    StringVector ret;

    XmlElement stringElem = GetChild("string");
    while (stringElem)
    {
        ret.Push(stringElem.GetAttributeCString("value"));
        stringElem = stringElem.GetNext("string");
    }

    return ret;
}

VariantMap XmlElement::GetVariantMap() const
{
    VariantMap ret;

    XmlElement variantElem = GetChild("variant");
    while (variantElem)
    {
        // If this is a manually edited map, user can not be expected to calculate hashes manually. Also accept "name" attribute
        if (variantElem.HasAttribute("name"))
            ret[StringHash(variantElem.GetAttribute("name"))] = variantElem.GetVariant();
        else if (variantElem.HasAttribute("hash"))
            ret[StringHash(variantElem.GetU32("hash"))] = variantElem.GetVariant();

        variantElem = variantElem.GetNext("variant");
    }

    return ret;
}

Vector2 XmlElement::GetVector2(const String& name) const
{
    return ToVector2(GetAttribute(name));
}

Vector3 XmlElement::GetVector3(const String& name) const
{
    return ToVector3(GetAttribute(name));
}

Vector4 XmlElement::GetVector4(const String& name) const
{
    return ToVector4(GetAttribute(name));
}

Vector4 XmlElement::GetVector(const String& name) const
{
    return ToVector4(GetAttribute(name), true);
}

Variant XmlElement::GetVectorVariant(const String& name) const
{
    return ToVectorVariant(GetAttribute(name));
}

Matrix3 XmlElement::GetMatrix3(const String& name) const
{
    return ToMatrix3(GetAttribute(name));
}

Matrix3x4 XmlElement::GetMatrix3x4(const String& name) const
{
    return ToMatrix3x4(GetAttribute(name));
}

Matrix4 XmlElement::GetMatrix4(const String& name) const
{
    return ToMatrix4(GetAttribute(name));
}

XmlFile* XmlElement::GetFile() const
{
    return file_;
}

XmlElement XmlElement::NextResult() const
{
    if (!xpathResultSet_ || !xpathNode_)
        return XmlElement();

    return xpathResultSet_->operator [](++xpathResultIndex_);
}

XPathResultSet::XPathResultSet() :
    resultSet_(nullptr)
{
}

XPathResultSet::XPathResultSet(XmlFile* file, pugi::xpath_node_set* resultSet) :
    file_(file),
    resultSet_(resultSet ? new pugi::xpath_node_set(resultSet->begin(), resultSet->end()) : nullptr)
{
    // Sort the node set in forward document order
    if (resultSet_)
        resultSet_->sort();
}

XPathResultSet::XPathResultSet(const XPathResultSet& rhs) :
    file_(rhs.file_),
    resultSet_(rhs.resultSet_ ? new pugi::xpath_node_set(rhs.resultSet_->begin(), rhs.resultSet_->end()) : nullptr)
{
}

XPathResultSet::~XPathResultSet()
{
    delete resultSet_;
    resultSet_ = nullptr;
}

XPathResultSet& XPathResultSet::operator =(const XPathResultSet& rhs)
{
    file_ = rhs.file_;
    resultSet_ = rhs.resultSet_ ? new pugi::xpath_node_set(rhs.resultSet_->begin(), rhs.resultSet_->end()) : nullptr;
    return *this;
}

XmlElement XPathResultSet::operator [](i32 index) const
{
    assert(index >= 0);

    if (!resultSet_)
        DV_LOGERRORF(
            "Could not return result at index: %u. Most probably this is caused by the XPathResultSet not being stored in a lhs variable.",
            index);

    return resultSet_ && index < Size() ? XmlElement(file_, this, &resultSet_->operator [](index), index) : XmlElement();
}

XmlElement XPathResultSet::FirstResult()
{
    return operator [](0);
}

i32 XPathResultSet::Size() const
{
    return resultSet_ ? (i32)resultSet_->size() : 0;
}

bool XPathResultSet::Empty() const
{
    return resultSet_ ? resultSet_->empty() : true;
}

XPathQuery::XPathQuery() = default;

XPathQuery::XPathQuery(const String& queryString, const String& variableString)
{
    SetQuery(queryString, variableString);
}

XPathQuery::~XPathQuery() = default;

void XPathQuery::Bind()
{
    // Delete previous query object and create a new one binding it with variable set
    query_ = make_unique<pugi::xpath_query>(query_string_.c_str(), variables_.get());
}

bool XPathQuery::SetVariable(const String& name, bool value)
{
    if (!variables_)
        variables_ = make_unique<pugi::xpath_variable_set>();

    return variables_->set(name.c_str(), value);
}

bool XPathQuery::SetVariable(const String& name, float value)
{
    if (!variables_)
        variables_ = make_unique<pugi::xpath_variable_set>();

    return variables_->set(name.c_str(), value);
}

bool XPathQuery::SetVariable(const String& name, const String& value)
{
    return SetVariable(name.c_str(), value.c_str());
}

bool XPathQuery::SetVariable(const char* name, const char* value)
{
    if (!variables_)
        variables_ = make_unique<pugi::xpath_variable_set>();

    return variables_->set(name, value);
}

bool XPathQuery::SetVariable(const String& name, const XPathResultSet& value)
{
    if (!variables_)
        variables_ = make_unique<pugi::xpath_variable_set>();

    pugi::xpath_node_set* nodeSet = value.GetXPathNodeSet();
    if (!nodeSet)
        return false;

    return variables_->set(name.c_str(), *nodeSet);
}

bool XPathQuery::SetQuery(const String& queryString, const String& variableString, bool bind)
{
    if (!variableString.Empty())
    {
        Clear();
        variables_ = make_unique<pugi::xpath_variable_set>();

        // Parse the variable string having format "name1:type1,name2:type2,..." where type is one of "Bool", "Float", "String", "ResultSet"
        Vector<String> vars = variableString.Split(',');
        for (Vector<String>::ConstIterator i = vars.Begin(); i != vars.End(); ++i)
        {
            Vector<String> tokens = i->Trimmed().Split(':');
            if (tokens.Size() != 2)
                continue;

            pugi::xpath_value_type type;
            if (tokens[1] == "Bool")
                type = pugi::xpath_type_boolean;
            else if (tokens[1] == "Float")
                type = pugi::xpath_type_number;
            else if (tokens[1] == "String")
                type = pugi::xpath_type_string;
            else if (tokens[1] == "ResultSet")
                type = pugi::xpath_type_node_set;
            else
                return false;

            if (!variables_->add(tokens[0].c_str(), type))
                return false;
        }
    }

    query_string_ = queryString;

    if (bind)
        Bind();

    return true;
}

void XPathQuery::Clear()
{
    query_string_.Clear();

    variables_.reset();
    query_.reset();
}

bool XPathQuery::EvaluateToBool(const XmlElement& element) const
{
    if (!query_ || ((!element.GetFile() || !element.GetNode()) && !element.GetXPathNode()))
        return false;

    const pugi::xml_node& node = element.GetXPathNode() ? element.GetXPathNode()->node() : pugi::xml_node(element.GetNode());
    return query_->evaluate_boolean(node);
}

float XPathQuery::EvaluateToFloat(const XmlElement& element) const
{
    if (!query_ || ((!element.GetFile() || !element.GetNode()) && !element.GetXPathNode()))
        return 0.0f;

    const pugi::xml_node& node = element.GetXPathNode() ? element.GetXPathNode()->node() : pugi::xml_node(element.GetNode());
    return (float)query_->evaluate_number(node);
}

String XPathQuery::EvaluateToString(const XmlElement& element) const
{
    if (!query_ || ((!element.GetFile() || !element.GetNode()) && !element.GetXPathNode()))
        return String::EMPTY;

    const pugi::xml_node& node = element.GetXPathNode() ? element.GetXPathNode()->node() : pugi::xml_node(element.GetNode());
    String result;
    // First call get the size
    result.Reserve((i32)query_->evaluate_string(nullptr, 0, node));
    // Second call get the actual string
    query_->evaluate_string(const_cast<pugi::char_t*>(result.c_str()), result.Capacity(), node);
    return result;
}

XPathResultSet XPathQuery::Evaluate(const XmlElement& element) const
{
    if (!query_ || ((!element.GetFile() || !element.GetNode()) && !element.GetXPathNode()))
        return XPathResultSet();

    const pugi::xml_node& node = element.GetXPathNode() ? element.GetXPathNode()->node() : pugi::xml_node(element.GetNode());
    pugi::xpath_node_set result = query_->evaluate_node_set(node);
    return XPathResultSet(element.GetFile(), &result);
}

}
