/****************************************************************************
** Meta object code from reading C++ file 'CustomerVisitPage.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../src/pages/CustomerVisitPage.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CustomerVisitPage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CustomerVisitPage_t {
    QByteArrayData data[8];
    char stringdata0[104];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CustomerVisitPage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CustomerVisitPage_t qt_meta_stringdata_CustomerVisitPage = {
    {
QT_MOC_LITERAL(0, 0, 17), // "CustomerVisitPage"
QT_MOC_LITERAL(1, 18, 19), // "onLoadSettledOrders"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 13), // "onSelectOrder"
QT_MOC_LITERAL(4, 53, 11), // "QModelIndex"
QT_MOC_LITERAL(5, 65, 5), // "index"
QT_MOC_LITERAL(6, 71, 20), // "onOrderDoubleClicked"
QT_MOC_LITERAL(7, 92, 11) // "onSaveVisit"

    },
    "CustomerVisitPage\0onLoadSettledOrders\0"
    "\0onSelectOrder\0QModelIndex\0index\0"
    "onOrderDoubleClicked\0onSaveVisit"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CustomerVisitPage[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x08 /* Private */,
       3,    1,   35,    2, 0x08 /* Private */,
       6,    1,   38,    2, 0x08 /* Private */,
       7,    0,   41,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void,

       0        // eod
};

void CustomerVisitPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CustomerVisitPage *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onLoadSettledOrders(); break;
        case 1: _t->onSelectOrder((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 2: _t->onOrderDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->onSaveVisit(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CustomerVisitPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CustomerVisitPage.data,
    qt_meta_data_CustomerVisitPage,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CustomerVisitPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CustomerVisitPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CustomerVisitPage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CustomerVisitPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
