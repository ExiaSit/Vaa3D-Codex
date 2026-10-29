#include "../v3d_main/basic_c_fun/TiffVoxelSpacing.h"
#include <QCoreApplication>
#include <cassert>
#include <limits>

static QByteArray xml(QString attrs) {
    return ("<OME xmlns=\"http://www.openmicroscopy.org/Schemas/OME/2015-01\"><Image><Pixels SizeX=\"32\" SizeY=\"24\" SizeZ=\"16\" SizeC=\"4\" SizeT=\"1\" "+attrs+" /></Image></OME>").toUtf8();
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);double spacing[3]={9,8,7};
    auto parse=[&](QByteArray text){return parseOmeVoxelSpacing(text,32,24,16,4,spacing);};
    assert(parse(xml("PhysicalSizeX=\"0.564542\" PhysicalSizeY=\"0.564542\" PhysicalSizeZ=\"1.444444\"")));
    assert(spacing[0]==.564542&&spacing[1]==.564542&&spacing[2]==1.444444);
    assert(parse(xml("PhysicalSizeX=\"564.542\" PhysicalSizeXUnit=\"nm\" PhysicalSizeY=\"0.000564542\" PhysicalSizeYUnit=\"mm\" PhysicalSizeZ=\"1.444444\" PhysicalSizeZUnit=\"µm\"")));
    assert(std::abs(spacing[0]-.564542)<1e-12&&std::abs(spacing[1]-.564542)<1e-12);
    const double first=spacing[0];
    for(QString attrs:{QString(),QString("PhysicalSizeX=\"0\" PhysicalSizeY=\"1\" PhysicalSizeZ=\"1\""),
        QString("PhysicalSizeX=\"nan\" PhysicalSizeY=\"1\" PhysicalSizeZ=\"1\""),
        QString("PhysicalSizeX=\"-1\" PhysicalSizeY=\"1\" PhysicalSizeZ=\"1\""),
        QString("PhysicalSizeX=\"1\" PhysicalSizeXUnit=\"pixel\" PhysicalSizeY=\"1\" PhysicalSizeZ=\"1\"")}) {
        assert(!parse(xml(attrs)));assert(spacing[0]==first);
    }
    auto good=xml("PhysicalSizeX=\"1\" PhysicalSizeY=\"2\" PhysicalSizeZ=\"3\"");
    assert(!parse(good.left(good.size()-5)));
    assert(!parse(QByteArray(good).replace("SizeZ=\"16\"","SizeZ=\"17\"")));
    assert(!parse(QByteArray(good).replace("SizeT=\"1\"","SizeT=\"2\"")));
    assert(!parse(QByteArray(good).replace("</Image>","<Pixels /></Image>")));
    if(argc>1) {
        assert(readTiffVoxelSpacing(QString::fromLocal8Bit(argv[1]),1024,1024,28,4,spacing));
        assert(spacing[0]==.564542 && spacing[1]==.564542 && spacing[2]==1.444444);
    }
    puts("TIFF_VOXEL_SPACING_PASS: OME defaults, units, missing/invalid/mismatched/ambiguous metadata and unchanged output on failure");
}
