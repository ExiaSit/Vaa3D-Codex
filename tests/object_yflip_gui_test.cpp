// Links against application objects, replacing main.o; production main is not modified.
#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_surfaceDialog.h"
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include "../v3d_main/3drenderer/marchingcubes.h"
#include <cassert>
#include <QTimer>
struct FlipTestDialog:V3dr_surfaceDialog {
    using V3dr_surfaceDialog::table;
    FlipTestDialog(V3dR_GLWidget *w):V3dr_surfaceDialog(w){}
};
int main(int argc,char **argv){
    Q_INIT_RESOURCE(v3d);V3dApplication &app=*V3dApplication::getInstance(argc,argv);MainWindow &mainWindow=*app.getMainWindow();mainWindow.show();
    XFormWidget *child=mainWindow.createMdiChild();
    child->newProcessedImage("Y flip test",new unsigned char[32*32*32](),32,32,32,1,V3D_UINT8);
    child->show();child->doImage3DView();
    QTimer timer;timer.setInterval(200);
    QObject::connect(&timer,&QTimer::timeout,[&](){
        auto win=child->mypara_3Dview.window3D;if(!win)return;
        auto w=win->getGLWidget();auto r=dynamic_cast<Renderer_gl1*>(w->getRenderer());if(!r)return;
        timer.stop();
        NeuronTree tree;tree.editable=true;tree.name="first";tree.color.r=0;tree.color.g=255;tree.color.b=0;
        NeuronSWC n;n.seg_id=0;n.n=1;n.pn=-1;n.x=5;n.y=6;n.z=15;n.r=1;tree.listNeuron.append(n);tree.hashNeuron[1]=0;
        n.n=2;n.pn=1;n.x=20;n.y=9;tree.listNeuron.append(n);tree.hashNeuron[2]=1;
        r->listNeuronTree<<tree<<tree;r->listNeuronTree[1].on=false;
        ImageMarker marker;marker.x=8;marker.y=10;marker.z=12;marker.name="A";r->listMarker<<marker;marker.name="B";r->listMarker<<marker;
        CellAPO point;point.x=10;point.y=7;point.z=12;point.volsize=8;r->listCell<<point<<point;
        r->map_APOFile_IndexList["test.apo"]=QList<int>()<<0<<1;
        LabelSurf surf;surf.label=1;surf.label2=1;r->listLabelSurf<<surf;
        Triangle *t=new Triangle;t->next=0;
        const float coords[3][3]={{6,4,10},{15,4,10},{6,10,10}};
        for(int i=0;i<3;++i)for(int j=0;j<3;++j){t->vertex[i][j]=coords[i][j];t->normal[i][j]=(j==2?1:0);}
        r->list_listTriangle<<t;w->makeCurrent();r->compileLabelfieldSurf();r->updateNeuronBoundingBox();r->updateBoundingBox();
        V_NeuronSWC segment;V_NeuronSWC_unit unit;unit.n=1;unit.type=3;unit.x=5;unit.y=6;unit.z=15;unit.parent=-1;segment.row.push_back(unit);
        child->getImageData()->tracedNeuron.seg.push_back(segment);
        FlipTestDialog dialog(w);dialog.show();
        auto toggle=[&](int kind,int row,bool on){auto tab=dialog.table[kind];assert(tab);assert(tab->horizontalHeaderItem(tab->columnCount()-1)->text()=="Y flip");tab->item(row,tab->columnCount()-1)->setCheckState(on?Qt::Checked:Qt::Unchecked);};
        w->update();QApplication::processEvents();QImage before=w->grabFramebuffer();before.save("/tmp/yflip-before.png");
        toggle(stNeuronStructure,0,true);assert(r->listNeuronTree[0].yFlipDisplay && !r->listNeuronTree[1].yFlipDisplay);
        assert(r->listNeuronTree[0].listNeuron[0].y==6);
        w->update();QApplication::processEvents();QImage after=w->grabFramebuffer();after.save("/tmp/yflip-after.png");assert(before!=after);
        toggle(stNeuronSegment,0,true);assert(child->getImageData()->tracedNeuron.seg[0].yFlipDisplay);
        toggle(stNeuronSegment,0,false);assert(!child->getImageData()->tracedNeuron.seg[0].yFlipDisplay);
        toggle(stLabelSurface,0,true);assert(r->listLabelSurf[0].yFlipDisplay && t->vertex[0][1]==4);
        toggle(stImageMarker,0,true);assert(r->listMarker[0].yFlipDisplay && r->listMarker[0].y==10);
        auto markerTable=dialog.table[stImageMarker];markerTable->setSortingEnabled(true);markerTable->sortItems(6,Qt::DescendingOrder);
        toggle(stImageMarker,0,true);assert(r->listMarker[1].yFlipDisplay);
        toggle(stImageMarker,0,false);assert(!r->listMarker[1].yFlipDisplay && r->listMarker[0].yFlipDisplay);
        toggle(stPointCloud,0,true);assert(r->listCell[0].yFlipDisplay && !r->listCell[1].yFlipDisplay);
        toggle(stPointSet,0,true);assert(r->listCell[0].yFlipDisplay && r->listCell[1].yFlipDisplay);
        for(int i=0;i<20;++i){toggle(stLabelSurface,0,i%2);w->grabFramebuffer();}
        w->makeCurrent();GLint stack=0;glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&stack);assert(stack==0);
        toggle(stNeuronStructure,0,false);assert(!r->listNeuronTree[0].yFlipDisplay && r->listNeuronTree[0].listNeuron[0].y==6);
        qInfo("Y_FLIP_GUI_PASS: manager checkbox routing, independent objects, framebuffer change, raw geometry preserved, GL stack balanced");
        app.exit(0);
    });timer.start();QTimer::singleShot(30000,&app,[](){qFatal("Y flip GUI test timed out");});return app.exec();
}
