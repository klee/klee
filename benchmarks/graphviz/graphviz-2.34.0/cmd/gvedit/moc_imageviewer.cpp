/****************************************************************************
** Meta object code from reading C++ file 'imageviewer.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "imageviewer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'imageviewer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ImageViewer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      27,   18,   13,   12, 0x0a,
      41,   12,   12,   12, 0x0a,
      49,   12,   12,   12, 0x0a,
      58,   12,   12,   12, 0x0a,
      68,   12,   12,   12, 0x0a,
      81,   12,   12,   12, 0x0a,
      95,   12,   12,   12, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ImageViewer[] = {
    "ImageViewer\0\0bool\0fileName\0open(QString)\0"
    "print()\0zoomIn()\0zoomOut()\0normalSize()\0"
    "fitToWindow()\0about()\0"
};

void ImageViewer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ImageViewer *_t = static_cast<ImageViewer *>(_o);
        switch (_id) {
        case 0: { bool _r = _t->open((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: _t->print(); break;
        case 2: _t->zoomIn(); break;
        case 3: _t->zoomOut(); break;
        case 4: _t->normalSize(); break;
        case 5: _t->fitToWindow(); break;
        case 6: _t->about(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ImageViewer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ImageViewer::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_ImageViewer,
      qt_meta_data_ImageViewer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ImageViewer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ImageViewer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ImageViewer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ImageViewer))
        return static_cast<void*>(const_cast< ImageViewer*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int ImageViewer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
