// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "resource.h"
#include "xml_element.h"

#include <memory>

namespace pugi
{

class xml_document;
class xml_node;
class xpath_node;

}

namespace dviglo
{

/// XML document resource.
class DV_API XmlFile : public Resource
{
    DV_OBJECT(XmlFile, Resource);

public:
    /// Construct.
    explicit XmlFile();
    /// Destruct.
    ~XmlFile() override;
    /// Register object factory.
    static void register_object();

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool begin_load(Deserializer& source) override;
    /// Save resource with default indentation (one tab). Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Save resource with user-defined indentation. Return true if successful.
    bool Save(Serializer& dest, const String& indentation) const;

    /// Deserialize from a string. Return true if successful.
    bool FromString(const String& source);
    /// Clear the document and create a root element.
    XmlElement CreateRoot(const String& name);
    /// Get the root element if it has matching name, otherwise create it and clear the document.
    XmlElement GetOrCreateRoot(const String& name);

    /// Return the root element, with optionally specified name. Return null element if not found.
    XmlElement GetRoot(const String& name = String::EMPTY);

    /// Return the pugixml document.
    pugi::xml_document* GetDocument() const { return document_.get(); }

    /// Serialize the XML content to a string.
    String ToString(const String& indentation = "\t") const;

    /// Patch the XmlFile with another XmlFile. Based on RFC 5261.
    void Patch(XmlFile* patchFile);
    /// Patch the XmlFile with another XmlElement. Based on RFC 5261.
    void Patch(const XmlElement& patchElement);

private:
    /// Add an node in the Patch.
    void PatchAdd(const pugi::xml_node& patch, pugi::xpath_node& original) const;
    /// Replace a node or attribute in the Patch.
    void PatchReplace(const pugi::xml_node& patch, pugi::xpath_node& original) const;
    /// Remove a node or attribute in the Patch.
    void PatchRemove(const pugi::xpath_node& original) const;

    /// Add a node in the Patch.
    void AddNode(const pugi::xml_node& patch, const pugi::xpath_node& original) const;
    /// Add an attribute in the Patch.
    void AddAttribute(const pugi::xml_node& patch, const pugi::xpath_node& original) const;
    /// Combine two text nodes.
    bool CombineText(const pugi::xml_node& patch, const pugi::xml_node& original, bool prepend) const;

    /// Pugixml document.
    std::unique_ptr<pugi::xml_document> document_;
};

}
