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

#include "Button.h"

#include "Theme.h"
#include "Util.h"

#include <imgui.h>

#include <cmath>
#include <string>

namespace open3d {
namespace gui {

struct Button::Impl {
    std::string title;
    bool isToggleable = false;
    bool isOn = false;
    std::function<void()> onClicked;
};

Button::Button(const char* title) : impl_(new Button::Impl()) {
    impl_->title = title;
}

Button::~Button() {}

bool Button::GetIsToggleable() const { return impl_->isToggleable; }

void Button::SetToggleable(bool toggles) { impl_->isToggleable = toggles; }

bool Button::GetIsOn() const { return impl_->isOn; }

void Button::SetOn(bool isOn) {
    if (impl_->isToggleable) {
        impl_->isOn = isOn;
    }
}

void Button::SetOnClicked(std::function<void()> onClicked) {
    impl_->onClicked = onClicked;
}

Size Button::CalcPreferredSize(const Theme& theme) const {
    auto font = ImGui::GetFont();
    auto em = std::ceil(ImGui::GetTextLineHeight());
    auto size = font->CalcTextSizeA(theme.fontSize, 10000, 10000,
                                    impl_->title.c_str());
    return Size(std::ceil(size.x) + 2.0 * em, 2 * em);
}

Widget::DrawResult Button::Draw(const DrawContext& context) {
    auto& frame = GetFrame();
    auto result = Widget::DrawResult::NONE;

    bool oldIsOn = impl_->isOn;
    if (oldIsOn) {
        ImGui::PushStyleColor(
                ImGuiCol_Text,
                util::colorToImgui(context.theme.buttonOnTextColor));
        ImGui::PushStyleColor(ImGuiCol_Button,
                              util::colorToImgui(context.theme.buttonOnColor));
        ImGui::PushStyleColor(
                ImGuiCol_ButtonHovered,
                util::colorToImgui(context.theme.buttonOnHoverColor));
        ImGui::PushStyleColor(
                ImGuiCol_ButtonActive,
                util::colorToImgui(context.theme.buttonOnActiveColor));
    }
    DrawImGuiPushEnabledState();
    ImGui::SetCursorPos(
            ImVec2(frame.x - context.uiOffsetX, frame.y - context.uiOffsetY));
    if (ImGui::Button(impl_->title.c_str(),
                      ImVec2(GetFrame().width, GetFrame().height))) {
        if (impl_->isToggleable) {
            impl_->isOn = !impl_->isOn;
        }
        if (impl_->onClicked) {
            impl_->onClicked();
        }
        result = Widget::DrawResult::REDRAW;
    }
    DrawImGuiPopEnabledState();
    if (oldIsOn) {
        ImGui::PopStyleColor(4);
    }

    return result;
}

}  // namespace gui
}  // namespace open3d
