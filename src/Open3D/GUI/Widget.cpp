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

#include "Widget.h"

#include "Color.h"
#include "Events.h"

namespace open3d {
namespace gui {

static const Color DEFAULT_BGCOLOR(0, 0, 0, 0);

struct Widget::Impl {
    Rect frame;
    Color bgColor = DEFAULT_BGCOLOR;
    std::vector<std::shared_ptr<Widget>> children;
};

Widget::Widget() : impl_(new Widget::Impl()) {}

Widget::Widget(const std::vector<std::shared_ptr<Widget>>& children)
    : impl_(new Widget::Impl()) {
    impl_->children = children;
}

Widget::~Widget() {}

void Widget::AddChild(std::shared_ptr<Widget> child) {
    impl_->children.push_back(child);
}

const std::vector<std::shared_ptr<Widget>> Widget::GetChildren() const {
    return impl_->children;
}

const Rect& Widget::GetFrame() const { return impl_->frame; }

void Widget::SetFrame(const Rect& f) { impl_->frame = f; }

const Color& Widget::GetBackgroundColor() const {
    return impl_->bgColor;
}

bool Widget::IsDefaultBackgroundColor() const {
    return (impl_->bgColor == DEFAULT_BGCOLOR);
}

void Widget::SetBackgroundColor(const Color& color) {
    impl_->bgColor = color;
}

Size Widget::CalcPreferredSize(const Theme&) const {
    return Size(DIM_GROW, DIM_GROW);
}

void Widget::Layout(const Theme& theme) {
    for (auto& child : impl_->children) {
        child->Layout(theme);
    }
}

Widget::DrawResult Widget::Draw(const DrawContext& context) {
    DrawResult result = DrawResult::NONE;
    for (auto& child : impl_->children) {
        auto r = child->Draw(context);
        // The mouse can only be over one item, so there should never
        // be multiple items returning non-NONE.
        if (r != DrawResult::NONE) {
            result = r;
        }
    }
    return result;
}

void Widget::Mouse(const MouseEvent& e) {
    // Iterate backwards so that we send mouse events from the top down.
    for (auto it = impl_->children.rbegin();
        it != impl_->children.rend();  ++it) {
        if ((*it)->GetFrame().Contains(e.x, e.y)) {
            (*it)->Mouse(e);
            break;
        }
    }
}

void Widget::Key(const KeyEvent& e) {
}

} // namespace gui
} // namespace open3d
