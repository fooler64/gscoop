// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ui/theme.h"

#include <QColor>

namespace Theme {

QColor blend(const QColor& a, const QColor& b, double t) {
    return QColor(
        int(a.red() + (b.red() - a.red()) * t),
        int(a.green() + (b.green() - a.green()) * t),
        int(a.blue() + (b.blue() - a.blue()) * t));
}

} // namespace Theme
