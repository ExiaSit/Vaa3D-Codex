#ifndef VAA3D_PROTOCOL_OPEN_SERVICE_H
#define VAA3D_PROTOCOL_OPEN_SERVICE_H
#include <QString>
class MainWindow;
class ProtocolOpenService;
QString protocolOpenDirectory();
ProtocolOpenService *startProtocolOpenService(MainWindow *window);
void protocolOpenServiceReady(ProtocolOpenService *service);
#endif
