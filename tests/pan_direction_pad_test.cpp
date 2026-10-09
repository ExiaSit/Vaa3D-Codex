#include "../v3d_main/3drenderer/PanDirectionPad.h"
#include <QApplication>
#include <QSlider>
#include <QElapsedTimer>
#include <QThread>
#include <cassert>
#include <iostream>
static void wait(int ms){QElapsedTimer t;t.start();while(t.elapsed()<ms){QApplication::processEvents();QThread::msleep(1);}}
int main(int argc,char **argv){
 QApplication app(argc,argv);QSlider x,y;x.setRange(-100,100);y.setRange(-100,100);
 PanDirectionPad pad(&x,&y);pad.resize(280,180);pad.show();
 auto step=pad.findChild<QSpinBox*>("panStep");step->setValue(7);
 for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy){
  if(!dx&&!dy)continue;x.setValue(0);y.setValue(0);
  pad.findChild<QPushButton*>(QString("pan_%1_%2").arg(dx).arg(dy))->click();
  assert(x.value()==7*dx && y.value()==7*dy);
 }
 x.setValue(98);y.setValue(10);pad.findChild<QPushButton*>("pan_1_1")->click();assert(x.value()==100 && y.value()==12);
 pad.findChild<QPushButton*>("pan_1_1")->click();assert(y.value()==12);
 x.setValue(0);y.setValue(0);step->setValue(40);
 auto downLeft=pad.findChild<QPushButton*>("pan_-1_-1");
 assert(!downLeft->autoRepeat());
 downLeft->click();wait(350);assert(x.value()==-40 && y.value()==-40);
 downLeft->click();assert(x.value()==-80 && y.value()==-80);
 downLeft->click();assert(x.value()==-100 && y.value()==-100);
 downLeft->click();assert(x.value()==-100 && y.value()==-100);
 std::cout<<"Eight directions, one step per click, no delayed movement and boundary clamp passed\n";
}
