#pragma once
// Opt-in local diagnostic. Does not edit annotations or simulate input events.
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include "../v3d_main/3drenderer/v3dr_glwidget.h"
#include "../v3d_main/3drenderer/renderer_gl1.h"

inline void scheduleNeuronRenderBenchmark()
{
    const QString output = qEnvironmentVariable("VAA3D_NEURON_BENCHMARK");
    if (output.isEmpty()) return;
    QTimer::singleShot(8000, qApp, [output]() {
        QFile file(output);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { qApp->exit(2); return; }
        QTextStream log(&file);
        for (QWidget *widget : QApplication::allWidgets()) {
            auto *view = dynamic_cast<V3dR_GLWidget*>(widget);
            auto *renderer = view ? dynamic_cast<Renderer_gl1*>(view->getRenderer()) : nullptr;
            if (!renderer || renderer->listNeuronTree.isEmpty()) continue;
            qsizetype nodes = 0;
            for (const auto &tree : renderer->listNeuronTree) nodes += tree.listNeuron.size();
            log << "nodes=" << nodes << " trees=" << renderer->listNeuronTree.size()
                << " lineType=" << renderer->lineType << "\n";
            const QVariant saved = view->property("vaa3dNeuronDragPreview");
            for (int preview = 0; preview < 2; ++preview) {
                view->setProperty("vaa3dNeuronDragPreview", bool(preview));
                view->makeCurrent();
                view->paintGL(); glFinish(); // warm up
                QElapsedTimer timer; timer.start();
                for (int frame = 0; frame < 12; ++frame) {
                    view->paintGL();
                    glFinish(); // include GPU completion, not just CPU submission
                }
                log << (preview ? "preview" : "full") << "_frame_ms="
                    << timer.nsecsElapsed() / 12000000.0 << "\n";
            }
            view->setProperty("vaa3dNeuronDragPreview", saved);
            view->paintGL(); glFinish();
            log << "restored=" << !view->property("vaa3dNeuronDragPreview").toBool() << "\n";
            log.flush();
            qApp->exit(0);
            return;
        }
        log << "ERROR: no loaded SWC renderer\n"; log.flush(); qApp->exit(3);
    });
}
