
#include "renderer_gl1.h"
#include "v3dr_glwidget.h"
#include "../control/SphereReconnect.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QUndoCommand>

#include "PAnoToolBar.h"
#include "PMain.h"
#include "../control/CViewer.h"

using namespace terafly;

namespace { bool sphereHistoryReady(const QUndoCommand *command); }

PAnoToolBar* PAnoToolBar::uniqueInstance = 0;

PAnoToolBar::~PAnoToolBar()
{
    //QMessageBox::information(0, "info", "Destroying PAnoToolbar!");
}

PAnoToolBar::PAnoToolBar(QWidget *parent) : QWidget(parent)
{
    /**/tf::debug(tf::LEV1, 0, __itm__current__function__);

    // create toolbar
    toolBar = new QToolBar("ToolBar", this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setIconSize(QSize(35,35));

    // add already existing buttons from the main GUI
    toolBar->addAction(PMain::getInstance()->loadAnnotationsAction);
    toolBar->addAction(PMain::getInstance()->saveAnnotationsAction);
    //toolBar->addAction(PMain::getInstance()->saveandchangetype);
    //toolBar->addAction(PMain::getInstance()->returntochangedtype);
    toolBar->addAction(PMain::getInstance()->saveAnnotationsAsAction);
    toolBar->addAction(PMain::getInstance()->clearAnnotationsAction);
    toolBar->addSeparator();


    /**/
    buttonUndo = new QToolButton();
    buttonUndo->setIcon(QIcon(":/icons/undo.png"));
    buttonUndo->setToolTip("Undo (Ctrl+Z)");
    buttonUndo->setEnabled(false);
    buttonUndo->setShortcut(QKeySequence("Ctrl+Z"));
    connect(buttonUndo, SIGNAL(clicked()), this, SLOT(buttonUndoClicked()));
    toolBar->insertWidget(0, buttonUndo);
    buttonRedo = new QToolButton();
    buttonRedo->setIcon(QIcon(":/icons/redo.png"));
    buttonRedo->setToolTip("Redo (Ctrl+Shift+Z)");
    buttonRedo->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    buttonRedo->setEnabled(false);
    connect(buttonRedo, SIGNAL(clicked()), this, SLOT(buttonRedoClicked()));
    toolBar->insertWidget(0, buttonRedo);
    toolBar->addSeparator();

    QToolButton *sphereButton = new QToolButton(toolBar);
    sphereButton->setObjectName("sphereReconnectButton");
    sphereButton->setText(QString::fromUtf8("球内\n重连"));
    sphereButton->setToolTip(QString::fromUtf8("画一个三维球，将球内线段重连到中心点"));
    connect(sphereButton, SIGNAL(clicked()), this, SLOT(startSphereReconnect()));
    toolBar->addWidget(sphereButton);

    // add new buttons
    buttonMarkerCreate = new QToolButton();
    buttonMarkerCreate->setIcon(QIcon(":/icons/marker_add.png"));
    buttonMarkerCreate->setCheckable(true);
    buttonMarkerCreate->setToolTip("1-right-click to define a marker");
    connect(buttonMarkerCreate, SIGNAL(toggled(bool)), this, SLOT(buttonMarkerCreateChecked(bool)));
    toolBar->insertWidget(0, buttonMarkerCreate);
    /**/
    buttonMarkerCreate2 = new QToolButton();
    buttonMarkerCreate2->setIcon(QIcon(":/icons/marker_add_2.png"));
    buttonMarkerCreate2->setCheckable(true);
    buttonMarkerCreate2->setToolTip("2-right-clicks to define a marker");
    connect(buttonMarkerCreate2, SIGNAL(toggled(bool)), this, SLOT(buttonMarkerCreate2Checked(bool)));
    toolBar->insertWidget(0, buttonMarkerCreate2);
    /**/
    buttonMarkerDelete = new QToolButton();
    buttonMarkerDelete->setIcon(QIcon(":/icons/marker_delete.png"));
    buttonMarkerDelete->setCheckable(true);
    buttonMarkerDelete->setToolTip("1-right-click to delete a marker");
    connect(buttonMarkerDelete, SIGNAL(toggled(bool)), this, SLOT(buttonMarkerDeleteChecked(bool)));
    toolBar->insertWidget(0, buttonMarkerDelete);
    /**/
    buttonMarkerRoiDelete = new QToolButton();
    buttonMarkerRoiDelete->setIcon(QIcon(":/icons/marker_delete_roi.png"));
    buttonMarkerRoiDelete->setCheckable(true);
    buttonMarkerRoiDelete->setToolTip("1-right-stroke to delete a group of markers");
    buttonMarkerRoiDelete->setShortcut(QKeySequence("Alt+M")); //by XZ, 20190724
    connect(buttonMarkerRoiDelete, SIGNAL(toggled(bool)), this, SLOT(buttonMarkerRoiDeleteChecked(bool)));
    toolBar->insertWidget(0, buttonMarkerRoiDelete);
    /**/
    buttonMarkerRoiView = new QToolButton();
    buttonMarkerRoiView->setIcon(QIcon(":/icons/marker_roi_view.png"));
    buttonMarkerRoiView->setCheckable(true);
    buttonMarkerRoiView->setToolTip("Show/hide markers around the displayed ROI");
    connect(buttonMarkerRoiView, SIGNAL(toggled(bool)), this, SLOT(buttonMarkerRoiViewChecked(bool)));
    toolBar->insertWidget(0, buttonMarkerRoiView);

	// Fragmented auto-tracing, MK, Dec 2018
	/*toolBar->addSeparator();
	buttonFragmentTrace = new QToolButton();
	buttonFragmentTrace->setIcon(QIcon(":/icons/extend.png"));
	buttonFragmentTrace->setCheckable(true);
	buttonFragmentTrace->setToolTip("Fragmented auto-trace");
	connect(buttonFragmentTrace, SIGNAL(toggled(bool)), this, SLOT(buttonFragmenTraceChecked(bool)));
	toolBar->insertWidget(0, buttonFragmentTrace);*/

    /**/
    buttonOptions = new QToolButton();
    buttonOptions->setIcon(QIcon(":/icons/options.png"));
    buttonOptions->setMenu(PMain::getInstance()->annotationMenu);
    buttonOptions->setPopupMode(QToolButton::InstantPopup);
    buttonOptions->setToolTip("Options");
    toolBar->addSeparator();
    toolBar->insertWidget(0, buttonOptions);

    // layout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(toolBar);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);


    // make it appear as a true toolbar
    this->setContentsMargins(0,0,0,0);
    //this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    //setAttribute(Qt::WA_TranslucentBackground);
    toolBar->setStyleSheet("QToolBar{background:qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,"
                           "stop: 0 rgb(180,180,180), stop: 1 rgb(220,220,220));"
                           "border-style: solid;"
                           //"border-style: outset;"
                           "border-width: 1px;"
                           "border-radius: 3px;"
                           "border-color: rgb(140,140,140);} ");
                           //"QToolBar::separator::horizontal{width: 5px; height: 10px;}");

    // install event filter
    QCoreApplication::instance()->installEventFilter(this);

    // show tooltips also when has no focus on it
    //setAttribute(Qt::WA_AlwaysShowToolTips, true);

    // set autosave interval
    autosaveTimer.start(autosaveInterval);
    connect(&autosaveTimer, SIGNAL(timeout()), this, SLOT(autosaveAnnotations()));
}


void PAnoToolBar::buttonMarkerCreateChecked(bool checked)
{
     /**/tf::debug(tf::LEV3, strprintf("checked = %s", checked ? "true" : "false").c_str(), __itm__current__function__);

    CViewer* expl = CViewer::getCurrent();

    if(checked)
    {
        // uncheck other buttons but the current one      
        if(buttonMarkerCreate2->isChecked())
            buttonMarkerCreate2->setChecked(false);
        if(buttonMarkerDelete->isChecked())
            buttonMarkerDelete->setChecked(false);
        if(buttonMarkerRoiDelete->isChecked())
            buttonMarkerRoiDelete->setChecked(false);


        // change cursor
        QPixmap cur_img(":/icons/cursor_marker_add.png");
        cur_img = cur_img.scaled(32,32,Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setCursor(QCursor(cur_img, 0, 0));
        CViewer::setCursor(QCursor(cur_img, 0, 0), true);

        // switch to Vaa3D's 1-right-click marker create mode
        if(expl)
        {
            expl->view3DWidget->getRenderer()->selectMode = Renderer::smMarkerCreate1;
            static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_addthismarker = true;
        }
    }
    else
    {
        // set default cursor
        setCursor(Qt::ArrowCursor);
        CViewer::setCursor(Qt::ArrowCursor, true);

        // end marker create mode
        if(expl)
            expl->view3DWidget->getRenderer()->endSelectMode();
    }
}

void PAnoToolBar::buttonMarkerCreate2Checked(bool checked)
{
     /**/tf::debug(tf::LEV3, strprintf("checked = %s", checked ? "true" : "false").c_str(), __itm__current__function__);

    CViewer* expl = CViewer::getCurrent();

    if(checked)
    {
        // uncheck other buttons but the current one
        if(buttonMarkerCreate->isChecked())
            buttonMarkerCreate->setChecked(false);
        if(buttonMarkerDelete->isChecked())
            buttonMarkerDelete->setChecked(false);
        if(buttonMarkerRoiDelete->isChecked())
            buttonMarkerRoiDelete->setChecked(false);


        // change cursor
        QPixmap cur_img(":/icons/cursor_marker_add_2.png");
        cur_img = cur_img.scaled(32,32,Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setCursor(QCursor(cur_img, 0, 0));
        CViewer::setCursor(QCursor(cur_img, 0, 0), true);

        // switch to Vaa3D's 2-right-clicks marker create mode
        if(expl)
        {
            expl->view3DWidget->getRenderer()->selectMode = Renderer::smMarkerCreate2;
            static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_addthismarker = true;
        }
    }
    else
    {
        // set default cursor
        setCursor(Qt::ArrowCursor);
        CViewer::setCursor(Qt::ArrowCursor, true);

        // end marker create mode
        if(expl)
            expl->view3DWidget->getRenderer()->endSelectMode();
    }
}


void PAnoToolBar::buttonMarkerDeleteChecked(bool checked)
{
     /**/tf::debug(tf::LEV3, strprintf("checked = %s", checked ? "true" : "false").c_str(), __itm__current__function__);

    CViewer* expl = CViewer::getCurrent();

    if(checked)
    {
        // uncheck other buttons but the current one
        if(buttonMarkerCreate2->isChecked())
            buttonMarkerCreate2->setChecked(false);
        if(buttonMarkerCreate->isChecked())
            buttonMarkerCreate->setChecked(false);
        if(buttonMarkerRoiDelete->isChecked())
            buttonMarkerRoiDelete->setChecked(false);

        if(expl)
        {
            // change cursor
            QPixmap cur_img(":/icons/cursor_marker_delete.png");
            cur_img = cur_img.scaled(32,32,Qt::KeepAspectRatio, Qt::SmoothTransformation);
            setCursor(QCursor(cur_img, 0,0));
            CViewer::setCursor(QCursor(cur_img, 0, 0), true);

//            if(expl)
//            {
//                expl->view3DWidget->getRenderer()->selectMode = Renderer::smMarkerCreate1;
//                static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_addthismarker = false;
//                static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_imaging = false;
//                static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_grabhighrez = true;
//            }
        }
    }
    else
    {
        // set default cursor
        setCursor(Qt::ArrowCursor);
        CViewer::setCursor(Qt::ArrowCursor, true);

        // end marker delete mode
//        if(expl)
//        {
//            expl->view3DWidget->getRenderer()->endSelectMode();
//            expl->view3DWidget->getRenderer()->selectMode = Renderer::smObject;
//            static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_grabhighrez = false;
//        }
    }
}


void PAnoToolBar::buttonMarkerRoiDeleteChecked(bool checked)
{
     /**/tf::debug(tf::LEV3, strprintf("checked = %s", checked ? "true" : "false").c_str(), __itm__current__function__);

    CViewer* expl = CViewer::getCurrent();

    if(checked)
    {
//        QMessageBox::information(this, "Warning", "Not yet implemented. But stay tuned!");
//        buttonMarkerRoiDelete->setChecked(false);
        // uncheck other buttons but the current one
        if(buttonMarkerCreate2->isChecked())
            buttonMarkerCreate2->setChecked(false);
        if(buttonMarkerCreate->isChecked())
            buttonMarkerCreate->setChecked(false);
        if(buttonMarkerDelete->isChecked())
            buttonMarkerDelete->setChecked(false);

        // change cursor
        QPixmap cur_img(":/icons/cursor_marker_delete_roi.png");
        cur_img = cur_img.scaled(32,32,Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setCursor(QCursor(cur_img, 0,0));
        CViewer::setCursor(QCursor(cur_img, 0, 0), true);

        // switch to Vaa3D's mode "Zoom-in HighRezImage: 1-right stroke ROI"
        if(expl)
        {
            //expl->view3DWidget->getRenderer()->selectMode = Renderer::smCurveCreate1;
            expl->view3DWidget->getRenderer()->selectMode = Renderer::smSelectMultiMarkers;
            static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_addthiscurve = false;
            //static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_imaging = false;
            //static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_grabhighrez = true;
        }
    }
    else
    {
        // set default cursor
        setCursor(Qt::ArrowCursor);
        CViewer::setCursor(Qt::ArrowCursor, true);

        // end Vaa3D's mode "Zoom-in HighRezImage: 1-right stroke ROI"
        if(expl)
        {
            expl->view3DWidget->getRenderer()->selectMode = Renderer::smObject;
            static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->b_grabhighrez = false;
        }
    }
}


void PAnoToolBar::buttonMarkerRoiViewChecked(bool checked)
{
    /**/tf::debug(tf::LEV3, strprintf("checked = %s", checked ? "true" : "false").c_str(), __itm__current__function__);

    CViewer* expl = CViewer::getCurrent();
    if(expl)
    {
        // get TRUE markers' handle (others won't work, e.g. LandmarkList)
        QList <ImageMarker>& markers = static_cast<Renderer_gl1*>(expl->view3DWidget->getRenderer())->listMarker;

        // get virtual margin and apply it to each directions separately
        int vmPerc = CSettings::instance()->getAnnotationVirtualMargin();
        int vmX = (expl->volH1-expl->volH0)*(vmPerc/100.0f)/2;
        int vmY = (expl->volV1-expl->volV0)*(vmPerc/100.0f)/2;
        int vmZ = (expl->volD1-expl->volD0)*(vmPerc/100.0f)/2;

        // apply changes to existing markers
        for(int i=0; i<markers.size(); i++)
        {
            // select markers within margin
            if(  (markers[i].x < 0 && markers[i].x >= -vmX) ||
                 (markers[i].y < 0 && markers[i].y >= -vmY) ||
                 (markers[i].z < 0 && markers[i].z >= -vmZ) ||
                 (markers[i].x  >= (expl->volH1-expl->volH0) && markers[i].x <=  (expl->volH1-expl->volH0+vmX)) ||
                 (markers[i].y  >= (expl->volV1-expl->volV0) && markers[i].y <=  (expl->volV1-expl->volV0+vmY)) ||
                 (markers[i].z  >= (expl->volD1-expl->volD0) && markers[i].x <=  (expl->volD1-expl->volD0+vmZ)))
            {
                // activate / deactivate markers
                markers[i].on = checked;

                // markers within margin are assigned gray color so as to be better distinguishable
                markers[i].color.r = markers[i].color.g = markers[i].color.b = 220;
            }

            // always turn off markers outside the visible area
            if(  (markers[i].x < -vmX) || (markers[i].y < -vmY)   || (markers[i].z < -vmZ) ||
                 (markers[i].x  >= (expl->volH1-expl->volH0+vmX)) || (markers[i].y  >= (expl->volV1-expl->volV0+vmY)) || (markers[i].z  >= (expl->volD1-expl->volD0+vmZ)))
                markers[i].on = false;
        }

        expl->view3DWidget->updateTool();
        expl->view3DWidget->update();
    }
}

void PAnoToolBar::buttonFragmenTraceChecked(bool checked)
{
	// Not implemented yet at the moment due to limited space on the toolbar. May or may not procceed in tne future. MK, Dec 2018.
}


void PAnoToolBar::buttonUndoClicked()
{
    /**/tf::debug(tf::LEV3, 0, __itm__current__function__);

    CViewer* expl = CViewer::getCurrent();
    if(expl && expl->undoStack.canUndo())
    {
        if (!sphereHistoryReady(expl->undoStack.command(expl->undoStack.index()-1))) return;
        expl->undoStack.undo();
        if(!expl->undoStack.canUndo())
            buttonUndo->setEnabled(false);
        buttonRedo->setEnabled(true);
    }
}

void PAnoToolBar::buttonRedoClicked()
{
    /**/tf::debug(tf::LEV3, 0, __itm__current__function__);

    CViewer* expl = CViewer::getCurrent();
    if(expl && expl->undoStack.canRedo())
    {
        if (!sphereHistoryReady(expl->undoStack.command(expl->undoStack.index()))) return;
        expl->undoStack.redo();
        if(!expl->undoStack.canRedo())
            buttonRedo->setEnabled(false);
        buttonUndo->setEnabled(true);
    }
}

/**********************************************************************************
* Intercepts global key pressed events
***********************************************************************************/
bool PAnoToolBar::eventFilter(QObject *object, QEvent *event)
{
    if (sphereEvent(object, event)) return true;
    if(event->type() == 6)
    {
        QKeyEvent *KeyEvent = (QKeyEvent*)event;
        if(KeyEvent->key() == Qt::Key_Escape)
            releaseTools();
        else if(KeyEvent->key() == Qt::Key_Backspace && buttonMarkerRoiDelete->isChecked())
        {
            CViewer::getCurrent()->deleteSelectedMarkers();
            releaseTools();
        }
    }
    return false;
}

/**********************************************************************************
* Release currently activated tools, if any
***********************************************************************************/
void PAnoToolBar::releaseTools()
{
    cancelSphere();
    /**/tf::debug(tf::LEV3, 0, __itm__current__function__);

    if(buttonMarkerCreate->isChecked())
        buttonMarkerCreate->setChecked(false);
    if(buttonMarkerCreate2->isChecked())
        buttonMarkerCreate2->setChecked(false);
    if(buttonMarkerDelete->isChecked())
        buttonMarkerDelete->setChecked(false);
    if(buttonMarkerRoiDelete->isChecked())
        buttonMarkerRoiDelete->setChecked(false);
}

/**********************************************************************************
* Refresh currently activated tools, if any
***********************************************************************************/
void PAnoToolBar::refreshTools()
{
    /**/tf::debug(tf::LEV_MAX, 0, __itm__current__function__);

    if(buttonMarkerCreate->isChecked())
        buttonMarkerCreateChecked(true);
    if(buttonMarkerCreate2->isChecked())
        buttonMarkerCreate2Checked(true);
    if(buttonMarkerDelete->isChecked())
        buttonMarkerDeleteChecked(true);
    if(buttonMarkerRoiDelete->isChecked())
        buttonMarkerRoiDeleteChecked(true);
    update();
}

void PAnoToolBar::saveAnnotations()
{
    if(!PMain::getInstance()->getAnnotationPathRLU().empty())
        PMain::getInstance()->saveAnnotations();
}

void PAnoToolBar::autosaveAnnotations()
{
    PMain::getInstance()->autosaveAnnotations();
}



namespace {
QString sphereTreeSignature(const NeuronTree &tree)
{
    QHash<V3DLONG, NeuronSWC> nodes;
    for(const NeuronSWC &n:tree.listNeuron) nodes.insert(n.n,n);
    auto point=[](const NeuronSWC &n) {return QString::number(n.x,'g',9)+","+QString::number(n.y,'g',9)+","+QString::number(n.z,'g',9);};
    QStringList rows;
    for(const NeuronSWC &n:tree.listNeuron)
        rows << point(n)+":"+QString::number(n.type)+":"+QString::number(n.r,'g',9)+":"+
                (nodes.contains(n.pn)?point(nodes[n.pn]):QString("root"));
    rows.sort();return rows.join(";");
}
class SphereReconnectCommand : public QUndoCommand
{
    NeuronTree before, after;
    QString expected;
    bool first=true;
    void restore(NeuronTree tree) {
        // The host also has a separate tracing history. Do not overwrite later edits.
        if(!first && sphereTreeSignature(terafly::PluginInterface::getSWC())!=expected) {
            QMessageBox::warning(0,QString::fromUtf8("无法撤销/重做"),QString::fromUtf8("标注已有其他修改，请先撤销这些修改，再撤销球内重连。"));return;
        }
        if(!terafly::PluginInterface::setSWC(tree)) return;
        first=false;
        expected=sphereTreeSignature(terafly::PluginInterface::getSWC());
        terafly::PMain::getInstance()->annotationsChanged();
    }
public:
    bool canRestore() const {return first || sphereTreeSignature(terafly::PluginInterface::getSWC())==expected;}
    SphereReconnectCommand(const NeuronTree &a,const NeuronTree &b):before(a),after(b) {setText(QString::fromUtf8("球内重连到中心"));}
    void redo() override {restore(after);}
    void undo() override {restore(before);}
};
bool sphereHistoryReady(const QUndoCommand *command)
{
    const auto sphere=dynamic_cast<const SphereReconnectCommand*>(command);
    if(!sphere || sphere->canRestore()) return true;
    QMessageBox::warning(0,QString::fromUtf8("无法撤销/重做"),QString::fromUtf8("标注已有其他修改，请先撤销这些修改，再撤销球内重连。"));
    return false;
}
}

void PAnoToolBar::startSphereReconnect()
{
    CViewer *v=CViewer::getCurrent();
    if(!v || !v->isReady()) return;
    releaseTools();
    auto r=static_cast<Renderer_gl1*>(v->getGLWidget()->getRenderer());
    r->endSelectMode();
    sphereView=v->getGLWidget();
    sphereDialog=new QDialog(this,Qt::Tool);
    sphereDialog->setObjectName("sphereReconnectDialog");
    sphereDialog->setWindowTitle(QString::fromUtf8("球内重连到中心"));
    auto layout=new QVBoxLayout(sphereDialog);
    auto help=new QLabel(QString::fromUtf8("在图像上按住右键选球心，拖动确定半径。\n左键可旋转检查；可调整下方坐标和半径。\n单位：最高分辨率体素坐标（从 0 开始）。"));
    layout->addWidget(help);
    auto form=new QFormLayout;layout->addLayout(form);
    const char *names[]={"sphereX","sphereY","sphereZ","sphereRadius"};
    QStringList labels;labels<<"X"<<"Y"<<"Z"<<QString::fromUtf8("半径");
    double values[]={v->coord2global<double>(r->imageX/2.,iim::horizontal,false),
                     v->coord2global<double>(r->imageY/2.,iim::vertical,false),
                     v->coord2global<double>(r->imageZ/2.,iim::depth,false),20.};
    for(int i=0;i<4;++i) {
        sphereValues[i]=new QDoubleSpinBox(sphereDialog);
        sphereValues[i]->setObjectName(names[i]);sphereValues[i]->setDecimals(2);
        sphereValues[i]->setRange(i==3?0.01:0,1e9);sphereValues[i]->setValue(values[i]);
        form->addRow(labels[i],sphereValues[i]);
        connect(sphereValues[i],static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),this,[this](double){updateSpherePreview();});
    }
    auto buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,sphereDialog);
    buttons->button(QDialogButtonBox::Apply)->setText(QString::fromUtf8("应用重连"));
    buttons->button(QDialogButtonBox::Apply)->setObjectName("applySphereReconnect");
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    layout->addWidget(buttons);
    connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this](){applySphere();});
    connect(buttons,&QDialogButtonBox::rejected,sphereDialog,&QDialog::reject);
    connect(sphereDialog.data(),&QDialog::finished,this,[this](int){cancelSphere();});
    updateSpherePreview();sphereDialog->show();
}

void PAnoToolBar::cancelSphere()
{
    sphereDragging=false;
    if(sphereView) {
        auto w=static_cast<V3dR_GLWidget*>(sphereView.data());
        if(w->getRenderer()) static_cast<Renderer_gl1*>(w->getRenderer())->reconnectSphereVisible=false;
        w->update();
    }
    sphereView=0;
    if(sphereDialog) {
        QDialog *dialog=sphereDialog.data();sphereDialog=0;
        dialog->hide();dialog->deleteLater();
    }
}

void PAnoToolBar::updateSpherePreview()
{
    CViewer *v=CViewer::getCurrent();
    if(!sphereDialog || !v || sphereView!=v->getGLWidget()) return;
    auto r=static_cast<Renderer_gl1*>(v->getGLWidget()->getRenderer());
    double x=sphereValues[0]->value(),y=sphereValues[1]->value(),z=sphereValues[2]->value(),radius=sphereValues[3]->value();
    XYZ c(v->coord2local<double>(x,iim::horizontal,false),v->coord2local<double>(y,iim::vertical,false),v->coord2local<double>(z,iim::depth,false));
    r->reconnectSphereCenter=c;
    r->reconnectSphereRadii=XYZ(v->coord2local<double>(x+radius,iim::horizontal,false)-c.x,
                              v->coord2local<double>(y+radius,iim::vertical,false)-c.y,
                              v->coord2local<double>(z+radius,iim::depth,false)-c.z);
    r->reconnectSphereVisible=true;v->getGLWidget()->update();
}

bool PAnoToolBar::sphereEvent(QObject *object,QEvent *event)
{
    if(!sphereDialog) return false;
    CViewer *v=CViewer::getCurrent();
    if(!v || sphereView!=v->getGLWidget()) {cancelSphere();return false;}
    if(event->type()==QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key()==Qt::Key_Escape) {
        cancelSphere();return true;
    }
    if(object!=sphereView.data()) return false;
    if(event->type()==QEvent::Wheel) return true; // keep the selected resolution fixed during preview
    if(event->type()!=QEvent::MouseButtonPress && event->type()!=QEvent::MouseMove && event->type()!=QEvent::MouseButtonRelease) return false;
    QMouseEvent *e=static_cast<QMouseEvent*>(event);
    auto w=v->getGLWidget();auto r=static_cast<Renderer_gl1*>(w->getRenderer());
    if(event->type()==QEvent::MouseButtonPress && e->button()==Qt::RightButton) {
        w->makeCurrent();
        Renderer_gl1::MarkerPos pos;pos.x=e->x();pos.y=e->y();pos.drawn=false;
        for(int i=0;i<4;++i)pos.view[i]=r->viewport[i];
        for(int i=0;i<16;++i){pos.P[i]=r->projectionMatrix[i];pos.MV[i]=r->markerViewMatrix[i];}
        XYZ c=r->getCenterOfMarkerPos(pos);
        if(!std::isfinite(c.x)||!std::isfinite(c.y)||!std::isfinite(c.z)||c.x<0||c.y<0||c.z<0) return true;
        double sx,sy;
        if(!gluProject(c.x,c.y,c.z,pos.MV,pos.P,pos.view,&sx,&sy,&sphereDepth)) return true;
        sphereValues[0]->setValue(v->coord2global<double>(c.x,iim::horizontal,false));
        sphereValues[1]->setValue(v->coord2global<double>(c.y,iim::vertical,false));
        sphereValues[2]->setValue(v->coord2global<double>(c.z,iim::depth,false));
        sphereDragging=true;return true;
    }
    if(event->type()==QEvent::MouseMove && sphereDragging) {
        double x,y,z;
        if(gluUnProject(e->x(),r->viewport[3]-e->y(),sphereDepth,r->markerViewMatrix,r->projectionMatrix,r->viewport,&x,&y,&z)) {
            x=v->coord2global<double>(x,iim::horizontal,false)-sphereValues[0]->value();
            y=v->coord2global<double>(y,iim::vertical,false)-sphereValues[1]->value();
            z=v->coord2global<double>(z,iim::depth,false)-sphereValues[2]->value();
            sphereValues[3]->setValue(qMax(0.01,sqrt(x*x+y*y+z*z)));
        }
        return true;
    }
    if(event->type()==QEvent::MouseButtonRelease && e->button()==Qt::RightButton) {sphereDragging=false;return true;}
    return false;
}

void PAnoToolBar::applySphere()
{
    CViewer *v=CViewer::getCurrent();
    if(!sphereDialog || !v || !v->isReady() || sphereView!=v->getGLWidget()) return;
    NeuronTree before=PluginInterface::getSWC(),after;
    QString error;int removed,connections;
    if(!reconnectSphere(before,sphereValues[0]->value(),sphereValues[1]->value(),sphereValues[2]->value(),sphereValues[3]->value(),after,error,removed,connections)) {
        QMessageBox::warning(sphereDialog,QString::fromUtf8("无法重连"),error);return;
    }
    v->undoStack.push(new SphereReconnectCommand(before,after));
    buttonUndo->setEnabled(true);buttonRedo->setEnabled(false);
    cancelSphere();
}
