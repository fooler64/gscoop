#pragma once

#include <QIcon>
#include <QColor>

class QPainter;
class QRect;

// 自绘图标系统：所有图标用 QPainter 矢量绘制，不依赖 emoji/图片资源
// 图标颜色跟随主题（传入 QColor），尺寸可缩放
namespace IconPainter {

// ---- 通用图标（单色，可传任意颜色）----
QIcon lock(const QColor& color, int size = 16);          // 锁（已 hold）
QIcon lockOpen(const QColor& color, int size = 16);      // 开锁（未 hold）
QIcon trash(const QColor& color, int size = 16);         // 垃圾桶（删除/卸载）
QIcon refresh(const QColor& color, int size = 16);       // 刷新（循环箭头）
QIcon updateAll(const QColor& color, int size = 16);     // 更新全部（向下箭头+下划线）
QIcon search(const QColor& color, int size = 16);        // 搜索（放大镜）
QIcon plus(const QColor& color, int size = 16);          // 加号（添加）
QIcon close(const QColor& color, int size = 16);         // 关闭 X
QIcon warning(const QColor& color, int size = 16);       // 警告三角
QIcon check(const QColor& color, int size = 16);         // 对勾（已安装/完成）
QIcon star(const QColor& color, int size = 16);          // 星标（stars）
QIcon folder(const QColor& color, int size = 16);        // 文件夹（bucket）
QIcon globe(const QColor& color, int size = 16);         // 地球（探索/网络）
QIcon bug(const QColor& color, int size = 16);           // bug（doctor/自检）
QIcon broom(const QColor& color, int size = 16);         // 扫帚（清理）
QIcon shield(const QColor& color, int size = 16);        // 盾牌（virustotal/安全）
QIcon info(const QColor& color, int size = 16);          // 信息 i
QIcon downArrow(const QColor& c, int size);       // 向下箭头（更新可用标记）
QIcon upArrow(const QColor& c, int size);         // 向上箭头
QIcon download(const QColor& c, int size);        // 下载（箭头+托盘线）
QIcon minimize(const QColor& c, int size);        // 最小化（横线）
QIcon maximize(const QColor& c, int size);        // 最大化（方框）
QIcon restore(const QColor& c, int size);         // 还原（双框）
QIcon gear(const QColor& c, int size);            // 齿轮（设置）

// 可更新徽标：小三角+数字（绘制在卡片右上）
void paintUpdateBadge(QPainter& p, const QRect& rect, const QColor& color);

} // namespace IconPainter
