#ifndef VAA3D_MIP_VIEWER_H
#define VAA3D_MIP_VIEWER_H
#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QCheckBox>
#include <QMessageBox>
#include <QTimer>
#include <cmath>
#include <vector>
#include "../basic_c_fun/MaximumZProjection.h"
#include "../basic_c_fun/FastFileDialog.h"
#include "../basic_c_fun/basic_4dimage.h"
#include "../basic_c_fun/basic_surf_objs.h"

// Independent snapshots: source image windows may be closed after opening a MIP.
class MIPViewer : public QDialog {
    QGraphicsScene *scene;
    QGraphicsView *view;
    QVBoxLayout *layers;
    int width,height;
    V3DLONG depth;
    void addLayer(const QString &name,QGraphicsItem *item) {
        auto toggle=new QCheckBox(name,this);toggle->setChecked(true);
        layers->insertWidget(layers->count()-1,toggle);
        connect(toggle,&QCheckBox::toggled,this,[item](bool on){item->setVisible(on);});
    }
    void importSWC() {
        const QString file=FastFileDialog::getOpenFileName(this,QString::fromUtf8("导入 SWC/ESWC"),QString(),"Neuron (*.swc *.eswc *.SWC *.ESWC)");
        if(file.isEmpty())return;
        const NeuronTree tree=readSWC_file(file);
        if(tree.listNeuron.isEmpty()){QMessageBox::warning(this,"MIP",QString::fromUtf8("文件中没有可显示的神经元节点。"));return;}
        QPainterPath path;
        for(const NeuronSWC &node:tree.listNeuron) {
            if(!std::isfinite(node.x)||!std::isfinite(node.y))continue;
            if(tree.hashNeuron.contains(node.pn)) {
                const NeuronSWC &parent=tree.listNeuron[tree.hashNeuron.value(node.pn)];
                if(!std::isfinite(parent.x)||!std::isfinite(parent.y))continue;
                path.moveTo(parent.x,parent.y);path.lineTo(node.x,node.y);
            } else path.addEllipse(QPointF(node.x,node.y),1,1);
        }
        QPen pen(QColor(0,220,255));pen.setWidthF(1);pen.setCosmetic(true);
        auto item=scene->addPath(path,pen);item->setZValue(2);
        addLayer("SWC: "+QFileInfo(file).fileName(),item);
    }
    void importMask() {
        const QString file=FastFileDialog::getOpenFileName(this,QString::fromUtf8("导入 TIFF Mask"),QString(),"TIFF (*.tif *.tiff *.TIF *.TIFF)");
        if(file.isEmpty())return;
        Image4DSimple mask;mask.loadImage(qPrintable(file));
        if(!mask.valid()){QMessageBox::warning(this,"MIP",QString::fromUtf8("无法读取 Mask。"));return;}
        if(mask.getXDim()!=width || mask.getYDim()!=height || mask.getZDim()!=depth || mask.getCDim()!=1) {
            QMessageBox::warning(this,"MIP",QString::fromUtf8("Mask 必须是单通道，且 XYZ 尺寸与原图一致：%1 × %2 × %3。") .arg(width).arg(height).arg(depth));return;
        }
        QImage overlay(width,height,QImage::Format_ARGB32);overlay.fill(Qt::transparent);
        const size_t plane=size_t(width)*height;
        const unsigned char *data=mask.getRawData();
        std::vector<unsigned char> foreground(plane);
        if(mask.getDatatype()==V3D_UINT8)nonzeroZProjection(data,foreground.data(),plane,depth);
        else if(mask.getDatatype()==V3D_UINT16)nonzeroZProjection(reinterpret_cast<const unsigned short*>(data),foreground.data(),plane,depth);
        else if(mask.getDatatype()==V3D_FLOAT32)nonzeroZProjection(reinterpret_cast<const float*>(data),foreground.data(),plane,depth);
        for(int y=0;y<height;++y) {
            QRgb *row=reinterpret_cast<QRgb*>(overlay.scanLine(y));
            for(int x=0;x<width;++x)
                if(foreground[size_t(y)*width+x])row[x]=qRgba(255,70,70,100);
        }
        auto item=scene->addPixmap(QPixmap::fromImage(overlay));item->setOffset(-.5,-.5);item->setZValue(1);
        addLayer("Mask: "+QFileInfo(file).fileName(),item);
    }
public:
    MIPViewer(QWidget *parent,const QString &title,const QImage &image,V3DLONG z):QDialog(parent),width(image.width()),height(image.height()),depth(z) {
        setAttribute(Qt::WA_DeleteOnClose);setWindowTitle(title+" — XY MIP");resize(1000,800);
        auto layout=new QVBoxLayout(this);auto buttons=new QHBoxLayout;layout->addLayout(buttons);
        auto swc=new QPushButton(QString::fromUtf8("导入 SWC/ESWC…"));
        auto mask=new QPushButton(QString::fromUtf8("导入 Mask…"));
        auto fit=new QPushButton(QString::fromUtf8("适应窗口"));
        auto plus=new QPushButton("+");auto minus=new QPushButton("−");
        buttons->addWidget(swc);buttons->addWidget(mask);buttons->addStretch();buttons->addWidget(minus);buttons->addWidget(plus);buttons->addWidget(fit);
        scene=new QGraphicsScene(this);view=new QGraphicsView(scene,this);layout->addWidget(view,1);
        view->setBackgroundBrush(Qt::black);view->setDragMode(QGraphicsView::ScrollHandDrag);
        scene->setSceneRect(-.5,-.5,width,height);
        auto background=scene->addPixmap(QPixmap::fromImage(image));background->setOffset(-.5,-.5);
        layers=new QVBoxLayout;layout->addLayout(layers);layers->addStretch();addLayer(QString::fromUtf8("原图 MIP"),background);
        auto hint=new QLabel(QString::fromUtf8("沿 Z 轴投影 · 青色：SWC · 半透明红色：Mask 非零区域 · 勾选控制显示/隐藏"));layout->addWidget(hint);
        connect(swc,&QPushButton::clicked,this,[this](){importSWC();});
        connect(mask,&QPushButton::clicked,this,[this](){importMask();});
        connect(fit,&QPushButton::clicked,this,[this](){view->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);});
        connect(plus,&QPushButton::clicked,this,[this](){view->scale(1.25,1.25);});
        connect(minus,&QPushButton::clicked,this,[this](){view->scale(.8,.8);});
        QTimer::singleShot(0,this,[this](){view->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);});
    }
};
#endif
