#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include "../v3d_main/3drenderer/AxisLabelLayout.h"
#include <QElapsedTimer>
#include <QTimer>
#include <cassert>

static void settle(){QElapsedTimer t;t.start();while(t.elapsed()<80)qApp->processEvents();}
static int colored(const QImage &frame,const QRectF &box,int axis)
{
    int count=0;
    for(int x=qMax(0,int(box.left()));x<qMin(frame.width(),int(std::ceil(box.right())));++x)
        for(int y=qMax(0,int(box.top()));y<qMin(frame.height(),int(std::ceil(box.bottom())));++y)
        {
            QColor c=frame.pixelColor(x,frame.height()-1-y);int rgb[]={c.red(),c.green(),c.blue()};
            if(rgb[axis]>160&&rgb[(axis+1)%3]<70&&rgb[(axis+2)%3]<70)++count;
        }
    return count;
}
int main(int argc,char **argv)
{
    // Geometry corner cases: no image overlap, no label collisions, no off-screen clamping.
    QRectF viewport(0,0,800,600),image(200,150,400,300);QPointF origin(200,150);
    auto check=[&](const QRectF &v,const QRectF &body,const QPointF &o,std::array<QPointF,3> tips,double scale){
        auto boxes=axisLabelLayout(v,body,o,tips,scale);
        for(int i=0;i<3;++i)if(!boxes[i].isEmpty()){
            assert(v.contains(boxes[i])&&!body.adjusted(-5*scale,-5*scale,5*scale,5*scale).intersects(boxes[i]));
            for(int j=0;j<i;++j)assert(boxes[j].isEmpty()||!boxes[j].intersects(boxes[i]));}
        return boxes;
    };
    auto boxes=check(viewport,image,origin,{{QPointF(650,150),QPointF(200,480),QPointF(650,500)}},1);
    for(auto box:boxes)assert(!box.isEmpty());
    boxes=check(viewport,image,origin,{{QPointF(650,150),QPointF(650,150),QPointF(650,150)}},1);
    for(auto box:boxes)assert(!box.isEmpty());
    boxes=check(viewport,viewport,origin,{{QPointF(650,150),QPointF(200,480),QPointF(650,500)}},1);
    for(auto box:boxes)assert(box.isEmpty());
    check(QRectF(0,0,1600,1200),QRectF(400,300,800,600),QPointF(400,300),
        {{QPointF(1300,300),QPointF(400,960),QPointF(1300,1000)}},2);
    check(viewport,image,image.center(),{{image.center(),image.center(),image.center()}},1);

    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.show();
    auto child=mw.createMdiChild();auto data=new unsigned char[64*48*32];
    for(int i=0;i<64*48*32;++i)data[i]=i%240+15;
    child->newProcessedImage("Axis labels test",data,64,48,32,1,V3D_UINT8);child->show();child->doImage3DView();
    QTimer::singleShot(500,&app,[&](){
        auto win=child->mypara_3Dview.window3D;assert(win);win->resize(1200,980);settle();
        auto gl=win->getGLWidget();auto r=dynamic_cast<Renderer_gl1*>(gl->getRenderer());assert(r);
        auto verify=[&](int minimumLabels,const QString &screenshot){
            gl->enableShowAxes(true);gl->update();settle();QImage frame=gl->grabFramebuffer();gl->makeCurrent();
            GLint mode,vp[4];GLdouble model[16],projection[16];glGetIntegerv(GL_MATRIX_MODE,&mode);
            glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadMatrixd(r->volumeViewMatrix);r->setObjectSpace();
            glGetDoublev(GL_MODELVIEW_MATRIX,model);glGetDoublev(GL_PROJECTION_MATRIX,projection);glGetIntegerv(GL_VIEWPORT,vp);
            glPopMatrix();glMatrixMode(mode);
            auto project=[&](XYZ p){GLdouble x,y,z;assert(gluProject(p.x,p.y,p.z,model,projection,vp,&x,&y,&z));return QPointF(x,y);};
            auto bb=r->boundingBox;QPointF points[8];
            for(int i=0;i<8;++i)points[i]=project(XYZ(i&1?bb.x1:bb.x0,i&2?bb.y1:bb.y0,i&4?bb.z1:bb.z0));
            double left=points[0].x(),right=left,bottom=points[0].y(),top=bottom;
            for(auto p:points){left=qMin(left,p.x());right=qMax(right,p.x());bottom=qMin(bottom,p.y());top=qMax(top,p.y());}
            XYZ start=bb.Vabsmin(),end=bb.V1()+bb.Dmax()*.05;
            std::array<QPointF,3> tips{{project(XYZ(end.x,start.y,start.z)),project(XYZ(start.x,end.y,start.z)),project(XYZ(start.x,start.y,end.z))}};
            auto actual=check(QRectF(vp[0],vp[1],vp[2],vp[3]),QRectF(QPointF(left,bottom),QPointF(right,top)),
                project(start),tips,qMax(1.0,vp[2]/double(gl->width())));
            int labels=0;
            for(int axis=0;axis<3;++axis)if(!actual[axis].isEmpty()){
                ++labels;assert(colored(frame,actual[axis],axis)>12);}
            qInfo("AXIS_LABEL_VIEW labels=%d image=(%.0f,%.0f,%.0f,%.0f)",labels,left,bottom,right,top);assert(labels>=minimumLabels);
            frame.save(screenshot);
            gl->enableShowAxes(false);settle();QImage hidden=gl->grabFramebuffer();
            for(int axis=0;axis<3;++axis)if(!actual[axis].isEmpty())assert(colored(hidden,actual[axis],axis)==0);
            gl->enableShowAxes(true);settle();
        };
        verify(3,"/tmp/vaa3d-axis-labels-default.png");
        auto background=r->color_background;
        r->color_background.r=r->color_background.g=r->color_background.b=1;
        verify(3,"/tmp/vaa3d-axis-labels-light.png");r->color_background=background;
        gl->setXRotation(40);gl->setYRotation(25);gl->setZRotation(55);gl->setZoom(-30);
        verify(2,"/tmp/vaa3d-axis-labels-rotated.png");
        gl->enableOrthoView(true);gl->setXShift(20);gl->setYShift(-10);
        verify(1,"/tmp/vaa3d-axis-labels-ortho.png");
        win->resize(850,800);gl->setZoom(80);verify(0,"/tmp/vaa3d-axis-labels-zoom.png");
        qInfo("AXIS_LABELS_GUI_PASS: visible RGB XYZ pixels on dark/light backgrounds, projected image exclusion, collision/edge/scale cases, rotation, zoom, shift, ortho and axes toggle");
        win->close();app.quit();
    });
    QTimer::singleShot(30000,&app,[](){qFatal("Axis labels GUI timeout");});return app.exec();
}
