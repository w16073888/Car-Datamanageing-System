/****************************************************************************
** Meta object code from reading C++ file 'FrontDeskPage.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../src/pages/FrontDeskPage.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FrontDeskPage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FrontDeskPage_t {
    QByteArrayData data[24];
    char stringdata0[363];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FrontDeskPage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FrontDeskPage_t qt_meta_stringdata_FrontDeskPage = {
    {
QT_MOC_LITERAL(0, 0, 13), // "FrontDeskPage"
QT_MOC_LITERAL(1, 14, 16), // "workOrderCreated"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 11), // "workorderId"
QT_MOC_LITERAL(4, 44, 7), // "orderNo"
QT_MOC_LITERAL(5, 52, 14), // "orderNoChanged"
QT_MOC_LITERAL(6, 67, 19), // "onVehicleLiveSearch"
QT_MOC_LITERAL(7, 87, 23), // "onVehicleSearchFinalize"
QT_MOC_LITERAL(8, 111, 13), // "onLockVehicle"
QT_MOC_LITERAL(9, 125, 14), // "onClearVehicle"
QT_MOC_LITERAL(10, 140, 12), // "onFeeChanged"
QT_MOC_LITERAL(11, 153, 17), // "onCreateWorkOrder"
QT_MOC_LITERAL(12, 171, 16), // "onPrintWorkOrder"
QT_MOC_LITERAL(13, 188, 12), // "onPrintQuote"
QT_MOC_LITERAL(14, 201, 17), // "onPrintSettlement"
QT_MOC_LITERAL(15, 219, 24), // "onShowMaintenanceHistory"
QT_MOC_LITERAL(16, 244, 16), // "onExportQuotePdf"
QT_MOC_LITERAL(17, 261, 12), // "onSaveNewCar"
QT_MOC_LITERAL(18, 274, 14), // "onCancelNewCar"
QT_MOC_LITERAL(19, 289, 16), // "onCancelDispatch"
QT_MOC_LITERAL(20, 306, 17), // "onSaveVehicleInfo"
QT_MOC_LITERAL(21, 324, 23), // "onPartSearchTextChanged"
QT_MOC_LITERAL(22, 348, 4), // "text"
QT_MOC_LITERAL(23, 353, 9) // "onAddPart"

    },
    "FrontDeskPage\0workOrderCreated\0\0"
    "workorderId\0orderNo\0orderNoChanged\0"
    "onVehicleLiveSearch\0onVehicleSearchFinalize\0"
    "onLockVehicle\0onClearVehicle\0onFeeChanged\0"
    "onCreateWorkOrder\0onPrintWorkOrder\0"
    "onPrintQuote\0onPrintSettlement\0"
    "onShowMaintenanceHistory\0onExportQuotePdf\0"
    "onSaveNewCar\0onCancelNewCar\0"
    "onCancelDispatch\0onSaveVehicleInfo\0"
    "onPartSearchTextChanged\0text\0onAddPart"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FrontDeskPage[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,  109,    2, 0x06 /* Public */,
       5,    1,  114,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,  117,    2, 0x08 /* Private */,
       7,    0,  118,    2, 0x08 /* Private */,
       8,    0,  119,    2, 0x08 /* Private */,
       9,    0,  120,    2, 0x08 /* Private */,
      10,    0,  121,    2, 0x08 /* Private */,
      11,    0,  122,    2, 0x08 /* Private */,
      12,    0,  123,    2, 0x08 /* Private */,
      13,    0,  124,    2, 0x08 /* Private */,
      14,    0,  125,    2, 0x08 /* Private */,
      15,    0,  126,    2, 0x08 /* Private */,
      16,    0,  127,    2, 0x08 /* Private */,
      17,    0,  128,    2, 0x08 /* Private */,
      18,    0,  129,    2, 0x08 /* Private */,
      19,    0,  130,    2, 0x08 /* Private */,
      20,    0,  131,    2, 0x08 /* Private */,
      21,    1,  132,    2, 0x08 /* Private */,
      23,    0,  135,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString,    4,

 // slots: parameters
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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   22,
    QMetaType::Void,

       0        // eod
};

void FrontDeskPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FrontDeskPage *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->workOrderCreated((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->orderNoChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->onVehicleLiveSearch(); break;
        case 3: _t->onVehicleSearchFinalize(); break;
        case 4: _t->onLockVehicle(); break;
        case 5: _t->onClearVehicle(); break;
        case 6: _t->onFeeChanged(); break;
        case 7: _t->onCreateWorkOrder(); break;
        case 8: _t->onPrintWorkOrder(); break;
        case 9: _t->onPrintQuote(); break;
        case 10: _t->onPrintSettlement(); break;
        case 11: _t->onShowMaintenanceHistory(); break;
        case 12: _t->onExportQuotePdf(); break;
        case 13: _t->onSaveNewCar(); break;
        case 14: _t->onCancelNewCar(); break;
        case 15: _t->onCancelDispatch(); break;
        case 16: _t->onSaveVehicleInfo(); break;
        case 17: _t->onPartSearchTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 18: _t->onAddPart(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FrontDeskPage::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FrontDeskPage::workOrderCreated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FrontDeskPage::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FrontDeskPage::orderNoChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject FrontDeskPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_FrontDeskPage.data,
    qt_meta_data_FrontDeskPage,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *FrontDeskPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FrontDeskPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FrontDeskPage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int FrontDeskPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void FrontDeskPage::workOrderCreated(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FrontDeskPage::orderNoChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
