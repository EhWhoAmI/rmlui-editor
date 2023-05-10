#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include "RmlUi_Backend.h"
#define IM_VEC2_CLASS_EXTRA
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "ImFileDialog.h"
#include "TextEditor.h"
#include <iostream>
#include <fstream>
#include <fmt/format.h>

bool ProcessKeyDownShortcuts(Rml::Context* context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority)
{
    if (!context)
        return true;

    // Result should return true to allow the event to propagate to the next handler.
    bool result = false;

    // This function is intended to be called twice by the backend, before and after submitting the key event to the context. This way we can
    // intercept shortcuts that should take priority over the context, and then handle any shortcuts of lower priority if the context did not
    // intercept it.
    if (priority)
    {
        // Priority shortcuts are handled before submitting the key to the context.

        // Toggle debugger and set dp-ratio using Ctrl +/-/0 keys.
        if (key == Rml::Input::KI_F8)
        {
            Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
        }
        else if (key == Rml::Input::KI_0 && key_modifier & Rml::Input::KM_CTRL)
        {
            context->SetDensityIndependentPixelRatio(native_dp_ratio);
        }
        else if (key == Rml::Input::KI_1 && key_modifier & Rml::Input::KM_CTRL)
        {
            context->SetDensityIndependentPixelRatio(1.f);
        }
        else if ((key == Rml::Input::KI_OEM_MINUS || key == Rml::Input::KI_SUBTRACT) && key_modifier & Rml::Input::KM_CTRL)
        {
            const float new_dp_ratio = Rml::Math::Max(context->GetDensityIndependentPixelRatio() / 1.2f, 0.5f);
            context->SetDensityIndependentPixelRatio(new_dp_ratio);
        }
        else if ((key == Rml::Input::KI_OEM_PLUS || key == Rml::Input::KI_ADD) && key_modifier & Rml::Input::KM_CTRL)
        {
            const float new_dp_ratio = Rml::Math::Min(context->GetDensityIndependentPixelRatio() * 1.2f, 2.5f);
            context->SetDensityIndependentPixelRatio(new_dp_ratio);
        }
        else
        {
            // Propagate the key down event to the context.
            result = true;
        }
    }
    else
    {
        // We arrive here when no priority keys are detected and the key was not consumed by the context. Check for shortcuts of lower priority.
        if (key == Rml::Input::KI_R && key_modifier & Rml::Input::KM_CTRL)
        {
            for (int i = 0; i < context->GetNumDocuments(); i++)
            {
                Rml::ElementDocument* document = context->GetDocument(i);
                const Rml::String& src = document->GetSourceURL();
                if (src.size() > 4 && src.substr(src.size() - 4) == ".rml")
                {
                    document->ReloadStyleSheet();
                }
            }
        }
        else
        {
            result = true;
        }
    }

    return result;
}

int main(int argc, char* argv[]) {
    int window_width = 1024;
    int window_height = 768;

    // Constructs the system and render interfaces, creates a window, and attaches the renderer.
    if (!Backend::Initialize("Template Tutorial", window_width, window_height, true))
    {
        return -1;
    }

    // Install the custom interfaces constructed by the backend before initializing RmlUi.
    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());
    // RmlUi initialisation.
    Rml::Initialise();

    // Create the main RmlUi context.
    Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
    if (!context)
    {
        Rml::Shutdown();
        Backend::Shutdown();
        return -1;
    }

    Rml::Debugger::Initialise(context);

    Rml::LoadFontFace("fonts/LatoLatin-Regular.ttf", true);
    struct Document {
        TextEditor text_editor;
        Rml::ElementDocument* doc;
        bool saved = true;
        std::string file_name;
    };
    std::vector<Document> text_editors;
    TextEditor editor;

    bool running = true;

    Rml::ElementDocument* doc = nullptr;
    while (running)
    {
        running = Backend::ProcessEvents(context, &ProcessKeyDownShortcuts, true);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New File")) {
                // then do an option window for rcss or something like that
            }
            if (ImGui::MenuItem("Open File")) {
                // Open the rmlui file
                ifd::FileDialog::Instance().Open("FileOpenDialog", "Open a file",
                    "RML/RCSS (*.rcss;*.rml){.rcss,.rml},.*");
            }
            if (ImGui::MenuItem("Save File")) {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();

        int count = 0;
        if (!text_editors.empty()) {
            ImGui::Begin("Main Window");
            if (ImGui::BeginTabBar("bartab")) {
                for (auto& doc : text_editors) {
                    std::string name = fmt::format("{}##{}", doc.file_name, count);
                    if (!doc.saved) {
                        name = fmt::format("{} *##{}", doc.file_name, count);
                    }
                    count++;
                    if (!ImGui::BeginTabItem(name.c_str())) {
                        continue;
                    }
                    doc.text_editor.Render(fmt::format("Editor##{}", count).c_str());
                    if (doc.text_editor.IsTextChanged()) {
                        doc.saved = false;
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }

        /*        ImGui::Begin("Test");
        if (ImGui::Button("Render")) {
            // Render the rmlui into a new document
            std::cout << editor.GetText() << "\n";
            doc = context->LoadDocumentFromMemory(editor.GetText());
            doc->Show();
        }
        editor.Render("TextEditor");
        ImGui::End();
        */

        if (ifd::FileDialog::Instance().IsDone("FileOpenDialog")) {
            if (ifd::FileDialog::Instance().HasResult()) {
                std::string res = ifd::FileDialog::Instance().GetResult().string();
                Document doc;
                // Read from file
                std::ifstream ifs(res);
                std::stringstream ss;
                ss << ifs.rdbuf();
                doc.text_editor.SetText(ss.str());
                doc.file_name = ifd::FileDialog::Instance().GetResult().filename().string();
                doc.doc = context->LoadDocumentFromMemory(ss.str());
                doc.doc->Show();
                // Open document
                text_editors.push_back(doc);
            }
            ifd::FileDialog::Instance().Close();
        }

        editor.GetText();
        // Edit the context and update?
        // Make the stuff
        context->Update();

        Backend::BeginFrame();
        context->Render();
        Backend::PresentFrame();
    }

    // Shutdown RmlUi.
    Rml::Shutdown();

    Backend::Shutdown();
}
