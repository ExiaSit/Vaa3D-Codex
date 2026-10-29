#include "GLee2glew.h"
#include "renderer_gl1.h"
#include "v3dr_glwidget.h"
#include <QTimer>

void Renderer_gl1::emitLocalHideQuad(const LocalHidePolygon &quad,bool texture3D)
{
    for(const auto &polygon:subtractLocalHideBoxes(quad,localHiddenBoxes,volTimePoint)) {
        glBegin(GL_TRIANGLE_FAN);
        for(const auto &v:polygon) {
            if(texture3D)glTexCoord3dv(v.texture.data());else glTexCoord2dv(v.texture.data());
            glVertex3dv(v.position.data());
        }
        glEnd();
    }
}

bool Renderer_gl1::locateLocalHideBox(int mouseX,int mouseY,int radius)
{
    localHidePreviewValid=false;auto w=static_cast<QWidget*>(widget);
    if(!w||!rgbaBuf||bufSize[0]<1||bufSize[1]<1||bufSize[2]<1||viewport[2]<=0||viewport[3]<=0)return false;
    GLint mode;GLdouble matrix[16];glGetIntegerv(GL_MATRIX_MODE,&mode);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadMatrixd(volumeViewMatrix);setUnitVolumeSpace();
    glGetDoublev(GL_MODELVIEW_MATRIX,matrix);glPopMatrix();glMatrixMode(mode);
    double x=viewport[0]+mouseX*double(viewport[2])/qMax(1,w->width());
    double y=viewport[1]+(w->height()-mouseY)*double(viewport[3])/qMax(1,w->height());
    std::array<double,3> near,far,direction;
    if(!gluUnProject(x,y,0,matrix,projectionMatrix,viewport,&near[0],&near[1],&near[2])
        ||!gluUnProject(x,y,1,matrix,projectionMatrix,viewport,&far[0],&far[1],&far[2]))return false;
    const double lows[]={VOL_X0,VOL_Y0,VOL_Z0},highs[]={VOL_X1,VOL_Y1,VOL_Z1};
    double first=0,last=1;
    for(int i=0;i<3;++i) {
        direction[i]=far[i]-near[i];if(!std::isfinite(direction[i])||!std::isfinite(near[i]))return false;
        double lo=renderMode==rmCrossSection?0:qMax(0.0,lows[i]);
        double hi=renderMode==rmCrossSection?1:qMin(1.0,highs[i]);if(lo>hi)return false;
        if(std::abs(direction[i])<1e-12){if(near[i]<lo||near[i]>hi)return false;}
        else {double a=(lo-near[i])/direction[i],b=(hi-near[i])/direction[i];if(a>b)std::swap(a,b);
            first=qMax(first,a);last=qMin(last,b);}
    }
    if(first>last)return false;
    double extent=0;for(int i=0;i<3;++i)extent=qMax(extent,std::abs(direction[i]*(last-first))*bufSize[i]);
    int steps=qBound(1,int(std::ceil(qMin(4096.0,extent*2))),4096);
    std::vector<double> samples;
    if(renderMode==rmCrossSection) {
        const bool enabled[]={bXSlice,bYSlice,bZSlice};
        for(int axis=0;axis<3;++axis)if(enabled[axis]&&std::abs(direction[axis])>1e-12)
            samples.push_back((lows[axis]-near[axis])/direction[axis]);
        double eye0=matrix[2]*near[0]+matrix[6]*near[1]+matrix[10]*near[2]+matrix[14];
        double eyeD=matrix[2]*direction[0]+matrix[6]*direction[1]+matrix[10]*direction[2];
        if(bFSlice&&std::abs(eyeD)>1e-12)samples.push_back((viewClip-eye0)/eyeD);
    } else for(int step=0;step<=steps;++step)samples.push_back(first+(last-first)*step/steps);
    double best=0;std::array<double,3> center{{0,0,0}};
    for(double t:samples) {
        if(t<first||t>last)continue;
        std::array<double,3> p;
        for(int i=0;i<3;++i)p[i]=near[i]+t*direction[i];
        bool hidden=false;for(const auto &box:localHiddenBoxes)if(box.frame==volTimePoint&&box.contains(p)){hidden=true;break;}
        if(hidden)continue;
        double eyeZ=matrix[2]*p[0]+matrix[6]*p[1]+matrix[10]*p[2]+matrix[14];
        if(renderMode!=rmCrossSection&&eyeZ>viewClip+1e-6)continue;
        V3DLONG index[3];for(int i=0;i<3;++i)index[i]=qBound<V3DLONG>(0,V3DLONG(std::floor(p[i]*(bufSize[i]-1)+.5)),bufSize[i]-1);
        const RGBA8 &pixel=rgbaBuf[(index[2]*bufSize[1]+index[1])*bufSize[0]+index[0]];
        double score=qMax(pixel.r*double(color_proxy.r),qMax(pixel.g*double(color_proxy.g),pixel.b*double(color_proxy.b)));
        if(score>best){best=score;for(int i=0;i<3;++i)center[i]=bufSize[i]>1?index[i]/double(bufSize[i]-1):0;}
    }
    if(best<=0)return false;
    const V3DLONG sizes[]={size1,size2,size3};
    for(int i=0;i<3;++i){double half=radius/double(qMax<V3DLONG>(1,sizes[i]-1));
        localHidePreview.low[i]=qMax(0.0,center[i]-half);localHidePreview.high[i]=qMin(1.0,center[i]+half);}
    localHidePreview.frame=volTimePoint;localHidePreviewValid=true;return true;
}

void Renderer_gl1::drawLocalHidePreview()
{
    if(!localHidePreviewValid||localHidePreview.frame!=volTimePoint||b_selecting)return;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_1D);glDisable(GL_TEXTURE_2D);glDisable(GL_TEXTURE_3D);
    glDisable(GL_BLEND);glDisable(GL_ALPHA_TEST);glDisable(GL_LINE_STIPPLE);
    for(int i=0;i<6;++i)glDisable(GL_CLIP_PLANE0+i);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadMatrixd(volumeViewMatrix);setUnitVolumeSpace();
    glLineWidth(2);glColor3f(1,1,0);glBegin(GL_LINES);
    for(int corner=0;corner<8;++corner)for(int axis=0;axis<3;++axis)if(!(corner&(1<<axis)))
        for(int endpoint=0;endpoint<2;++endpoint){int c=corner|(endpoint<<axis);
            glVertex3d(c&1?localHidePreview.high[0]:localHidePreview.low[0],
                c&2?localHidePreview.high[1]:localHidePreview.low[1],c&4?localHidePreview.high[2]:localHidePreview.low[2]);}
    glEnd();glPopMatrix();glPopAttrib();
}

void V3dR_GLWidget::setLocalHideMode(bool enabled)
{
    if(localHideMode==enabled)return;
    if(!localHideHoverTimer) {
        localHideHoverTimer=new QTimer(this);localHideHoverTimer->setSingleShot(true);
        connect(localHideHoverTimer,&QTimer::timeout,this,&V3dR_GLWidget::updateLocalHideHover);
        connect(this,&V3dR_GLWidget::rendererAboutToBeDeleted,this,[this](){setLocalHideMode(false);emit localHideCountChanged(0);});
    }
    if(enabled){localHidePreviousTracking=hasMouseTracking();setMouseTracking(true);}
    else {setMouseTracking(localHidePreviousTracking);localHideHoverTimer->stop();}
    localHideMode=enabled;localHideCursorValid=false;
    if(auto r=dynamic_cast<Renderer_gl1*>(renderer))r->localHidePreviewValid=false;
    emit localHideModeChanged(enabled);update();
}
void V3dR_GLWidget::setLocalHideRadius(int radius)
{
    localHideRadius=qBound(1,radius,4096);updateLocalHideHover();
}
void V3dR_GLWidget::updateLocalHideHover()
{
    auto r=dynamic_cast<Renderer_gl1*>(renderer);if(!r)return;
    r->localHidePreviewValid=false;
    if(localHideMode&&localHideCursorValid&&rect().contains(localHideCursor)) {
        makeCurrent();r->locateLocalHideBox(localHideCursor.x(),localHideCursor.y(),localHideRadius);
    }
    update();
}
void V3dR_GLWidget::undoLocalHide()
{
    if(auto r=dynamic_cast<Renderer_gl1*>(renderer)) {
        if(!r->localHiddenBoxes.empty())r->localHiddenBoxes.pop_back();
        r->localHidePreviewValid=false;emit localHideCountChanged(int(r->localHiddenBoxes.size()));update();
    }
}
void V3dR_GLWidget::restoreLocalHidden()
{
    if(auto r=dynamic_cast<Renderer_gl1*>(renderer)) {
        r->localHiddenBoxes.clear();r->localHidePreviewValid=false;emit localHideCountChanged(0);update();
    }
}
