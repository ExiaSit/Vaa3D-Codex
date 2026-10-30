#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/v3dr_surfaceDialog.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include <QTemporaryDir>
#include <QColorDialog>
#include <QTimer>
#include <QThread>
#include <cassert>

static void settle()
{
    QElapsedTimer t;t.start();while(t.elapsed()<100){qApp->processEvents();QThread::msleep(1);}
}
static QTableWidget *neurons(V3dr_surfaceDialog *d)
{
    return qobject_cast<QTableWidget*>(d->findChild<QTabWidget*>()->widget(stNeuronStructure-1));
}
static void answerColor(QColor color,bool accept=true)
{
    QTimer::singleShot(0,qApp,[=]() mutable {
        for(auto w:QApplication::topLevelWidgets())if(auto d=qobject_cast<QColorDialog*>(w)){
            // Changing RGB alone in the existing dialog preserves its initial alpha/mode.
            color.setAlpha(d->currentColor().alpha());d->setCurrentColor(color);
            if(accept)d->accept();else d->reject();return;
        }
        qFatal("Color dialog missing");
    });
}
static int pixels(const QImage &image,int channel)
{
    int n=0;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){
        const QColor c=image.pixelColor(x,y);
        const int rgb[3]={c.red(),c.green(),c.blue()};
        if(rgb[channel]>100 && rgb[channel]>rgb[(channel+1)%3]+50 && rgb[channel]>rgb[(channel+2)%3]+50)++n;
    }
    return n;
}
static int matching(const QImage &image,RGBA8 color)
{
    int n=0;for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){auto c=image.pixelColor(x,y);
        if(std::abs(c.red()-color.r)<25&&std::abs(c.green()-color.g)<25&&std::abs(c.blue()-color.b)<25)++n;}
    return n;
}

int main(int argc,char **argv)
{
    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.show();
    QTemporaryDir dir;assert(dir.isValid());QStringList files;
    for(int i=0;i<2;++i){QString path=dir.path()+QString("/neuron%1.").arg(i)+(i?"eswc":"swc");QFile f(path);assert(f.open(QIODevice::WriteOnly));
        f.write(QString("1 2 4 %1 12 1 -1\n2 2 16 %1 12 1 1\n3 3 28 %1 12 1 2\n").arg(i?23:6).toUtf8());f.close();files<<path;}
    QString ano=dir.path()+"/batch.ano";{QFile f(ano);assert(f.open(QIODevice::WriteOnly));for(auto path:files)f.write(("SWCFILE="+path+"\n").toUtf8());}
    mw.loadV3DFile(ano,false,false);
    QTimer::singleShot(500,&app,[&](){
        auto window=mw.find3DViewer(ano);assert(window);auto gl=window->getGLWidget();
        auto r=dynamic_cast<Renderer_gl1*>(gl->getRenderer());assert(r&&r->listNeuronTree.size()==2);
        auto color0=r->listNeuronTree[0].color,color1=r->listNeuronTree[1].color;
        assert(color0.a==255&&color1.a==255&&color0.i!=color1.i);
        r->bShowAxes=false;r->bShowBoundingBox=false;r->bShowBoundingBox2=false;
        r->lineWidth=6;r->rootSize=8;r->nodeSize=6;gl->surfaceSelectDialog(stNeuronStructure-1);settle();
        auto d=window->findChild<V3dr_surfaceDialog*>();assert(d&&neurons(d)->rowCount()==2);
        r->listNeuronTree[0].linemode=r->listNeuronTree[1].linemode=1;gl->update();settle();
        auto automatic=gl->grabFramebuffer();assert(matching(automatic,color0)>30&&matching(automatic,color1)>30);
        for(int i=0;i<2;++i){auto c=neurons(d)->item(i,1)->data(0).value<QColor>();auto expected=r->listNeuronTree[i].color;
            assert(c.red()==expected.r&&c.green()==expected.g&&c.blue()==expected.b);}
        automatic.save("/tmp/vaa3d-swc-batch-colors.png");
        for(int mode:{1,0}){
            r->listNeuronTree[0].linemode=mode;r->listNeuronTree[1].linemode=mode;r->neuronColorMode=0;
            answerColor(QColor(0,255,0));QMetaObject::invokeMethod(d,"doubleClickHandler",Qt::DirectConnection,Q_ARG(int,0),Q_ARG(int,1));settle();
            assert(r->listNeuronTree[0].color.g==255&&r->listNeuronTree[0].color.a==255);
            if(mode==1)assert(r->listNeuronTree[1].color.i==color1.i);
            answerColor(QColor(0,0,255));QMetaObject::invokeMethod(d,"doubleClickHandler",Qt::DirectConnection,Q_ARG(int,1),Q_ARG(int,1));settle();
            auto frame=gl->grabFramebuffer();assert(pixels(frame,1)>30&&pixels(frame,2)>30);
            if(mode==0)frame.save("/tmp/vaa3d-swc-custom-colors.png");
            // Explicit per-object colors survive confidence/ancestry display modes.
            for(int globalMode:{1,3,5}){r->neuronColorMode=globalMode;gl->update();settle();auto image=gl->grabFramebuffer();assert(pixels(image,1)>30&&pixels(image,2)>30);}
        }
        r->neuronColorMode=0;
        answerColor(QColor(255,0,0),false);QMetaObject::invokeMethod(d,"doubleClickHandler",Qt::DirectConnection,Q_ARG(int,0),Q_ARG(int,1));settle();assert(r->listNeuronTree[0].color.g==255);
        // Dedicated automatic palette actions retain their legacy mode tags.
        for(int map:{1,2}){auto table=neurons(d);table->clearSelection();table->selectRow(0);QMetaObject::invokeMethod(d,"selectedColor",Qt::DirectConnection,Q_ARG(int,map));settle();
            assert(r->listNeuronTree[0].color.a==map&&r->listNeuronTree[1].color.a==255);}
        auto table=neurons(d);table->clearSelection();table->selectRow(0);answerColor(QColor(255,0,0));
        QMetaObject::invokeMethod(d,"selectedColor",Qt::DirectConnection,Q_ARG(int,0));settle();assert(r->listNeuronTree[0].color.r==255&&r->listNeuronTree[0].color.a==255);
        qInfo("SWC_COLOR_GUI_PASS: ANO SWC/ESWC batch auto colors and framebuffer/manager agreement, real color dialog, line/tube pixels, per-object independence, global palettes, cancel and automatic modes");app.quit();
    });QTimer::singleShot(30000,&app,[](){qFatal("SWC color test timeout");});return app.exec();
}
