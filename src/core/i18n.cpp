// SPDX-License-Identifier: LGPL-3.0-or-later
#include "core/i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>

I18n& I18n::instance() {
    static I18n inst;
    return inst;
}

I18n::I18n(QObject* parent) : QObject(parent) {}

QString I18n::i18nDir() {
    const QString appDir = QCoreApplication::applicationDirPath();
    QDir d1(appDir + "/i18n");
    if (d1.exists()) return d1.absolutePath();
    QDir d2(QDir::currentPath() + "/i18n");
    if (d2.exists()) return d2.absolutePath();
    return appDir + "/i18n";
}

void I18n::applyLanguage(const QString& lang) {
    m_lang = lang;

    // 移除旧的翻译器
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    // 中文是源语言，无需翻译文件
    if (lang.startsWith("zh", Qt::CaseInsensitive)) {
        emit languageChanged();
        return;
    }

    const QString path = i18nDir() + "/gscoop_" + lang.section('-', 0, 0) + ".qm";
    m_translator = new QTranslator(this);
    if (m_translator->load(path)) {
        QCoreApplication::installTranslator(m_translator);
    }
    emit languageChanged();
}
