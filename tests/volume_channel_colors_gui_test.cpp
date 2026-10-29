#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl2.h"
#include "../v3d_main/3drenderer/v3dr_colormapDialog.h"
#include <QColorDialog>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QPointer>
#include <QTemporaryDir>
#include <QTimer>
#include <cassert>

static void settle(int ms=180) { QElapsedTimer t;t.start();while(t.elapsed()<ms)qApp->processEvents(); }
static V3dr_colormapDialog *dialogFor(V3dR_GLWidget *gl) {
    gl->volumeColormapDialog();settle();
    for(auto w:QApplication::topLevelWidgets())
        if(auto d=qobject_cast<V3dr_colormapDialog*>(w))
            if(d->windowTitle().contains(gl->getDataTitle()))return d;
    assert(false);return nullptr;
}
static void choose(V3dr_colormapDialog *d,int channel,QColor color,bool accept=true) {
    auto button=d->findChild<QPushButton*>(QString("volumeChannelColor%1").arg(channel+1));assert(button);
    QTimer::singleShot(50,[=](){
        for(auto w:QApplication::topLevelWidgets()) if(auto picker=qobject_cast<QColorDialog*>(w)) {
            picker->setCurrentColor(color);
            if(accept)picker->accept();else picker->reject();
            return;
        }
        qFatal("No color picker");
    });
    button->click();settle();
}
static void isolate(V3dr_colormapDialog *d,int channel,int count=4) {
    for(int c=0;c<count;++c) {
        auto box=d->findChild<QCheckBox*>(QString("volumeChannelEnabled%1").arg(c+1));assert(box);
        box->setChecked(c==channel);
    }
    settle();
}
static int colorPixels(QImage frame,int component) {
    int n=0;
    for(int y=0;y<frame.height();++y)for(int x=0;x<frame.width();++x) {
        QColor c=frame.pixelColor(x,y);int rgb[]={c.red(),c.green(),c.blue()};
        if(rgb[component]>30 && rgb[(component+1)%3]<rgb[component]/3 && rgb[(component+2)%3]<rgb[component]/3)++n;
    }
    return n;
}
static QByteArray sourceHash(My4DImage *img) {
    return QCryptographicHash::hash(QByteArray::fromRawData(reinterpret_cast<char*>(img->getRawData()),
            img->getTotalBytes()),QCryptographicHash::Sha256);
}
int main(int argc,char **argv) {
    const QString actual=argc>1?QString::fromLocal8Bit(argv[1]):QString();
    Q_INIT_RESOURCE(v3d);
    auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();
    mw.global_setting.b_autoConvert2_8bit=false;mw.global_setting.b_yaxis_up=false;
    mw.global_setting.b_autoOpenImg3DViewer=false;mw.show();
    auto fixture=[&](QString name,int channels){
        auto child=mw.createMdiChild();const int n=48*48*48;
        auto raw=new unsigned char[n*channels*2]();auto data=reinterpret_cast<quint16*>(raw);
        for(int c=0;c<channels;++c)for(int z=8;z<40;++z)
            for(int y=5+(c/2)*24;y<19+(c/2)*24;++y)for(int x=5+(c%2)*24;x<19+(c%2)*24;++x)
                data[c*n+z*48*48+y*48+x]=4095;
        child->newProcessedImage(name,raw,48,48,48,channels,V3D_UINT16);child->show();child->doImage3DView();return child;
    };
    XFormWidget *child=nullptr;
    if(actual.isEmpty()) child=fixture("Four channels",4);
    else {
        mw.loadV3DFile(actual,false,false);
        for(auto c:mw.findChildren<XFormWidget*>())if(c->getImageData() && QString::fromUtf8(c->getImageData()->getFileName())==actual)child=c;
        assert(child);child->doImage3DView();
    }
    QTimer::singleShot(500,&app,[&](){
        auto win=child->mypara_3Dview.window3D;assert(win);win->resize(1150,900);
        auto gl=win->getGLWidget();auto r=dynamic_cast<Renderer_gl2*>(gl->getRenderer());assert(r&&r->tryVolShader&&r->dim4==4);
        gl->enableShowAxes(false);gl->enableShowBoundingBox(false);gl->setRenderMode_Maxip(true);settle();
        auto before=sourceHash(child->getImageData());auto d=dialogFor(gl);assert(d);
        for(int ch=0;ch<4;++ch) {
            isolate(d,ch);choose(d,ch,Qt::red);gl->update();settle();
            int red=colorPixels(gl->grabFramebuffer(),0);assert(red>100);
            if(actual.isEmpty())assert(red<gl->width()*gl->height()/3);
            choose(d,ch,Qt::green);gl->update();settle();
            int green=colorPixels(gl->grabFramebuffer(),1);assert(green>100);
            qInfo("CHANNEL_COLOR_PIXELS ch=%d red=%d green=%d",ch+1,red,green);
        }
        isolate(d,-1);gl->update();settle();
        assert(colorPixels(gl->grabFramebuffer(),1)<20);
        isolate(d,3);choose(d,3,Qt::red);choose(d,3,Qt::blue,false);assert(r->colormap[3][255].r==255);
        gl->setContrast(40);assert(r->colormap[3][255].r==255 && r->colormap[3][255].b==0);
        gl->toggleTex2D3D();settle();assert(colorPixels(gl->grabFramebuffer(),0)>100);
        const int fourthPixels=colorPixels(gl->grabFramebuffer(),0);
        if(actual.isEmpty()) assert(fourthPixels>100 && fourthPixels<gl->width()*gl->height()/3);
        assert(r->texture_format==GL_RGBA8); // alpha must remain a scalar, not opaque padding
        gl->toggleTexCompression();settle();assert(colorPixels(gl->grabFramebuffer(),0)>100);
        gl->toggleTexCompression();settle();assert(colorPixels(gl->grabFramebuffer(),0)>100);
        if(actual.isEmpty()) {
            gl->toggleTexStream();r->tryTexStream=2;gl->update();settle();
            int streamed=colorPixels(gl->grabFramebuffer(),0);
            assert(streamed>100 && streamed<gl->width()*gl->height()/3);
            gl->toggleTexStream();settle();
        }
        gl->setRenderMode_Alpha(true);settle();assert(colorPixels(gl->grabFramebuffer(),0)>100);
        gl->setRenderMode_Cs3d(true);gl->setZCS(24);settle();assert(colorPixels(gl->grabFramebuffer(),0)>100);
        gl->setRenderMode_Maxip(true);
        QTemporaryDir temp;QString map=temp.path()+"/four.vaa3dcm";
        d->saveColormapFile(map);choose(d,3,Qt::blue);d->loadColormapFile(map);settle();
        assert(r->colormap[3][255].r==255 && r->colormap[3][255].b==0 && !r->volumeChannelEnabled[0]);
        { QFile f(temp.path()+"/legacy.vaa3dcm");assert(f.open(QIODevice::WriteOnly));
          f.write("curve 0 0 0 0\ncurve 0 0 1 0\ncurve 0 2 0 1\ncurve 0 2 1 1\ncurve 4 0 0 1\ncurve -1 0 0 1\ngarbage\n"); }
        d->loadColormapFile(temp.path()+"/legacy.vaa3dcm");assert(r->colormap[3][255].r==255);
        d->setDefault0();choose(d,0,Qt::blue);choose(d,1,Qt::green);choose(d,2,Qt::red);choose(d,3,Qt::magenta);
        d->grab().save(actual.isEmpty()?"/tmp/vaa3d-channel-controls.png":"/tmp/vaa3d-channel-controls-real.png");
        gl->update();settle();gl->grabFramebuffer().save(actual.isEmpty()?"/tmp/vaa3d-channel-volume.png":"/tmp/vaa3d-channel-volume-real.png");
        assert(sourceHash(child->getImageData())==before);
        if(actual.isEmpty()) {
            auto second=fixture("Single channel",1);settle(400);
            auto gl2=second->mypara_3Dview.window3D->getGLWidget();
            QPointer<V3dr_colormapDialog> d2=dialogFor(gl2);assert(d2&&d2!=d);
            assert(!d2->findChild<QPushButton*>("volumeChannelColor2")->isVisible());
            choose(d2,0,Qt::red);gl2->enableShowAxes(false);gl2->enableShowBoundingBox(false);gl2->setRenderMode_Maxip(true);settle();
            assert(colorPixels(gl2->grabFramebuffer(),0)>100);
            assert(r->colormap[0][255].b==255); // other window remains blue
            d2->undo();settle();auto r2=dynamic_cast<Renderer_gl2*>(gl2->getRenderer());
            assert(r2->colormap[0][255].r==255&&r2->colormap[0][255].g==255&&r2->colormap[0][255].b==255);
            gl2->deleteRenderer();assert(d2.isNull());
            second->mypara_3Dview.window3D->close();
        }
        qInfo("VOLUME_CHANNEL_COLORS_GUI_PASS: four independent channels, actual framebuffer colors, show/hide, cancel, contrast, 2D/3D textures, MIP/alpha/cross-section, map roundtrip/legacy, source unchanged, window isolation/lifetime");
        app.quit();
    });
    QTimer::singleShot(90000,&app,[](){qFatal("Channel colors test timeout");});
    return app.exec();
}
