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

#pragma once
#include "Widget.h"

namespace open3d {
namespace gui {

struct Margins {
    int left;
    int top;
    int right;
    int bottom;

    // Margins are specified in pixels, which are not the same size on all
    // monitors. It is best to use a multiple of
    // Window::GetTheme().fontSize to specify margins. Theme::fontSize,
    // represents 1em and is scaled according to the scaling factor of the
    // window. For example, 0.5em (that is, 0.5 * theme.fontSize) is typically
    // a good size for a margin.
    Margins();  // all values zero
    Margins(int px);
    Margins(int horizPx, int vertPx);
    Margins(int leftPx, int topPx, int rightPx, int bottomPx);

    int GetHoriz() const;
    int GetVert() const;
};

class Layout1D : public Widget {
    using Super = Widget;

public:
    enum Dir { VERT, HORIZ };

    Layout1D(Dir dir,
             int spacing,
             const Margins& margins,
             const std::vector<std::shared_ptr<Widget>>& children);
    virtual ~Layout1D();

    Size CalcPreferredSize(const Theme& theme) const override;
    void Layout(const Theme& theme) override;

    void AddFixed(int size);
    void AddStretch();

public:
    class Fixed : public Widget {
    public:
        Fixed(int size, Dir dir);
        Size CalcPreferredSize(const Theme& theme) const override;

    private:
        int size_;
        Dir dir_;
    };

    class Stretch : public Widget {
        Size CalcPreferredSize(const Theme& theme) const override;
    };

protected:
    Margins& GetMutableMargins();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class Vert : public Layout1D {
public:
    static std::shared_ptr<Layout1D::Fixed> MakeFixed(int size);
    static std::shared_ptr<Layout1D::Stretch> MakeStretch();

    Vert();
    // Spacing is in pixels; see the comment in Margin(). 1em is typically
    // a good value for spacing.
    Vert(int spacing, const Margins& margins = Margins());
    Vert(int spacing,
         const Margins& margins,
         const std::vector<std::shared_ptr<Widget>>& children);
    virtual ~Vert();
};

class CollapsableVert : public Vert {
    using Super = Vert;

public:
    CollapsableVert(const char* text);
    CollapsableVert(const char* text,
                    int spacing,
                    const Margins& margins = Margins());
    virtual ~CollapsableVert();

    // You will need to make sure the window knows it needs to layout.
    // (If you call this before the widnows is displayed everything
    // will work out fine, as layout will automatically be called when
    // the window is shown.)
    void SetIsOpen(bool isOpen);

    Size CalcPreferredSize(const Theme& theme) const override;
    void Layout(const Theme& theme) override;
    Widget::DrawResult Draw(const DrawContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class Horiz : public Layout1D {
public:
    static std::shared_ptr<Layout1D::Fixed> MakeFixed(int size);
    static std::shared_ptr<Layout1D::Stretch> MakeStretch();
    static std::shared_ptr<Horiz> MakeCentered(std::shared_ptr<Widget> w);

    Horiz();
    // Spacing is in pixels; see the comment in Margin(). 1em is typically
    // a good value for spacing.
    Horiz(int spacing, const Margins& margins = Margins());
    Horiz(int spacing,
          const Margins& margins,
          const std::vector<std::shared_ptr<Widget>>& children);
    ~Horiz();
};

class VGrid : public Widget {
    using Super = Widget;

public:
    VGrid(int nCols, int spacing = 0, const Margins& margins = Margins());
    virtual ~VGrid();

    Size CalcPreferredSize(const Theme& theme) const override;
    void Layout(const Theme& theme) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace gui
}  // namespace open3d
