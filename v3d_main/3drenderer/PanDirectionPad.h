#ifndef VAA3D_PAN_DIRECTION_PAD_H
#define VAA3D_PAN_DIRECTION_PAD_H
#include <QWidget>
#include <QAbstractSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>

// Drives the existing Shift sliders, preserving their renderer/spinbox connections.
class PanDirectionPad : public QWidget {
    QPointer<QAbstractSlider> x,y;
    QSpinBox *step;
    void advance(int dx,int dy) {
        if(!x || !y)return;
        int distance=step->value();
        if(dx>0)distance=qMin(distance,x->maximum()-x->value());
        if(dx<0)distance=qMin(distance,x->value()-x->minimum());
        if(dy>0)distance=qMin(distance,y->maximum()-y->value());
        if(dy<0)distance=qMin(distance,y->value()-y->minimum());
        // Stop at the first boundary, keeping a diagonal move diagonal.
        if(distance<=0)return;
        x->setValue(x->value()+dx*distance);
        y->setValue(y->value()+dy*distance);
    }
public:
    PanDirectionPad(QAbstractSlider *horizontal,QAbstractSlider *vertical,QWidget *parent=0):QWidget(parent),x(horizontal),y(vertical) {
        setObjectName("panDirectionPad");
        auto layout=new QGridLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(2);
        const char *arrows[]={"↖","↑","↗","←","平移","→","↙","↓","↘"};
        for(int row=0;row<3;++row)for(int col=0;col<3;++col) {
            const int vx=col-1,vy=1-row;
            if(vx==0 && vy==0) {
                auto label=new QLabel(QString::fromUtf8("平移"),this);label->setAlignment(Qt::AlignCenter);
                layout->addWidget(label,row,col);continue;
            }
            auto button=new QPushButton(QString::fromUtf8(arrows[row*3+col]),this);
            button->setObjectName(QString("pan_%1_%2").arg(vx).arg(vy));button->setAutoDefault(false);
            button->setMinimumHeight(25);layout->addWidget(button,row,col);
            button->setAutoRepeat(false);
            connect(button,&QPushButton::clicked,this,[this,vx,vy](){advance(vx,vy);});
        }
        step=new QSpinBox(this);step->setObjectName("panStep");step->setRange(1,100);step->setValue(5);
        step->setToolTip(QString::fromUtf8("每次移动的 Shift 数值；斜向每个轴使用同一步长"));
        layout->addWidget(new QLabel("Step",this),3,0);layout->addWidget(step,3,1,1,2);

    }
};
#endif
