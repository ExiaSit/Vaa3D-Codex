// Exercise the real 3D widget and its slider connections using synthetic wheel events.
#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include <QTimer>
#include <QWheelEvent>
#include <cassert>
int main(int argc,char **argv){
 Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mainWindow=*app.getMainWindow();mainWindow.show();
 auto child=mainWindow.createMdiChild();child->newProcessedImage("Wheel zoom test",new unsigned char[32*32*32](),32,32,32,1,V3D_UINT8);child->show();child->doImage3DView();
 QTimer timer;timer.setInterval(200);QObject::connect(&timer,&QTimer::timeout,[&](){
  auto win=child->mypara_3Dview.window3D;if(!win)return;auto w=win->getGLWidget();if(!w->getRenderer())return;timer.stop();
  auto wheel=[&](int angle,int pixels=0,Qt::KeyboardModifiers modifiers=Qt::NoModifier){
   QPointF pos(100,100);QWheelEvent e(pos,w->mapToGlobal(pos.toPoint()),QPoint(0,pixels),QPoint(0,angle),Qt::NoButton,modifiers,Qt::NoScrollPhase,false);
   QApplication::sendEvent(w,&e);QApplication::processEvents();assert(e.isAccepted());
  };
  w->setZoom(0);wheel(30);assert(w->zoom()!=0); // Previously delta/100 truncated to zero.
  const int direction=w->zoom()>0?1:-1;
  w->setZoom(0);wheel(120);assert(w->zoom()==direction*4);wheel(-120);assert(w->zoom()==0);
  w->setZoom(0);for(int i=0;i<120;++i)wheel(1);assert(w->zoom()==direction*4);
  wheel(-120);assert(w->zoom()==0);wheel(0,30);assert(w->zoom()==direction*4);wheel(0,-30);assert(w->zoom()==0);
  win->setFocus();wheel(30);assert(w->zoom()==direction); // Focus elsewhere must not swallow wheel.
  w->setZoom(0);wheel(120,0,Qt::ShiftModifier);assert(w->zoom()==0);wheel(30);assert(w->zoom()==direction);
  w->setZoom(direction*100);wheel(120);assert(w->zoom()==direction*100);wheel(-120);assert(w->zoom()==direction*96);
  qInfo("WHEEL_ZOOM_GUI_PASS: small deltas, accumulated deltas, pixel deltas, focus, rotation modifier, bounds and slider synchronization");app.exit(0);
 });timer.start();QTimer::singleShot(30000,&app,[](){qFatal("Wheel zoom test timed out");});return app.exec();
}
