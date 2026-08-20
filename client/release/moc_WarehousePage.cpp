/****************************************************************************
** Meta object code from reading C++ file 'WarehousePage.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../src/pages/WarehousePage.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WarehousePage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_WarehousePage_t {
    QByteArrayData data[21];
    char stringdata0[345];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WarehousePage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WarehousePage_t qt_meta_stringdata_WarehousePage = {
    {
QT_MOC_LITERAL(0, 0, 13), // "WarehousePage"
QT_MOC_LITERAL(1, 14, 13), // "onPartsSearch"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 12), // "onPartsIssue"
QT_MOC_LITERAL(4, 42, 29), // "onIssueOrderSearchTextChanged"
QT_MOC_LITERAL(5, 72, 4), // "text"
QT_MOC_LITERAL(6, 77, 20), // "onBillingSearchOrder"
QT_MOC_LITERAL(7, 98, 31), // "onBillingOrderSearchTextChanged"
QT_MOC_LITERAL(8, 130, 16), // "onCompareAndBill"
QT_MOC_LITERAL(9, 147, 12), // "onCancelBill"
QT_MOC_LITERAL(10, 160, 16), // "onPurchaseSearch"
QT_MOC_LITERAL(11, 177, 17), // "onPurchaseAddItem"
QT_MOC_LITERAL(12, 195, 20), // "onPurchaseRemoveItem"
QT_MOC_LITERAL(13, 216, 17), // "onPurchaseConfirm"
QT_MOC_LITERAL(14, 234, 13), // "onStockSearch"
QT_MOC_LITERAL(15, 248, 14), // "onReturnSearch"
QT_MOC_LITERAL(16, 263, 15), // "onReturnConfirm"
QT_MOC_LITERAL(17, 279, 22), // "onPurchaseReturnSearch"
QT_MOC_LITERAL(18, 302, 23), // "onPurchaseReturnConfirm"
QT_MOC_LITERAL(19, 326, 12), // "onTabChanged"
QT_MOC_LITERAL(20, 339, 5) // "index"

    },
    "WarehousePage\0onPartsSearch\0\0onPartsIssue\0"
    "onIssueOrderSearchTextChanged\0text\0"
    "onBillingSearchOrder\0"
    "onBillingOrderSearchTextChanged\0"
    "onCompareAndBill\0onCancelBill\0"
    "onPurchaseSearch\0onPurchaseAddItem\0"
    "onPurchaseRemoveItem\0onPurchaseConfirm\0"
    "onStockSearch\0onReturnSearch\0"
    "onReturnConfirm\0onPurchaseReturnSearch\0"
    "onPurchaseReturnConfirm\0onTabChanged\0"
    "index"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WarehousePage[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   99,    2, 0x08 /* Private */,
       3,    0,  100,    2, 0x08 /* Private */,
       4,    1,  101,    2, 0x08 /* Private */,
       6,    0,  104,    2, 0x08 /* Private */,
       7,    1,  105,    2, 0x08 /* Private */,
       8,    0,  108,    2, 0x08 /* Private */,
       9,    0,  109,    2, 0x08 /* Private */,
      10,    0,  110,    2, 0x08 /* Private */,
      11,    0,  111,    2, 0x08 /* Private */,
      12,    0,  112,    2, 0x08 /* Private */,
      13,    0,  113,    2, 0x08 /* Private */,
      14,    0,  114,    2, 0x08 /* Private */,
      15,    0,  115,    2, 0x08 /* Private */,
      16,    0,  116,    2, 0x08 /* Private */,
      17,    0,  117,    2, 0x08 /* Private */,
      18,    0,  118,    2, 0x08 /* Private */,
      19,    1,  119,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   20,

       0        // eod
};

void WarehousePage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WarehousePage *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onPartsSearch(); break;
        case 1: _t->onPartsIssue(); break;
        case 2: _t->onIssueOrderSearchTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->onBillingSearchOrder(); break;
        case 4: _t->onBillingOrderSearchTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->onCompareAndBill(); break;
        case 6: _t->onCancelBill(); break;
        case 7: _t->onPurchaseSearch(); break;
        case 8: _t->onPurchaseAddItem(); break;
        case 9: _t->onPurchaseRemoveItem(); break;
        case 10: _t->onPurchaseConfirm(); break;
        case 11: _t->onStockSearch(); break;
        case 12: _t->onReturnSearch(); break;
        case 13: _t->onReturnConfirm(); break;
        case 14: _t->onPurchaseReturnSearch(); break;
        case 15: _t->onPurchaseReturnConfirm(); break;
        case 16: _t->onTabChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject WarehousePage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_WarehousePage.data,
    qt_meta_data_WarehousePage,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *WarehousePage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WarehousePage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WarehousePage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int WarehousePage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 17;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
