#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include <QTimer>
#include <cassert>

// Run with a TIFF path and an output path for independent pixel comparison.
int main(int argc, char **argv)
{
    assert(argc==3);
    const QString input=QString::fromLocal8Bit(argv[1]);
    const QString output=QString::fromLocal8Bit(argv[2]);
    Q_INIT_RESOURCE(v3d);
    auto &app=*V3dApplication::getInstance(argc,argv);
    auto &mw=*app.getMainWindow();
    mw.global_setting.b_autoConvert2_8bit=false;
    mw.global_setting.b_yaxis_up=false;
    mw.global_setting.b_autoOpenImg3DViewer=false;
    mw.resize(1500,1050); mw.show();
    QTimer::singleShot(100,&app,[&](){
        mw.loadV3DFile(input,false,false);
        XFormWidget *loaded=nullptr;
        for(auto child:mw.findChildren<XFormWidget*>())
            if(child->getImageData() && QString::fromUtf8(child->getImageData()->getFileName())==input)
                loaded=child;
        assert(loaded);
        auto image=loaded->getImageData();
        assert(image->getCDim()==4 && image->getDatatype()==V3D_UINT16);
        QFile raw(output); assert(raw.open(QIODevice::WriteOnly));
        qint64 bytes=image->getTotalUnitNumber()*2;
        assert(raw.write(reinterpret_cast<const char*>(image->getRawData()),bytes)==bytes);
        raw.close();
        qInfo("MULTICHANNEL_TIFF_GUI_LOADED: %ld x %ld x %ld x %ld",long(image->getXDim()),
              long(image->getYDim()),long(image->getZDim()),long(image->getCDim()));
        loaded->showMaximized();
        loaded->doImage3DView();
        QTimer::singleShot(2500,&app,[&](){
            mw.grab().save("/tmp/vaa3d-multichannel-tiff.png");
            qInfo("MULTICHANNEL_TIFF_GUI_PASS"); app.exit(0);
        });
    });
    return app.exec();
}
