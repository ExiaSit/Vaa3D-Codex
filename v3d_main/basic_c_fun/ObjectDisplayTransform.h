#ifndef VAA3D_OBJECT_DISPLAY_TRANSFORM_H
#define VAA3D_OBJECT_DISPLAY_TRANSFORM_H
// ySum = lower + upper of the common image (or scene) Y extent.
inline double objectDisplayYCoordinate(double y,double ySum,bool flipped)
{
    return flipped ? ySum-y : y;
}
#endif
