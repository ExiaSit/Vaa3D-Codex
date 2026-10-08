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
 {FastFileDialog cancelled(0,"Cancel",temp.path(),QString(),false);cancelled.reject();}
 QApplication::processEvents();std::cout<<"Fast file dialog tests passed\n";
}
