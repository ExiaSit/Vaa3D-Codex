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
    // Labels stay near their own tip and extend continuously away from the image.
    QRectF viewport(0,0,800,600),image(200,150,400,300);QPointF origin(200,150);
    auto check=[&](const QRectF &v,const QPolygonF &body,const QPointF &o,std::array<QPointF,3> tips,double scale){
        auto boxes=axisLabelLayout(v,body,o,tips,scale);
        auto hull=AxisLabelDetail::convexHull(body);
        QPointF center;int n=body.size();
        for(int i=0;i<n;++i)center+=body[i];center/=n;
        for(int i=0;i<3;++i)if(!boxes[i].isEmpty()){
            assert(v.contains(boxes[i])&&!hull.intersects(QPolygonF(boxes[i].adjusted(-5*scale,-5*scale,5*scale,5*scale))));
            auto delta=boxes[i].center()-tips[i],direction=tips[i]-center;
            assert(std::hypot(delta.x(),delta.y())<=48*scale+1e-6);
            assert(std::abs(AxisLabelDetail::cross(delta,direction))<1e-6);
            assert(QPointF::dotProduct(delta,direction)>0);
            for(int j=0;j<i;++j)assert(boxes[j].isEmpty()||!boxes[j].intersects(boxes[i]));}
        return boxes;
    };
    auto boxes=check(viewport,QPolygonF(image),origin,{{QPointF(650,150),QPointF(200,480),QPointF(650,500)}},1);
    for(auto box:boxes)assert(!box.isEmpty());
    boxes=check(viewport,QPolygonF(image),origin,{{QPointF(650,150),QPointF(650,150),QPointF(650,150)}},1);
    assert(!boxes[0].isEmpty()&&boxes[1].isEmpty()&&boxes[2].isEmpty());
    boxes=check(viewport,QPolygonF(viewport),origin,{{QPointF(650,150),QPointF(200,480),QPointF(650,500)}},1);
    for(auto box:boxes)assert(box.isEmpty());
    check(QRectF(0,0,1600,1200),QPolygonF(QRectF(400,300,800,600)),QPointF(400,300),
        {{QPointF(1300,300),QPointF(400,960),QPointF(1300,1000)}},2);
    boxes=check(viewport,QPolygonF(image),image.center(),{{image.center(),image.center(),image.center()}},1);
    for(auto box:boxes)assert(box.isEmpty());
    QPolygonF diamond;diamond<<QPointF(200,300)<<QPointF(400,150)<<QPointF(600,300)<<QPointF(400,450);
    // Previously pushed 86.77 pixels from the tip by the enclosing rectangle.
    boxes=check(viewport,diamond,QPointF(200,300),{{QPointF(325,190),QPointF(200,480),QPointF(650,500)}},1);
    assert(!boxes[0].isEmpty()&&QLineF(boxes[0].center(),QPointF(325,190)).length()<30);
    boxes=check(viewport,diamond,QPointF(200,300),{{QPointF(325,175),QPointF(200,480),QPointF(650,500)}},1);
    assert(!boxes[0].isEmpty()&&diamond.boundingRect().contains(boxes[0].center()));
    auto delta=boxes[0].center()-QPointF(325,175);assert(std::hypot(delta.x(),delta.y())<26);
    // A tiny drag must not select a different sideways slot.
    QPointF last;bool lastVisible=false;int visible=0;
    for(int i=0;i<201;++i){QPointF tip(315+i*.1,175);
        auto b=check(viewport,diamond,QPointF(200,300),{{tip,QPointF(200,480),QPointF(650,500)}},1);
        if(!b[0].isEmpty()){++visible;if(lastVisible)assert(QLineF(last,b[0].center()).length()<.3);last=b[0].center();}
        lastVisible=!b[0].isEmpty();}
    assert(visible>=180);

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
            auto bb=r->boundingBox;QPolygonF points;
            for(int i=0;i<8;++i)points<<project(XYZ(i&1?bb.x1:bb.x0,i&2?bb.y1:bb.y0,i&4?bb.z1:bb.z0));
            XYZ start=bb.Vabsmin(),end=bb.V1()+bb.Dmax()*.05;
            std::array<QPointF,3> tips{{project(XYZ(end.x,start.y,start.z)),project(XYZ(start.x,end.y,start.z)),project(XYZ(start.x,start.y,end.z))}};
            auto actual=check(QRectF(vp[0],vp[1],vp[2],vp[3]),points,
                project(start),tips,qMax(1.0,vp[2]/double(gl->width())));
            int labels=0;
            for(int axis=0;axis<3;++axis)if(!actual[axis].isEmpty()){
                ++labels;assert(colored(frame,actual[axis],axis)>12);}
            if(!screenshot.isEmpty())frame.save(screenshot);
            qInfo("AXIS_LABEL_VIEW labels=%d",labels);
            for(int i=0;i<3;++i)qInfo("AXIS_TIP %d x=%g y=%g visible=%d",i,tips[i].x(),tips[i].y(),!actual[i].isEmpty());
            assert(labels>=minimumLabels);
            gl->enableShowAxes(false);settle();QImage hidden=gl->grabFramebuffer();
            for(int axis=0;axis<3;++axis)if(!actual[axis].isEmpty())assert(colored(hidden,actual[axis],axis)==0);
            gl->enableShowAxes(true);settle();
        };
        verify(2,"/tmp/vaa3d-axis-labels-default.png");
        gl->setZoom(-15);verify(3,"/tmp/vaa3d-axis-labels-fit.png");
        auto background=r->color_background;
        r->color_background.r=r->color_background.g=r->color_background.b=1;
        verify(3,"/tmp/vaa3d-axis-labels-light.png");r->color_background=background;
        gl->setXRotation(40);gl->setYRotation(25);gl->setZRotation(55);gl->setZoom(-30);
        verify(2,"/tmp/vaa3d-axis-labels-rotated.png");
        gl->enableOrthoView(true);gl->setXShift(20);gl->setYShift(-10);
        verify(1,"/tmp/vaa3d-axis-labels-ortho.png");
        win->resize(850,800);gl->setZoom(80);verify(0,"/tmp/vaa3d-axis-labels-zoom.png");
        // Sweep rotation and stretch/crop combinations from the live 3D viewer.
        win->resize(1200,980);gl->setZoom(-30);gl->setXShift(0);gl->setYShift(0);gl->enableOrthoView(false);
        for(int i=0;i<24;++i){gl->setXRotation(i*15);gl->setYRotation(i*7);gl->setZRotation(i*11);verify(0,QString());}
        assert(gl->setVoxelSpacing(1,.5,4));gl->setXCut0(8);gl->setYCut0(5);gl->setZCut0(3);
        verify(0,"/tmp/vaa3d-axis-labels-spacing-crop.png");
        QPoint p=gl->rect().center();
        QMouseEvent press(QEvent::MouseButtonPress,p,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(gl,&press);
        for(int i=1;i<=12;++i){QMouseEvent move(QEvent::MouseMove,p+QPoint(i,2*i),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(gl,&move);verify(0,QString());}
        QMouseEvent release(QEvent::MouseButtonRelease,p+QPoint(12,24),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(gl,&release);
        qInfo("AXIS_LABELS_GUI_PASS: RGB pixels, endpoint anchoring and 48px bound, convex silhouette exclusion, 201-step continuity, end-on/collision hiding, rotation and real mouse drag, spacing/crop, zoom/shift/ortho/axes toggle");
        win->close();app.quit();
    });
    QTimer::singleShot(30000,&app,[](){qFatal("Axis labels GUI timeout");});return app.exec();
}
