#pragma once
#include <QtWidgets>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <algorithm>
#include <cstring>

// No UI screen capture: prompt coordinates are measured on the model input itself.
class MVCanvas : public QWidget {
public:
    QImage image; QJsonArray points, boxes, keep, remove, automatic; int mode=0; QPointF first; bool dragging=false;
    explicit MVCanvas(QWidget* parent=nullptr):QWidget(parent){setMinimumSize(540,440);}
    QRectF imageRect() const {QSizeF s=image.size();s.scale(size(),Qt::KeepAspectRatio);return QRectF(QPointF((width()-s.width())/2,(height()-s.height())/2),s);}
    QPointF pixel(QPointF p) const {auto r=imageRect();return QPointF((p.x()-r.x())*image.width()/r.width(),(p.y()-r.y())*image.height()/r.height());}
    void setPath(QString p){image.load(p);update();}
    void clear(){automatic=QJsonArray();points=QJsonArray();boxes=QJsonArray();keep=QJsonArray();remove=QJsonArray();update();}
    void paintEvent(QPaintEvent*) override {QPainter p(this);p.fillRect(rect(),QColor(24,26,32));if(image.isNull())return;auto r=imageRect();p.drawImage(r,image);p.translate(r.topLeft());p.scale(r.width()/image.width(),r.height()/image.height());
        p.setPen(QPen(QColor(80,170,255),2));for(auto item:automatic){auto a=item.toArray();p.drawEllipse(QPointF(a[0].toDouble(),a[1].toDouble()),3,3);}
        for(auto item:points){auto a=item.toArray();p.setPen(QPen(a[2].toInt()?Qt::green:Qt::red,3));p.drawEllipse(QPointF(a[0].toDouble(),a[1].toDouble()),6,6);}
        p.setPen(QPen(Qt::yellow,3));for(auto item:boxes){auto a=item.toArray();p.drawRect(QRectF(QPointF(a[0].toDouble(),a[1].toDouble()),QPointF(a[2].toDouble(),a[3].toDouble())));}
        p.setPen(QPen(Qt::cyan,3));for(auto item:keep){auto a=item.toArray();p.drawEllipse(QPointF(a[0].toDouble(),a[1].toDouble()),9,9);}
        p.setPen(QPen(Qt::magenta,3));for(auto item:remove){auto a=item.toArray();p.drawEllipse(QPointF(a[0].toDouble(),a[1].toDouble()),9,9);}}
    void mousePressEvent(QMouseEvent* e) override {if(image.isNull()||!imageRect().contains(e->position()))return;first=pixel(e->position());dragging=true;}
    void mouseReleaseEvent(QMouseEvent* e) override {if(!dragging)return;dragging=false;if(!imageRect().contains(e->position()))return;auto q=pixel(e->position());
        if(mode==2)boxes.append(QJsonArray{qMin(first.x(),q.x()),qMin(first.y(),q.y()),qMax(first.x(),q.x()),qMax(first.y(),q.y())});
        else if(mode==3)keep.append(QJsonArray{q.x(),q.y()});else if(mode==4)remove.append(QJsonArray{q.x(),q.y()});else points.append(QJsonArray{q.x(),q.y(),mode==0?1:0});update();}
    QJsonObject prompts() const {return {{"points",points},{"boxes",boxes},{"keep_regions",keep},{"delete_regions",remove}};}
};

class MVCloud : public QWidget {
    QVector<QVector4D> points; QJsonArray currentBox, coverageBoxes; QPoint last; float rx=15,ry=20,high=.65;
public:
    MVCloud(){setMinimumSize(280,270);setToolTip("拖动旋转；最多显示 30,000 个采样体素，完整结果请打开分割体");}
    void selectedBlock(QJsonObject b,QJsonArray spacing){if(b.isEmpty()||spacing.size()!=3)return;auto origin=b["origin"].toArray(),shape=b["valid_shape"].toArray(),full=b["global_shape"].toArray();double extent[3],scale=0;for(int d=0;d<3;d++){extent[d]=(full[d].toDouble()-1)*spacing[d].toDouble();scale=qMax(scale,extent[d]);}if(scale<=0)return;QJsonArray lo,hi;for(int d=2;d>=0;d--){lo.append((origin[d].toDouble()*spacing[d].toDouble()-extent[d]/2)/scale*2);hi.append(((origin[d].toDouble()+shape[d].toDouble()-1)*spacing[d].toDouble()-extent[d]/2)/scale*2);}currentBox=QJsonArray{lo,hi};update();}
    void setBounds(QJsonObject o){currentBox=o["bounds"].toArray();coverageBoxes=o["coverage_boxes"].toArray();update();}
    void load(QString path,double threshold){QFile f(path);if(!f.open(QIODevice::ReadOnly))return;QByteArray data=f.readAll();points.clear();high=threshold;for(qsizetype i=0;i+16<=data.size();i+=16){float p[4];memcpy(p,data.constData()+i,16);points.append(QVector4D(p[0],p[1],p[2],p[3]));}update();}
    void paintEvent(QPaintEvent*) override {QPainter painter(this);painter.fillRect(rect(),QColor(20,22,30));QMatrix4x4 m;m.rotate(rx,1,0,0);m.rotate(ry,0,1,0);QVector<QVector4D> rotated;rotated.reserve(points.size());for(auto p:points){auto q=m*QVector4D(p.x(),p.y(),p.z(),1);q.setW(p.w());rotated.append(q);}std::sort(rotated.begin(),rotated.end(),[](QVector4D a,QVector4D b){return a.z()<b.z();});float scale=qMin(width(),height())*.4;for(auto p:rotated){painter.setPen(QPen(p.w()>=high?QColor(40,230,120):QColor(245,100,60),2));painter.drawPoint(QPointF(width()/2+p.x()*scale,height()/2-p.y()*scale));}auto drawBox=[&](QJsonArray box,QColor color){if(box.size()!=2)return;auto a=box[0].toArray(),b=box[1].toArray();QPointF q[8];for(int i=0;i<8;i++){auto v=m*QVector4D((i&1)?b[0].toDouble():a[0].toDouble(),(i&2)?b[1].toDouble():a[1].toDouble(),(i&4)?b[2].toDouble():a[2].toDouble(),1);q[i]=QPointF(width()/2+v.x()*scale,height()/2-v.y()*scale);}painter.setPen(QPen(color,1));for(int i=0;i<8;i++)for(int d:{1,2,4})if(!(i&d))painter.drawLine(q[i],q[i|d]);};for(auto b:coverageBoxes)drawBox(b.toArray(),QColor(60,90,125));drawBox(currentBox,QColor(255,215,60));painter.setPen(Qt::white);painter.drawText(8,18,"全局支持度采样 · 黄框当前块 · 蓝框已覆盖");}
    void mousePressEvent(QMouseEvent* e) override {last=e->position().toPoint();}
    void mouseMoveEvent(QMouseEvent* e) override {auto p=e->position().toPoint();ry+=p.x()-last.x();rx+=p.y()-last.y();last=p;update();}
};

class PMultiView : public QDialog {
    QProcess* process; QByteArray pending; QString root,job,frame; MVCanvas* canvas; QLabel *status,*progressDetail; QProgressBar *progress,*blockProgress,*angleProgress; MVCloud* preview;QLineEdit* dataset;QDoubleSpinBox *low,*high;QComboBox *frames,*schedule;QPlainTextEdit* log;QPushButton* apply;
    std::function<void(QString,int)> rotate;std::function<void(QString)> openVolume;
    void selectedBounds(){QFile camera(job+"/frames/"+frame+"/camera.json"),volume(job+"/volume.json");if(camera.open(QIODevice::ReadOnly)&&volume.open(QIODevice::ReadOnly))preview->selectedBlock(QJsonDocument::fromJson(camera.readAll()).object()["block"].toObject(),QJsonDocument::fromJson(volume.readAll()).object()["spacing_zyx"].toArray());}
    void send(QJsonObject o){if(process->state()!=QProcess::NotRunning)process->write(QJsonDocument(o).toJson(QJsonDocument::Compact)+"\n");}
    void launch(QJsonObject command){if(process->state()!=QProcess::NotRunning)return;
        if(!QFileInfo::exists(root+"/.venv/bin/python")){QMessageBox::warning(this,"SAM2","Run multiview/install.command first. Set VAA3D_MULTIVIEW_ROOT if moved.");return;}
        process->setWorkingDirectory(root);auto env=QProcessEnvironment::systemEnvironment();env.insert("PYTORCH_ENABLE_MPS_FALLBACK","1");process->setProcessEnvironment(env);
        process->start(root+"/.venv/bin/python",QStringList{"-u","-m","neuronmv.worker","--stdio"});
        if(!process->waitForStarted(3000)){status->setText(process->errorString());return;}send(command);}
    void event(QJsonObject o){auto kind=o["event"].toString();
        if(kind=="job"){job=o["path"].toString();frames->clear();canvas->clear();setWindowTitle("多视角神经元分割 — "+QFileInfo(job).fileName());}
        if(kind=="angle"){status->setText(o["axis"].toString().toUpper()+"  "+QString::number(o["angle"].toInt())+"°");rotate(o["axis"].toString(),o["angle"].toInt());}
        if(kind=="input"){frame=o["frame"].toString();canvas->setPath(o["path"].toString());canvas->clear();}
        if(kind=="frame"){frame=o["frame"].toString();if(frames->findText(frame)<0)frames->addItem(frame);frames->setCurrentText(frame);canvas->setPath(o["path"].toString());canvas->automatic=o["auto_points"].toArray();canvas->update();}
        if(kind=="preview"){preview->load(o["cloud"].toString(),o["high"].toDouble(.65));preview->setBounds(o);selectedBounds();}
        if(kind=="progress"){progressDetail->setText(QString("总输入 %1 / %2    完成块 %3 / %4    当前计算块 %5：%6 / %7 角度").arg(o["completed"].toInt()).arg(o["total"].toInt()).arg(o["completed_blocks"].toInt()).arg(o["total_blocks"].toInt()).arg(o["block"].toString()).arg(o["block_frames"].toInt()).arg(o["block_total"].toInt()));progress->setRange(0,o["total"].toInt());progress->setValue(o["completed"].toInt());if(o.contains("total_blocks")){blockProgress->setRange(0,o["total_blocks"].toInt());blockProgress->setValue(o["completed_blocks"].toInt());angleProgress->setRange(0,o["block_total"].toInt());angleProgress->setValue(o["block_frames"].toInt());angleProgress->setFormat(o["block"].toString()+" · %v / %m 角度");}}
        if(kind=="progress")status->setText(QString("已完成 %1 / %2；本帧 %3 秒").arg(o["completed"].toInt()).arg(o["total"].toInt()).arg(o["frame_seconds"].toDouble(),0,'f',1));
        if(kind=="paused") {status->setText("已暂停；可选择已完成帧进行纠正");apply->setEnabled(true);}
        if(kind=="running")apply->setEnabled(false);
        if(kind=="error")status->setText("任务已保留，可恢复："+o["message"].toString());
        if(kind=="completed")status->setText("计算完成；候选结果需要人工检查");
        if(!job.isEmpty() && kind!="paused"){QFile events(job+"/ui-events.jsonl");if(events.open(QIODevice::Append))events.write(QJsonDocument(o).toJson(QJsonDocument::Compact)+"\n");}
        if(kind!="paused")log->appendPlainText(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));}
public:
    PMultiView(QWidget* parent,std::function<void(QString,int)> r,std::function<void(QString)> v):QDialog(parent),rotate(r),openVolume(v){
        setAttribute(Qt::WA_DeleteOnClose);setWindowTitle("多视角神经元分割 — SAM2 研究原型");resize(1100,820);
        root=qEnvironmentVariable("VAA3D_MULTIVIEW_ROOT");if(root.isEmpty())root=QDir(QCoreApplication::applicationDirPath()+"/../../../multiview").absolutePath();
        process=new QProcess(this);auto layout=new QVBoxLayout(this);
        auto form=new QHBoxLayout;dataset=new QLineEdit(qEnvironmentVariable("VAA3D_MULTIVIEW_DATASET","/Users/pzy/Desktop/60019"));form->addWidget(new QLabel("TeraFly 数据"));form->addWidget(dataset);schedule=new QComboBox;schedule->addItems({"最高分辨率自动四分块"});form->addWidget(schedule);layout->addLayout(form);
        auto note=new QLabel("最高分辨率 XY 四分块 · Z 全厚度 · 原始亮度（无增强） · 重叠 64 · X/Y 每 10° · 排除 70–140°、250–320°（含端点） · 每块 40 帧 · 全局连续支持度拼接\n二维提示以左侧模型输入为准；绿色：高支持，红色：不确定。结果未经人工验收。");note->setWordWrap(true);layout->addWidget(note);
        auto buttons=new QHBoxLayout;layout->addLayout(buttons);
        auto button=[&](QString text,std::function<void()> fn){auto b=new QPushButton(text);buttons->addWidget(b);connect(b,&QPushButton::clicked,this,fn);return b;};
        button("开始",[this]{QJsonObject config{{"dataset",dataset->text()},{"low",low->value()},{"high",high->value()}};config["algorithm_version"]="0.3-blocks";config["partition"]="four_xy";config["preprocessing"]="none";config["gamma"]=1.0;config["validation_only"]=false;launch({{"command","start"},{"config",config}});});
        button("暂停",[this]{send({{"command","pause"}});status->setText("等待当前模型操作结束…");});button("继续",[this]{send({{"command","resume"}});});
        button("恢复任务",[this]{auto p=QFileDialog::getExistingDirectory(this,"选择任务目录",root+"/jobs");if(!p.isEmpty())launch({{"resume_job",p},{"paused",true}});});
        button("取消并保留",[this]{send({{"command","cancel"}});});button("导出当前结果",[this]{send({{"command","export"}});});
        button("预览分割体",[this]{if(QFileInfo::exists(job+"/mask.tif"))openVolume(job+"/mask.tif");else status->setText("请先导出当前结果");});
        auto controls=new QHBoxLayout;layout->addLayout(controls);low=new QDoubleSpinBox;high=new QDoubleSpinBox;for(auto s:{low,high}){s->setRange(.01,1);s->setSingleStep(.05);}low->setValue(.35);high->setValue(.65);controls->addWidget(new QLabel("低支持阈值"));controls->addWidget(low);controls->addWidget(new QLabel("高支持阈值"));controls->addWidget(high);
        auto threshold=new QPushButton("保存阈值");controls->addWidget(threshold);connect(threshold,&QPushButton::clicked,this,[this]{send({{"command","thresholds"},{"low",low->value()},{"high",high->value()}});});
        auto mode=new QComboBox;mode->addItems({"正点","负点","框选","仅保留所点区域","删除所点区域"});controls->addWidget(mode);frames=new QComboBox;controls->addWidget(frames);
        auto view=new QHBoxLayout;canvas=new MVCanvas;view->addWidget(canvas,1);auto side=new QVBoxLayout;preview=new MVCloud;side->addWidget(preview);apply=new QPushButton("应用纠正并重建累计");apply->setEnabled(false);side->addWidget(apply);auto clear=new QPushButton("清除本次提示");side->addWidget(clear);auto uncertain=new QPushButton("预览不确定区域");side->addWidget(uncertain);connect(uncertain,&QPushButton::clicked,this,[this]{if(QFileInfo::exists(job+"/uncertain.tif"))openVolume(job+"/uncertain.tif");});side->addStretch();view->addLayout(side);layout->addLayout(view,1);
        connect(mode,&QComboBox::currentIndexChanged,this,[this](int n){canvas->mode=n;});connect(clear,&QPushButton::clicked,this,[this]{canvas->clear();});
        connect(frames,&QComboBox::textActivated,this,[this](QString f){frame=f;canvas->clear();QFile promptFile(job+"/frames/"+f+"/prompts.json");if(promptFile.open(QIODevice::ReadOnly)){auto o=QJsonDocument::fromJson(promptFile.readAll()).object();canvas->points=o["points"].toArray();canvas->boxes=o["boxes"].toArray();canvas->keep=o["keep_regions"].toArray();canvas->remove=o["delete_regions"].toArray();}QFile candidates(job+"/frames/"+f+"/candidates.json");if(candidates.open(QIODevice::ReadOnly))for(auto c:QJsonDocument::fromJson(candidates.readAll()).array())if(c.toObject().contains("point"))canvas->automatic.append(c.toObject()["point"]);canvas->setPath(job+"/frames/"+f+"/overlay.png");selectedBounds();});
        connect(apply,&QPushButton::clicked,this,[this]{if(!frame.isEmpty())send({{"command","correct"},{"frame",frame},{"prompts",canvas->prompts()}});});
        status=new QLabel("就绪");layout->addWidget(status);progressDetail=new QLabel("总输入 0 · 等待任务");layout->addWidget(progressDetail);progress=new QProgressBar;progress->setRange(0,160);progress->setValue(0);progress->setFormat("%v / %m 帧 (%p%)");layout->addWidget(progress);blockProgress=new QProgressBar;blockProgress->setFormat("%v / %m 块完成");angleProgress=new QProgressBar;angleProgress->setFormat("%v / %m 块内角度");layout->addWidget(blockProgress);layout->addWidget(angleProgress);log=new QPlainTextEdit;log->setReadOnly(true);log->setMaximumHeight(120);log->setMaximumBlockCount(500);layout->addWidget(log);
        connect(process,&QProcess::readyReadStandardOutput,this,[this]{pending+=process->readAllStandardOutput();while(pending.contains('\n')){int i=pending.indexOf('\n');auto line=pending.left(i);pending.remove(0,i+1);auto doc=QJsonDocument::fromJson(line);if(doc.isObject())event(doc.object());}});
        connect(process,&QProcess::readyReadStandardError,this,[this]{log->appendPlainText(QString::fromUtf8(process->readAllStandardError()));});
        connect(process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){status->setText(process->errorString());});
        QString resume=qEnvironmentVariable("VAA3D_MULTIVIEW_RESUME");if(!resume.isEmpty())QTimer::singleShot(0,this,[this,resume]{launch({{"resume_job",resume},{"paused",qEnvironmentVariableIntValue("VAA3D_MULTIVIEW_RESUME_PAUSED")!=0}});});
    }
    void closeEvent(QCloseEvent* e) override {if(process->state()!=QProcess::NotRunning){hide();e->ignore();}else QDialog::closeEvent(e);}
};
