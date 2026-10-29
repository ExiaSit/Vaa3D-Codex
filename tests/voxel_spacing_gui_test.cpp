#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl2.h"
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QTimer>
#include <cassert>
#include <limits>

static void settle(int ms=150){QElapsedTimer t;t.start();while(t.elapsed()<ms)qApp->processEvents();}
static bool near(double a,double b){return std::abs(a-b)<1e-5*std::max(1.,std::abs(b));}
static QByteArray fingerprint(My4DImage *image){return QCryptographicHash::hash(QByteArray::fromRawData(
    reinterpret_cast<char*>(image->getRawData()),image->getTotalBytes()),QCryptographicHash::Sha256);}
static QRect brightBounds(QImage image){
    // Keep the largest connected bright object, excluding scale-bar text/lines.
    QRect result;int largest=0;
    QVector<bool> visited(image.width()*image.height(),false);
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){
        if(visited[y*image.width()+x])continue;
        QVector<QPoint> pending;pending.append(QPoint(x,y));QRect bounds;int count=0;
        while(!pending.isEmpty()){
            QPoint p=pending.takeLast();
            if(!image.rect().contains(p))continue;
            int index=p.y()*image.width()+p.x();if(visited[index])continue;visited[index]=true;
            auto c=image.pixelColor(p);if(c.red()<=80||c.green()<=80||c.blue()<=80)continue;
            ++count;bounds=bounds.united(QRect(p,QSize(1,1)));
            pending.append(p+QPoint(1,0));pending.append(p-QPoint(1,0));
            pending.append(p+QPoint(0,1));pending.append(p-QPoint(0,1));
        }
        if(count>largest){largest=count;result=bounds;}
    }
    return result;
}
int main(int argc,char **argv){
    const QString real=argc>1?QString::fromLocal8Bit(argv[1]):QString();
    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.show();
    mw.global_setting.b_autoOpenImg3DViewer=false;mw.global_setting.b_autoConvert2_8bit=false;mw.global_setting.b_yaxis_up=false;
    auto fixture=[&](QString name){
        auto child=mw.createMdiChild();auto pixels=new unsigned char[32*32*32]();
        for(int z=4;z<28;++z)for(int y=4;y<28;++y)for(int x=4;x<28;++x)pixels[z*32*32+y*32+x]=200;
        child->newProcessedImage(name,pixels,32,32,32,1,V3D_UINT8);child->show();child->doImage3DView();return child;};
    XFormWidget *child=nullptr;
    if(real.isEmpty())child=fixture("Voxel spacing A");
    else {
        mw.loadV3DFile(real,false,false);
        for(auto c:mw.findChildren<XFormWidget*>())if(c->getImageData()&&QString::fromUtf8(c->getImageData()->getFileName())==real)child=c;
        assert(child);assert(near(child->getImageData()->getRezX(),.564542)&&near(child->getImageData()->getRezZ(),1.444444));
        child->doImage3DView();
    }
    QTimer::singleShot(500,&app,[&](){
        auto win=child->mypara_3Dview.window3D;assert(win);win->resize(1350,900);settle();assert(win->height()==900);
        auto gl=win->getGLWidget();auto r=dynamic_cast<Renderer_gl1*>(gl->getRenderer());assert(r);
        QDoubleSpinBox *fields[3];for(int i=0;i<3;++i){fields[i]=win->findChild<QDoubleSpinBox*>(QString("voxelSpacing%1").arg("XYZ"[i]));assert(fields[i]&&fields[i]->isEnabled());}
        auto apply=win->findChild<QPushButton*>("applyVoxelSpacing");auto reset=win->findChild<QPushButton*>("resetVoxelSpacing");assert(apply&&reset);
        auto scroll=win->findChild<QScrollArea*>("volumeControlsScrollArea");assert(scroll);
        win->hideDisplayControls();assert(!scroll->isVisible());gl->setVoxSize();settle();
        assert(scroll->isVisible()&&fields[0]->isVisible());
        auto initial=fingerprint(child->getImageData());const double originalX=fields[0]->value(),originalZ=fields[2]->value();
        if(!real.isEmpty()){
            assert(near(originalX,.564542)&&near(originalZ,1.444444));
            assert(near(r->thicknessZ,1.444444/.564542));
            win->grab().save("/tmp/vaa3d-voxel-spacing-real.png");
        }
        else assert(originalX==1&&originalZ==1);
        auto setFields=[&](double x,double y,double z){fields[0]->setValue(x);fields[1]->setValue(y);fields[2]->setValue(z);apply->click();settle();
            assert(near(r->thicknessX,1)&&near(r->thicknessY,y/x)&&near(r->thicknessZ,z/x));
            assert(near(r->boundingBox.Dy()/r->boundingBox.Dx(),(r->dataBox.Dy()+1)/(r->dataBox.Dx()+1)*y/x));};
        gl->enableShowAxes(false);gl->enableShowBoundingBox(false);gl->setRenderMode_Maxip(true);gl->enableOrthoView(true);
        gl->setXRotation(0);gl->setYRotation(0);gl->setZRotation(0);settle();
        setFields(1,1,1);auto frameBefore=gl->grabFramebuffer();frameBefore.save("/tmp/vaa3d-spacing-before.png");auto before=brightBounds(frameBefore);
        setFields(1,2,.25);auto frameAfter=gl->grabFramebuffer();frameAfter.save("/tmp/vaa3d-spacing-after.png");auto after=brightBounds(frameAfter);
        if(real.isEmpty()){
            assert(!before.isEmpty()&&!after.isEmpty());
            double beforeRatio=double(before.height())/before.width(),afterRatio=double(after.height())/after.width();
            qInfo("VOXEL_FRAME_ASPECT before=%g after=%g",beforeRatio,afterRatio);assert(afterRatio>beforeRatio*1.7);
        }
        // Fractions below one must still draw slices; very large ratios must not repeat millions of slices.
        assert(!after.isEmpty());setFields(1,1,20);assert(r->thicknessZ==20);
        setFields(1,1,.5);assert(!brightBounds(gl->grabFramebuffer()).isEmpty());
        assert(!gl->setVoxelSpacing(0,1,1));assert(!gl->setVoxelSpacing(-1,1,1));
        assert(!gl->setVoxelSpacing(std::numeric_limits<double>::quiet_NaN(),1,1));assert(gl->voxelSpacing(2)==.5);
        assert(near(win->zthicknessBox->value(),.5));win->zthicknessBox->setValue(.75);
        assert(near(gl->voxelSpacing(2),.75)&&near(r->thicknessZ,.75));
        // Changing only absolute scale changes the displayed physical extent, not aspect ratio.
        setFields(2,4,1);assert(near(r->thicknessY,2)&&near(r->thicknessZ,.5));
        assert(win->findChild<QLabel*>("voxelPhysicalExtent")->text().contains(real.isEmpty()?"64":"2048"));
        // The marker and surface transforms must coincide, including a nonzero crop origin.
        gl->makeCurrent();auto oldStart1=r->start1,oldStart2=r->start2,oldStart3=r->start3;
        r->start1=3;r->start2=4;r->start3=5;r->b_surfStretch=true;
        GLdouble marker[16],surface[16];
        glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();r->setMarkerSpace();glGetDoublev(GL_MODELVIEW_MATRIX,marker);glPopMatrix();
        glPushMatrix();glLoadIdentity();r->setSurfaceStretchSpace();glGetDoublev(GL_MODELVIEW_MATRIX,surface);glPopMatrix();
        for(int i=0;i<16;++i)assert(near(marker[i],surface[i]));
        XYZ point(3,4,5);r->MarkerSpaceToNormalizeSpace(point);
        assert(near(point.x,marker[0]*3+marker[4]*4+marker[8]*5+marker[12]));
        assert(near(point.y,marker[1]*3+marker[5]*4+marker[9]*5+marker[13]));
        assert(near(point.z,marker[2]*3+marker[6]*4+marker[10]*5+marker[14]));
        r->start1=oldStart1;r->start2=oldStart2;r->start3=oldStart3;
        assert(fingerprint(child->getImageData())==initial);
        // Renderer/control reinitialization preserves per-view calibration.
        gl->createRenderer();settle(400);r=dynamic_cast<Renderer_gl1*>(gl->getRenderer());assert(r&&near(r->thicknessY,2)&&near(r->thicknessZ,.5));
        if(real.isEmpty()){
            auto other=fixture("Voxel spacing B");settle(400);auto second=other->mypara_3Dview.window3D->getGLWidget();
            assert(second->voxelSpacing(0)==1&&second->voxelSpacing(1)==1&&second->voxelSpacing(2)==1);
            assert(gl->voxelSpacing(0)==2&&gl->voxelSpacing(1)==4);
            assert(child->getImageData()->getRezX()==1&&child->getImageData()->getRezY()==1);
            other->mypara_3Dview.window3D->close();
        }
        reset->click();settle();assert(near(fields[0]->value(),originalX)&&near(fields[2]->value(),originalZ));
        // Enter commits a field without requiring the Apply button.
        fields[1]->setValue(3);QMetaObject::invokeMethod(fields[1],"editingFinished");assert(gl->voxelSpacing(1)==3);
        reset->click();settle();win->grab().save(real.isEmpty()?"/tmp/vaa3d-voxel-spacing-controls.png":"/tmp/vaa3d-voxel-spacing-real-final.png");
        assert(fingerprint(child->getImageData())==initial);
        qInfo("VOXEL_SPACING_GUI_PASS: metadata, fields, real framebuffer aspect, subunit/large ratios, invalid values, units/extents, marker/surface/crop transforms, view isolation, recreation, reset and unchanged pixels");app.quit();
    });
    QTimer::singleShot(120000,&app,[](){qFatal("Voxel spacing GUI timeout");});return app.exec();
}
