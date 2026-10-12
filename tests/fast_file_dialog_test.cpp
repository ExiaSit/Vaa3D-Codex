#include "../v3d_main/basic_c_fun/FastFileDialog.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <cassert>
#include <iostream>
static void loaded(FastFileDialog &d) {
 QElapsedTimer t;t.start();while(!d.property("directoryLoaded").toBool() && t.elapsed()<5000){QApplication::processEvents();QThread::msleep(1);}
 assert(d.property("directoryLoaded").toBool());
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);QTemporaryDir temp,settings;assert(temp.isValid());QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,settings.path());QDir dir(temp.path());
 for(int i=0;i<1205;++i){QFile f(dir.filePath(QString("image_%1.tif").arg(i,4,10,QChar('0'))));assert(f.open(QIODevice::WriteOnly));}
 assert(dir.mkdir("child"));QFile hidden(dir.filePath(".hidden"));assert(hidden.open(QIODevice::WriteOnly));hidden.close();
 QFile unicode(dir.filePath(QString::fromUtf8("中文标注.ano")));assert(unicode.open(QIODevice::WriteOnly));unicode.close();
 {
  FastFileDialog d(0,"Test",temp.path(),"TIFF (*.tif);;Annotations (*.ano)",false);loaded(d);
  assert(d.property("matchingEntries").toInt()==1206);auto list=d.findChild<QListWidget*>("fastEntries");assert(list->count()==500);
  d.findChild<QPushButton*>("fastNext")->click();assert(list->count()==500);
  d.findChild<QPushButton*>("fastNext")->click();assert(list->count()==206);
  d.findChild<QComboBox*>("fastFilter")->setCurrentIndex(1);assert(d.property("matchingEntries").toInt()==2);
  d.findChild<QLineEdit*>("fastFilename")->setText(QString::fromUtf8("中文标注.ano"));d.findChild<QPushButton*>("fastOpen")->click();
  assert(d.result()==QDialog::Accepted && d.selectedFile()==unicode.fileName());
 }
 {
  FastFileDialog d(0,"Directory",temp.path(),QString(),true);loaded(d);auto list=d.findChild<QListWidget*>("fastEntries");assert(list->count()==1);list->setCurrentRow(0);
  d.findChild<QPushButton*>("fastOpen")->click();assert(d.selectedFile()==dir.filePath("child"));
 }
 {
  FastFileDialog d(0,"Navigate",temp.path(),QString(),false);
  auto path=d.findChild<QLineEdit*>("fastPath");path->setText(dir.filePath("child"));QMetaObject::invokeMethod(path,"returnPressed",Qt::DirectConnection);loaded(d);
  assert(d.property("matchingEntries").toInt()==0);
  d.findChild<QLineEdit*>("fastFilename")->setText("missing.tif");d.findChild<QPushButton*>("fastOpen")->click();assert(d.selectedFile().isEmpty());
 }
 {
  FastFileDialog d(0,"History",temp.path(),QString(),false);
  auto files=d.findChild<QComboBox*>("fastFileHistory");
  auto dirs=d.findChild<QComboBox*>("fastPathHistory");
  assert(files && files->isEditable() && files->itemText(0)==unicode.fileName());
  assert(dirs && dirs->findText(dir.filePath("child"))>=0);
  files->setCurrentIndex(0);assert(d.selectedFile().isEmpty());
  d.findChild<QPushButton*>("fastOpen")->click();assert(d.selectedFile()==unicode.fileName());
  QSettings saved("HHMI","Vaa3D");
  auto history=saved.value("FastFileDialog/recentFiles").toStringList();
  assert(history.count(unicode.fileName())==1);
  QStringList many;for(int i=0;i<25;++i)many<<QString("/old/%1.swc").arg(i);
  saved.setValue("FastFileDialog/recentFiles",many);
 }
 {
  FastFileDialog d(0,"Bounded history",temp.path(),QString(),false);
  d.findChild<QLineEdit*>("fastFilename")->setText(unicode.fileName());
  d.findChild<QPushButton*>("fastOpen")->click();
  QSettings saved("HHMI","Vaa3D");auto history=saved.value("FastFileDialog/recentFiles").toStringList();
  assert(history.size()==20 && history.first()==unicode.fileName());
 }
 {
  FastFileDialog d(0,"Wheel paging",temp.path(),"TIFF (*.tif)",false);loaded(d);d.show();QApplication::processEvents();
  auto list=d.findChild<QListWidget*>("fastEntries");auto bar=list->verticalScrollBar();
  auto wheel=[&](int angle,int pixels=0){
   QPointF pos(20,20);QWheelEvent e(pos,list->viewport()->mapToGlobal(pos.toPoint()),QPoint(0,pixels),QPoint(0,angle),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
   QApplication::sendEvent(list->viewport(),&e);QApplication::processEvents();
  };
  QString first=list->item(0)->text();assert(bar->maximum()>0);
  wheel(-120);assert(bar->value()>0 && list->item(0)->text()==first);
  wheel(120);assert(bar->value()==0);wheel(120);assert(list->item(0)->text()==first);
  list->scrollToBottom();wheel(-120);assert(list->item(0)->text()!=first && bar->value()==0);
  QString second=list->item(0)->text();
  wheel(120);assert(list->item(0)->text()==first && bar->value()==bar->maximum());
  wheel(-120);assert(list->item(0)->text()==second);
  list->scrollToBottom();wheel(0,-25);assert(list->count()==206 && bar->value()==0);
  QString last=list->item(0)->text();list->scrollToBottom();wheel(-120);assert(list->item(0)->text()==last);
  list->scrollToTop();wheel(0,25);assert(list->item(0)->text()==second && bar->value()==bar->maximum());
  d.findChild<QLineEdit*>("fastFilename")->setText("image_0500.tif");
  d.findChild<QPushButton*>("fastOpen")->click();assert(d.selectedFile()==dir.filePath("image_0500.tif"));
 }
 {FastFileDialog cancelled(0,"Cancel",temp.path(),QString(),false);cancelled.reject();}
 QApplication::processEvents();std::cout<<"Fast file dialog tests passed\n";
}
