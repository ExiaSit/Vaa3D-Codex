// Build against the complete application using swc_navigation_gui_test.mk.
#define main vaa3d_original_main
#include "../v3d_main/v3d/main.cpp"
#undef main
#include "../v3d_main/3drenderer/v3dr_mainwindow.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"
#include <QTemporaryDir>
#include <QMouseEvent>
#include <QFocusEvent>
#include <cassert>

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(v3d);
    auto &app = *V3dApplication::getInstance(argc, argv);
    auto &mw = *app.getMainWindow();
    mw.show();
    QTemporaryDir dir;
    assert(dir.isValid());
    const QString path = dir.path() + "/navigation.swc";
    QFile file(path);
    assert(file.open(QIODevice::WriteOnly));
    const QByteArray data("1 2 4 6 12 1 -1\n2 2 16 6 12 1 1\n3 3 28 6 12 1 2\n");
    file.write(data); file.close();
    mw.loadV3DFile(path, false, false);
    QTimer::singleShot(500, &app, [&]() {
        auto window = mw.find3DViewer(path); assert(window);
        auto gl = window->getGLWidget();
        auto r = dynamic_cast<Renderer_gl1*>(gl->getRenderer());
        assert(r && r->listNeuronTree.size() == 1);
        const NeuronTree before = r->listNeuronTree.first();
        const int mode = before.linemode;
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(40,40), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        gl->mousePressEvent(&press);
        QMouseEvent move(QEvent::MouseMove, QPointF(70,60), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        gl->mouseMoveEvent(&move);
        assert(gl->property("vaa3dNeuronDragPreview").toBool());
        gl->makeCurrent(); gl->paintGL(); glFinish();
        assert(r->listNeuronTree.first().linemode == mode);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(70,60), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        gl->mouseReleaseEvent(&release);
        assert(!gl->property("vaa3dNeuronDragPreview").toBool());
        gl->makeCurrent(); gl->paintGL(); glFinish();
        gl->mousePressEvent(&press); gl->mouseMoveEvent(&move);
        QFocusEvent out(QEvent::FocusOut);
        gl->focusOutEvent(&out);
        assert(!gl->property("vaa3dNeuronDragPreview").toBool());
        const auto &after = r->listNeuronTree.first();
        assert(after.linemode == mode && after.color.i == before.color.i);
        assert(after.listNeuron.size() == before.listNeuron.size());
        for (int i=0;i<after.listNeuron.size();++i) {
            const auto &a=after.listNeuron[i], &b=before.listNeuron[i];
            assert(a.n==b.n && a.pn==b.pn && a.x==b.x && a.y==b.y && a.z==b.z && a.r==b.r && a.type==b.type);
        }
        assert(file.open(QIODevice::ReadOnly) && file.readAll()==data);
        qInfo("SWC_NAVIGATION_GUI_PASS"); app.quit();
    });
    QTimer::singleShot(15000, &app, [](){qFatal("SWC navigation test timeout");});
    return app.exec();
}
