#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class PlotterCanvas : public UIWidget {
    public:
        PlotterCanvas(UIFactory& factory);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void setPlot(std::string_view str);

    private:
        UIFactory& factory;
        Sprite bg;
        Sprite circle;

        Vector<Vector<Vertex>> polygons;
    };
}
