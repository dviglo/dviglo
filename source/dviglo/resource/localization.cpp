// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "localization.h"
#include "resource_cache.h"
#include "json_file.h"
#include "resource_events.h"
#include "../io/log.h"

#include "../common/debug_new.h"

namespace dviglo
{

Localization::Localization() :
    languageIndex_(-1)
{
}

Localization::~Localization() = default;

int Localization::GetLanguageIndex(const String& language)
{
    if (language.Empty())
    {
        DV_LOGWARNING("Localization::GetLanguageIndex(language): language name is empty");
        return -1;
    }
    if (GetNumLanguages() == 0)
    {
        DV_LOGWARNING("Localization::GetLanguageIndex(language): no loaded languages");
        return -1;
    }
    for (int i = 0; i < GetNumLanguages(); i++)
    {
        if (languages_[i] == language)
            return i;
    }
    return -1;
}

String Localization::GetLanguage()
{
    if (languageIndex_ == -1)
    {
        DV_LOGWARNING("Localization::GetLanguage(): no loaded languages");
        return String::EMPTY;
    }
    return languages_[languageIndex_];
}

String Localization::GetLanguage(int index)
{
    if (GetNumLanguages() == 0)
    {
        DV_LOGWARNING("Localization::GetLanguage(index): no loaded languages");
        return String::EMPTY;
    }
    if (index < 0 || index >= GetNumLanguages())
    {
        DV_LOGWARNING("Localization::GetLanguage(index): index out of range");
        return String::EMPTY;
    }
    return languages_[index];
}

void Localization::SetLanguage(int index)
{
    if (GetNumLanguages() == 0)
    {
        DV_LOGWARNING("Localization::SetLanguage(index): no loaded languages");
        return;
    }
    if (index < 0 || index >= GetNumLanguages())
    {
        DV_LOGWARNING("Localization::SetLanguage(index): index out of range");
        return;
    }
    if (index != languageIndex_)
    {
        languageIndex_ = index;
        VariantMap& eventData = GetEventDataMap();
        SendEvent(E_CHANGELANGUAGE, eventData);
    }
}

void Localization::SetLanguage(const String& language)
{
    if (language.Empty())
    {
        DV_LOGWARNING("Localization::SetLanguage(language): language name is empty");
        return;
    }
    if (GetNumLanguages() == 0)
    {
        DV_LOGWARNING("Localization::SetLanguage(language): no loaded languages");
        return;
    }
    int index = GetLanguageIndex(language);
    if (index == -1)
    {
        DV_LOGWARNING("Localization::SetLanguage(language): language not found");
        return;
    }
    SetLanguage(index);
}

String Localization::Get(const String& id)
{
    if (id.Empty())
        return String::EMPTY;
    if (GetNumLanguages() == 0)
    {
        DV_LOGWARNING("Localization::Get(id): no loaded languages");
        return id;
    }
    String result = strings_[StringHash(GetLanguage())][StringHash(id)];
    if (result.Empty())
    {
        DV_LOGWARNING("Localization::Get(\"" + id + "\") not found translation, language=\"" + GetLanguage() + "\"");
        return id;
    }
    return result;
}

void Localization::Reset()
{
    languages_.Clear();
    languageIndex_ = -1;
    strings_.Clear();
}

void Localization::LoadJSONFile(const String& name, const String& language)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* jsonFile = cache->GetResource<JSONFile>(name);
    if (jsonFile)
    {
        if (language.Empty())
            LoadMultipleLanguageJSON(jsonFile->GetRoot());
        else
            LoadSingleLanguageJSON(jsonFile->GetRoot(), language);
    }
}


void Localization::LoadMultipleLanguageJSON(const JSONValue& source)
{
    for (JSONObject::ConstIterator i = source.Begin(); i != source.End(); ++i)
    {
        String id = i->first_;
        if (id.Empty())
        {
            DV_LOGWARNING("Localization::LoadMultipleLanguageJSON(source): string ID is empty");
            continue;
        }
        const JSONValue& value = i->second_;
        if (value.IsObject())
        {
            for (JSONObject::ConstIterator j = value.Begin(); j != value.End(); ++j)
            {
                const String &lang = j->first_;
                if (lang.Empty())
                {
                    DV_LOGWARNING(
                            "Localization::LoadMultipleLanguageJSON(source): language name is empty, string ID=\"" + id + "\"");
                    continue;
                }
                const String &string = j->second_.GetString();
                if (string.Empty())
                {
                    DV_LOGWARNING(
                            "Localization::LoadMultipleLanguageJSON(source): translation is empty, string ID=\"" + id +
                            "\", language=\"" + lang + "\"");
                    continue;
                }
                if (strings_[StringHash(lang)][StringHash(id)] != String::EMPTY)
                {
                    DV_LOGWARNING(
                            "Localization::LoadMultipleLanguageJSON(source): override translation, string ID=\"" + id +
                            "\", language=\"" + lang + "\"");
                }
                strings_[StringHash(lang)][StringHash(id)] = string;
                if (!languages_.Contains(lang))
                    languages_.Push(lang);
                if (languageIndex_ == -1)
                    languageIndex_ = 0;
            }
        }
        else
            DV_LOGWARNING("Localization::LoadMultipleLanguageJSON(source): failed to load values, string ID=\"" + id + "\"");
    }
}

void Localization::LoadSingleLanguageJSON(const JSONValue& source, const String& language)
{
    for (JSONObject::ConstIterator i = source.Begin(); i != source.End(); ++i)
    {
        String id = i->first_;
        if (id.Empty())
        {
            DV_LOGWARNING("Localization::LoadSingleLanguageJSON(source, language): string ID is empty");
            continue;
        }
        const JSONValue& value = i->second_;
        if (value.IsString())
        {
            if (value.GetString().Empty())
            {
                DV_LOGWARNING(
                        "Localization::LoadSingleLanguageJSON(source, language): translation is empty, string ID=\"" + id +
                        "\", language=\"" + language + "\"");
                continue;
            }
            if (strings_[StringHash(language)][StringHash(id)] != String::EMPTY)
            {
                DV_LOGWARNING(
                        "Localization::LoadSingleLanguageJSON(source, language): override translation, string ID=\"" + id +
                        "\", language=\"" + language + "\"");
            }
            strings_[StringHash(language)][StringHash(id)] = value.GetString();
            if (!languages_.Contains(language))
                languages_.Push(language);
        }
        else
            DV_LOGWARNING(
                    "Localization::LoadSingleLanguageJSON(source, language): failed to load value, string ID=\"" + id +
                    "\", language=\"" + language + "\"");
    }
}

}
