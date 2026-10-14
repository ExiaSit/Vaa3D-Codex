#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include <QTimer>
#include <QMouseEvent>
#include <QStyleOptionTitleBar>
#include <QElapsedTimer>
#include <cassert>
static void settle(){QElapsedTimer t;t.start();while(t.elapsed()<80)QApplication::processEvents();}
int main(int argc,char **argv){
 Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);auto &mw=*app.getMainWindow();mw.resize(1500,1050);mw.show();
 auto child=mw.createMdiChild();auto data=new unsigned char[120*90*60];for(int i=0;i<120*90*60;++i)data[i]=i%251;
 child->newProcessedImage("Triview resize test",data,120,90,60,1,V3D_UINT8);child->show();
 QTimer::singleShot(200,&app,[&](){
  auto xy=child->findChild<XFormView*>("triViewXY");auto zy=child->findChild<XFormView*>("triViewZY");auto xz=child->findChild<XFormView*>("triViewXZ");auto canvas=child->findChild<QWidget*>("triViewCanvas");assert(xy&&zy&&xz&&canvas);
  auto verify=[&](){
   assert(qAbs(xy->get_disp_scale()-zy->get_disp_scale())<1e-8 && qAbs(xy->get_disp_scale()-xz->get_disp_scale())<1e-8);
   assert(qAbs(xy->width()-120*xy->get_disp_scale())<=1 && qAbs(xy->height()-90*xy->get_disp_scale())<=1);
   assert(zy->geometry().right()<canvas->width() && xz->geometry().bottom()<canvas->height());
   QPoint mouse(qRound(35*xy->get_disp_scale()),qRound(27*xy->get_disp_scale()));QPointF p=xy->mouseEventToImageCoords(mouse);
   assert(qAbs(p.x()-35.5)<1.5 && qAbs(p.y()-27.5)<1.5);
  };
  child->resize(950,850);settle();verify();double small=xy->get_disp_scale();
  QPolygon roi;roi<<QPoint(10,12)<<QPoint(40,12)<<QPoint(40,30);xy->roiPolygon=roi;
  const int focus=xy->focusPlaneCoord();child->resize(1300,1000);settle();verify();assert(xy->get_disp_scale()>small && xy->roiPolygon==roi && xy->focusPlaneCoord()==focus);
  child->grab().save("/tmp/vaa3d-triview-large.png");child->resize(800,800);settle();verify();assert(xy->get_disp_scale()<small);child->grab().save("/tmp/vaa3d-triview-small.png");
  auto titleClick=[&](QStyle::SubControl control){QStyleOptionTitleBar option;option.initFrom(child);option.rect=QRect(0,0,child->width(),child->style()->pixelMetric(QStyle::PM_TitleBarHeight,&option,child));option.titleBarFlags=child->windowFlags();option.titleBarState=int(child->windowState());auto point=child->style()->subControlRect(QStyle::CC_TitleBar,&option,control,child).center();QMouseEvent move(QEvent::MouseMove,point,child->mapToGlobal(point),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(child,&move);settle();QMouseEvent press(QEvent::MouseButtonPress,point,child->mapToGlobal(point),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(child,&press);QMouseEvent release(QEvent::MouseButtonRelease,point,child->mapToGlobal(point),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(child,&release);settle();};
  titleClick(QStyle::SC_TitleBarMaxButton);assert(child->isMaximized());verify();child->showNormal();settle();assert(!child->isMaximized());verify();
  titleClick(QStyle::SC_TitleBarMinButton);assert(child->isMinimized());child->showNormal();settle();verify();
  xy->changeScale(8);settle();
  QPointF panBefore=xy->mouseEventToImageCoords(QPoint(xy->width()/2,xy->height()/2));
  child->resize(1200,950);settle();
  QPointF panAfter=xy->mouseEventToImageCoords(QPoint(xy->width()/2,xy->height()/2));
  assert(qAbs(panBefore.x()-panAfter.x())<1 && qAbs(panBefore.y()-panAfter.y())<1);
  assert(xy->roiPolygon==roi && xy->focusPlaneCoord()==focus);
  xy->changeScale(4);settle();verify();
  child->triview_setzoom(1,false);settle();assert(xy->get_disp_scale()==1);child->fitTriviewToWindow();settle();verify();
  child->hideDisplayControls();settle();verify();child->hideDisplayInfo();settle();verify();
  qInfo("TRIVIEW_RESIZE_PASS: title buttons, restore, proportional resizing, ROI and coordinate mapping, manual zoom and auto fit");app.exit(0);
 });QTimer::singleShot(20000,&app,[](){qFatal("Triview resize timeout");});return app.exec();
}
