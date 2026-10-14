#ifndef VAA3D_RAW_VOLUME_READER_H
#define VAA3D_RAW_VOLUME_READER_H

#include <QFile>
#include <QThread>
#include <QtEndian>
#include <QString>
#include <atomic>
#include <memory>
#include <vector>
#include <limits>
#include <cstring>
#include <algorithm>

// Keep the streaming hot loops optimized even in the local debug build.
#if defined(__GNUC__) && !defined(__clang__)
#define VAA3D_RAW_HOT __attribute__((optimize("O3")))
#else
#define VAA3D_RAW_HOT
#endif

// No widgets, callbacks, or image ownership changes on the worker thread.
// The caller consumes the result only after wait(); cancel is checked every 8 MiB.
class RawVolumeReader : public QThread
{
public:
    explicit RawVolumeReader(const QString &path) : path(path) {}
    std::atomic<bool> cancel{false};
    std::atomic<int> progress{0};
    std::unique_ptr<unsigned char[]> data;
    quint64 dims[4] = {0,0,0,0};
    int datatype = 0;
    std::vector<double> minimum, maximum;
    QString error;
    bool success = false;

protected:
    void run() override
    {
        try { read(); }
        catch (const std::bad_alloc &) { error = QString::fromUtf8("内存分配失败，无法加载此图像。"); }
        catch (...) { error = QString::fromUtf8("读取图像时出现异常。"); }
        if (!success) data.reset();
    }

private:
    QString path;
    static quint64 number(const uchar *p, int width, bool little)
    {
        if (width == 2) return little ? qFromLittleEndian<quint16>(p) : qFromBigEndian<quint16>(p);
        return little ? qFromLittleEndian<quint32>(p) : qFromBigEndian<quint32>(p);
    }

    template<class T> VAA3D_RAW_HOT
    static void scan(T *p, size_t n, bool swap, bool first, double &low, double &high)
    {
        T lo = static_cast<T>(low), hi = static_cast<T>(high);
        if (first)
        {
            T v = p[0];
            if (swap)
            {
                auto bytes = reinterpret_cast<unsigned char*>(&v);
                for (size_t j=0; j<sizeof(T)/2; ++j) std::swap(bytes[j], bytes[sizeof(T)-1-j]);
            }
            lo = hi = v;
        }
        for (size_t i=0; i<n; ++i)
        {
            if (swap)
            {
                auto bytes = reinterpret_cast<unsigned char*>(p+i);
                for (size_t j=0; j<sizeof(T)/2; ++j) std::swap(bytes[j], bytes[sizeof(T)-1-j]);
            }
            T v = p[i];
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }
        low = lo; high = hi;
    }

    void read()
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return; }
        QByteArray h = file.read(43);
        if (h.size()<35 || std::memcmp(h.constData(), "raw_image_stack_by_hpeng", 24) != 0 ||
            (h[24]!='L' && h[24]!='B'))
        { error = QString::fromUtf8("无效的 V3DRAW 文件头。"); return; }
        const bool little = h[24]=='L';
        const uchar *bytes = reinterpret_cast<const uchar*>(h.constData());
        datatype = int(number(bytes+25, 2, little));
        if (datatype!=1 && datatype!=2 && datatype!=4)
        { error = QString::fromUtf8("不支持的 V3DRAW 像素类型。"); return; }
        quint64 total = 0;
        int headerSize = 0;
        // Standard 32-bit dimensions, then the original 16-bit dimension format.
        for (int width : {4, 2})
        {
            if (h.size()<27+4*width) continue;
            quint64 count = datatype;
            bool valid = true;
            for (int k=0; k<4; ++k)
            {
                dims[k] = number(bytes+27+k*width, width, little);
                if (!dims[k] || dims[k]>quint64(std::numeric_limits<int>::max()) ||
                    count > quint64(std::numeric_limits<qint64>::max())/dims[k])
                { valid = false; break; }
                count *= dims[k];
            }
            if (valid && count == quint64(file.size()-(27+4*width)))
            { total = count; headerSize = 27+4*width; break; }
        }
        if (!headerSize || total>std::numeric_limits<size_t>::max())
        { error = QString::fromUtf8("V3DRAW 尺寸与文件大小不符、文件不完整或尺寸溢出。"); return; }
        if (cancel.load()) return;
        if (!file.seek(headerSize)) { error=file.errorString(); return; }
        data.reset(new unsigned char[size_t(total)]);
        const double high = datatype==1 ? 255 : datatype==2 ? 65535 : std::numeric_limits<float>::max();
        const double low = datatype==4 ? -std::numeric_limits<float>::max() : 0;
        minimum.assign(size_t(dims[3]), high);
        maximum.assign(size_t(dims[3]), low);
        const quint64 channelBytes = total/dims[3];
        const bool swap = (little != (Q_BYTE_ORDER == Q_LITTLE_ENDIAN));
        quint64 done = 0;
        for (size_t channel=0; channel<size_t(dims[3]); ++channel)
        {
            for (quint64 pos=0; pos<channelBytes;)
            {
                if (cancel.load()) return;
                const qint64 chunk = qint64(std::min<quint64>(8*1024*1024, channelBytes-pos));
                unsigned char *out = data.get()+done;
                qint64 received=0;
                while (received<chunk)
                {
                    if (cancel.load()) return;
                    qint64 got = file.read(reinterpret_cast<char*>(out)+received, chunk-received);
                    if (got<=0) { error=QString::fromUtf8("读取中断或文件被截短：")+file.errorString(); return; }
                    received+=got;
                }
                if (datatype==1) scan(out, size_t(chunk), false, pos==0, minimum[channel], maximum[channel]);
                else if (datatype==2) scan(reinterpret_cast<quint16*>(out), size_t(chunk/2), swap, pos==0, minimum[channel], maximum[channel]);
                else scan(reinterpret_cast<float*>(out), size_t(chunk/4), swap, pos==0, minimum[channel], maximum[channel]);
                done += chunk; pos += chunk;
                progress.store(int(100.0*double(done)/double(total)));
            }
        }
        success = !cancel.load();
    }
};
#undef VAA3D_RAW_HOT
#endif
