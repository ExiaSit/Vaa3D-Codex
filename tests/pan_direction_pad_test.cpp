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
 auto step=pad.findChild<QSpinBox*>("panStep");auto mode=pad.findChild<QCheckBox*>("panContinuous");auto timer=pad.findChild<QTimer*>("panTimer");
 mode->setChecked(false);step->setValue(7);
 for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy){
  if(!dx&&!dy)continue;x.setValue(0);y.setValue(0);
  pad.findChild<QPushButton*>(QString("pan_%1_%2").arg(dx).arg(dy))->click();
  assert(x.value()==7*dx && y.value()==7*dy && !timer->isActive());
 }
 x.setValue(98);y.setValue(10);pad.findChild<QPushButton*>("pan_1_1")->click();assert(x.value()==100 && y.value()==12);
 pad.findChild<QPushButton*>("pan_1_1")->click();assert(y.value()==12);
 x.setValue(0);y.setValue(0);step->setValue(40);mode->setChecked(true);
 pad.findChild<QPushButton*>("pan_-1_-1")->click();wait(350);assert(x.value()==-100 && y.value()==-100 && !timer->isActive());
 x.setValue(0);y.setValue(0);step->setValue(5);pad.findChild<QPushButton*>("pan_1_0")->click();
 pad.findChild<QPushButton*>("pan_0_0")->click();int stopped=x.value();wait(150);assert(x.value()==stopped);
 pad.findChild<QPushButton*>("pan_1_0")->click();pad.hide();stopped=x.value();wait(150);assert(x.value()==stopped && !timer->isActive());
 std::cout<<"Eight directions, diagonal clamp, continuous boundary, stop and hide passed\n";
}
