#ifndef VAA3D_WHEEL_STEP_ACCUMULATOR_H
#define VAA3D_WHEEL_STEP_ACCUMULATOR_H
#include <QWheelEvent>

// Keep sub-step input across events: high-resolution wheels often send <120 units.
// mode: 0 zoom, 1 view rotation, 2 model rotation.
class WheelStepAccumulator {
    double remainder;
    int mode;
public:
    WheelStepAccumulator():remainder(0),mode(-1) {}
    int take(const QWheelEvent *event,int newMode,double sensitivity) {
        if(mode!=newMode) {remainder=0;mode=newMode;}
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        // 30 touchpad pixels correspond to one standard wheel detent.
        const double detents=event->pixelDelta().isNull()
            ? event->angleDelta().y()/120.0 : event->pixelDelta().y()/30.0;
#else
        const double detents=event->orientation()==Qt::Vertical ? event->delta()/120.0 : 0;
#endif
        remainder+=detents*(mode==0?4.0:8.0)*sensitivity;
        const int steps=int(remainder+(remainder>=0?1e-9:-1e-9));
        remainder-=steps;
        return steps;
    }
};
#endif
