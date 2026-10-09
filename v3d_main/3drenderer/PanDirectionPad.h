#ifndef VAA3D_PAN_DIRECTION_PAD_H
#define VAA3D_PAN_DIRECTION_PAD_H
#include <QWidget>
#include <QAbstractSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include <QHideEvent>
#include <QPointer>

// Drives the existing Shift sliders, preserving their renderer/spinbox connections.
class PanDirectionPad : public QWidget {
    QPointer<QAbstractSlider> x,y;
    QSpinBox *step;
    QCheckBox *continuous;
    QTimer *timer;
    int dx=0,dy=0;
    bool advance() {
        if(!x || !y){timer->stop();return false;}
        int distance=step->value();
        if(dx>0)distance=qMin(distance,x->maximum()-x->value());
        if(dx<0)distance=qMin(distance,x->value()-x->minimum());
        if(dy>0)distance=qMin(distance,y->maximum()-y->value());
        if(dy<0)distance=qMin(distance,y->value()-y->minimum());
        // Stop at the first boundary, keeping a diagonal move diagonal.
        if(distance<=0){timer->stop();return false;}
        x->setValue(x->value()+dx*distance);
        y->setValue(y->value()+dy*distance);
        const bool edge=(dx>0 && x->value()==x->maximum()) || (dx<0 && x->value()==x->minimum())
                     || (dy>0 && y->value()==y->maximum()) || (dy<0 && y->value()==y->minimum());
        if(edge)timer->stop();
        return !edge;
    }
protected:
    void hideEvent(QHideEvent *event) override {stop();QWidget::hideEvent(event);}
public:
    void stop(){timer->stop();dx=dy=0;}
    PanDirectionPad(QAbstractSlider *horizontal,QAbstractSlider *vertical,QWidget *parent=0):QWidget(parent),x(horizontal),y(vertical) {
        setObjectName("panDirectionPad");
        auto layout=new QGridLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(2);
        timer=new QTimer(this);timer->setInterval(100);timer->setObjectName("panTimer");
        connect(timer,&QTimer::timeout,this,[this](){advance();});
        const char *arrows[]={"↖","↑","↗","←","停止","→","↙","↓","↘"};
        for(int row=0;row<3;++row)for(int col=0;col<3;++col) {
            const int vx=col-1,vy=1-row;
            auto button=new QPushButton(QString::fromUtf8(arrows[row*3+col]),this);
            button->setObjectName(QString("pan_%1_%2").arg(vx).arg(vy));button->setAutoDefault(false);
            button->setMinimumHeight(25);layout->addWidget(button,row,col);
            if(vx==0 && vy==0)connect(button,&QPushButton::clicked,this,[this](){stop();});
            else connect(button,&QPushButton::clicked,this,[this,vx,vy](){
                timer->stop();dx=vx;dy=vy;
                if(advance() && continuous->isChecked())timer->start();
            });
        }
        step=new QSpinBox(this);step->setObjectName("panStep");step->setRange(1,100);step->setValue(5);
        step->setToolTip(QString::fromUtf8("每次移动的 Shift 数值；斜向每个轴使用同一步长"));
        layout->addWidget(new QLabel("Step",this),3,0);layout->addWidget(step,3,1,1,2);
        continuous=new QCheckBox(QString::fromUtf8("连续移动（到边界停止）"),this);
        continuous->setObjectName("panContinuous");continuous->setChecked(true);layout->addWidget(continuous,4,0,1,3);
        connect(continuous,&QCheckBox::toggled,this,[this](bool){stop();});
    }
};
#endif
