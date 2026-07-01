#ifndef GRID_H
#define GRID_H

#include <QPointF>
#include <QPoint>
#include <cmath>

namespace Grid {
    constexpr int kSize = 40;

    inline QPointF toPixel(int x, int y) {
        return QPointF(x * kSize, y * kSize);
    }

    inline QPoint toGrid(const QPointF &pixelPos) {
        int x = static_cast<int>(std::round(pixelPos.x() / kSize));
        int y = static_cast<int>(std::round(pixelPos.y() / kSize));
        return QPoint(x, y);
    }
}

#endif
