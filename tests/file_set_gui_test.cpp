#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/v3dr_glwidget.h"
#include "../v3d_main/3drenderer/v3dr_surfaceDialog.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include <tiffio.h>
#include <QTemporaryDir>
#include <QDataStream>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>
#include <cassert>
#include <functional>

static void waitFor(std::function<bool()> condition) {
    QElapsedTimer clock;clock.start();
    while(!condition()){assert(clock.elapsed()<60000);qApp->processEvents(QEventLoop::AllEvents,10);QThread::msleep(1);}
}
static void tiff(QString path,bool mask=false,int side=32) {
    auto f=TIFFOpen(QFile::encodeName(path).constData(),"w");assert(f);
    for(int z=0;z<side;++z) {
        TIFFSetField(f,TIFFTAG_IMAGEWIDTH,side);TIFFSetField(f,TIFFTAG_IMAGELENGTH,side);
        TIFFSetField(f,TIFFTAG_BITSPERSAMPLE,8);TIFFSetField(f,TIFFTAG_SAMPLESPERPIXEL,1);
        TIFFSetField(f,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_MINISBLACK);TIFFSetField(f,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
        QByteArray row(side,0);
        for(int y=0;y<side;++y) {
            for(int x=0;x<side;++x)row[x]=mask?(x>3&&x<28&&y>3&&y<28&&z>3&&z<28?(x<16?1:2):0):77;
            assert(TIFFWriteScanline(f,row.data(),y,0)>=0);
        }
        assert(TIFFWriteDirectory(f));
    }
    TIFFClose(f);
}
static QByteArray fileHash(QString path) {
    QFile file(path);assert(file.open(QIODevice::ReadOnly));return QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256);
}
static V3dR_GLWidget *view(MainWindow &mw,QString path) {
    auto image=mw.findMdiChild(path);assert(image&&image->mypara_3Dview.window3D);
    return image->mypara_3Dview.window3D->getGLWidget();
}
static QTableWidget *table(V3dR_GLWidget *v,int type) {
    auto manager=v->getMainWindow()->findChild<V3dr_surfaceDialog*>();assert(manager);
    auto tabs=manager->findChild<QTabWidget*>();assert(tabs);
    auto t=qobject_cast<QTableWidget*>(tabs->widget(type-1));assert(t);return t;
}
int main(int argc,char **argv) {
    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.show();
    QTemporaryDir dir;assert(dir.isValid());
    QString folder=dir.path()+QString::fromUtf8("/43199 中文 +#&%");assert(QDir().mkpath(folder));
    QString image=folder+"/43199.tif",imageB=folder+"/43199.v3dpbd";
    QString mask=folder+"/43199_mask1.tif",maskB=folder+"/43199_mask2.tif",wrong=folder+"/wrong.tif";
    QString swc=folder+"/43199_a.swc",eswc=folder+"/43199_b.eswc";
    tiff(image);tiff(mask,true);tiff(maskB,true);tiff(wrong,true,16);
    for(auto path:{swc,eswc}){QFile f(path);assert(f.open(QIODevice::WriteOnly));f.write("1 1 10 10 10 1 -1\n2 3 20 20 20 1 1\n");}
    {QFile f(imageB);assert(f.open(QIODevice::WriteOnly));QDataStream s(&f);s.setByteOrder(QDataStream::LittleEndian);
        QByteArray key="v3d_volume_pkbitdf_encod";s.writeRawData(key.constData(),key.size());s.writeRawData("L",1);
        s<<quint16(1)<<quint32(32)<<quint32(32)<<quint32(32)<<quint32(1);
        QByteArray block(32,77);for(int i=0;i<1024;++i){s<<quint8(31);s.writeRawData(block.constData(),32);}}
    QStringList paths{image,imageB,mask,maskB,swc,eswc};QList<QByteArray> before;for(auto p:paths)before<<fileHash(p);
    QString error;
    assert(!openFileSet(&mw,{"--image",image,"--mask",folder+"/missing.tif"},&error));assert(!mw.findMdiChild(image));
    assert(!openFileSet(&mw,{"--image",image,"--image",imageB},&error));
    assert(!openFileSet(&mw,{"--image",image,"--mask",image},&error));
    assert(openFileSet(&mw,{"--mask",mask,"--swc",swc,"--image",image,"--mask",maskB,"--swc",eswc,"--mesh-density","30"},&error));
    assert(openFileSet(&mw,{"--image",imageB,"--swc",swc,"--mask",mask,"--mesh-density","25"},&error));
    auto a=view(mw,image),b=view(mw,imageB);
    int beats=0;QTimer heartbeat;QObject::connect(&heartbeat,&QTimer::timeout,[&](){++beats;});heartbeat.start(10);
    waitFor([&](){return a->property("fileSetState")=="complete"&&b->property("fileSetState")=="complete";});heartbeat.stop();assert(beats>0);
    auto ra=dynamic_cast<Renderer_gl1*>(a->getRenderer());auto rb=dynamic_cast<Renderer_gl1*>(b->getRenderer());assert(ra&&rb);
    assert(ra->listNeuronTree.size()==2&&rb->listNeuronTree.size()==1);
    assert(ra->listLabelSurf.size()==4&&rb->listLabelSurf.size()==2);
    for(int i=0;i<4;++i){assert(ra->listLabelSurf[i].sourceFile==(i<2?mask:maskB));assert(!ra->listLabelSurf[i].name.isEmpty());}
    assert(table(a,stNeuronStructure)->rowCount()==2&&table(a,stLabelSurface)->rowCount()==4);
    assert(table(b,stLabelSurface)->rowCount()==2);
    table(a,stLabelSurface)->item(0,0)->setCheckState(Qt::Unchecked);assert(!ra->listLabelSurf[0].on);
    assert(ra->listLabelSurf[1].on&&ra->listLabelSurf[2].on&&rb->listLabelSurf[0].on);
    table(a,stNeuronStructure)->item(0,0)->setCheckState(Qt::Unchecked);assert(!ra->listNeuronTree[0].on&&ra->listNeuronTree[1].on&&rb->listNeuronTree[0].on);
    assert(openFileSet(&mw,{"--image",image,"--swc",swc,"--swc",eswc,"--mask",mask,"--mask",maskB},&error));
    waitFor([&](){return a->property("fileSetState")=="complete";});
    assert(ra->listNeuronTree.size()==2&&ra->listLabelSurf.size()==4);
    assert(!ra->listNeuronTree[0].on&&!ra->listLabelSurf[0].on);
    // Verify queue stops on dimensions failure, retains existing objects, and no later mask starts.
    assert(openFileSet(&mw,{"--image",image,"--mask",wrong,"--mask",maskB,"--mesh-density","20"},&error));
    waitFor([&](){return a->property("fileSetState")=="failed";});
    assert(a->property("fileSetError").toString().contains("dimensions"));assert(a->property("fileSetMaskIndex").toInt()==1);
    assert(ra->listLabelSurf.size()==4&&ra->listNeuronTree.size()==2);
    for(auto w:QApplication::topLevelWidgets())if(auto message=qobject_cast<QMessageBox*>(w))message->accept();
    // Cancellation also stops the remaining queue; empty and invalid partial sets never report complete.
    assert(openFileSet(&mw,{"--image",imageB,"--mask",maskB,"--mask",wrong,"--mesh-density","500"},&error));
    waitFor([&](){return b->property("maskImportRunning").toBool();});
    auto progress=b->getMainWindow()->findChild<QProgressDialog*>("maskImportProgress");assert(progress);
    auto cancel=progress->findChild<QPushButton*>();assert(cancel);cancel->click();
    waitFor([&](){return b->property("fileSetState")=="failed";});
    assert(b->property("fileSetMaskIndex").toInt()==1&&rb->listLabelSurf.size()==2);
    for(auto w:QApplication::topLevelWidgets())if(auto message=qobject_cast<QMessageBox*>(w))message->accept();
    for(int i=0;i<paths.size();++i)assert(before[i]==fileHash(paths[i]));
    // Closing a viewer while a set is active is safe and stops the rest of its queue.
    assert(openFileSet(&mw,{"--image",imageB,"--mask",maskB,"--mesh-density","500"},&error));
    waitFor([&](){return b->property("maskImportRunning").toBool();});
    b->getMainWindow()->close();qApp->processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    assert(openFileSet(&mw,{"--image",imageB,"--swc",swc,"--mask",mask,"--mesh-density","25"},&error));
    auto reopened=view(mw,imageB);
    waitFor([&](){return reopened->property("fileSetState")=="complete";});
    auto rr=dynamic_cast<Renderer_gl1*>(reopened->getRenderer());assert(rr&&rr->listNeuronTree.size()==1&&rr->listLabelSurf.size()==2);
    qInfo("FILE_SET_GUI_PASS: image/SWC/ESWC/multiple masks, TIFF/PBD, manager visibility/source isolation, failure/cancel/close, unchanged files");
    return 0;
}
