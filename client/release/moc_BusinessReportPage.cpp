/****************************************************************************
** Meta object code from reading C++ file 'BusinessReportPage.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../src/pages/BusinessReportPage.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BusinessReportPage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BusinessReportPage_t {
    QByteArrayData data[11];
    char stringdata0[133];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BusinessReportPage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BusinessReportPage_t qt_meta_stringdata_BusinessReportPage = {
    {
QT_MOC_LITERAL(0, 0, 18), // "BusinessReportPage"
QT_MOC_LITERAL(1, 19, 18), // "onDateRangeChanged"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 5), // "start"
QT_MOC_LITERAL(4, 45, 3), // "end"
QT_MOC_LITERAL(5, 49, 12), // "onTabChanged"
QT_MOC_LITERAL(6, 62, 5), // "index"
QT_MOC_LITERAL(7, 68, 8), // "onExport"
QT_MOC_LITERAL(8, 77, 21), // "onSettleDoubleClicked"
QT_MOC_LITERAL(9, 99, 11), // "QModelIndex"
QT_MOC_LITERAL(10, 111, 21) // "onRepairDoubleClicked"

    },
    "BusinessReportPage\0onDateRangeChanged\0"
    "\0start\0end\0onTabChanged\0index\0onExport\0"
    "onSettleDoubleClicked\0QModelIndex\0"
    "onRepairDoubleClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BusinessReportPage[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x08 /* Private */,
       5,    1,   44,    2, 0x08 /* Private */,
       7,    0,   47,    2, 0x08 /* Private */,
       8,    1,   48,    2, 0x08 /* Private */,
      10,    1,   51,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QDate, QMetaType::QDate,    3,    4,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,    6,
    QMetaType::Void, 0x80000000 | 9,    6,

       0        // eod
};

void BusinessReportPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BusinessReportPage *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onDateRangeChanged((*reinterpret_cast< const QDate(*)>(_a[1])),(*reinterpret_cast< const QDate(*)>(_a[2]))); break;
        case 1: _t->onTabChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->onExport(); break;
        case 3: _t->onSettleDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 4: _t->onRepairDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BusinessReportPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_BusinessReportPage.data,
    qt_meta_data_BusinessReportPage,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BusinessReportPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BusinessReportPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BusinessReportPage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int BusinessReportPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
