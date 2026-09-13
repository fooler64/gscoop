#include "ui/icon_painter.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>
#include <QtMath>
#include <functional>

// 通用辅助：把 QPainter 画笔设置成指定颜色
static void setupPen(QPainter& p, const QColor& c, qreal w = 1.6) {
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
}

// 计算与 c 对比的"前景/背景"色：深色图标内画白色，浅色图标内画深灰
static QColor contrastColor(const QColor& c) {
    const qreal lum = 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue();
    return (lum > 140) ? QColor(45, 60, 80) : QColor(255, 255, 255);
}

static QIcon makePixmap(int size, std::function<void(QPainter&, const QRectF&)> draw) {
    QPixmap pm(size * 2, size * 2);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.scale(2.0, 2.0);
    const QRectF r(0.5, 0.5, size - 1, size - 1);
    draw(p, r);
    p.end();
    return QIcon(pm);
}

namespace IconPainter {

QIcon lock(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        // 锁体
        QRectF body(r.left() + r.width() * 0.2, r.top() + r.height() * 0.45,
                    r.width() * 0.6, r.height() * 0.4);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(body, r.width() * 0.08, r.height() * 0.08);
        // 锁梁
        setupPen(p, c, r.width() * 0.09);
        p.drawArc(QRectF(r.left() + r.width() * 0.28, r.top() + r.height() * 0.12,
                         r.width() * 0.44, r.height() * 0.44),
                  0, 180 * 16);
        // 钥匙孔
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(QPointF(r.center().x(), body.center().y() - r.height() * 0.02),
                      r.width() * 0.07, r.height() * 0.09);
        p.drawRect(QRectF(r.center().x() - r.width() * 0.04, body.center().y(),
                          r.width() * 0.08, r.height() * 0.12));
    });
}

QIcon lockOpen(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        // 锁体
        QRectF body(r.left() + r.width() * 0.2, r.top() + r.height() * 0.45,
                    r.width() * 0.6, r.height() * 0.4);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(body, r.width() * 0.08, r.height() * 0.08);
        // 开锁梁（只画一半，向右开口）
        setupPen(p, c, r.width() * 0.09);
        p.drawArc(QRectF(r.left() + r.width() * 0.16, r.top() + r.height() * 0.12,
                         r.width() * 0.44, r.height() * 0.44),
                  0, 150 * 16);
        // 钥匙孔
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(QPointF(r.center().x(), body.center().y() - r.height() * 0.02),
                      r.width() * 0.07, r.height() * 0.09);
        p.drawRect(QRectF(r.center().x() - r.width() * 0.04, body.center().y(),
                          r.width() * 0.08, r.height() * 0.12));
    });
}

QIcon trash(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        // 盖子
        p.drawLine(QPointF(r.left() + r.width() * 0.25, r.top() + r.height() * 0.22),
                   QPointF(r.left() + r.width() * 0.75, r.top() + r.height() * 0.22));
        // 把手
        p.drawArc(QRectF(r.left() + r.width() * 0.4, r.top() + r.height() * 0.08,
                         r.width() * 0.2, r.height() * 0.2),
                  0, 180 * 16);
        // 桶身
        QPainterPath body;
        body.moveTo(r.left() + r.width() * 0.3, r.top() + r.height() * 0.28);
        body.lineTo(r.left() + r.width() * 0.35, r.top() + r.height() * 0.82);
        body.lineTo(r.left() + r.width() * 0.65, r.top() + r.height() * 0.82);
        body.lineTo(r.left() + r.width() * 0.7, r.top() + r.height() * 0.28);
        p.setBrush(Qt::NoBrush);
        p.drawPath(body);
        // 桶内线
        p.drawLine(QPointF(r.left() + r.width() * 0.43, r.top() + r.height() * 0.4),
                   QPointF(r.left() + r.width() * 0.47, r.top() + r.height() * 0.7));
        p.drawLine(QPointF(r.left() + r.width() * 0.57, r.top() + r.height() * 0.4),
                   QPointF(r.left() + r.width() * 0.53, r.top() + r.height() * 0.7));
    });
}

QIcon refresh(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        // 圆形箭头（画 300 度圆弧 + 箭头）
        p.drawArc(r.adjusted(r.width() * 0.12, r.height() * 0.12, -r.width() * 0.12, -r.height() * 0.12),
                  70 * 16, 260 * 16);
        // 箭头
        QPolygonF arrow;
        arrow << QPointF(r.left() + r.width() * 0.78, r.top() + r.height() * 0.16)
              << QPointF(r.left() + r.width() * 0.92, r.top() + r.height() * 0.32)
              << QPointF(r.left() + r.width() * 0.72, r.top() + r.height() * 0.38);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPolygon(arrow);
    });
}

QIcon updateAll(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        // 三条向下箭头 + 底线
        const qreal gap = r.width() * 0.13;
        for (int i = 0; i < 3; ++i) {
            const qreal cx = r.left() + r.width() * (0.24 + i * gap * 2);
            const qreal topY = r.top() + r.height() * 0.12;
            const qreal botY = r.top() + r.height() * 0.48;
            p.drawLine(QPointF(cx, topY), QPointF(cx, botY));
            // 箭头
            QPolygonF tri;
            tri << QPointF(cx - r.width() * 0.07, botY - r.height() * 0.08)
                << QPointF(cx + r.width() * 0.07, botY - r.height() * 0.08)
                << QPointF(cx, botY);
            p.setBrush(c);
            p.setPen(Qt::NoPen);
            p.drawPolygon(tri);
            p.setBrush(Qt::NoBrush);
            setupPen(p, c, r.width() * 0.09);
        }
        // 底线
        p.drawLine(QPointF(r.left() + r.width() * 0.15, r.top() + r.height() * 0.7),
                   QPointF(r.left() + r.width() * 0.85, r.top() + r.height() * 0.7));
    });
}

QIcon search(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        // 放大镜圆 + 把手
        p.drawEllipse(QRectF(r.left() + r.width() * 0.1, r.top() + r.height() * 0.1,
                             r.width() * 0.55, r.height() * 0.55));
        p.drawLine(QPointF(r.left() + r.width() * 0.55, r.top() + r.height() * 0.55),
                   QPointF(r.left() + r.width() * 0.88, r.top() + r.height() * 0.88));
    });
}

QIcon plus(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        p.drawLine(QPointF(r.center().x(), r.top() + r.height() * 0.2),
                   QPointF(r.center().x(), r.top() + r.height() * 0.8));
        p.drawLine(QPointF(r.left() + r.width() * 0.2, r.center().y()),
                   QPointF(r.left() + r.width() * 0.8, r.center().y()));
    });
}

QIcon close(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        p.drawLine(QPointF(r.left() + r.width() * 0.2, r.top() + r.height() * 0.2),
                   QPointF(r.left() + r.width() * 0.8, r.top() + r.height() * 0.8));
        p.drawLine(QPointF(r.left() + r.width() * 0.8, r.top() + r.height() * 0.2),
                   QPointF(r.left() + r.width() * 0.2, r.top() + r.height() * 0.8));
    });
}

QIcon warning(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        // 三角
        QPolygonF tri;
        tri << QPointF(r.center().x(), r.top() + r.height() * 0.06)
            << QPointF(r.left() + r.width() * 0.08, r.bottom() - r.height() * 0.06)
            << QPointF(r.right() - r.width() * 0.08, r.bottom() - r.height() * 0.06);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
        // 感叹号
        p.setPen(QPen(contrastColor(c), r.width() * 0.08));
        p.drawLine(QPointF(r.center().x(), r.top() + r.height() * 0.3),
                   QPointF(r.center().x(), r.top() + r.height() * 0.62));
        p.setBrush(contrastColor(c));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(r.center().x(), r.top() + r.height() * 0.78),
                      r.width() * 0.07, r.height() * 0.07);
    });
}

QIcon check(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.1);
        p.drawLine(QPointF(r.left() + r.width() * 0.15, r.center().y()),
                   QPointF(r.left() + r.width() * 0.42, r.top() + r.height() * 0.72));
        p.drawLine(QPointF(r.left() + r.width() * 0.42, r.top() + r.height() * 0.72),
                   QPointF(r.right() - r.width() * 0.15, r.top() + r.height() * 0.22));
    });
}

QIcon star(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        QPolygonF star;
        const QPointF ctr(r.center().x(), r.center().y() + r.height() * 0.05);
        const qreal R = r.width() * 0.42;
        const qreal rSmall = R * 0.45;
        for (int i = 0; i < 10; ++i) {
            const qreal ang = qDegreesToRadians(-90.0 + i * 36.0);
            const qreal rad = (i % 2 == 0) ? R : rSmall;
            star << QPointF(ctr.x() + rad * qCos(ang), ctr.y() + rad * qSin(ang));
        }
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPolygon(star);
    });
}

QIcon folder(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        // 文件夹背面
        QPainterPath back;
        back.moveTo(r.left() + r.width() * 0.08, r.top() + r.height() * 0.25);
        back.lineTo(r.left() + r.width() * 0.35, r.top() + r.height() * 0.25);
        back.lineTo(r.left() + r.width() * 0.47, r.top() + r.height() * 0.4);
        back.lineTo(r.right() - r.width() * 0.08, r.top() + r.height() * 0.4);
        back.lineTo(r.right() - r.width() * 0.08, r.bottom() - r.height() * 0.15);
        back.lineTo(r.left() + r.width() * 0.08, r.bottom() - r.height() * 0.15);
        back.closeSubpath();
        p.drawPath(back);
        // 正面
        QPainterPath front;
        front.moveTo(r.left() + r.width() * 0.08, r.top() + r.height() * 0.4);
        front.lineTo(r.right() - r.width() * 0.08, r.top() + r.height() * 0.4);
        front.lineTo(r.right() - r.width() * 0.08, r.bottom() - r.height() * 0.15);
        front.lineTo(r.left() + r.width() * 0.08, r.bottom() - r.height() * 0.15);
        front.closeSubpath();
        p.drawPath(front);
    });
}

QIcon globe(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.07);
        p.drawEllipse(r.adjusted(r.width() * 0.08, r.height() * 0.08, -r.width() * 0.08, -r.height() * 0.08));
        // 经线
        p.drawEllipse(QRectF(r.left() + r.width() * 0.32, r.top() + r.height() * 0.08,
                             r.width() * 0.36, r.height() * 0.84));
        // 纬线
        p.drawLine(QPointF(r.left() + r.width() * 0.1, r.center().y()),
                   QPointF(r.right() - r.width() * 0.1, r.center().y()));
        p.drawLine(QPointF(r.left() + r.width() * 0.2, r.top() + r.height() * 0.25),
                   QPointF(r.right() - r.width() * 0.2, r.top() + r.height() * 0.25));
        p.drawLine(QPointF(r.left() + r.width() * 0.2, r.bottom() - r.height() * 0.25),
                   QPointF(r.right() - r.width() * 0.2, r.bottom() - r.height() * 0.25));
    });
}

QIcon bug(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        const qreal w = r.width();
        const qreal h = r.height();
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        // 身体（椭圆）
        p.drawEllipse(QRectF(r.left() + w * 0.2, r.top() + h * 0.3, w * 0.6, h * 0.5));
        // 头
        p.drawEllipse(QRectF(r.left() + w * 0.38, r.top() + h * 0.12, w * 0.24, h * 0.2));
        // 触角
        setupPen(p, c, w * 0.06);
        p.drawLine(QPointF(r.left() + w * 0.42, r.top() + h * 0.12),
                   QPointF(r.left() + w * 0.3, r.top() + h * 0.02));
        p.drawLine(QPointF(r.left() + w * 0.58, r.top() + h * 0.12),
                   QPointF(r.left() + w * 0.7, r.top() + h * 0.02));
        // 腿
        for (int i = 0; i < 3; ++i) {
            const qreal yy = r.top() + h * (0.4 + i * 0.12);
            p.drawLine(QPointF(r.left() + w * 0.2, yy), QPointF(r.left() + w * 0.08, yy - h * 0.05));
            p.drawLine(QPointF(r.right() - w * 0.2, yy), QPointF(r.right() - w * 0.08, yy - h * 0.05));
        }
    });
}

QIcon broom(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.08);
        // 柄（斜线）
        p.drawLine(QPointF(r.left() + r.width() * 0.3, r.top() + r.height() * 0.1),
                   QPointF(r.left() + r.width() * 0.7, r.top() + r.height() * 0.75));
        // 刷毛（底部扇形线）
        for (int i = -2; i <= 2; ++i) {
            const qreal x = r.left() + r.width() * 0.5 + i * r.width() * 0.08;
            p.drawLine(QPointF(x, r.top() + r.height() * 0.7),
                       QPointF(x + i * r.width() * 0.03, r.bottom() - r.height() * 0.08));
        }
    });
}

QIcon shield(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        // 盾形
        QPainterPath shield;
        shield.moveTo(r.left() + r.width() * 0.5, r.top() + r.height() * 0.05);
        shield.lineTo(r.right() - r.width() * 0.12, r.top() + r.height() * 0.18);
        shield.lineTo(r.right() - r.width() * 0.12, r.center().y());
        shield.quadTo(r.right() - r.width() * 0.12, r.bottom() - r.height() * 0.1,
                      r.center().x(), r.bottom() - r.height() * 0.08);
        shield.quadTo(r.left() + r.width() * 0.12, r.bottom() - r.height() * 0.1,
                      r.left() + r.width() * 0.12, r.center().y());
        shield.lineTo(r.left() + r.width() * 0.12, r.top() + r.height() * 0.18);
        shield.closeSubpath();
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPath(shield);
        // 中间对勾
        p.setPen(QPen(contrastColor(c), r.width() * 0.07));
        p.drawLine(QPointF(r.left() + r.width() * 0.32, r.center().y()),
                   QPointF(r.left() + r.width() * 0.45, r.top() + r.height() * 0.62));
        p.drawLine(QPointF(r.left() + r.width() * 0.45, r.top() + r.height() * 0.62),
                   QPointF(r.right() - r.width() * 0.32, r.top() + r.height() * 0.35));
    });
}

QIcon info(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(r.adjusted(r.width() * 0.06, r.height() * 0.06, -r.width() * 0.06, -r.height() * 0.06));
        // i 字
        p.setPen(QPen(contrastColor(c), r.width() * 0.09));
        p.drawPoint(QPointF(r.center().x(), r.top() + r.height() * 0.3));
        p.drawLine(QPointF(r.center().x(), r.top() + r.height() * 0.45),
                   QPointF(r.center().x(), r.top() + r.height() * 0.75));
    });
}

QIcon downArrow(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        p.drawLine(QPointF(r.center().x(), r.top() + r.height() * 0.1),
                   QPointF(r.center().x(), r.top() + r.height() * 0.6));
        QPolygonF tri;
        tri << QPointF(r.left() + r.width() * 0.25, r.top() + r.height() * 0.5)
            << QPointF(r.right() - r.width() * 0.25, r.top() + r.height() * 0.5)
            << QPointF(r.center().x(), r.top() + r.height() * 0.78);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    });
}

QIcon upArrow(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        p.drawLine(QPointF(r.center().x(), r.top() + r.height() * 0.9),
                   QPointF(r.center().x(), r.top() + r.height() * 0.4));
        QPolygonF tri;
        tri << QPointF(r.left() + r.width() * 0.25, r.top() + r.height() * 0.5)
            << QPointF(r.right() - r.width() * 0.25, r.top() + r.height() * 0.5)
            << QPointF(r.center().x(), r.top() + r.height() * 0.22);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    });
}

QIcon download(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        setupPen(p, c, r.width() * 0.09);
        // 竖线（箭头杆）
        p.drawLine(QPointF(r.center().x(), r.top() + r.height() * 0.1),
                   QPointF(r.center().x(), r.top() + r.height() * 0.55));
        // 箭头三角
        QPolygonF tri;
        tri << QPointF(r.left() + r.width() * 0.2, r.top() + r.height() * 0.45)
            << QPointF(r.right() - r.width() * 0.2, r.top() + r.height() * 0.45)
            << QPointF(r.center().x(), r.top() + r.height() * 0.75);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
        // 托盘底线
        setupPen(p, c, r.width() * 0.09);
        p.drawLine(QPointF(r.left() + r.width() * 0.15, r.bottom() - r.height() * 0.12),
                   QPointF(r.right() - r.width() * 0.15, r.bottom() - r.height() * 0.12));
    });
}

QIcon gear(const QColor& c, int size) {
    return makePixmap(size, [&c](QPainter& p, const QRectF& r) {
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        // 齿轮：外圆 + 齿
        const QPointF ctr(r.center());
        p.drawEllipse(ctr, r.width() * 0.26, r.height() * 0.26);
        for (int i = 0; i < 8; ++i) {
            const qreal ang = qDegreesToRadians(i * 45.0);
            const qreal x1 = ctr.x() + r.width() * 0.3 * qCos(ang);
            const qreal y1 = ctr.y() + r.height() * 0.3 * qSin(ang);
            const qreal x2 = ctr.x() + r.width() * 0.42 * qCos(ang);
            const qreal y2 = ctr.y() + r.height() * 0.42 * qSin(ang);
            setupPen(p, c, r.width() * 0.11);
            p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }
        // 中心孔（背景色）
        p.setPen(Qt::NoPen);
        p.setBrush(contrastColor(c));
        p.drawEllipse(ctr, r.width() * 0.1, r.height() * 0.1);
    });
}

void paintUpdateBadge(QPainter& p, const QRect& rect, const QColor& color) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    // 小圆角矩形徽标
    p.drawRoundedRect(rect, 3, 3);
    // 内部向下箭头（背景色）
    p.setPen(QPen(contrastColor(color), 1.6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(rect.center().x(), rect.top() + rect.height() * 0.25),
               QPointF(rect.center().x(), rect.top() + rect.height() * 0.65));
    QPolygonF tri;
    tri << QPointF(rect.center().x() - rect.width() * 0.18, rect.top() + rect.height() * 0.6)
        << QPointF(rect.center().x() + rect.width() * 0.18, rect.top() + rect.height() * 0.6)
        << QPointF(rect.center().x(), rect.bottom() - rect.height() * 0.15);
    p.setBrush(contrastColor(color));
    p.drawPolygon(tri);
    p.restore();
}

} // namespace IconPainter
