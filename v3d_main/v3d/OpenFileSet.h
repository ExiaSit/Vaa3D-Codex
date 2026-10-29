#ifndef VAA3D_OPEN_FILE_SET_H
#define VAA3D_OPEN_FILE_SET_H

#include <QStringList>
class MainWindow;

// Arguments: --image <path> [--swc <path>]... [--mask <path>]...
//            [--mesh-density <1..1000>]
bool openFileSet(MainWindow *window, const QStringList &arguments, QString *error);

#endif
