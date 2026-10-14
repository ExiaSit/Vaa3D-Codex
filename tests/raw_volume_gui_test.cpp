#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/v3d/ChannelTable.h"
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QTimer>
#include <QProgressDialog>
#include <QtEndian>
#include <cassert>

static void compareSlices()
{
    const int nx=260,ny=261,nz=262,n=nx*ny*nz;
    My4DImage integer, floating;
    auto u=new unsigned char[size_t(n)*2]; auto f=new unsigned char[size_t(n)*4];
    for(int i=0;i<n;++i) { reinterpret_cast<quint16*>(u)[i]=(i*37)%4096; reinterpret_cast<float*>(f)[i]=(i*37)%4096; }
    integer.setData(u,nx,ny,nz,1,V3D_UINT16); floating.setData(f,nx,ny,nz,1,V3D_FLOAT32);
    double lo=0,hi=4095; integer.setupData4D(&lo,&hi); floating.setupData4D(&lo,&hi);
    QList<Channel> channels; Channel c; c.color.r=57;c.color.g=210;c.color.b=123;c.color.a=190;channels<<c;
    QVector<QVector<RGBA8>> luts(1,QVector<RGBA8>(256));make_linear_lut_one(c.color,luts[0]);
    for(int mode=0;mode<12;++mode)
    {
        MixOP op;op.op=mode%4;op.brightness=mode>=4?0.1:0;op.contrast=mode>=4?1.3:1;
        op.maskG=mode<8; channels[0].on=mode!=11;
        for(auto plane:{imgPlaneX,imgPlaneY,imgPlaneZ})
        {
            QImage a=copyRaw2QImage_Slice(channels,op,&luts,plane,123,integer.data4d_uint16,nx,ny,nz,1,&hi,&lo);
            QImage b=copyRaw2QImage_Slice(channels,op,&luts,plane,123,floating.data4d_float32,nx,ny,nz,1,&hi,&lo);
            assert(a==b);
        }
    }
    // UInt8/non-rescaled, disabled channels, generated LUT lifetime and clamped planes.
    My4DImage bytes;auto b=new unsigned char[n];
    for(int i=0;i<n;++i) { b[i]=i%256; reinterpret_cast<float*>(f)[i]=i%256; }
    bytes.setData(b,nx,ny,nz,1,V3D_UINT8);hi=255;bytes.setupData4D(&lo,&hi);
    for(int mode=0;mode<4;++mode)
    {
        MixOP op;op.op=mode;op.rescale=false;channels[0].on=mode!=3;
        for(auto plane:{imgPlaneX,imgPlaneY,imgPlaneZ})
        {
            auto a=copyRaw2QImage_Slice(channels,op,nullptr,plane,-1,bytes.data4d_uint8,nx,ny,nz,1,&hi,&lo);
            auto b=copyRaw2QImage_Slice(channels,op,nullptr,plane,-1,floating.data4d_float32,nx,ny,nz,1,&hi,&lo);
            assert(a==b);
        }
    }
    qInfo("RAW_SLICE_PASS: exact pixel equality against generic float path, all three planes, mixing, colors, alpha, brightness, masks, channel-off, uint8/16");
}

int main(int argc,char **argv)
{
    const QString actual = argc>1 ? QString::fromLocal8Bit(argv[1]) : QString();
    Q_INIT_RESOURCE(v3d);
    auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();
    mw.global_setting.b_autoConvert2_8bit=false;mw.global_setting.b_yaxis_up=false;mw.global_setting.b_autoOpenImg3DViewer=false;
    mw.resize(1500,1050);mw.show();
    QTemporaryDir temp;
    QString fixture=temp.path()+"/large.v3draw";
    { QFile f(fixture);assert(f.open(QIODevice::WriteOnly));QByteArray h("raw_image_stack_by_hpengL");
      h.append(char(2));h.append(char(0));for(quint32 d:{512u,512u,160u,1u}){quint32 le=qToLittleEndian(d);h.append(reinterpret_cast<char*>(&le),4);}
      assert(f.write(h)==h.size());assert(f.resize(43+512LL*512*160*2)); }
    QTimer::singleShot(100,&app,[&](){
        compareSlices();
        auto child=mw.createMdiChild();auto bytes=new unsigned char[64]();
        child->newProcessedImage("preserved",bytes,4,4,4,1,V3D_UINT8);child->show();
        auto original=child->getImageData();
        QTimer cancel;
        QObject::connect(&cancel,&QTimer::timeout,&app,[&](){
            for(QWidget *widget:QApplication::topLevelWidgets())
                if(auto p=qobject_cast<QProgressDialog*>(widget))
                    if(p->objectName()=="rawVolumeLoadProgress")
                        if(auto button=p->findChild<QPushButton*>()) button->click();
        });
        cancel.start(1);assert(!child->loadFile(fixture));cancel.stop();
        assert(child->property("imageLoadCanceled").toBool() && child->getImageData()==original);
        cancel.start(1);mw.loadV3DFile(fixture,false,false);cancel.stop();
        qInfo("RAW_CANCEL_GUI_PASS: cancel preserves prior image and closes new load without plugin prompt");
        child->close();QApplication::processEvents();
        const QString path=actual.isEmpty()?fixture:actual;
        QElapsedTimer elapsed;elapsed.start();qint64 last=0,maxGap=0;int ticks=0;bool sawProgress=false;
        QTimer heartbeat;QObject::connect(&heartbeat,&QTimer::timeout,&app,[&](){
            qint64 now=elapsed.elapsed();maxGap=qMax(maxGap,now-last);last=now;++ticks;
            for(auto widget:QApplication::topLevelWidgets()) if(auto p=qobject_cast<QProgressDialog*>(widget))
                if(p->objectName()=="rawVolumeLoadProgress" && p->isVisible()) sawProgress=true;
        });heartbeat.start(20);
        mw.loadV3DFile(path,false,false);
        maxGap=qMax(maxGap,elapsed.elapsed()-last);
        qInfo("RAW_LOAD_GUI: elapsed_ms=%lld max_event_gap_ms=%lld heartbeat_ticks=%d",elapsed.elapsed(),maxGap,ticks);
        assert(sawProgress && ticks>0);
        XFormWidget *loaded=nullptr;
        for(auto c:mw.findChildren<XFormWidget*>()) if(c->getImageData() && QString::fromUtf8(c->getImageData()->getFileName())==path) loaded=c;
        assert(loaded);auto img=loaded->getImageData();assert(img->getDatatype()==V3D_UINT16);
        QFile file(path);assert(file.open(QIODevice::ReadOnly));
        const qint64 count=img->getTotalUnitNumber();
        for(qint64 index:{qint64(0),count/7,count/2,count-1})
        { assert(file.seek(43+2*index));auto raw=file.read(2);assert(raw.size()==2);assert(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(raw.constData()))==reinterpret_cast<quint16*>(img->getRawData())[index]); }
        if(!actual.isEmpty()) {assert(img->getXDim()==7682&&img->getYDim()==7288&&img->getZDim()==210);assert(img->p_vmin[0]==0&&img->p_vmax[0]==4095);}
        assert(maxGap<2000);
        loaded->showMaximized();
        QTimer::singleShot(100,&app,[&](){mw.grab().save("/tmp/vaa3d-raw-loaded.png");qInfo("RAW_VOLUME_GUI_PASS");app.exit(0);});
        heartbeat.stop();
    });
    return app.exec();
}
