// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "component.h"

namespace dviglo
{

/// Placeholder for allowing unregistered components to be loaded & saved along with scenes.
class DV_API UnknownComponent : public Component
{
public:
    /// Construct.
    explicit UnknownComponent();

    /// Register object factory.
    static void register_object();

    /// Return type of the stored component.
    StringHash GetType() const override { return typeHash_; }

    /// Return type name of the stored component.
    const String& GetTypeName() const override { return typeName_; }

    /// Return attribute descriptions, or null if none defined.
    const Vector<AttributeInfo>* GetAttributes() const override { return &xmlAttributeInfos_; }

    /// Load from binary data. Return true if successful.
    bool Load(Deserializer& source) override;
    /// Load from XML data. Return true if successful.
    bool load_xml(const XmlElement& source) override;
    /// Load from JSON data. Return true if successful.
    bool load_json(const JSONValue& source) override;
    /// Save as binary data. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Save as XML data. Return true if successful.
    bool save_xml(XmlElement& dest) const override;
    /// Save as JSON data. Return true if successful.
    bool save_json(JSONValue& dest) const override;

    /// Initialize the type name. Called by Node when loading.
    void SetTypeName(const String& typeName);
    /// Initialize the type hash only when type name not known. Called by Node when loading.
    void SetType(StringHash typeHash);

    /// Return the XML format attributes. Empty when loaded with binary serialization.
    const Vector<String>& GetXMLAttributes() const { return xmlAttributes_; }

    /// Return the binary attributes. Empty when loaded with XML serialization.
    const Vector<unsigned char>& GetBinaryAttributes() const { return binaryAttributes_; }

    /// Return whether was loaded using XML data.
    bool GetUseXML() const { return useXML_; }

    /// Return static type.
    static dviglo::StringHash GetTypeStatic()
    {
        static const StringHash typeStatic("UnknownComponent");
        return typeStatic;
    }
    /// Return static type name.
    static const dviglo::String& GetTypeNameStatic()
    {
        static const String typeNameStatic("UnknownComponent");
        return typeNameStatic;
    }

private:
    /// Type of stored component.
    StringHash typeHash_;
    /// Type name of the stored component.
    String typeName_;
    /// XML format attribute infos.
    Vector<AttributeInfo> xmlAttributeInfos_;
    /// XML format attribute data (as strings).
    Vector<String> xmlAttributes_;
    /// Binary attributes.
    Vector<unsigned char> binaryAttributes_;
    /// Flag of whether was loaded using XML/JSON data.
    bool useXML_;

};

}
