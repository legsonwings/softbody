#include "beziermaths.h"
#include "immintrin.h"

namespace beziermaths
{
vector3 evaluateavx2(beziervolume<2> const& v, vector3 const& bt0, vector3 const& bt1, vector3 const& bt2)
{
    // literally vectorizes the following expression
    // bt2.x * (bt1.x * (v[0] * bt0.x + v[1] * bt0.y + v[2] * bt0.z) + bt1.y * (v[3] * bt0.x + v[4] * bt0.y + v[5] * bt0.z) + bt1.z * (v[6] * bt0.x + v[7] * bt0.y + v[8] * bt0.z))
    // + bt2.y * (bt1.x * (v[9] * bt0.x + v[10] * bt0.y + v[11] * bt0.z) + bt1.y * (v[12] * bt0.x + v[13] * bt0.y + v[14] * bt0.z) + bt1.z * (v[15] * bt0.x + v[16] * bt0.y + v[17] * bt0.z))
    // + bt2.z * (bt1.x * (v[18] * bt0.x + v[19] * bt0.y + v[20] * bt0.z) + bt1.y * (v[21] * bt0.x + v[22] * bt0.y + v[23] * bt0.z) + bt1.z * (v[24] * bt0.x + v[25] * bt0.y + v[26] * bt0.z));

    // vab is line along 3rd dim. ab represent first two dims.
    __m256 v00 = _mm256_setr_ps(v[0].x, v[0].y, v[0].z, v[9].x, v[9].y, v[9].z, v[18].x, v[18].y);
    __m256 v10 = _mm256_setr_ps(v[1].x, v[1].y, v[1].z, v[10].x, v[10].y, v[10].z, v[19].x, v[19].y);
    __m256 v20 = _mm256_setr_ps(v[2].x, v[2].y, v[2].z, v[11].x, v[11].y, v[11].z, v[20].x, v[20].y);

    __m256 v01 = _mm256_setr_ps(v[3].x, v[3].y, v[3].z, v[12].x, v[12].y, v[12].z, v[21].x, v[21].y);
    __m256 v11 = _mm256_setr_ps(v[4].x, v[4].y, v[4].z, v[13].x, v[13].y, v[13].z, v[22].x, v[22].y);
    __m256 v21 = _mm256_setr_ps(v[5].x, v[5].y, v[5].z, v[14].x, v[14].y, v[14].z, v[23].x, v[23].y);

    __m256 v02 = _mm256_setr_ps(v[6].x, v[6].y, v[6].z, v[15].x, v[15].y, v[15].z, v[24].x, v[24].y);
    __m256 v12 = _mm256_setr_ps(v[7].x, v[7].y, v[7].z, v[16].x, v[16].y, v[16].z, v[25].x, v[25].y);
    __m256 v22 = _mm256_setr_ps(v[8].x, v[8].y, v[8].z, v[17].x, v[17].y, v[17].z, v[26].x, v[26].y);

    __m256 t0x = _mm256_set1_ps(bt0.x);
    __m256 t0y = _mm256_set1_ps(bt0.y);
    __m256 t0z = _mm256_set1_ps(bt0.z);
    __m256 t1x = _mm256_set1_ps(bt1.x);
    __m256 t1y = _mm256_set1_ps(bt1.y);
    __m256 t1z = _mm256_set1_ps(bt1.z);

    __m256 t2 = _mm256_setr_ps(bt2.x, bt2.x, bt2.x, bt2.y, bt2.y, bt2.y, bt2.z, bt2.z);

    // rnbx is plane with xth coordinate at n
    __m256 r0b1 = _mm256_fmadd_ps(t0x, v00, _mm256_fmadd_ps(t0y, v10, _mm256_mul_ps(t0z, v20)));
    __m256 r1b1 = _mm256_fmadd_ps(t0x, v01, _mm256_fmadd_ps(t0y, v11, _mm256_mul_ps(t0z, v21)));
    __m256 r2b1 = _mm256_fmadd_ps(t0x, v02, _mm256_fmadd_ps(t0y, v12, _mm256_mul_ps(t0z, v22)));

    float res[8];
    _mm256_store_ps(res, _mm256_mul_ps(t2, _mm256_fmadd_ps(t1x, r0b1, _mm256_fmadd_ps(t1y, r1b1, _mm256_mul_ps(t1z, r2b1)))));

    // compute the remaining value the scalar way
    float const l = bt2.z * (bt1.x * (v[18].z * bt0.x + v[19].z * bt0.y + v[20].z * bt0.z) + bt1.y * (v[21].z * bt0.x + v[22].z * bt0.y + v[23].z * bt0.z) + bt1.z * (v[24].z * bt0.x + v[25].z * bt0.y + v[26].z * bt0.z));
    return vector3{ res[0] + res[3] + res[6], res[1] + res[4] + res[7], res[2] + res[5] + l };
}

// some computations are duplicated, but performance actually is worse when caching them
std::vector<geometry::vertex> bulkevaluate(beziervolume<2> const& v, std::vector<geometry::vertex> const& vertices)
{
    std::vector<geometry::vertex> ret;
                          
    // vab is line along 3rd dim. ab represent first two dims.
    __m256 v00 = _mm256_setr_ps(v[0].x, v[0].y, v[0].z, v[9].x, v[9].y, v[9].z, v[18].x, v[18].y);
    __m256 v10 = _mm256_setr_ps(v[1].x, v[1].y, v[1].z, v[10].x, v[10].y, v[10].z, v[19].x, v[19].y);
    __m256 v20 = _mm256_setr_ps(v[2].x, v[2].y, v[2].z, v[11].x, v[11].y, v[11].z, v[20].x, v[20].y);

    __m256 v01 = _mm256_setr_ps(v[3].x, v[3].y, v[3].z, v[12].x, v[12].y, v[12].z, v[21].x, v[21].y);
    __m256 v11 = _mm256_setr_ps(v[4].x, v[4].y, v[4].z, v[13].x, v[13].y, v[13].z, v[22].x, v[22].y);
    __m256 v21 = _mm256_setr_ps(v[5].x, v[5].y, v[5].z, v[14].x, v[14].y, v[14].z, v[23].x, v[23].y);

    __m256 v02 = _mm256_setr_ps(v[6].x, v[6].y, v[6].z, v[15].x, v[15].y, v[15].z, v[24].x, v[24].y);
    __m256 v12 = _mm256_setr_ps(v[7].x, v[7].y, v[7].z, v[16].x, v[16].y, v[16].z, v[25].x, v[25].y);
    __m256 v22 = _mm256_setr_ps(v[8].x, v[8].y, v[8].z, v[17].x, v[17].y, v[17].z, v[26].x, v[26].y);

    float res[8];
    for (auto const& vtx : vertices)
    {
        auto const [p0, p2, p1] = vtx.position;
        vector3 const bt0 = qbasis(p0);
        vector3 const bt1 = qbasis(p1);
        vector3 const bt2 = qbasis(p2);
        vector3 const dt0 = dqbasis(p0);
        vector3 const dt1 = dqbasis(p1);
        vector3 const dt2 = dqbasis(p2);

        vector3 pos, tn0, tn1, tn2;

        {
            __m256 t0x = _mm256_set1_ps(bt0.x);
            __m256 t0y = _mm256_set1_ps(bt0.y);
            __m256 t0z = _mm256_set1_ps(bt0.z);
            __m256 t1x = _mm256_set1_ps(bt1.x);
            __m256 t1y = _mm256_set1_ps(bt1.y);
            __m256 t1z = _mm256_set1_ps(bt1.z);

            __m256 t2 = _mm256_setr_ps(bt2.x, bt2.x, bt2.x, bt2.y, bt2.y, bt2.y, bt2.z, bt2.z);

            // rnbx is plane with xth coordinate at n
            __m256 r0b1 = _mm256_fmadd_ps(t0x, v00, _mm256_fmadd_ps(t0y, v10, _mm256_mul_ps(t0z, v20)));
            __m256 r1b1 = _mm256_fmadd_ps(t0x, v01, _mm256_fmadd_ps(t0y, v11, _mm256_mul_ps(t0z, v21)));
            __m256 r2b1 = _mm256_fmadd_ps(t0x, v02, _mm256_fmadd_ps(t0y, v12, _mm256_mul_ps(t0z, v22)));

            _mm256_store_ps(res, _mm256_mul_ps(t2, _mm256_fmadd_ps(t1x, r0b1, _mm256_fmadd_ps(t1y, r1b1, _mm256_mul_ps(t1z, r2b1)))));

            // compute the remaining value the scalar way
            float const l = bt2.z * (bt1.x * (v[18].z * bt0.x + v[19].z * bt0.y + v[20].z * bt0.z) + bt1.y * (v[21].z * bt0.x + v[22].z * bt0.y + v[23].z * bt0.z) + bt1.z * (v[24].z * bt0.x + v[25].z * bt0.y + v[26].z * bt0.z));
            pos = vector3{ res[0] + res[3] + res[6], res[1] + res[4] + res[7], res[2] + res[5] + l };
        }

        // compute the normals
        {
            __m256 t0x = _mm256_set1_ps(dt0.x);
            __m256 t0y = _mm256_set1_ps(dt0.y);
            __m256 t0z = _mm256_set1_ps(dt0.z);
            __m256 t1x = _mm256_set1_ps(bt1.x);
            __m256 t1y = _mm256_set1_ps(bt1.y);
            __m256 t1z = _mm256_set1_ps(bt1.z);

            __m256 t2 = _mm256_setr_ps(bt2.x, bt2.x, bt2.x, bt2.y, bt2.y, bt2.y, bt2.z, bt2.z);

            __m256 r0b1 = _mm256_fmadd_ps(t0x, v00, _mm256_fmadd_ps(t0y, v10, _mm256_mul_ps(t0z, v20)));
            __m256 r1b1 = _mm256_fmadd_ps(t0x, v01, _mm256_fmadd_ps(t0y, v11, _mm256_mul_ps(t0z, v21)));
            __m256 r2b1 = _mm256_fmadd_ps(t0x, v02, _mm256_fmadd_ps(t0y, v12, _mm256_mul_ps(t0z, v22)));

            _mm256_store_ps(res, _mm256_mul_ps(t2, _mm256_fmadd_ps(t1x, r0b1, _mm256_fmadd_ps(t1y, r1b1, _mm256_mul_ps(t1z, r2b1)))));

            float const l = bt2.z * (bt1.x * (v[18].z * dt0.x + v[19].z * dt0.y + v[20].z * dt0.z) + bt1.y * (v[21].z * dt0.x + v[22].z * dt0.y + v[23].z * dt0.z) + bt1.z * (v[24].z * dt0.x + v[25].z * dt0.y + v[26].z * dt0.z));
            tn0 = vector3{ res[0] + res[3] + res[6], res[1] + res[4] + res[7], res[2] + res[5] + l };
        }

        {
            __m256 t0x = _mm256_set1_ps(bt0.x);
            __m256 t0y = _mm256_set1_ps(bt0.y);
            __m256 t0z = _mm256_set1_ps(bt0.z);
            __m256 t1x = _mm256_set1_ps(bt1.x);
            __m256 t1y = _mm256_set1_ps(bt1.y);
            __m256 t1z = _mm256_set1_ps(bt1.z);

            __m256 t2 = _mm256_setr_ps(dt2.x, dt2.x, dt2.x, dt2.y, dt2.y, dt2.y, dt2.z, dt2.z);

            __m256 r0b1 = _mm256_fmadd_ps(t0x, v00, _mm256_fmadd_ps(t0y, v10, _mm256_mul_ps(t0z, v20)));
            __m256 r1b1 = _mm256_fmadd_ps(t0x, v01, _mm256_fmadd_ps(t0y, v11, _mm256_mul_ps(t0z, v21)));
            __m256 r2b1 = _mm256_fmadd_ps(t0x, v02, _mm256_fmadd_ps(t0y, v12, _mm256_mul_ps(t0z, v22)));

            _mm256_store_ps(res, _mm256_mul_ps(t2, _mm256_fmadd_ps(t1x, r0b1, _mm256_fmadd_ps(t1y, r1b1, _mm256_mul_ps(t1z, r2b1)))));

            float const l = dt2.z * (bt1.x * (v[18].z * bt0.x + v[19].z * bt0.y + v[20].z * bt0.z) + bt1.y * (v[21].z * bt0.x + v[22].z * bt0.y + v[23].z * bt0.z) + bt1.z * (v[24].z * bt0.x + v[25].z * bt0.y + v[26].z * bt0.z));
            tn1 = vector3{ res[0] + res[3] + res[6], res[1] + res[4] + res[7], res[2] + res[5] + l };
        }

        {
            __m256 t0x = _mm256_set1_ps(bt0.x);
            __m256 t0y = _mm256_set1_ps(bt0.y);
            __m256 t0z = _mm256_set1_ps(bt0.z);
            __m256 t1x = _mm256_set1_ps(dt1.x);
            __m256 t1y = _mm256_set1_ps(dt1.y);
            __m256 t1z = _mm256_set1_ps(dt1.z);

            __m256 t2 = _mm256_setr_ps(bt2.x, bt2.x, bt2.x, bt2.y, bt2.y, bt2.y, bt2.z, bt2.z);

            __m256 r0b1 = _mm256_fmadd_ps(t0x, v00, _mm256_fmadd_ps(t0y, v10, _mm256_mul_ps(t0z, v20)));
            __m256 r1b1 = _mm256_fmadd_ps(t0x, v01, _mm256_fmadd_ps(t0y, v11, _mm256_mul_ps(t0z, v21)));
            __m256 r2b1 = _mm256_fmadd_ps(t0x, v02, _mm256_fmadd_ps(t0y, v12, _mm256_mul_ps(t0z, v22)));

            _mm256_store_ps(res, _mm256_mul_ps(t2, _mm256_fmadd_ps(t1x, r0b1, _mm256_fmadd_ps(t1y, r1b1, _mm256_mul_ps(t1z, r2b1)))));

            float const l = bt2.z * (dt1.x * (v[18].z * bt0.x + v[19].z * bt0.y + v[20].z * bt0.z) + dt1.y * (v[21].z * bt0.x + v[22].z * bt0.y + v[23].z * bt0.z) + dt1.z * (v[24].z * bt0.x + v[25].z * bt0.y + v[26].z * bt0.z));
            tn2 = vector3{ res[0] + res[3] + res[6], res[1] + res[4] + res[7], res[2] + res[5] + l };
        }

        ret.emplace_back(pos, vector3::TransformNormal(vtx.normal, matrix(tn0, tn1, tn2)).Normalized());
    }

    return ret;
}

voleval evaluatefast(beziervolume<2> const& v, vector3 const& uwv)
{
    auto const [t0, t2, t1] = uwv;
    vector3 const bt0 = qbasis(t0);
    vector3 const bt1 = qbasis(t1);
    vector3 const bt2 = qbasis(t2);
    vector3 const dt0 = dqbasis(t0);
    vector3 const dt1 = dqbasis(t1);
    vector3 const dt2 = dqbasis(t2);

    // create orientation using partial derivates
    return { evaluateavx2(v, bt0, bt1, bt2), matrix{ evaluateavx2(v, dt0, bt1, bt2).Normalized(), evaluateavx2(v, bt0, bt1, dt2).Normalized(), evaluateavx2(v, bt0, dt1, bt2).Normalized() } };
}
}