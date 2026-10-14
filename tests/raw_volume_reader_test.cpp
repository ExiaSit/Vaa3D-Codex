#include "../v3d_main/basic_c_fun/RawVolumeReader.h"
#include <QtTest>
#include <QTemporaryDir>

class RawReaderTest : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    QString path() const { return temp.path()+"/图像.v3draw"; }
    static QByteArray encode(quint32 v, int width, bool little)
    {
        QByteArray b(width,0);
        for (int i=0;i<width;++i) b[little ? i : width-1-i]=char(v>>(8*i));
        return b;
    }
    QByteArray header(int type, bool little, int width, const QList<quint32> &dims)
    {
        QByteArray b("raw_image_stack_by_hpeng"); b+=little?'L':'B'; b+=encode(type,2,little);
        for (auto d:dims) b+=encode(d,width,little);
        return b;
    }
    void save(const QByteArray &b) { QFile f(path()); QVERIFY(f.open(QIODevice::WriteOnly)); QCOMPARE(f.write(b),qint64(b.size())); }
private slots:
    void formats()
    {
        for (int type : {1,2,4}) for (bool little : {false,true}) for (int width : {2,4})
        {
            QByteArray h=header(type,little,width,{3,2,2,2}), native;
            std::vector<double> lo(2,1e9), hi(2,-1e9);
            for (int i=0;i<24;++i)
            {
                double value = type==4 ? (i-15)*0.25 : type==2 ? i*291 : i*7;
                lo[i/12]=std::min(lo[i/12],value); hi[i/12]=std::max(hi[i/12],value);
                quint32 bits=0;
                if (type==4) { float v=value; std::memcpy(&bits,&v,4); native.append(reinterpret_cast<char*>(&v),4); }
                else if (type==2) { quint16 v=value; bits=v; native.append(reinterpret_cast<char*>(&v),2); }
                else { bits=value; native+=char(bits); }
                h+=encode(bits,type,little);
            }
            save(h); RawVolumeReader r(path()); r.start(); QVERIFY(r.wait(10000));
            QVERIFY2(r.success,qPrintable(r.error)); QCOMPARE(r.datatype,type); QCOMPARE(r.dims[3],quint64(2));
            QVERIFY(std::memcmp(native.constData(),r.data.get(),size_t(native.size()))==0);
            QVERIFY(r.minimum==lo); QVERIFY(r.maximum==hi); QCOMPARE(r.progress.load(),100);
        }
    }
    void invalidFiles()
    {
        QList<QByteArray> invalid;
        invalid << QByteArray("invalid") << header(3,true,4,{2,2,2,1})+QByteArray(24,0)
                << header(2,true,4,{0,2,2,1}) << header(2,true,4,{2,2,2,1})+QByteArray(15,0)
                << header(4,true,4,{0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff})
                << header(2,true,4,{2,2,2,1})+QByteArray(17,0);
        for (const auto &bytes:invalid)
        {
            save(bytes); RawVolumeReader r(path()); r.start(); QVERIFY(r.wait(10000));
            QVERIFY(!r.success); QVERIFY(!r.data); QVERIFY(!r.error.isEmpty());
        }
        RawVolumeReader missing(temp.path()+"/missing"); missing.start(); QVERIFY(missing.wait(10000));
        QVERIFY(!missing.success); QVERIFY(!missing.error.isEmpty());
    }
    void chunkBoundariesAndCancel()
    {
        // Two channels, each just beyond the 8 MiB chunk boundary; exact ranges and ownership.
        QByteArray h=header(2,true,4,{4194305,1,1,2});
        QByteArray pixels(4194305*4,0); pixels[8388608]=char(123); pixels[pixels.size()-2]=char(234);
        save(h+pixels); RawVolumeReader r(path()); r.start(); QVERIFY(r.wait(10000)); QVERIFY(r.success);
        QCOMPARE(r.maximum[0],123.0); QCOMPARE(r.maximum[1],234.0);
        RawVolumeReader canceled(path()); canceled.cancel=true; canceled.start(); QVERIFY(canceled.wait(10000));
        QVERIFY(!canceled.success); QVERIFY(!canceled.data); QVERIFY(canceled.error.isEmpty());
        // Sparse fixture: cancel during streaming without writing a multi-GB test image.
        QFile f(path()); QVERIFY(f.open(QIODevice::WriteOnly));
        h=header(2,true,4,{1024,1024,1024,1}); QCOMPARE(f.write(h),qint64(h.size()));
        QVERIFY(f.resize(qint64(h.size())+2LL*1024*1024*1024)); f.close();
        RawVolumeReader running(path()); running.start();
        QTRY_VERIFY_WITH_TIMEOUT(running.progress.load()>0,10000);
        running.cancel=true; QVERIFY(running.wait(10000)); QVERIFY(!running.success); QVERIFY(!running.data);
    }
};
QTEST_GUILESS_MAIN(RawReaderTest)
#include "raw_volume_reader_test.moc"
