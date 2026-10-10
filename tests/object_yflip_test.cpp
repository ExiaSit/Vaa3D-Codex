#include "../v3d_main/basic_c_fun/basic_surf_objs.h"
#include "../v3d_main/basic_c_fun/ObjectDisplayTransform.h"
#include "../v3d_main/neuron_editing/v_neuronswc.h"
#include <cassert>
#include <iostream>
int main(){
    NeuronTree a,b;NeuronSWC node;node.x=20;node.y=37;node.z=8;a.listNeuron.append(node);
    assert(!a.yFlipDisplay && !b.yFlipDisplay);
    a.yFlipDisplay=true;
    assert(objectDisplayYCoordinate(a.listNeuron[0].y,391,a.yFlipDisplay)==354);
    assert(objectDisplayYCoordinate(37,391,b.yFlipDisplay)==37);
    assert(a.listNeuron[0].x==20 && a.listNeuron[0].y==37 && a.listNeuron[0].z==8);
    NeuronTree copy;copy.deepCopy(a);assert(copy.yFlipDisplay && copy.listNeuron[0].y==37);
    a.yFlipDisplay=false;assert(objectDisplayYCoordinate(37,391,a.yFlipDisplay)==37);
    ImageMarker marker;marker.y=38;marker.yFlipDisplay=true;
    assert(objectDisplayYCoordinate(marker.y-1,391,marker.yFlipDisplay)==354);
    CellAPO cell;assert(!cell.yFlipDisplay);cell.y=37;cell.yFlipDisplay=true;
    assert(objectDisplayYCoordinate(cell.y,391,cell.yFlipDisplay)==354 && cell.y==37);
    V_NeuronSWC segment;assert(!segment.yFlipDisplay);segment.yFlipDisplay=true;
    assert(objectDisplayYCoordinate(37,391,copy.yFlipDisplay!=segment.yFlipDisplay)==37);
    assert(objectDisplayYCoordinate(120,100+199,true)==179); // local crop / standalone scene pivot
    for(double y: {0.,37.,391.,-5.,420.})
        assert(objectDisplayYCoordinate(objectDisplayYCoordinate(y,391,true),391,true)==y);
    std::cout<<"Independent flags, copy, unchanged coordinates, marker origin, segment composition and reversible reflection passed\n";
}
