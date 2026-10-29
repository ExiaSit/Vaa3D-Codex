#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl2.h"
#include <QProgressDialog>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QTimer>
#include <cassert>
static void settle(int ms){QElapsedTimer t;t.start();while(t.elapsed()<ms)qApp->processEvents();}
static QByteArray imageHash(My4DImage *im){return QCryptographicHash::hash(QByteArray::fromRawData((char*)im->getRawData(),im->getTotalBytes()),QCryptographicHash::Sha256);}
int main(int argc,char **argv){
    QString real=argc>1?QString::fromLocal8Bit(argv[1]):QString();
    if(!real.isEmpty()&&!QFileInfo(real).isFile())qFatal("Input TIFF is not available");
    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();
    mw.global_setting.b_autoOpenImg3DViewer=false;mw.global_setting.b_autoConvert2_8bit=false;mw.show();
    XFormWidget *child=nullptr;
    if(real.isEmpty()){
        child=mw.createMdiChild();auto raw=new unsigned char[32*24*12*4*2];auto data=(uint16_t*)raw;
        for(int c=0;c<4;++c)for(int z=0;z<12;++z)for(int y=0;y<24;++y)for(int x=0;x<32;++x)
            data[((c*12+z)*24+y)*32+x]=1000*c+100*z+10*y+x;
        child->newProcessedImage("Rescale fixture",raw,32,24,12,4,V3D_UINT16);
        child->getImageData()->setRezX(.5);child->getImageData()->setRezY(.75);child->getImageData()->setRezZ(2);
    }else{
        mw.loadV3DFile(real,false,false);
        for(auto c:mw.findChildren<XFormWidget*>())if(c->getImageData()&&QString::fromUtf8(c->getImageData()->getFileName())==real)child=c;
    }
    assert(child);child->show();child->doImage3DView();
    QTimer::singleShot(500,&app,[&](){
        auto win=child->mypara_3Dview.window3D;assert(win);win->resize(1300,900);settle(200);
        auto gl=win->getGLWidget();auto r=dynamic_cast<Renderer_gl1*>(gl->getRenderer());assert(r);
        auto button=win->findChild<QPushButton*>("rescaleOneMicron");assert(button&&button->isEnabled());
        auto initial=imageHash(child->getImageData());auto beforeCount=mw.findChildren<XFormWidget*>().size();
        auto field=win->findChild<QDoubleSpinBox*>("voxelSpacingY");double old=field->value(),ratio=r->thicknessY;
        field->setValue(old*2);QMetaObject::invokeMethod(field,"editingFinished");assert(r->thicknessY==ratio);
        field->setValue(old);
        bool done=false,success=false;int beats=0;QTimer heartbeat;heartbeat.setInterval(10);
        QObject::connect(&heartbeat,&QTimer::timeout,[&](){++beats;});heartbeat.start();
        auto conn=QObject::connect(gl,&V3dR_GLWidget::rescaleFinished,[&](bool ok){done=true;success=ok;});
        button->click();assert(!button->isEnabled());
        QElapsedTimer elapsed;elapsed.start();while(!done&&elapsed.elapsed()<120000)qApp->processEvents();
        assert(done&&success);settle(500);assert(button->isEnabled());
        XFormWidget *output=nullptr;
        for(auto c:mw.findChildren<XFormWidget*>())if(c->property("rescaledOneMicron").toBool())output=c;
        assert(output&&mw.findChildren<XFormWidget*>().size()==beforeCount+1);
        auto im=output->getImageData();assert(im&&im->getCDim()==4&&im->getDatatype()==V3D_UINT16);
        assert(im->getRezX()==1&&im->getRezY()==1&&im->getRezZ()==1);
        assert(im->getXDim()==(real.isEmpty()?16:578)&&im->getYDim()==(real.isEmpty()?18:578)&&im->getZDim()==(real.isEmpty()?24:40));
        auto outwin=output->mypara_3Dview.window3D;assert(outwin);outwin->resize(1300,900);settle(300);
        auto outr=dynamic_cast<Renderer_gl1*>(outwin->getGLWidget()->getRenderer());assert(outr&&outr->thicknessX==1&&outr->thicknessY==1&&outr->thicknessZ==1);
        assert(imageHash(child->getImageData())==initial);
        if(real.isEmpty()){
            auto out=(uint16_t*)im->getRawData();assert(out[0]==3); // X area mean .5, Y area mean 2.5, Z edge 0.
            assert(out[16*18*24]-out[0]==1000);
        }else{
            assert(beats>2);QFile f("/tmp/vaa3d-rescale-real.raw");assert(f.open(QIODevice::WriteOnly));
            assert(f.write((char*)im->getRawData(),im->getTotalBytes())==im->getTotalBytes());f.close();
        }
        outwin->grab().save(real.isEmpty()?"/tmp/vaa3d-rescale-controls.png":"/tmp/vaa3d-rescale-real.png");
        done=false;success=true;button->click();
        auto progress=win->findChild<QProgressDialog*>("rescaleProgress");assert(progress);
        auto cancel=progress->findChild<QPushButton*>();assert(cancel);cancel->click();
        elapsed.restart();while(!done&&elapsed.elapsed()<120000)qApp->processEvents();
        assert(done&&!success);assert(mw.findChildren<XFormWidget*>().size()==beforeCount+1);
        assert(imageHash(child->getImageData())==initial);
        QObject::disconnect(conn);
        if(real.isEmpty()){
            gl->rescaleOneMicron(.5,.75,2);win->close();settle(800);
            assert(mw.findChildren<XFormWidget*>().size()==beforeCount+1);
            assert(imageHash(child->getImageData())==initial);
        }
        qInfo("RESCALE_GUI_PASS: dims, four uint16 channels, 1um geometry, unchanged source, no live reinterpretation, cancellation, heartbeat=%d",beats);
        app.quit();
    });
    QTimer::singleShot(180000,&app,[](){qFatal("Rescale timeout");});return app.exec();
}
