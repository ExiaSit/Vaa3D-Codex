#include "../3drenderer/v3dr_common.h"
#include "OpenFileSet.h"
#include "mainwindow.h"
#include "../3drenderer/v3dr_mainwindow.h"
#include "../3drenderer/v3dr_glwidget.h"
#include "../3drenderer/renderer_gl1.h"
#include <QFileInfo>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QMessageBox>

namespace {
struct Request {
    QString image;
    QStringList neurons, masks;
    int density=100;
};

bool parse(const QStringList &args, Request &out, QString *error)
{
    auto fail=[&](QString text){if(error)*error=text;return false;};
    QSet<QString> paths;bool haveDensity=false;
    if(args.size()%2)return fail(QString::fromUtf8("文件组参数必须是角色和路径成对传入。"));
    for(int i=0;i<args.size();i+=2) {
        const QString role=args[i],value=args[i+1];
        if(role=="--mesh-density") {
            bool ok=false;int n=value.toInt(&ok);
            if(haveDensity||!ok||n<1||n>1000)return fail("Invalid mesh density (1..1000)");
            haveDensity=true;out.density=n;continue;
        }
        const QFileInfo file(value);const QString ext=file.suffix().toLower();
        bool valid=role=="--image" ? QStringList({"tif","tiff","v3dpbd"}).contains(ext)
            :role=="--swc" ? QStringList({"swc","eswc"}).contains(ext)
            :role=="--mask" && QStringList({"tif","tiff"}).contains(ext);
        if(!valid || !file.isAbsolute() || !file.isFile() || !file.isReadable())
            return fail(QString::fromUtf8("文件角色、类型或路径无效：\n")+role+" "+value);
        for(QChar c:value)if(c.unicode()<32||c.unicode()==127)return fail("Invalid control character in path");
        const QString canonical=file.canonicalFilePath();
        if(paths.contains(canonical))return fail(QString::fromUtf8("文件组包含重复路径：\n")+value);
        paths.insert(canonical);
        if(role=="--image") {
            if(!out.image.isEmpty())return fail(QString::fromUtf8("一组文件只能有一个原图。"));
            out.image=value;
        } else if(role=="--swc")out.neurons<<value;
        else out.masks<<value;
    }
    if(out.image.isEmpty())return fail(QString::fromUtf8("文件组缺少 image 原图。"));
    return true;
}

// Lifetime is tied to this viewer, including while background Mask work is active.
class ImportQueue : public QObject {
    QPointer<V3dR_GLWidget> view;
    Request request;
    int neuron=0,mask=0;
    bool started=false,done=false;
    QString activeMask;
    void finish(QString error=QString()) {
        if(done)return;done=true;
        if(view) {
            view->setProperty("fileSetState",error.isEmpty()?"complete":"failed");
            view->setProperty("fileSetError",error);
            if(error.isEmpty()) {
                view->surfaceSelectDialog(request.masks.isEmpty()?stNeuronStructure-1:stLabelSurface-1);
                qInfo("FILE_SET_READY: neurons=%d masks=%d",request.neurons.size(),request.masks.size());
            } else {
                qWarning()<<"FILE_SET_FAILED:"<<error;
                auto message=new QMessageBox(QMessageBox::Warning,QString::fromUtf8("文件组加载中止"),
                    error+QString::fromUtf8("\n已加载的图像和对象仍保留，后续文件未继续加载。"),QMessageBox::Ok,view->getMainWindow());
                message->setAttribute(Qt::WA_DeleteOnClose);message->setWindowModality(Qt::WindowModal);message->open();
            }
        }
        deleteLater();
    }
    void next() {
        if(done||!view)return;
        auto renderer=dynamic_cast<Renderer_gl1*>(view->getRenderer());
        if(!renderer){finish("3D renderer is unavailable");return;}
        if(neuron<request.neurons.size()) {
            const QString file=request.neurons[neuron++];
            for(const auto &tree:renderer->listNeuronTree)
                if(!tree.listNeuron.isEmpty()&&QFileInfo(tree.file).canonicalFilePath()==QFileInfo(file).canonicalFilePath()) {
                    QTimer::singleShot(0,this,[this](){next();});return;
                }
            const int count=renderer->listNeuronTree.size();
            QPointer<ImportQueue> self=this;
            view->loadObjectFromFile(file);
            if(!self||!view||done)return;
            if(view->getRenderer()!=renderer){finish("3D renderer changed during import");return;}
            if(renderer->listNeuronTree.size()!=count+1 || renderer->listNeuronTree.last().listNeuron.isEmpty()) {
                finish(QString::fromUtf8("无法加载神经元：\n")+file);return;
            }
            if(request.neurons.size()>1)renderer->assignDistinctNeuronColors(count);
            QTimer::singleShot(0,this,[this](){next();});return;
        }
        if(mask<request.masks.size()) {
            if(view->property("maskImportRunning").toBool()){finish("Another mask import is active");return;}
            activeMask=request.masks[mask++];
            for(const auto &surface:renderer->listLabelSurf)
                if(QFileInfo(surface.sourceFile).canonicalFilePath()==QFileInfo(activeMask).canonicalFilePath()) {
                    activeMask.clear();QTimer::singleShot(0,this,[this](){next();});return;
                }
            view->setProperty("fileSetMaskIndex",mask);
            renderer->loadTiffMaskAsync(activeMask,0,false,request.density);
            if(!view||!view->property("maskImportRunning").toBool())finish("Cannot start mask import");
            return;
        }
        finish();
    }
public:
    ImportQueue(V3dR_GLWidget *v,const Request &r):QObject(v),view(v),request(r) {
        v->setProperty("fileSetState","waiting");
        connect(v,&V3dR_GLWidget::viewerReady,this,[this](){start();});
        connect(v,&V3dR_GLWidget::rendererAboutToBeDeleted,this,[this](){
            if(done)return;done=true;
            if(view){view->setProperty("fileSetState","canceled");view->setProperty("fileSetError","3D viewer closed or rebuilt");}
            deleteLater();
        });
        connect(v,&V3dR_GLWidget::maskImportFinished,this,[this](QString file,bool ok,QString error){
            if(done||file!=activeMask)return;
            activeMask.clear();
            if(!ok){finish(file+"\n"+error);return;}
            QTimer::singleShot(0,this,[this](){next();});
        });
        if(v->property("viewerReady").toBool())QTimer::singleShot(0,this,[this](){start();});
    }
    void start() {
        if(started||done||!view)return;started=true;
        view->setProperty("fileSetState","loading");
        QTimer::singleShot(0,this,[this](){next();});
    }
};
}

bool validateFileSetArguments(const QStringList &arguments,QString *error)
{
    Request request;return parse(arguments,request,error);
}

bool openFileSet(MainWindow *window,const QStringList &arguments,QString *error)
{
    Request request;
    if(!parse(arguments,request,error))return false;
    QPointer<MainWindow> owner=window;
    window->loadV3DFile(request.image,true,true);
    if(!owner){if(error)*error="Main window closed during image load";return false;}
    auto child=window->findMdiChild(request.image);
    if(child&&child->getImageData()&&!child->mypara_3Dview.window3D) {
        // The explicit queue imports requested Masks without the remembered
        // last-mask file reopening its interactive options dialog first.
        child->mypara_3Dview.labelfield_file.clear();child->doImage3DView();
    }
    if(!child||!child->getImageData()||!child->mypara_3Dview.window3D) {
        if(error)*error=QString::fromUtf8("原图加载失败或取消，未导入任何叠加对象。");return false;
    }
    auto view=child->mypara_3Dview.window3D->getGLWidget();
    if(!view || view->property("fileSetState").toString()=="waiting" || view->property("fileSetState").toString()=="loading") {
        if(error)*error="The viewer is unavailable or already loading a file set";return false;
    }
    child->mypara_3Dview.window3D->raise_and_activate();
    new ImportQueue(view,request);
    return true;
}
