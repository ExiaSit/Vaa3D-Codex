#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl2.h"
#include <QCryptographicHash>
#include <QTimer>
#include <QElapsedTimer>
#include <cassert>

static void settle(int ms=100){QElapsedTimer t;t.start();while(t.elapsed()<ms)qApp->processEvents();}
static double area(const std::vector<LocalHidePolygon> &pieces)
{
    double sum=0;
    for(const auto &p:pieces){double a=0;for(size_t i=0;i<p.size();++i){const auto &v=p[i].unit,&w=p[(i+1)%p.size()].unit;a+=v[0]*w[1]-w[0]*v[1];}sum+=std::abs(a)/2;}
    return sum;
}
static QByteArray digest(const void *p,int n){return QCryptographicHash::hash(QByteArray::fromRawData(static_cast<const char*>(p),n),QCryptographicHash::Sha256);}
static QPoint project(V3dR_GLWidget *gl,Renderer_gl1 *r,double x,double y,double z)
{
    gl->makeCurrent();GLint mode;GLdouble matrix[16];glGetIntegerv(GL_MATRIX_MODE,&mode);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadMatrixd(r->volumeViewMatrix);r->setUnitVolumeSpace();
    glGetDoublev(GL_MODELVIEW_MATRIX,matrix);glPopMatrix();glMatrixMode(mode);
    GLdouble px,py,pz;assert(gluProject(x,y,z,matrix,r->projectionMatrix,r->viewport,&px,&py,&pz));
    return QPoint(qRound((px-r->viewport[0])*gl->width()/r->viewport[2]),qRound(gl->height()-(py-r->viewport[1])*gl->height()/r->viewport[3]));
}
static void hover(V3dR_GLWidget *gl,QPoint p)
{
    QMouseEvent e(QEvent::MouseMove,p,Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(gl,&e);settle();
}
static void key(V3dR_GLWidget *gl,int k,Qt::KeyboardModifiers mods=Qt::NoModifier,bool repeat=false)
{
    QKeyEvent e(QEvent::KeyPress,k,mods,QString(),repeat);QApplication::sendEvent(gl,&e);settle();
}
static int signal(const QImage &frame,QPoint p)
{
    int maximum=0;
    for(int y=qMax(0,p.y()-3);y<=qMin(frame.height()-1,p.y()+3);++y)
        for(int x=qMax(0,p.x()-3);x<=qMin(frame.width()-1,p.x()+3);++x)
            maximum=qMax(maximum,qGray(frame.pixel(x,y)));
    return maximum;
}
int main(int argc,char **argv)
{
    LocalHidePolygon quad;
    for(auto p:std::vector<std::array<double,3>>{{{0,0,.5}},{{1,0,.5}},{{1,1,.5}},{{0,1,.5}}})quad.push_back({p,p,p});
    LocalHideBox box;box.low={{.25,.25,.25}};box.high={{.75,.75,.75}};
    assert(std::abs(area(subtractLocalHideBoxes(quad,{box},0))-.75)<1e-10);
    std::vector<LocalHideBox> duplicates(80,box);auto clipped=subtractLocalHideBoxes(quad,duplicates,0);
    assert(std::abs(area(clipped)-.75)<1e-10&&clipped.size()==4);
    assert(std::abs(area(subtractLocalHideBoxes(quad,{box},1))-1)<1e-10);
    box.low={{0,0,0}};box.high={{1,1,1}};assert(subtractLocalHideBoxes(quad,{box},0).empty());

    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.show();
    const int bytes=64*64*64;auto data=new unsigned char[bytes]();
    for(int z=30;z<=34;++z)for(int y=30;y<=34;++y)for(int x=14;x<=18;++x)data[(z*64+y)*64+x]=220;
    for(int z=30;z<=34;++z)for(int y=30;y<=34;++y)for(int x=46;x<=50;++x)data[(z*64+y)*64+x]=170;
    const QByteArray original=digest(data,bytes);
    auto a=mw.createMdiChild(),b=mw.createMdiChild();auto copy=new unsigned char[bytes];memcpy(copy,data,bytes);
    a->newProcessedImage("Local hide test",data,64,64,64,1,V3D_UINT8);
    b->newProcessedImage("Independent hide test",copy,64,64,64,1,V3D_UINT8);
    a->show();b->show();a->doImage3DView();b->doImage3DView();
    QTimer::singleShot(500,&app,[&](){
        auto win=a->mypara_3Dview.window3D,other=b->mypara_3Dview.window3D;assert(win&&other);win->resize(1200,980);
        auto gl=win->getGLWidget();auto r=dynamic_cast<Renderer_gl2*>(gl->getRenderer());assert(r);
        r->bShowAxes=r->bShowBoundingBox=false;r->color_background.r=r->color_background.g=r->color_background.b=0;
        r->tryTexStream=0;r->tryTexCompress=0;gl->setRenderMode_Maxip(true);gl->enableOrthoView(true);gl->setZoom(-15);settle();
        auto mode=win->findChild<QCheckBox*>("localHideMode");auto radius=win->findChild<QSpinBox*>("localHideRadius");
        auto undo=win->findChild<QPushButton*>("undoLocalHide"),restore=win->findChild<QPushButton*>("restoreLocalHidden");assert(mode&&radius&&undo&&restore);
        radius->setValue(5);
        for(int texture3D:{1,0})for(int shader:{1,0}) {
            gl->makeCurrent();r->tryTex3D=texture3D;r->tryVolShader=shader;r->loadVol();settle();
            assert(bool(r->tex3D)==bool(texture3D));
            if(shader)assert(r->shaderTex2D&&r->shaderTex3D);
            const auto rgba=digest(r->total_rgbaBuf,bytes*4);
            mode->setChecked(true);gl->update();settle();
            QPoint left=project(gl,r,16./63,32./63,32./63),right=project(gl,r,48./63,32./63,32./63);
            auto frame=gl->grabFramebuffer();assert(signal(frame,left)>80&&signal(frame,right)>60);
            // Moving to another target invalidates the old box immediately;
            // Delete before the new hover preview appears must do nothing.
            hover(gl,left);
            QMouseEvent pending(QEvent::MouseMove,right,Qt::NoButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(gl,&pending);key(gl,Qt::Key_Delete);assert(r->localHiddenBoxes.empty());
            hover(gl,left);assert(r->localHidePreviewValid);
            assert(r->localHidePreview.contains({{16./63,32./63,32./63}}));
            if(texture3D&&shader)win->grab().save("/tmp/vaa3d-local-hide-preview.png");
            key(gl,Qt::Key_Delete);assert(r->localHiddenBoxes.size()==1&&!r->localHidePreviewValid);
            frame=gl->grabFramebuffer();qInfo("HIDE_RENDER path=%d/%d left=%d right=%d",texture3D,shader,signal(frame,left),signal(frame,right));
            assert(signal(frame,left)<20&&signal(frame,right)>60);
            assert(win->findChild<QLabel*>("localHideCount")->text().endsWith("1"));
            assert(digest(data,bytes)==original&&digest(r->total_rgbaBuf,bytes*4)==rgba);
            key(gl,Qt::Key_Delete,Qt::NoModifier,true);assert(r->localHiddenBoxes.size()==1);
            assert(dynamic_cast<Renderer_gl1*>(other->getGLWidget()->getRenderer())->localHiddenBoxes.empty());
            if(texture3D&&shader)win->grab().save("/tmp/vaa3d-local-hide-deleted.png");
            mode->setChecked(false);assert(!r->localHidePreviewValid);key(gl,Qt::Key_Delete);assert(r->localHiddenBoxes.size()==1);
            mode->setChecked(true);key(gl,Qt::Key_Z,Qt::ControlModifier);assert(r->localHiddenBoxes.empty());
            assert(signal(gl->grabFramebuffer(),left)>80);
            hover(gl,left);key(gl,Qt::Key_Delete);hover(gl,right);key(gl,Qt::Key_Delete);assert(r->localHiddenBoxes.size()==2);
            restore->click();settle();assert(r->localHiddenBoxes.empty()&&!undo->isEnabled());
            assert(signal(gl->grabFramebuffer(),left)>80&&digest(data,bytes)==original);
        }
        // Alpha and slice rendering use the same non-destructive exclusion.
        gl->resetRotation();gl->setFrontCut(0);gl->setRenderMode_Alpha(true);settle();
        auto exerciseSlice=[&](){
            auto p=project(gl,r,16./63,32./63,32./63),q=project(gl,r,48./63,32./63,32./63);
            assert(signal(gl->grabFramebuffer(),p)>30);hover(gl,p);assert(r->localHidePreviewValid);
            key(gl,Qt::Key_Delete);assert(r->localHiddenBoxes.size()==1);
            assert(signal(gl->grabFramebuffer(),p)<20&&signal(gl->grabFramebuffer(),q)>30);
            restore->click();settle();assert(signal(gl->grabFramebuffer(),p)>30);
        };
        exerciseSlice();
        gl->setRenderMode_Cs3d(true);r->bXSlice=r->bYSlice=false;r->bZSlice=true;gl->setZCS(32);settle();exerciseSlice();
        r->bZSlice=false;r->bFSlice=true;gl->setFrontCut(CLIP_RANGE/2);settle();exerciseSlice();
        r->bFSlice=false;gl->setFrontCut(0);gl->setRenderMode_Maxip(true);settle();
        // View rotation must not turn the hidden region into a screen-space hole.
        hover(gl,project(gl,r,16./63,32./63,32./63));key(gl,Qt::Key_Delete);assert(r->localHiddenBoxes.size()==1);
        gl->setXRotation(75);gl->setYRotation(25);settle();
        auto left=project(gl,r,16./63,32./63,32./63),right=project(gl,r,48./63,32./63,32./63);
        auto frame=gl->grabFramebuffer();assert(signal(frame,left)<20&&signal(frame,right)>60);
        restore->click();settle();assert(signal(gl->grabFramebuffer(),left)>80);
        // X-oriented slices: blocks overlap on screen but remain separate in depth.
        gl->resetRotation();gl->setYRotation(90);settle();left=project(gl,r,16./63,32./63,32./63);
        assert(signal(gl->grabFramebuffer(),left)>200);hover(gl,left);key(gl,Qt::Key_Delete);
        assert(r->localHiddenBoxes.size()==1);
        int behind=signal(gl->grabFramebuffer(),left);assert(behind>=150&&behind<=180);
        restore->click();settle();assert(signal(gl->grabFramebuffer(),left)>200);
        hover(gl,left);QEvent leave(QEvent::Leave);QApplication::sendEvent(gl,&leave);key(gl,Qt::Key_Delete);assert(r->localHiddenBoxes.empty());
        hover(gl,left);key(gl,Qt::Key_Escape);assert(!mode->isChecked()&&!r->localHidePreviewValid);
        // Closing/rebuilding a renderer clears the feature's window-owned controls.
        mode->setChecked(true);hover(gl,left);key(gl,Qt::Key_Delete);assert(undo->isEnabled());gl->createRenderer();settle();
        assert(!mode->isChecked()&&!undo->isEnabled()&&dynamic_cast<Renderer_gl1*>(gl->getRenderer())->localHiddenBoxes.empty());
        assert(digest(data,bytes)==original&&digest(copy,bytes)==original);
        qInfo("LOCAL_VOLUME_HIDE_GUI_PASS: hover cube, Delete, raw/RGBA preservation, 2D/3D shader/fixed rendering, rotation, undo, restore, view isolation, repeat guard, leave/Esc and renderer cleanup");
        win->close();other->close();app.quit();
    });QTimer::singleShot(60000,&app,[](){qFatal("Local hiding GUI timeout");});return app.exec();
}
