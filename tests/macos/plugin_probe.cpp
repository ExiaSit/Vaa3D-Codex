#include <QtWidgets>
#include <QPluginLoader>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "v3d_interface.h"
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc != 2) return 2;
    QPluginLoader loader(QString::fromLocal8Bit(argv[1]));
    QObject *instance = loader.instance();
    QJsonObject result;
    result["library"] = QString::fromLocal8Bit(argv[1]);
    if (!instance) {
        result["error"] = loader.errorString();
        puts(QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
        return 1;
    }
    V3DPluginInterface2_1 *plugin = qobject_cast<V3DPluginInterface2_1 *>(instance);
    V3DPluginInterface2 *older = qobject_cast<V3DPluginInterface2 *>(instance);
    V3DPluginInterface *legacy = qobject_cast<V3DPluginInterface *>(instance);
    if (!plugin && !older && !legacy) return 3;
    const QStringList menus = plugin ? plugin->menulist() : older ? older->menulist() : legacy->menulist();
    result["menus"] = QJsonArray::fromStringList(menus);
    if (plugin) result["functions"] = QJsonArray::fromStringList(plugin->funclist());
    else if (older) result["functions"] = QJsonArray::fromStringList(older->funclist());
    puts(QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
    return menus.isEmpty() ? 4 : 0;
}
