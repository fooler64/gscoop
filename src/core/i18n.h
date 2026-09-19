// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>
#include <QTranslator>

// 多语言管理：加载/切换 .qm 翻译文件
// 翻译文件位于 <app>/i18n/gscoop_<lang>.qm
class I18n : public QObject {
    Q_OBJECT
public:
    static I18n& instance();

    // 应用语言（"zh-CN" = 原文中文，不加载翻译；"en-US" 加载英文）
    void applyLanguage(const QString& lang);
    QString currentLanguage() const { return m_lang; }

signals:
    void languageChanged();

private:
    explicit I18n(QObject* parent = nullptr);
    static QString i18nDir();

    QString m_lang = "zh-CN";
    QTranslator* m_translator = nullptr;
};
