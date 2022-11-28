#pragma once
#include"types.h"

C_BEGIN

typedef struct {
    f32 data[16];
} Mat4;

static Mat4 mat4_ident()
{
    Mat4 m = {{
             1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1
           }};
    return m;
}

#ifdef _WIN32
/* WTF windows */
#undef far
#undef near
#endif

static Mat4 mat4_ortho(f32 left, f32 right,
                       f32 bottom, f32 top,
                       f32 near, f32 far)
{
    /*
     * The arguments describe the size and position of a 3d box in camera
     * coordinates, assuming camera is at 0,0,0 and is pointing down the -z
     * axis (so near and far will be negated)
     * We need a matrix that transforms this box into opengl CVV
     * coordinates; a 2 unit wide cube centered at the origin, with z axis
     * pointing forward
     *
     * Break it down into two steps; translation (T) and scale (S);
     * Our ortho matrix M will be M = S * T
     * i.e. translate to the origin first, then scale
     *
     * 1. Translation
     * For this we need the vector from the origin to the centre of the box
     * For each axis, the value will be c1 + (c2-c1) / 2, or (c1 + c2) / 2
     * Then negate it, because we want to move from camera to origin
     * Negate near and far to account for the camera looking along -z
     * v = {
     *      -(left + right) / 2.0F,
     *      -(bottom + top) / 2.0F,
     *      (near + far) / 2.0F
     *     }
     * Translation matrix T = identity matrix with v as rightmost column
     * (note for openGL data layout; rightmost column -> bottommost row)
     *
     * 2. Scale to a 2x2x2 box
     * We scale each camera box axis to the box CVV side length (2)
     * For each axis, divide by the camera box length on that axis, and
     * multiply by 2; i.e. the scaled value will be 2/(c2-c1)
     * Negate near and far to account for the camera looking along -z
     * S = {2.0F/(right - left), 0,                   0,                   0,                  0,
     *                           0,                   2.0F/(top - bottom), 0,                  0,
     *                           0,                   0,                   -2.0F/(far - near), 0,
     *                           0,                   0,                   0,                  1
     *     }
     *
     * Note the matrix data layout matches openGL's
     * S * T  gives:
     */
    Mat4 m = {{
             2.0F/(right - left),            0,                              0,                           0,
             0,                              2.0F/(top - bottom),            0,                           0,
             0,                              0,                              -2.0F/(far - near),          0,
             -(right + left)/(right - left), -(top + bottom)/(top - bottom), -(far + near)/(far - near),  1
           }};
    return m;
}

C_END
