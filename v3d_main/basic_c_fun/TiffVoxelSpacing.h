#ifndef VAA3D_TIFF_VOXEL_SPACING_H
#define VAA3D_TIFF_VOXEL_SPACING_H
#include <tiffio.h>
#include <QXmlStreamReader>
#include <QString>
#include <QFile>
#include <cmath>
#include <cstring>
#include <memory>

// OME length values default to micrometres. Reject ambiguous/mismatched series.
inline bool parseOmeVoxelSpacing(const QByteArray &xml, qint64 x, qint64 y,
                                qint64 z, qint64 channels, double spacing[3])
{
    QXmlStreamReader reader(xml);
    double candidate[3]={1,1,1};
    int pixelSets=0;
    bool valid=false, ome=false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement()) continue;
        if (reader.name()=="OME") ome=true;
        if (reader.name()!="Pixels") continue;
        if (++pixelSets>1) return false;
        auto a=reader.attributes();
        if (a.value("SizeX").toLongLong()!=x || a.value("SizeY").toLongLong()!=y ||
            a.value("SizeZ").toLongLong()!=z || a.value("SizeC").toLongLong()!=channels ||
            a.value("SizeT").toLongLong()!=1) continue;
        valid=true;
        for (int axis=0;axis<3;++axis) {
            const QString key=QString("PhysicalSize%1").arg("XYZ"[axis]);
            bool ok=false;
            double value=a.value(key).toDouble(&ok), factor=1;
            const QString unit=a.value(key+"Unit").toString();
            if (unit.isEmpty() || unit==QString::fromUtf8("µm") || unit=="um") factor=1;
            else if (unit=="nm") factor=.001;
            else if (unit=="mm") factor=1000;
            else if (unit=="cm") factor=10000;
            else if (unit=="m") factor=1000000;
            else if (unit=="pm") factor=.000001;
            else if (unit==QString::fromUtf8("Å")) factor=.0001;
            else ok=false;
            value*=factor;
            if (!ok || !std::isfinite(value) || value<.000001 || value>1000000) valid=false;
            candidate[axis]=value;
        }
    }
    if (!ome || reader.hasError() || !valid || pixelSets!=1) return false;
    for (int axis=0;axis<3;++axis) spacing[axis]=candidate[axis];
    return true;
}

inline bool readTiffVoxelSpacing(const QString &path, qint64 x, qint64 y,
                               qint64 z, qint64 channels, double spacing[3])
{
    if (!path.endsWith(".tif",Qt::CaseInsensitive) && !path.endsWith(".tiff",Qt::CaseInsensitive)) return false;
    std::unique_ptr<TIFF,decltype(&TIFFClose)> tif(TIFFOpen(QFile::encodeName(path).constData(),"r"),TIFFClose);
    if (!tif) return false;
    char *description=nullptr;
    if (!TIFFGetField(tif.get(),TIFFTAG_IMAGEDESCRIPTION,&description) || !description) return false;
    // Bound XML parsing work; this metadata is optional and never blocks pixel loading.
    const size_t limit=4*1024*1024;
    size_t length=0;while (length<limit && description[length])++length;
    if (length==limit) return false;
    return parseOmeVoxelSpacing(QByteArray(description,int(length)),x,y,z,channels,spacing);
}
#endif
