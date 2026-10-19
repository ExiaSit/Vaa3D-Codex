#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_surfaceDialog.h"
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include "../v3d_main/3drenderer/marchingcubes.h"
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <cassert>

static QList<V3dr_surfaceDialog*> managers()
{
    QList<V3dr_surfaceDialog*> result;
    for (auto w:QApplication::topLevelWidgets())
        if (auto d=qobject_cast<V3dr_surfaceDialog*>(w)) result<<d;
    return result;
}
static QTableWidget *table(V3dr_surfaceDialog *d,int kind)
{
    auto tabs=d->findChild<QTabWidget*>(); assert(tabs);
    auto t=qobject_cast<QTableWidget*>(tabs->widget(kind-1)); assert(t);return t;
}
static void populate(V3dR_GLWidget *w,QString name,int count)
{
    auto r=dynamic_cast<Renderer_gl1*>(w->getRenderer()); assert(r);
    for(int i=0;i<count;++i)
    {
        NeuronTree tree;tree.name=name+QString::number(i);tree.on=true;tree.editable=false;
        NeuronSWC n;n.n=1;n.pn=-1;n.x=4;n.y=5;n.z=6;n.r=1;tree.listNeuron<<n;tree.hashNeuron[1]=0;
        n.n=2;n.pn=1;n.x=20;n.y=22;tree.listNeuron<<n;tree.hashNeuron[2]=1;r->listNeuronTree<<tree;
        ImageMarker marker;marker.name=name+QString::number(i);marker.x=4;marker.y=7;marker.z=10;r->listMarker<<marker;
    }
    LabelSurf surface;surface.label=1;surface.label2=1;surface.name=name;r->listLabelSurf<<surface;
    Triangle *triangle=new Triangle;triangle->next=0;
    for(int i=0;i<3;++i)for(int j=0;j<3;++j) {triangle->vertex[i][j]=4+i+j;triangle->normal[i][j]=(j==2?1:0);}
    r->list_listTriangle<<triangle;w->makeCurrent();r->compileLabelfieldSurf();r->updateBoundingBox();
}
static void openManager(V3dR_MainWindow *window)
{
    for(auto button:window->findChildren<QPushButton*>())
        if(button->text().contains("Object Manager")) {button->click();return;}
    qFatal("Object Manager button missing");
}
static void settle()
{
    QElapsedTimer t;t.start();while(t.elapsed()<100) QApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
int main(int argc,char **argv)
{
    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.show();
    auto a=mw.createMdiChild();auto b=mw.createMdiChild();
    a->newProcessedImage("Window-A",new unsigned char[32*32*32](),32,32,32,1,V3D_UINT8);
    b->newProcessedImage("Window-B",new unsigned char[32*32*32](),32,32,32,1,V3D_UINT8);
    a->show();b->show();a->doImage3DView();b->doImage3DView();
    QTimer ready;ready.setInterval(100);
    QObject::connect(&ready,&QTimer::timeout,&app,[&](){
        auto wa=a->mypara_3Dview.window3D;auto wb=b->mypara_3Dview.window3D;
        if(!wa||!wb||!wa->getGLWidget()->getRenderer()||!wb->getGLWidget()->getRenderer())return;
        ready.stop();auto ga=wa->getGLWidget();auto gb=wb->getGLWidget();
        populate(ga,"A-",2);populate(gb,"B-",1);
        auto ra=dynamic_cast<Renderer_gl1*>(ga->getRenderer());auto rb=dynamic_cast<Renderer_gl1*>(gb->getRenderer());
        openManager(wa);assert(managers().size()==1);QPointer<V3dr_surfaceDialog> da=managers().first();
        // Reproduce the old globally shared manager even when attached to window A.
        for(auto check:da->findChildren<QCheckBox*>()) if(check->text().contains("Attached"))check->setChecked(true);
        openManager(wb);assert(managers().size()==2);
        QPointer<V3dr_surfaceDialog> db;for(auto d:managers())if(d!=da)db=d;assert(db);
        assert(da->windowTitle().contains("Window-A") && db->windowTitle().contains("Window-B"));
        ga->surfaceSelectDialog(stNeuronStructure-1);
        assert(da->getCurTab()==stNeuronStructure-1 && db->getCurTab()!=stNeuronStructure-1);
        auto verify=[&](){assert(table(da,stNeuronStructure)->rowCount()==2);assert(table(db,stNeuronStructure)->rowCount()==1);
            assert(table(da,stNeuronStructure)->item(0,5)->text()=="A-0");assert(table(db,stNeuronStructure)->item(0,5)->text()=="B-0");};verify();
        for(int i=0;i<3;++i){wa->activateWindow();ga->updateTool();wb->activateWindow();gb->updateTool();verify();}
        // A stale external relink must not retarget a manager owned by another view.
        da->linkTo(gb);verify();
        for(int kind:{stNeuronStructure,stImageMarker,stLabelSurface})
        {
            auto t=table(da,kind);t->item(0,0)->setCheckState(Qt::Unchecked);
            t->item(0,t->columnCount()-1)->setCheckState(Qt::Checked);
        }
        assert(!ra->listNeuronTree[0].on && ra->listNeuronTree[0].yFlipDisplay && rb->listNeuronTree[0].on && !rb->listNeuronTree[0].yFlipDisplay);
        assert(!ra->listMarker[0].on && ra->listMarker[0].yFlipDisplay && rb->listMarker[0].on && !rb->listMarker[0].yFlipDisplay);
        assert(!ra->listLabelSurf[0].on && ra->listLabelSurf[0].yFlipDisplay && rb->listLabelSurf[0].on && !rb->listLabelSurf[0].yFlipDisplay);
        da->undo();assert(ra->listNeuronTree[0].on && !ra->listNeuronTree[0].yFlipDisplay && rb->listNeuronTree[0].on);
        auto t=table(db,stNeuronStructure);t->item(0,0)->setCheckState(Qt::Unchecked);
        assert(!rb->listNeuronTree[0].on && ra->listNeuronTree[0].on);
        da->close();assert(!da->isVisible() && db->isVisible());openManager(wa);assert(managers().size()==2 && da->isVisible());
        NeuronTree extra=rb->listNeuronTree[0];extra.name="B-new";rb->listNeuronTree<<extra;gb->updateTool();
        assert(table(db,stNeuronStructure)->rowCount()==2 && table(da,stNeuronStructure)->rowCount()==2);
        assert(table(da,stNeuronStructure)->item(1,5)->text()=="A-1");
        da->move(0,0);db->move(0,340);settle();
        da->grab().save("/tmp/vaa3d-manager-A.png");db->grab().save("/tmp/vaa3d-manager-B.png");
        wa->close();settle();assert(!da && db && db->isVisible());
        table(db,stNeuronStructure)->item(0,0)->setCheckState(Qt::Checked);assert(rb->listNeuronTree[0].on);
        wb->close();settle();assert(!db && managers().isEmpty());
        // Renderer replacement invalidates its manager before renderer storage is freed.
        a->doImage3DView();
        QTimer::singleShot(300,&app,[&](){auto w=a->mypara_3Dview.window3D;assert(w);openManager(w);assert(managers().size()==1);
            QPointer<V3dr_surfaceDialog> d=managers().first();auto gl=w->getGLWidget();gl->createRenderer();assert(!d);
            openManager(w);assert(managers().size()==1);w->close();settle();assert(managers().isEmpty());
            qInfo("OBJECT_MANAGER_WINDOWS_PASS: per-view lists, attachment, activation, visibility, Y flip, undo, refresh, reopen and renderer/window cleanup");app.exit(0);});
    });ready.start();QTimer::singleShot(30000,&app,[](){qFatal("Object manager test timeout");});return app.exec();
}
