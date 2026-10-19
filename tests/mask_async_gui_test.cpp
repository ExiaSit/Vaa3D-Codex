#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/v3dr_surfaceDialog.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include "../v3d_main/3drenderer/marchingcubes.h"
#include <tiffio.h>
#include <QTemporaryDir>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <cassert>
#include <functional>

static void waitFor(std::function<bool()> condition,int timeout=60000)
{
    QElapsedTimer time;time.start();
    while(!condition()) {assert(time.elapsed()<timeout);qApp->processEvents(QEventLoop::AllEvents,10);QThread::msleep(1);}
}
static void click(QWidget *button)
{
    QMouseEvent press(QEvent::MouseButtonPress,button->rect().center(),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,button->rect().center(),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(button,&press);QApplication::sendEvent(button,&release);
}
static void fixture(QString path)
{
    TIFF *t=TIFFOpen(QFile::encodeName(path).constData(),"w");assert(t);
    for(int z=0;z<32;++z)
    {
        TIFFSetField(t,TIFFTAG_IMAGEWIDTH,32);TIFFSetField(t,TIFFTAG_IMAGELENGTH,32);
        TIFFSetField(t,TIFFTAG_BITSPERSAMPLE,8);TIFFSetField(t,TIFFTAG_SAMPLESPERPIXEL,1);
        TIFFSetField(t,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_MINISBLACK);TIFFSetField(t,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
        unsigned char row[32];
        for(int y=0;y<32;++y){for(int x=0;x<32;++x)row[x]=(x>3&&x<28&&y>4&&y<27&&z>5&&z<26)?(x<16?1:2):0;assert(TIFFWriteScanline(t,row,y,0)>=0);}
        assert(TIFFWriteDirectory(t));
    }
    TIFFClose(t);
}
static void start(V3dR_GLWidget *view,QString path,int density,bool replace=false,int method=0)
{
    QTimer::singleShot(0,qApp,[=](){
        QDialog *d=nullptr;for(auto w:QApplication::topLevelWidgets())if(w->objectName()=="maskImportOptions")d=qobject_cast<QDialog*>(w);assert(d);
        d->findChild<QSpinBox*>("maskMeshDensity")->setValue(density);
        d->findChild<QCheckBox*>("maskReplaceSurfaces")->setChecked(replace);
        d->findChild<QComboBox*>("maskMeshMethod")->setCurrentIndex(method);d->accept();
    });
    view->loadObjectFromFile(path);assert(view->property("maskImportRunning").toBool());
}
static bool idle(V3dR_GLWidget *view){return !view->property("maskImportRunning").toBool();}
static void cancel(V3dR_MainWindow *window)
{
    auto p=window->findChild<QProgressDialog*>("maskImportProgress");assert(p);auto b=p->findChild<QPushButton*>();assert(b);click(b);
}
int main(int argc,char **argv)
{
    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.show();
    QTemporaryDir dir;assert(dir.isValid());QString path=dir.path()+"/mask.tif";fixture(path);
    auto a=mw.createMdiChild();auto b=mw.createMdiChild();
    a->newProcessedImage("Mask-A",new unsigned char[32*32*32](),32,32,32,1,V3D_UINT8);
    b->newProcessedImage("Mask-B",new unsigned char[32*32*32](),32,32,32,1,V3D_UINT8);
    a->show();b->show();a->doImage3DView();b->doImage3DView();
    QTimer::singleShot(500,&app,[&](){
        auto wa=a->mypara_3Dview.window3D;auto wb=b->mypara_3Dview.window3D;assert(wa&&wb);
        auto ga=wa->getGLWidget();auto gb=wb->getGLWidget();
        auto ra=dynamic_cast<Renderer_gl1*>(ga->getRenderer());auto rb=dynamic_cast<Renderer_gl1*>(gb->getRenderer());assert(ra&&rb);
        QPushButton mainButton("Main interaction",&mw);mw.statusBar()->addWidget(&mainButton);mainButton.show();
        QPushButton otherButton("Other interaction",wb);otherButton.show();
        int mainClicks=0,otherClicks=0,beats=0; qint64 maxGap=0;QElapsedTimer clock;clock.start();qint64 last=clock.elapsed();
        QObject::connect(&mainButton,&QPushButton::clicked,[&](){++mainClicks;});
        QObject::connect(&otherButton,&QPushButton::clicked,[&](){++otherClicks;});
        QTimer heartbeat;heartbeat.setInterval(20);QObject::connect(&heartbeat,&QTimer::timeout,[&](){
            qint64 now=clock.elapsed();maxGap=qMax(maxGap,now-last);last=now;++beats;
            if(!QApplication::activeModalWidget()){click(&mainButton);click(&otherButton);}
        });heartbeat.start();
        start(ga,path,70);start(gb,path,60,false,1);
        assert(!QApplication::activeModalWidget());
        int zoom=gb->zoom();QPointF pos(100,100);
        QWheelEvent wheel(pos,gb->mapToGlobal(pos.toPoint()),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(gb,&wheel);assert(wheel.isAccepted()&&gb->zoom()!=zoom);
        waitFor([&](){return idle(ga)&&idle(gb);});
        assert(beats>5&&mainClicks>5&&otherClicks>5);assert(maxGap<2000);
        for(auto r:{ra,rb}){assert(r->listLabelSurf.size()==2&&r->list_listTriangle.size()==2);assert(r->listLabelSurf[0].label==1&&r->listLabelSurf[1].label==2);
            assert(numTriangles(r->list_listTriangle[0])>0&&numTriangles(r->list_listTriangle[1])>0);assert(!r->maskChunkLists.isEmpty());}
        // GPU lists really render, and independent manager visibility/Y-flip remain effective.
        ga->surfaceSelectDialog(stLabelSurface-1);gb->surfaceSelectDialog(stLabelSurface-1);
        auto da=wa->findChild<V3dr_surfaceDialog*>();auto db=wb->findChild<V3dr_surfaceDialog*>();assert(da&&db&&da!=db);
        auto tabs=da->findChild<QTabWidget*>();auto table=qobject_cast<QTableWidget*>(tabs->widget(stLabelSurface-1));assert(table&&table->rowCount()==2);
        wa->raise();wa->activateWindow();ra->sShowSurfObjects=2;ga->update();qApp->processEvents();QImage shown=ga->grabFramebuffer();
        table=qobject_cast<QTableWidget*>(tabs->widget(stLabelSurface-1));
        table->item(0,0)->setCheckState(Qt::Unchecked);table->item(1,0)->setCheckState(Qt::Unchecked);
        assert(!ra->listLabelSurf[0].on&&!ra->listLabelSurf[1].on);
        ga->update();QImage hidden;waitFor([&](){hidden=ga->grabFramebuffer();return shown!=hidden;},5000);shown.save("/tmp/vaa3d-mask-async.png");hidden.save("/tmp/vaa3d-mask-hidden.png");assert(shown!=hidden);
        table=qobject_cast<QTableWidget*>(tabs->widget(stLabelSurface-1));
        assert(rb->listLabelSurf[0].on);table->item(0,0)->setCheckState(Qt::Checked);
        table->item(0,table->columnCount()-1)->setCheckState(Qt::Checked);assert(ra->listLabelSurf[0].yFlipDisplay&&!rb->listLabelSurf[0].yFlipDisplay);
        Triangle *old=ra->list_listTriangle[0];
        start(ga,path,1000,true);QTimer::singleShot(200,wa,[=](){cancel(wa);});waitFor([&](){return idle(ga);});
        assert(ra->list_listTriangle[0]==old&&ra->listLabelSurf.size()==2);
        // Cancel after CPU work, during chunked GPU upload; staged GL lists must be discarded.
        bool cancelledUpload=false;QTimer uploadCancel;uploadCancel.setInterval(1);
        QObject::connect(&uploadCancel,&QTimer::timeout,[&](){auto p=wa->findChild<QProgressDialog*>("maskImportProgress");
            if(p&&p->labelText().contains(QString::fromUtf8("上传"))){cancelledUpload=true;uploadCancel.stop();cancel(wa);}});
        start(ga,path,85,true);uploadCancel.start();waitFor([&](){return idle(ga);});uploadCancel.stop();
        assert(cancelledUpload&&ra->list_listTriangle[0]==old&&ra->listLabelSurf.size()==2);
        // A replacement succeeds atomically, without appending or changing B.
        start(ga,path,45,true);waitFor([&](){return idle(ga);});assert(ra->listLabelSurf.size()==2&&ra->list_listTriangle[0]!=old&&rb->listLabelSurf.size()==2);
        // A failed read reports a window-local error and preserves the existing surfaces.
        QString invalid=dir.path()+"/broken.tif";QFile bad(invalid);assert(bad.open(QIODevice::WriteOnly));bad.write("broken");bad.close();
        old=ra->list_listTriangle[0];start(ga,invalid,30,true);waitFor([&](){return idle(ga);});
        assert(ra->list_listTriangle[0]==old&&ra->listLabelSurf.size()==2);
        auto message=wa->findChild<QMessageBox*>();assert(message);message->accept();qApp->processEvents();
        // Closing a target with a live job cancels it, and another target can finish.
        start(ga,path,1000);start(gb,path,40);QPointer<V3dR_GLWidget> dead=ga;wa->close();
        waitFor([&](){return !dead&&idle(gb);});assert(rb->listLabelSurf.size()==4);
        // Also cover renderer replacement while a job is active.
        start(gb,path,1000);gb->deleteRenderer();waitFor([&](){return idle(gb);});
        heartbeat.stop();qInfo("MASK_ASYNC_GUI_PASS heartbeats=%d mainClicks=%d otherClicks=%d maxGapMs=%lld",beats,mainClicks,otherClicks,maxGap);
        // otherButton is stack-owned; detach before its parent closes.
        otherButton.setParent(nullptr);wb->close();app.quit();
    });
    QTimer::singleShot(120000,&app,[](){qFatal("Mask GUI test timeout");});return app.exec();
}
