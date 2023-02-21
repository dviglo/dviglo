// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

class Button;
class DropDownList;
class Font;
class LineEdit;
class ListView;
class ResourceCache;
class Text;
class UIElement;
class Window;
class XMLFile;

/// %File selector's list entry (file or directory).
struct FileSelectorEntry
{
    /// Name.
    String name_;
    /// Directory flag.
    bool directory_;
};

/// %File selector dialog.
class DV_API FileSelector : public Object
{
    DV_OBJECT(FileSelector, Object);

public:
    /// Construct.
    explicit FileSelector();
    /// Destruct.
    ~FileSelector() override;
    /// Register object factory.
    static void RegisterObject();

    /// Set fileselector UI style.
    void SetDefaultStyle(XMLFile* style);
    /// Set title text.
    void SetTitle(const String& text);
    /// Set button texts.
    void SetButtonTexts(const String& okText, const String& cancelText);
    /// Set current path.
    void SetPath(const String& path);
    /// Set current filename.
    void SetFileName(const String& fileName);
    /// Set filters.
    void SetFilters(const Vector<String>& filters, unsigned defaultIndex);
    /// Set directory selection mode. Default false.
    void SetDirectoryMode(bool enable);
    /// Update elements to layout properly. Call this after manually adjusting the sub-elements.
    void UpdateElements();

    /// Return the UI style file.
    XMLFile* GetDefaultStyle() const;

    /// Return fileselector window.
    Window* GetWindow() const { return window_; }

    /// Return window title text element.
    Text* GetTitleText() const { return titleText_; }

    /// Return file list.
    ListView* GetFileList() const { return fileList_; }

    /// Return path editor.
    LineEdit* GetPathEdit() const { return pathEdit_; }

    /// Return filename editor.
    LineEdit* GetFileNameEdit() const { return fileNameEdit_; }

    /// Return filter dropdown.
    DropDownList* GetFilterList() const { return filterList_; }

    /// Return OK button.
    Button* GetOKButton() const { return okButton_; }

    /// Return cancel button.
    Button* GetCancelButton() const { return cancelButton_; }

    /// Return close button.
    Button* GetCloseButton() const { return closeButton_; }

    /// Return window title.
    const String& GetTitle() const;

    /// Return current path.
    const String& GetPath() const { return path_; }

    /// Return current filename.
    const String& GetFileName() const;
    /// Return current filter.
    const String& GetFilter() const;
    /// Return current filter index.
    unsigned GetFilterIndex() const;

    /// Return directory mode flag.
    bool GetDirectoryMode() const { return directoryMode_; }

private:
    /// Set the text of an edit field and ignore the resulting event.
    void SetLineEditText(LineEdit* edit, const String& text);
    /// Refresh the directory listing.
    void RefreshFiles();
    /// Enter a directory or confirm a file. Return true if a directory entered.
    bool EnterFile();
    /// Handle filter changed.
    void HandleFilterChanged(StringHash eventType, VariantMap& eventData);
    /// Handle path edited.
    void HandlePathChanged(StringHash eventType, VariantMap& eventData);
    /// Handle file selected from the list.
    void HandleFileSelected(StringHash eventType, VariantMap& eventData);
    /// Handle file doubleclicked from the list (enter directory / OK the file selection).
    void HandleFileDoubleClicked(StringHash eventType, VariantMap& eventData);
    /// Handle file list key pressed.
    void HandleFileListKey(StringHash eventType, VariantMap& eventData);
    /// Handle OK button pressed.
    void HandleOKPressed(StringHash eventType, VariantMap& eventData);
    /// Handle cancel button pressed.
    void HandleCancelPressed(StringHash eventType, VariantMap& eventData);

    /// Fileselector window.
    SharedPtr<Window> window_;
    /// Title layout.
    UIElement* titleLayout;
    /// Window title text.
    Text* titleText_;
    /// File list.
    ListView* fileList_;
    /// Path editor.
    LineEdit* pathEdit_;
    /// Filename editor.
    LineEdit* fileNameEdit_;
    /// Filter dropdown.
    DropDownList* filterList_;
    /// OK button.
    Button* okButton_;
    /// OK button text.
    Text* okButtonText_;
    /// Cancel button.
    Button* cancelButton_;
    /// Cancel button text.
    Text* cancelButtonText_;
    /// Close button.
    Button* closeButton_;
    /// Filename and filter layout.
    UIElement* fileNameLayout_;
    /// Separator layout.
    UIElement* separatorLayout_;
    /// Button layout.
    UIElement* buttonLayout_;
    /// Current directory.
    String path_;
    /// Filters.
    Vector<String> filters_;
    /// File entries.
    Vector<FileSelectorEntry> fileEntries_;
    /// Filter used to get the file list.
    String lastUsedFilter_;
    /// Directory mode flag.
    bool directoryMode_;
    /// Ignore events flag, used when changing line edits manually.
    bool ignoreEvents_;
};

}
