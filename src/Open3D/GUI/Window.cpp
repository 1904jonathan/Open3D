// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "Window.h"

#include "Application.h"
#include "ImguiFilamentBridge.h"
#include "Menu.h"
#include "Native.h"
#include "Renderer.h"
#include "Theme.h"
#include "Util.h"
#include "Widget.h"

#include "Open3D/Visualization/Rendering/Filament/FilamentRenderer.h"
#include "Open3D/Visualization/Rendering/Filament/FilamentEngine.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <SDL.h>
#include <filament/Engine.h>

#include <cmath>
#include <vector>

#ifdef WIN32
    #include <SDL_syswm.h>
#endif

using namespace open3d::gui::util;

// ----------------------------------------------------------------------------
namespace open3d {
namespace gui {

namespace {

// Assumes the correct ImGuiContext is current
void updateImGuiForScaling(float newScaling) {
    ImGuiStyle &style = ImGui::GetStyle();
    // FrameBorderSize is not adjusted (we want minimal borders)
    style.FrameRounding *= newScaling;
}

} // (anonymous)

struct Window::Impl
{
    SDL_Window *window = nullptr;
    Theme theme;  // so that the font size can be different based on scaling
    visualization::FilamentRenderer *renderer;
    struct {
        std::unique_ptr<ImguiFilamentBridge> imguiBridge = nullptr;
        ImGuiContext *context;
        ImFont *systemFont;  // is a reference; owned by imguiContext
        float scaling = 1.0;
    } imgui;
    std::shared_ptr<Menu> menubar;
    std::vector<std::shared_ptr<Widget>> children;
    Widget *focusWidget = nullptr; // only used if ImGUI isn't taking keystrokes
    bool needsLayout = true;
    int nSkippedFrames = 0;
};

Window::Window(const std::string& title, int width, int height)
: Window(title, -1, -1, width, height) {
}

Window::Window(const std::string& title, int x, int y, int width, int height)
: impl_(new Window::Impl()) {
    if (x < 0) {
        x = SDL_WINDOWPOS_CENTERED;
    }
    if (y < 0) {
        y = SDL_WINDOWPOS_CENTERED;
    }
    uint32_t flags = SDL_WINDOW_SHOWN |  // so SDL's context gets created
                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    impl_->window = SDL_CreateWindow(title.c_str(), x, y, width, height, flags);

    // On single-threaded platforms, Filament's OpenGL context must be current,
    // not SDL's context, so create the renderer after the window.

    // ImGUI creates a bitmap atlas from a font, so we need to have the correct
    // size when we create it, because we can't change the bitmap without
    // reloading the whole thing (expensive).
    float scaling = GetScaling();
    impl_->theme = Application::GetInstance().GetTheme();
    impl_->theme.fontSize *= scaling;
    impl_->theme.defaultMargin *= scaling;
    impl_->theme.defaultLayoutSpacing *= scaling;

    auto& engineInstance = visualization::EngineInstance::GetInstance();
    auto& resourceManager = visualization::EngineInstance::GetResourceManager();

    impl_->renderer = new visualization::FilamentRenderer(engineInstance, GetNativeDrawable(), resourceManager);

    auto &theme = impl_->theme;  // shorter alias
    impl_->imgui.context = ImGui::CreateContext();
    ImGui::SetCurrentContext(impl_->imgui.context);

    impl_->imgui.imguiBridge = std::make_unique<ImguiFilamentBridge>(impl_->renderer, GetSize());

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(0, 0);
    style.WindowRounding = 0;
    style.WindowBorderSize = 0;
    style.FrameBorderSize = theme.borderWidth;
    style.FrameRounding = theme.borderRadius;
    style.Colors[ImGuiCol_WindowBg] = colorToImgui(theme.backgroundColor);
    style.Colors[ImGuiCol_Text] = colorToImgui(theme.textColor);
    style.Colors[ImGuiCol_Border] = colorToImgui(theme.borderColor);
    style.Colors[ImGuiCol_Button] = colorToImgui(theme.buttonColor);
    style.Colors[ImGuiCol_ButtonHovered] = colorToImgui(theme.buttonHoverColor);
    style.Colors[ImGuiCol_ButtonActive] = colorToImgui(theme.buttonActiveColor);
    style.Colors[ImGuiCol_CheckMark] = colorToImgui(theme.checkboxCheckColor);
    style.Colors[ImGuiCol_FrameBg] = colorToImgui(theme.comboboxBackgroundColor);
    style.Colors[ImGuiCol_FrameBgHovered] = colorToImgui(theme.comboboxHoverColor);
    style.Colors[ImGuiCol_FrameBgActive] = style.Colors[ImGuiCol_FrameBgHovered];
    style.Colors[ImGuiCol_SliderGrab] = colorToImgui(theme.sliderGrabColor);
    style.Colors[ImGuiCol_SliderGrabActive] = colorToImgui(theme.sliderGrabColor);
    style.Colors[ImGuiCol_Tab] = colorToImgui(theme.tabInactiveColor);
    style.Colors[ImGuiCol_TabHovered] = colorToImgui(theme.tabHoverColor);
    style.Colors[ImGuiCol_TabActive] = colorToImgui(theme.tabActiveColor);

    // If the given font path is invalid, ImGui will silently fall back to
    // proggy, which is a tiny "pixel art" texture that is compiled into the
    // library.
    if (!theme.fontPath.empty()) {
        ImGuiIO &io = ImGui::GetIO();
        impl_->imgui.systemFont = io.Fonts->AddFontFromFileTTF(theme.fontPath.c_str(), theme.fontSize);
        /*static*/ unsigned char* pixels;
        int textureW, textureH, bytesPerPx;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &textureW, &textureH, &bytesPerPx);
        impl_->imgui.imguiBridge->createAtlasTextureAlpha8(pixels, textureW, textureH, bytesPerPx);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
#ifdef WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(impl_->window, &wmInfo);
    io.ImeWindowHandle = wmInfo.info.win.window;
#endif
    // ImGUI's io.KeysDown is indexed by our scan codes, and we fill out
    // io.KeyMap to map from our code to ImGui's code.
    io.KeyMap[ImGuiKey_Tab] = KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = KEY_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = KEY_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = KEY_HOME;
    io.KeyMap[ImGuiKey_End] = KEY_END;
    io.KeyMap[ImGuiKey_Insert] = KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = ' ';
    io.KeyMap[ImGuiKey_Enter] = KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'a';
    io.KeyMap[ImGuiKey_C] = 'c';
    io.KeyMap[ImGuiKey_V] = 'v';
    io.KeyMap[ImGuiKey_X] = 'x';
    io.KeyMap[ImGuiKey_Y] = 'y';
    io.KeyMap[ImGuiKey_Z] = 'z';
    io.SetClipboardTextFn = [](void*, const char* text) {
        SDL_SetClipboardText(text);
    };
    io.GetClipboardTextFn = [](void*) -> const char* {
        return SDL_GetClipboardText();
    };
    io.ClipboardUserData = nullptr;
}

Window::~Window() {
    impl_->children.clear();  // needs to happen before deleting renderer
    ImGui::SetCurrentContext(impl_->imgui.context);
    ImGui::DestroyContext();
    delete impl_->renderer;
    SDL_DestroyWindow(impl_->window);
}

void* Window::GetNativeDrawable() const {
    return open3d::gui::GetNativeDrawable(impl_->window);
}

uint32_t Window::GetID() const {
    return SDL_GetWindowID(impl_->window);
}

const Theme& Window::GetTheme() const {
    return impl_->theme;
}

visualization::AbstractRenderInterface& Window::GetRenderer() const {
    return *impl_->renderer;
}

Size Window::GetSize() const {
    uint32_t w, h;
    SDL_GL_GetDrawableSize(impl_->window, (int*) &w, (int*) &h);
    return Size(w, h);
}

Rect Window::GetContentRect() const {
    auto size = GetSize();
    int menuHeight = 0;
    ImGui::SetCurrentContext(impl_->imgui.context);
    if (impl_->menubar) {
        menuHeight = impl_->menubar->CalcHeight(GetTheme());
    }

    return Rect(0, menuHeight, size.width, size.height - menuHeight);
}

float Window::GetScaling() const {
    uint32_t wPx, hPx;
    SDL_GL_GetDrawableSize(impl_->window, (int*)&wPx, (int*)&hPx);
    int wVpx, hVpx;
    SDL_GetWindowSize(impl_->window, &wVpx, &hVpx);
    return (float(wPx) / float(wVpx));
}

Point Window::GlobalToWindowCoord(int globalX, int globalY) {
    int wx, wy;
    SDL_GetWindowPosition(impl_->window, &wx, &wy);
    return Point(globalX - wx, globalY - wy);
}

bool Window::IsVisible() const {
    return (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_SHOWN);
}

void Window::Show(bool vis /*= true*/) {
    if (vis) {
        SDL_ShowWindow(impl_->window);
    } else {
        SDL_HideWindow(impl_->window);
    }
}

void Window::Close() {
    Application::GetInstance().RemoveWindow(this);
}

std::shared_ptr<Menu> Window::GetMenubar() const {
    return impl_->menubar;
}

void Window::SetMenubar(std::shared_ptr<Menu> menu) {
    impl_->menubar = menu;
    impl_->needsLayout = true;  // in case wasn't a menu before
}

void Window::AddChild(std::shared_ptr<Widget> w) {
    impl_->children.push_back(w);
    impl_->needsLayout = true;
}

void Window::Layout(const Theme& theme) {
    if (impl_->children.size() == 1) {
        auto r = GetContentRect();
        impl_->children[0]->SetFrame(r);
    } else {
        for (auto &child : impl_->children) {
            child->Layout(theme);
        }
    }
}

Window::DrawResult Window::OnDraw(float dtSec) {
    // These are here to provide fast unique window names. If you find yourself
    // needing more than a handful, you should probably be using a container
    // of some sort (see Layout.h).
    static const char* winNames[] = { "win1", "win2", "win3", "win4", "win5",
                                      "win6", "win7", "win8", "win9", "win10",
                                      "win11", "win12", "win13", "win14", "win15",
                                      "win16", "win17", "win18", "win19", "win20" };

    impl_->renderer->BeginFrame();  // this can return false if Filament wants to skip a frame

    // Set current context
    ImGui::SetCurrentContext(impl_->imgui.context);
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = dtSec;

    // Set mouse information
    int mx, my;
    Uint32 buttons = SDL_GetGlobalMouseState(&mx, &my);
    auto mousePos = GlobalToWindowCoord(mx, my);
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseDown[0] = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    io.MouseDown[1] = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    if ((SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_INPUT_FOCUS) != 0) {
        auto scaling = GetScaling();
        io.MousePos = ImVec2((float)mousePos.x * scaling,
                             (float)mousePos.y * scaling);
    }

    // Set key information
    io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
    io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
    io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
    io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);

    // Begin ImGUI frame
    ImGui::NewFrame();
    ImGui::PushFont(impl_->imgui.systemFont);

    // Layout if necessary.  This must happen within ImGui setup so that widgets
    // can query font information.
    auto &theme = this->impl_->theme;
    if (this->impl_->needsLayout) {
        this->Layout(theme);
        // Clear needsLayout below
    }

    auto size = GetSize();
    int em = theme.fontSize;  // em = font size in digital type (from Wikipedia)
    DrawContext dc{ theme, 0, 0, size.width, size.height, em };

    bool needsRedraw = false;

    // Now draw all the 2D widgets. These will get recorded by ImGui.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse;
    int winIdx = 0;
    for (auto &child : this->impl_->children) {
        auto frame = child->GetFrame();
        bool bgColorNotDefault = !child->IsDefaultBackgroundColor();
        auto isContainer = !child->GetChildren().empty();
        if (isContainer) {
            dc.uiOffsetX = frame.x;
            dc.uiOffsetY = frame.y;
            ImGui::SetNextWindowPos(ImVec2(frame.x, frame.y));
            ImGui::SetNextWindowSize(ImVec2(frame.width, frame.height));
            if (bgColorNotDefault) {
                auto &bgColor = child->GetBackgroundColor();
                ImGui::PushStyleColor(ImGuiCol_WindowBg, util::colorToImgui(bgColor));
            }
            ImGui::Begin(winNames[winIdx++], nullptr, flags);
        } else {
            dc.uiOffsetX = 0;
            dc.uiOffsetY = 0;
        }

        if (child->Draw(dc) != Widget::DrawResult::NONE) {
            needsRedraw = true;
        }

        if (isContainer) {
            ImGui::End();
            if (bgColorNotDefault) {
                ImGui::PopStyleColor();
            }
        }
    }

    // Draw menubar last, so it is always on top (although it shouldn't matter,
    // as there shouldn't be anything under it)
    if (impl_->menubar) {
        auto id = impl_->menubar->DrawMenuBar(dc);
        if (id != Menu::NO_ITEM) {
            if (OnMenuItemSelected) {
                OnMenuItemSelected(id);
                needsRedraw = true;
            }
        }
    }

    // Finish frame and generate the commands
    ImGui::PopFont();
    ImGui::EndFrame();
    ImGui::Render(); // creates the draw data (i.e. Render()s to data)

    // Draw the ImGui commands
    impl_->imgui.imguiBridge->update(ImGui::GetDrawData());

    impl_->renderer->Draw();

    impl_->renderer->EndFrame();

    return (needsRedraw ? REDRAW : NONE);
}

Window::DrawResult Window::DrawOnce(float dtSec)
{
    auto needsRedraw = OnDraw(dtSec);

    // ImGUI can take two frames to do its layout, so if we did a layout
    // redraw a second time. This helps prevent a brief red flash when the
    // window first appears, as well as corrupted images if the
    // window appears underneath the mouse.
    if (impl_->needsLayout) {
        impl_->needsLayout = false;
        OnDraw(0.001);
    }

    return needsRedraw;
}

void Window::OnResize() {
    impl_->needsLayout = true;

    impl_->imgui.imguiBridge->onWindowResized(*this);

    auto size = GetSize();
    auto scaling = GetScaling();

    ImGui::SetCurrentContext(impl_->imgui.context);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(size.width, size.height);
    if (impl_->imgui.scaling != scaling) {
        updateImGuiForScaling(1.0 / impl_->imgui.scaling);  // undo previous
        updateImGuiForScaling(scaling);
        impl_->imgui.scaling = scaling;
    }
    io.DisplayFramebufferScale.x = 1.0f;
    io.DisplayFramebufferScale.y = 1.0f;
}

void Window::OnMouseEvent(const MouseEvent &e) {
    ImGui::SetCurrentContext(impl_->imgui.context);
    switch (e.type) {
        case MouseEvent::MOVE:
        case MouseEvent::BUTTON_DOWN:
        case MouseEvent::DRAG:
        case MouseEvent::BUTTON_UP:
            break;
        case MouseEvent::WHEEL: {
            ImGuiIO& io = ImGui::GetIO();
            io.MouseWheelH += (e.wheel.dx > 0 ? 1 : -1);
            io.MouseWheel  += (e.wheel.dy > 0 ? 1 : -1);
            break;
        }
    }
    // Iterate backwards so that we send mouse events from the top down.
    for (auto it = impl_->children.rbegin();
         it != impl_->children.rend();  ++it) {
        if ((*it)->GetFrame().Contains(e.x, e.y)) {
            if (e.type == MouseEvent::BUTTON_DOWN) {
                impl_->focusWidget = it->get();
            }
            (*it)->Mouse(e);
            break;
        }
    }
}

void Window::OnKeyEvent(const KeyEvent& e) {
    ImGuiIO& io = ImGui::GetIO();
    if (e.key < IM_ARRAYSIZE(io.KeysDown)) {
        io.KeysDown[e.key] = (e.type == KeyEvent::DOWN);
    }

    // If an ImGUI widget is not getting keystrokes, we can send them to
    // non-ImGUI widgets
    if (ImGui::GetCurrentContext()->ActiveId == 0 && impl_->focusWidget) {
        impl_->focusWidget->Key(e);
    }
}

void Window::OnTextInput(const TextInputEvent& e) {
    ImGui::SetCurrentContext(impl_->imgui.context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(e.utf8);
}

} // gui
} // opend3d
