/****************************************************************************
** Meta object code from reading C++ file 'csettings.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "csettings.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'csettings.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CFrmSettings[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x08,
      27,   13,   13,   13, 0x08,
      37,   13,   13,   13, 0x08,
      48,   13,   13,   13, 0x08,
      61,   13,   13,   13, 0x08,
      70,   13,   13,   13, 0x08,
      80,   13,   13,   13, 0x08,
      91,   13,   13,   13, 0x08,
     102,   13,   13,   13, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CFrmSettings[] = {
    "CFrmSettings\0\0outputSlot()\0addSlot()\0"
    "helpSlot()\0cancelSlot()\0okSlot()\0"
    "newSlot()\0openSlot()\0saveSlot()\0"
    "scopeChangedSlot(int)\0"
};

void CFrmSettings::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CFrmSettings *_t = static_cast<CFrmSettings *>(_o);
        switch (_id) {
        case 0: _t->outputSlot(); break;
        case 1: _t->addSlot(); break;
        case 2: _t->helpSlot(); break;
        case 3: _t->cancelSlot(); break;
        case 4: _t->okSlot(); break;
        case 5: _t->newSlot(); break;
        case 6: _t->openSlot(); break;
        case 7: _t->saveSlot(); break;
        case 8: _t->scopeChangedSlot((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CFrmSettings::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CFrmSettings::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_CFrmSettings,
      qt_meta_data_CFrmSettings, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CFrmSettings::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CFrmSettings::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CFrmSettings::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CFrmSettings))
        return static_cast<void*>(const_cast< CFrmSettings*>(this));
    return QDialog::qt_metacast(_clname);
}

int CFrmSettings::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
