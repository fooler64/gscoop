/****************************************************************************
** Meta object code from reading C++ file 'scoop_service.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/core/scoop_service.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scoop_service.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN12ScoopServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto ScoopService::qt_create_metaobjectdata<qt_meta_tag_ZN12ScoopServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ScoopService",
        "scoopDetected",
        "",
        "installed",
        "installedPackagesLoaded",
        "QList<InstalledPackage>",
        "packages",
        "searchResultsReady",
        "QList<ScoopPackage>",
        "isCold",
        "bucketsLoaded",
        "QList<BucketInfo>",
        "buckets",
        "opStarted",
        "ScoopOpType",
        "type",
        "package",
        "opProgress",
        "ScoopOpProgress",
        "progress",
        "opFinished",
        "success",
        "error",
        "errorOccurred",
        "message",
        "onProcessOutput",
        "onProcessFinished",
        "exitCode",
        "onScanFinished"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'scoopDetected'
        QtMocHelpers::SignalData<void(bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Signal 'installedPackagesLoaded'
        QtMocHelpers::SignalData<void(QVector<InstalledPackage>)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Signal 'searchResultsReady'
        QtMocHelpers::SignalData<void(QVector<ScoopPackage>, bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 6 }, { QMetaType::Bool, 9 },
        }}),
        // Signal 'bucketsLoaded'
        QtMocHelpers::SignalData<void(QVector<BucketInfo>)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Signal 'opStarted'
        QtMocHelpers::SignalData<void(ScoopOpType, const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 15 }, { QMetaType::QString, 16 },
        }}),
        // Signal 'opProgress'
        QtMocHelpers::SignalData<void(const ScoopOpProgress &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 19 },
        }}),
        // Signal 'opFinished'
        QtMocHelpers::SignalData<void(ScoopOpType, const QString &, bool, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 15 }, { QMetaType::QString, 16 }, { QMetaType::Bool, 21 }, { QMetaType::QString, 22 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'onProcessOutput'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Slot 'onScanFinished'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ScoopService, qt_meta_tag_ZN12ScoopServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ScoopService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ScoopServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ScoopServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12ScoopServiceE_t>.metaTypes,
    nullptr
} };

void ScoopService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ScoopService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->scoopDetected((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->installedPackagesLoaded((*reinterpret_cast<std::add_pointer_t<QList<InstalledPackage>>>(_a[1]))); break;
        case 2: _t->searchResultsReady((*reinterpret_cast<std::add_pointer_t<QList<ScoopPackage>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 3: _t->bucketsLoaded((*reinterpret_cast<std::add_pointer_t<QList<BucketInfo>>>(_a[1]))); break;
        case 4: _t->opStarted((*reinterpret_cast<std::add_pointer_t<ScoopOpType>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->opProgress((*reinterpret_cast<std::add_pointer_t<ScoopOpProgress>>(_a[1]))); break;
        case 6: _t->opFinished((*reinterpret_cast<std::add_pointer_t<ScoopOpType>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 7: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->onProcessOutput(); break;
        case 9: _t->onProcessFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->onScanFinished(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<InstalledPackage> >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<ScoopPackage> >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<BucketInfo> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(bool )>(_a, &ScoopService::scoopDetected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(QVector<InstalledPackage> )>(_a, &ScoopService::installedPackagesLoaded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(QVector<ScoopPackage> , bool )>(_a, &ScoopService::searchResultsReady, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(QVector<BucketInfo> )>(_a, &ScoopService::bucketsLoaded, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(ScoopOpType , const QString & )>(_a, &ScoopService::opStarted, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(const ScoopOpProgress & )>(_a, &ScoopService::opProgress, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(ScoopOpType , const QString & , bool , const QString & )>(_a, &ScoopService::opFinished, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ScoopService::*)(const QString & )>(_a, &ScoopService::errorOccurred, 7))
            return;
    }
}

const QMetaObject *ScoopService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScoopService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ScoopServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ScoopService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void ScoopService::scoopDetected(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ScoopService::installedPackagesLoaded(QVector<InstalledPackage> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ScoopService::searchResultsReady(QVector<ScoopPackage> _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void ScoopService::bucketsLoaded(QVector<BucketInfo> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ScoopService::opStarted(ScoopOpType _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void ScoopService::opProgress(const ScoopOpProgress & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void ScoopService::opFinished(ScoopOpType _t1, const QString & _t2, bool _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 7
void ScoopService::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
