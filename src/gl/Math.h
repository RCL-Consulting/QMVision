#pragma once
#include <cmath>
#include <array>

struct Mat4
{
    std::array<float, 16> m{};
    const float* data() const { return m.data(); }
    static Mat4 identity()
    {
        Mat4 r; r.m = { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 }; return r;
    }
};

struct Vec3
{
    float x, y, z;
    Vec3 operator-( const Vec3& o ) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator+( const Vec3& o ) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator*( float s ) const { return { x * s, y * s, z * s }; }
    Vec3 cross( const Vec3& o ) const
    {
        return { y * o.z - z * o.y,
                 z * o.x - x * o.z,
                 x * o.y - y * o.x };
    }
    float dot( const Vec3& o ) const { return x * o.x + y * o.y + z * o.z; }
    float length() const { return std::sqrt( x * x + y * y + z * z ); }
    Vec3 normalized() const
    {
        float len = length();
        if ( len > 1e-8f ) return { x / len, y / len, z / len };
        return { 0,0,0 };
    }
};

inline Mat4 perspective( float fovyRad, float aspect, float znear, float zfar )
{
    const float f = 1.0f / std::tan( fovyRad * 0.5f );
    Mat4 r{};
    r.m = { f / aspect,0,0,0,
            0,f,0,0,
            0,0,(zfar + znear) / (znear - zfar),-1,
            0,0,(2 * zfar * znear) / (znear - zfar),0 };
    return r;
}

inline Mat4 lookAt( float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz )
{
    const float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    const float fl = std::sqrt( fx * fx + fy * fy + fz * fz );
    const float f0 = fx / fl, f1 = fy / fl, f2 = fz / fl;

    float sx = f1 * uz - f2 * uy;
    float sy = f2 * ux - f0 * uz;
    float sz = f0 * uy - f1 * ux;
    const float sl = std::sqrt( sx * sx + sy * sy + sz * sz );
    sx /= sl; sy /= sl; sz /= sl;

    const float ux2 = sy * f2 - sz * f1;
    const float uy2 = sz * f0 - sx * f2;
    const float uz2 = sx * f1 - sy * f0;

    Mat4 r = Mat4::identity();
    r.m = { sx, ux2, -f0, 0,
            sy, uy2, -f1, 0,
            sz, uz2, -f2, 0,
            -(sx * ex + sy * ey + sz * ez),
            -(ux2 * ex + uy2 * ey + uz2 * ez),
            f0 * ex + f1 * ey + f2 * ez,
            1 };
    return r;
}

// Row-major 4x4 (like your other Mat4 helpers)
inline Mat4 ortho( float l, float r, float b, float t, float n, float f )
{
    Mat4 m{};
    m.m[0] = 2.f / (r - l);
    m.m[5] = 2.f / (t - b);
    m.m[10] = -2.f / (f - n);
    m.m[12] = -(r + l) / (r - l);
    m.m[13] = -(t + b) / (t - b);
    m.m[14] = -(f + n) / (f - n);
    m.m[15] = 1.f;
    return m;
}

inline Mat4 translate( float x, float y, float z )
{
	Mat4 m = Mat4::identity();
    m.m[12] = x; m.m[13] = y; m.m[14] = z;
    return m;
}

inline bool projectToScreen( const Vec3& p,
                             const Mat4& view, const Mat4& proj,
                             int vpW, int vpH,
                             float& outX, float& outY )
{
    // Multiply by view*proj (column-major here; adjust if yours is row-major)
    auto mul = []( const Mat4& m, const Vec3& v )
        {
            float x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12];
            float y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13];
            float z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14];
            float w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15];
            return std::array<float, 4>{x, y, z, w};
        };

    // clip-space
    auto vp = mul( view, p );
    auto cp = std::array<float, 4>{
        proj.m[0] * vp[0] + proj.m[4] * vp[1] + proj.m[8] * vp[2] + proj.m[12] * vp[3],
        proj.m[1] * vp[0] + proj.m[5] * vp[1] + proj.m[9] * vp[2] + proj.m[13] * vp[3],
        proj.m[2] * vp[0] + proj.m[6] * vp[1] + proj.m[10] * vp[2] + proj.m[14] * vp[3],
        proj.m[3] * vp[0] + proj.m[7] * vp[1] + proj.m[11] * vp[2] + proj.m[15]
    };

    if ( cp[3] <= 0.0f ) return false; // behind camera

    // NDC
    float ndcX = cp[0] / cp[3];
    float ndcY = cp[1] / cp[3];
    float ndcZ = cp[2] / cp[3];
    if ( ndcZ < -1.f || ndcZ > 1.f ) return false; // outside depth

    // to pixels (origin top-left for wx)
    outX = (ndcX * 0.5f + 0.5f) * vpW;
    outY = (1.f - (ndcY * 0.5f + 0.5f)) * vpH;
    return true;
}

inline Mat4 orthoPixels( float w, float h )
{
    // maps (0,0) top-left to (w,h) bottom-right
    return ortho( 0.f, w, h, 0.f, -1.f, 1.f );
}