// Exercise the real cut sliders, renderer connections and one-click controls.
#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include <QTimer>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <array>
#include <cassert>

static void settle(int ms=60)
{
    QElapsedTimer t;t.start();while(t.elapsed()<ms)qApp->processEvents();
}
int main(int argc,char **argv)
{
    Q_INIT_RESOURCE(v3d);auto &app=*V3dApplication::getInstance(argc,argv);
    auto &mw=*app.getMainWindow();mw.show();
    auto a=mw.createMdiChild(),b=mw.createMdiChild();
    auto data=new unsigned char[64*48*32];
    for(int z=0;z<32;++z)for(int y=0;y<48;++y)for(int x=0;x<64;++x)
        data[(z*48+y)*64+x]=((x>5&&x<16)||(x>35&&x<53))&&y>8&&y<38&&z>5&&z<26?180+x:0;
    a->newProcessedImage("Cut movement test",data,64,48,32,1,V3D_UINT8);
    b->newProcessedImage("Independent viewer",new unsigned char[32*32*32](),32,32,32,1,V3D_UINT8);
    a->show();b->show();a->doImage3DView();b->doImage3DView();
    QTimer::singleShot(500,&app,[&](){
        auto win=a->mypara_3Dview.window3D,other=b->mypara_3Dview.window3D;assert(win&&other);
        auto gl=win->getGLWidget();auto r=gl->getRenderer();assert(r);
        win->checkBox_surfZLock->setChecked(false);gl->setRenderMode_Maxip(true);
        gl->setFrontCut(20);int front=gl->frontCut();
        const int zoom=gl->zoom(),xrot=gl->xRot(),yrot=gl->yRot(),zrot=gl->zRot();
        const int xshift=gl->xShift(),yshift=gl->yShift(),zshift=gl->zShift();
        for(bool surface:{false,true})
        {
            auto pad=win->findChild<QWidget*>(surface?"surfaceCutMovePad":"volumeCutMovePad");assert(pad);
            win->tabCutPlane->setCurrentIndex(surface?1:0);
            auto step=pad->findChild<QSpinBox*>("cutMoveStep");assert(step);
            QAbstractSlider *low[]={surface?win->xSminSlider:win->xcminSlider,surface?win->ySminSlider:win->ycminSlider,surface?win->zSminSlider:win->zcminSlider};
            QAbstractSlider *high[]={surface?win->xSmaxSlider:win->xcmaxSlider,surface?win->ySmaxSlider:win->ycmaxSlider,surface?win->zSmaxSlider:win->zcmaxSlider};
            QAbstractButton *locks[]={win->xcLock,win->ycLock,win->zcLock};
            const char *axes[]={"X","Y","Z"};
            auto verify=[&](int axis,int lo,int hi){
                assert(low[axis]->value()==lo&&high[axis]->value()==hi);
                int actualLo[]={surface?gl->xClip0():gl->xCut0(),surface?gl->yClip0():gl->yCut0(),surface?gl->zClip0():gl->zCut0()};
                int actualHi[]={surface?gl->xClip1():gl->xCut1(),surface?gl->yClip1():gl->yCut1(),surface?gl->zClip1():gl->zCut1()};
                assert(actualLo[axis]==lo&&actualHi[axis]==hi);
                double rendererLo[]={surface?r->xClip0:double(r->xCut0),surface?r->yClip0:double(r->yCut0),surface?r->zClip0:double(r->zCut0)};
                double rendererHi[]={surface?r->xClip1:double(r->xCut1),surface?r->yClip1:double(r->yCut1),surface?r->zClip1:double(r->zCut1)};
                double factor=surface?CLIP_RANGE:1;
                assert(qAbs(rendererLo[axis]-lo/factor)<1e-6&&qAbs(rendererHi[axis]-hi/factor)<1e-6);
            };
            for(int axis=0;axis<3;++axis)
            {
                auto minus=pad->findChild<QPushButton*>(QString("cutMove%1Minus").arg(axes[axis]));
                auto plus=pad->findChild<QPushButton*>(QString("cutMove%1Plus").arg(axes[axis]));assert(minus&&plus);
                assert(!minus->autoRepeat()&&!plus->autoRepeat());
                for(int locked=0;locked<(surface?1:2);++locked)
                {
                    locks[axis]->setChecked(false);low[axis]->setValue(0);high[axis]->setValue(high[axis]->maximum());
                    int lo=surface?40:5,hi=surface?60:7,width=hi-lo;
                    high[axis]->setValue(hi);low[axis]->setValue(lo);
                    if(!surface)locks[axis]->setChecked(locked);
                    step->setValue(surface?30:10);const int distance=surface?60:10;
                    plus->click();verify(axis,lo+distance,hi+distance);
                    minus->click();verify(axis,lo,hi);
                    for(int n=0;n<30;++n)plus->click();verify(axis,high[axis]->maximum()-width,high[axis]->maximum());
                    for(int n=0;n<30;++n)minus->click();verify(axis,0,width);
                    step->setValue(1);plus->click();verify(axis,surface?2:1,width+(surface?2:1));
                    locks[axis]->setChecked(false);low[axis]->setValue(0);high[axis]->setValue(high[axis]->maximum());
                    plus->click();minus->click();verify(axis,0,high[axis]->maximum()); // Full extent cannot translate.
                    // Zero-width slice, with a step larger than the entire axis.
                    high[axis]->setValue(0);step->setValue(step->maximum());plus->click();
                    verify(axis,high[axis]->maximum(),high[axis]->maximum());minus->click();verify(axis,0,0);
                    high[axis]->setValue(high[axis]->maximum());
                }
            }
            if(surface) {assert(gl->xCut0()==0&&gl->xCut1()==63&&gl->yCut0()==0&&gl->yCut1()==47&&gl->zCut0()==0&&gl->zCut1()==31);}
            else {assert(gl->xClip0()==0&&gl->xClip1()==CLIP_RANGE&&gl->yClip0()==0&&gl->zClip0()==0);}
        }
        // A held mouse press must not start continuous movement.
        win->tabCutPlane->setCurrentIndex(0);win->xcmaxSlider->setValue(20);
        auto pad=win->findChild<QWidget*>("volumeCutMovePad");pad->findChild<QSpinBox*>("cutMoveStep")->setValue(5);
        auto right=pad->findChild<QPushButton*>("cutMoveXPlus");
        QMouseEvent press(QEvent::MouseButtonPress,right->rect().center(),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease,right->rect().center(),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(right,&press);settle(500);assert(gl->xCut0()==0);
        QApplication::sendEvent(right,&release);assert(gl->xCut0()==5&&gl->xCut1()==25);
        settle();QImage before=gl->grabFramebuffer();right->click();settle();QImage after=gl->grabFramebuffer();assert(before!=after);
        assert(gl->zoom()==zoom&&gl->xRot()==xrot&&gl->yRot()==yrot&&gl->zRot()==zrot);
        assert(gl->xShift()==xshift&&gl->yShift()==yshift&&gl->zShift()==zshift&&gl->frontCut()==front);
        assert(other->getGLWidget()->xCut0()==0&&other->getGLWidget()->xCut1()==31&&other->getGLWidget()->xClip0()==0);
        // Existing surface/volume Z-Lock reads the final pair, not one old edge.
        win->checkBox_surfZLock->setChecked(true);right->click();
        auto surfaceRenderer=dynamic_cast<Renderer_gl1*>(r);assert(surfaceRenderer);
        assert(surfaceRenderer->xMin==gl->xCut0()-2&&surfaceRenderer->xMax==gl->xCut1()+2);
        assert(surfaceRenderer->yMin==gl->yCut0()-2&&surfaceRenderer->yMax==gl->yCut1()+2);
        assert(surfaceRenderer->zMin==gl->zCut0()-2&&surfaceRenderer->zMax==gl->zCut1()+2);
        win->checkBox_surfZLock->setChecked(false);
        // Compact controls must remain reachable in an ordinary viewer window.
        win->resize(1120,980);settle();win->grab().save("/tmp/vaa3d-volume-cut-move.png");
        win->tabCutPlane->setCurrentIndex(1);settle();win->grab().save("/tmp/vaa3d-surface-cut-move.png");
        qInfo("CUT_MOVE_GUI_PASS: six directions, preserved size, locks, boundaries, step units, held click, renderer, pixels, Z-Lock and viewer isolation");
        win->close();other->close();app.quit();
    });
    QTimer::singleShot(30000,&app,[](){qFatal("Cut movement GUI timeout");});return app.exec();
}
