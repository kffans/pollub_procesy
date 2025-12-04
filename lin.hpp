#ifndef LIN_HPP
#define LIN_HPP

typedef std::array<GLfloat,16> Mat4;

/* @NOTE matrices default transforming order: identity * translate * scale * shear * rotate   */
Mat4 operator*(Mat4 const& mL, Mat4 const& mR) {
    Mat4 m = { 0.0f };
    int i, k, j;
    for(i = 0; i < 4; ++i) {        
        for(k = 0; k < 4; ++k) {       
            for(j = 0; j < 4; ++j) {                
                m[i*4+j] += mL[i*4+k] * mR[k*4+j];     
            } 
        }    
    }
    return m;
}

const Mat4 MAT4_IDENTITY = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

#define M_PI 3.14159265358979323846 
const float M_DEGREES = M_PI / 180.0f;
const float M_RADIANS = 180.0f / M_PI;

struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };
float Vec3Length (Vec3 vec) { return (float)sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z); }
float Vec2Length (Vec2 vec) { return (float)sqrt(vec.x * vec.x + vec.y * vec.y); }

namespace lin {
    Vec3 GetPos (const Mat4& mat) {
        return { mat[3], mat[7], mat[11] };
    }
    Vec3 GetScale (const Mat4& mat) {
        return {
            Vec3Length({ mat[0], mat[4], mat[8] }),
            Vec3Length({ mat[1], mat[5], mat[9] }),
            Vec3Length({ mat[2], mat[6], mat[10] })
        };
    }

    Mat4 Translate (float x, float y, float z) { Mat4 mat = MAT4_IDENTITY; mat[3] += x; mat[7] += y; mat[11] += z; return mat; }
    Mat4 Translate (float x, float y) { Mat4 mat = MAT4_IDENTITY; mat[3] += x; mat[7] += y; return mat; }
    /* @TODO rotating x, y, z; rotating around an axis */
    Mat4 Rotate (float degrees) {
        Mat4 mat = MAT4_IDENTITY;
        float sin_b = (float)sin(degrees * M_DEGREES);
        float cos_b = (float)cos(degrees * M_DEGREES);
        mat[0] = cos_b; mat[1] = -sin_b; mat[4] = sin_b; mat[5] = cos_b;
        return mat;
    }
    /* @TODO scaling x, y or z */
    Mat4 Scale (float s) {
        Mat4 mat = MAT4_IDENTITY;
        mat[0] = s; mat[5] = s; mat[10] = s;
        return mat;
    }
    Mat4 Shear (float degrees) {
        Mat4 mat = MAT4_IDENTITY;
        mat[1] = (float)cos(degrees * M_DEGREES)/(float)sin(degrees * M_DEGREES);
        return mat;
    }
}

#endif // lin.hpp
