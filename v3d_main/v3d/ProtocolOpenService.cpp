#include "../3drenderer/v3dr_common.h"
#include "ProtocolOpenService.h"
#if defined(USE_Qt5) && defined(Q_OS_UNIX)
#include "OpenFileSet.h"
#include "mainwindow.h"
#include "../3drenderer/v3dr_mainwindow.h"
#include "../3drenderer/v3dr_glwidget.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QTimer>
#include <QRegularExpression>
#include <memory>
#ifdef Q_OS_UNIX
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

QString protocolOpenDirectory()
{
#ifdef Q_OS_UNIX
    auto privateDir=[](QString path){struct stat s;
        return !::lstat(QFile::encodeName(path).constData(),&s)&&S_ISDIR(s.st_mode)&&s.st_uid==::getuid()&&!(s.st_mode&0077);};
    QString base=QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));
    if(!QDir::isAbsolutePath(base)||!privateDir(base))base="/tmp";
    QString key=QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath()+"\n"
        +QString::fromLocal8Bit(qgetenv("DISPLAY"))+"|"+QString::fromLocal8Bit(qgetenv("WAYLAND_DISPLAY"));
    QString hash=QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(),QCryptographicHash::Sha256).toHex().left(16));
    QString folder=base+QString("/vaa3d-open-%1-%2").arg(::getuid()).arg(hash);
    ::mkdir(QFile::encodeName(folder).constData(),0700);
    return privateDir(folder)?folder:QString();
#else
    return QString();
#endif
}

class ProtocolOpenService : public QObject
{
    QPointer<MainWindow> window;
    QLocalServer server;
    QTimer timer;
    QQueue<QStringList> pending;
    QQueue<QString> ids;
    QSet<QString> known;
    bool ready=false,processing=false;
    void touch() {
#ifdef Q_OS_UNIX
        ::utimensat(AT_FDCWD,QFile::encodeName(server.fullServerName()).constData(),nullptr,0);
#endif
    }
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()==QEvent::WindowActivate && (object==window||qobject_cast<V3dR_MainWindow*>(object)))touch();
        return false;
    }
    bool validate(QStringList args,QString &error) {
        if(!args.isEmpty()&&args.first()=="--open-set")return validateFileSetArguments(args.mid(1),&error);
        if(args.size()==1) {
            QFileInfo file(args.first());
            if(file.isAbsolute()&&file.isFile()&&file.isReadable()
                &&QStringList({"tif","tiff","v3dpbd","swc","eswc"}).contains(file.suffix().toLower())) {
                for(QChar c:args.first())if(c.unicode()<32||c.unicode()==127){error="Invalid path";return false;}
                return true;
            }
        }
        error=QString::fromUtf8("无效的本机打开请求。");return false;
    }
    void acceptMessage(QLocalSocket *socket,QByteArray line) {
        QJsonParseError parse;
        auto document=QJsonDocument::fromJson(line,&parse);auto message=document.object();
        QString error,id=message.value("id").toString();QStringList args;
        auto array=message.value("args").toArray();
        bool valid=parse.error==QJsonParseError::NoError&&document.isObject()
            &&message.value("version").toInt()==1&&QRegularExpression("^[a-f0-9]{32}$").match(id).hasMatch()
            &&array.size()>0&&array.size()<=1024;
        for(auto value:array){if(!value.isString()){valid=false;break;}args<<value.toString();}
        if(valid&&!known.contains(id)) {
            valid=validate(args,error);
            if(valid&&pending.size()>=64){valid=false;error=QString::fromUtf8("打开请求队列已满，请稍后重试。");}
            if(valid){known.insert(id);ids.enqueue(id);if(ids.size()>256)known.remove(ids.dequeue());pending.enqueue(args);}
        }
        QJsonObject reply;reply["status"]=valid?"accepted":"rejected";
        reply["pid"]=double(QCoreApplication::applicationPid());
        if(!valid)reply["error"]=error.isEmpty()?QString("Invalid protocol message"):error;
        socket->write(QJsonDocument(reply).toJson(QJsonDocument::Compact)+"\n");socket->disconnectFromServer();
        if(valid){touch();qInfo("PROTOCOL_OPEN_ACCEPTED pid=%lld",QCoreApplication::applicationPid());}
    }
    void dispatch() {
        if(!ready||processing||!window||pending.isEmpty()||QApplication::activeModalWidget())return;
        // Never re-enter loading through nested progress events, and let a
        // previous file set finish its background Mask queue first.
        for(auto widget:QApplication::topLevelWidgets())if(auto win=qobject_cast<V3dR_MainWindow*>(widget)) {
            auto view=win->getGLWidget();if(!view)continue;
            QString state=view->property("fileSetState").toString();
            if(state=="waiting"||state=="loading"||view->property("maskImportRunning").toBool())return;
        }
        processing=true;auto args=pending.dequeue();QPointer<ProtocolOpenService> self=this;
        window->setWindowState(window->windowState()&~Qt::WindowMinimized);window->show();window->raise();window->activateWindow();
        qInfo("PROTOCOL_OPEN_DISPATCH pid=%lld",QCoreApplication::applicationPid());
        QString error;
        if(validate(args,error)) {
            if(args.first()=="--open-set")openFileSet(window,args.mid(1),&error);
            else window->loadV3DFile(args.first(),true,false);
        }
        if(!self)return;
        if(!error.isEmpty()&&window){auto box=new QMessageBox(QMessageBox::Warning,QString::fromUtf8("链接加载失败"),error,QMessageBox::Ok,window);
            box->setAttribute(Qt::WA_DeleteOnClose);box->setWindowModality(Qt::WindowModal);box->open();}
        processing=false;
    }
public:
    explicit ProtocolOpenService(MainWindow *win):QObject(win),window(win) {
        const QString folder=protocolOpenDirectory();
        if(folder.isEmpty()){qWarning("Cannot create private Vaa3D protocol directory");return;}
        const QString endpoint=folder+QString("/%1.sock").arg(QCoreApplication::applicationPid());
        QLocalServer::removeServer(endpoint);server.setSocketOptions(QLocalServer::UserAccessOption);
        if(!server.listen(endpoint)){qWarning()<<"Protocol receiver:"<<server.errorString();return;}
        window->setProperty("protocolOpenEndpoint",endpoint);
        connect(&server,&QLocalServer::newConnection,this,[this](){
            while(auto socket=server.nextPendingConnection()) {
                auto buffer=std::make_shared<QByteArray>();
                connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
                auto read=[this,socket,buffer](){
                    if(socket->property("handled").toBool())return;
                    *buffer+=socket->readAll();
                    if(buffer->size()>1024*1024){socket->abort();return;}
                    int end=buffer->indexOf('\n');if(end<0)return;
                    socket->setProperty("handled",true);acceptMessage(socket,buffer->left(end));
                };
                connect(socket,&QLocalSocket::readyRead,this,read);
                if(socket->bytesAvailable())read();
                QTimer::singleShot(15000,socket,[socket](){socket->disconnectFromServer();});
            }
        });
        connect(&timer,&QTimer::timeout,this,[this](){dispatch();});timer.start(100);
        qApp->installEventFilter(this);
        qInfo()<<"PROTOCOL_OPEN_LISTEN"<<endpoint;
    }
    void setReady(){ready=true;}
};

ProtocolOpenService *startProtocolOpenService(MainWindow *window){return new ProtocolOpenService(window);}
void protocolOpenServiceReady(ProtocolOpenService *service){if(service)service->setReady();}
#else
QString protocolOpenDirectory(){return QString();}
ProtocolOpenService *startProtocolOpenService(MainWindow *){return nullptr;}
void protocolOpenServiceReady(ProtocolOpenService *){}
#endif
