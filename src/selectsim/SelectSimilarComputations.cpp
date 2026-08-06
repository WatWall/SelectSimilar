#include "SelectSimilarComputations.h"

#include <mnmesh.h>
#include <mesh.h>

#include <vector>
#include <set>
#include <cmath>
#include <algorithm>

namespace {

// ------------------------------------------------------------------------------------------------
// Helpers: dead-component check, NaN-safe
// ------------------------------------------------------------------------------------------------
inline bool FaceAlive(const MNMesh& m, int i)  { return i >= 0 && i < m.FNum() && !m.f[i].GetFlag(MN_DEAD); }
inline bool EdgeAlive(const MNMesh& m, int i)  { return i >= 0 && i < m.ENum() && !m.e[i].GetFlag(MN_DEAD); }
inline bool VertAlive(const MNMesh& m, int i)  { return i >= 0 && i < m.VNum() && !m.v[i].GetFlag(MN_DEAD); }

// Note: MNMesh's normal/angle getters are non-const (they may compute on demand),
// so the value extractors below intentionally take a non-const MNMesh&.

constexpr float kPi    = 3.14159265358979323846f;
constexpr float kRad2Deg = 180.0f / kPi;

inline float SafeAcos(float x)
{
    if (x >  1.0f) x =  1.0f;
    if (x < -1.0f) x = -1.0f;
    return std::acos(x);
}

// Angle in degrees between two (assumed non-zero) vectors.
inline float AngleBetweenDeg(const Point3& a, const Point3& b)
{
    const Point3 na = Normalize(a);
    const Point3 nb = Normalize(b);
    return SafeAcos(DotProd(na, nb)) * kRad2Deg;
}

// ------------------------------------------------------------------------------------------------
// Scalar match logic (for DiscreteCompare / ContinuousRatio / ContinuousAbsolute)
// ------------------------------------------------------------------------------------------------
// For DiscreteCompare, tolerance is ignored and matching is exact-int.
// For ContinuousRatio, tol is a fraction (0..1) applied to the reference magnitude.
// For ContinuousAbsolute, tol is an absolute value.
bool ScalarMatch(float val, float refLo, float refHi, CompareMode cm, float tol, float refMag)
{
    switch (cm)
    {
    case CompareMode::Equal:
    {
        const float band = tol * refMag + 1e-6f;
        return val >= refLo - band && val <= refHi + band;
    }
    case CompareMode::Greater:
        return val > refHi;
    case CompareMode::Less:
        return val < refLo;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
// FACE value extractors
// ------------------------------------------------------------------------------------------------
float FaceArea(MNMesh& m, int i)
{
    const MNFace& f = m.f[i];
    Tab<int> tri;
    f.GetTriangles(tri);
    float area = 0.0f;
    for (int t = 0; t + 2 < tri.Count(); t += 3)
    {
        const Point3 a = m.v[f.vtx[tri[t  ]]].p;
        const Point3 b = m.v[f.vtx[tri[t+1]]].p;
        const Point3 c = m.v[f.vtx[tri[t+2]]].p;
        area += 0.5f * Length(CrossProd(b - a, c - a));
    }
    return area;
}

float FacePerimeter(MNMesh& m, int i)
{
    const MNFace& f = m.f[i];
    float perim = 0.0f;
    for (int j = 0; j < f.deg; ++j)
    {
        const int a = f.vtx[j];
        const int b = f.vtx[(j + 1) % f.deg];
        perim += Length(m.v[b].p - m.v[a].p);
    }
    return perim;
}

Point3 FaceNormal(MNMesh& m, int i)
{
    return m.GetFaceNormal(i, TRUE);
}

// ------------------------------------------------------------------------------------------------
// EDGE value extractors
// ------------------------------------------------------------------------------------------------
float EdgeLengthVal(MNMesh& m, int i)
{
    const MNEdge& e = m.e[i];
    if (e.v1 < 0 || e.v2 < 0 || e.v1 >= m.VNum() || e.v2 >= m.VNum()) return 0.0f;
    return Length(m.v[e.v2].p - m.v[e.v1].p);
}

Point3 EdgeDir(MNMesh& m, int i)
{
    const MNEdge& e = m.e[i];
    if (e.v1 < 0 || e.v2 < 0 || e.v1 >= m.VNum() || e.v2 >= m.VNum()) return Point3(0, 0, 0);
    return m.v[e.v2].p - m.v[e.v1].p;
}

int EdgeFaceCount(MNMesh& m, int i)
{
    const MNEdge& e = m.e[i];
    return (e.f2 >= 0) ? 2 : 1;
}

float EdgeDihedralDeg(MNMesh& m, int i)
{
    // MNMesh::EdgeAngle returns radians; convert to degrees.
    return m.EdgeAngle(i) * kRad2Deg;
}

float EdgeCreaseVal(MNMesh& m, int i)
{
    if (!m.eDataSupport(EDATA_CREASE)) return 0.0f;
    const float* cf = m.edgeFloat(EDATA_CREASE);
    return cf ? cf[i] : 0.0f;
}

// ------------------------------------------------------------------------------------------------
// VERTEX value extractors
// ------------------------------------------------------------------------------------------------
int VertAdjacentFaces(MNMesh& m, int i)
{
    // Prefer the cached vfac table; fall back to direct iteration.
    Tab<int>* vfac = m.GetVFac();
    if (vfac) return vfac[i].Count();

    int count = 0;
    for (int f = 0; f < m.FNum(); ++f)
    {
        if (m.f[f].GetFlag(MN_DEAD)) continue;
        const MNFace& fc = m.f[f];
        for (int j = 0; j < fc.deg; ++j)
            if (fc.vtx[j] == i) { ++count; break; }
    }
    return count;
}

int VertConnectingEdges(MNMesh& m, int i)
{
    Tab<int>* vedg = m.GetVEdge();
    if (vedg) return vedg[i].Count();

    int count = 0;
    for (int e = 0; e < m.ENum(); ++e)
    {
        if (m.e[e].GetFlag(MN_DEAD)) continue;
        if (m.e[e].v1 == i || m.e[e].v2 == i) ++count;
    }
    return count;
}

Point3 VertNormal(MNMesh& m, int i)
{
    return m.GetVertexNormal(i);
}

// ------------------------------------------------------------------------------------------------
// Reference-range collection: scans the current selection to find the
// representative range [lo, hi] of scalar values to compare against.
// ------------------------------------------------------------------------------------------------
template <typename Fn>
bool CollectScalarRange(const SelectSimilarContext& ctx, SelLevel lvl, Fn extract, float& lo, float& hi)
{
    MNMesh& m = *ctx.mesh;
    int n = 0;
    switch (lvl)
    {
    case SelLevel::Face:   n = m.FNum(); break;
    case SelLevel::Edge:   n = m.ENum(); break;
    case SelLevel::Vertex: n = m.VNum(); break;
    default: return false;
    }

    lo =  FLT_MAX;
    hi = -FLT_MAX;
    bool any = false;
    for (int i = 0; i < n; ++i)
    {
        if (i >= ctx.currentSelection.GetSize() || !ctx.currentSelection[i]) continue;
        const float v = extract(m, i);
        if (v < lo) lo = v;
        if (v > hi) hi = v;
        any = true;
    }
    return any;
}

template <typename Fn>
std::vector<Point3> CollectVectorRefs(const SelectSimilarContext& ctx, SelLevel lvl, Fn extract)
{
    std::vector<Point3> refs;
    MNMesh& m = *ctx.mesh;
    int n = 0;
    switch (lvl)
    {
    case SelLevel::Face:   n = m.FNum(); break;
    case SelLevel::Edge:   n = m.ENum(); break;
    case SelLevel::Vertex: n = m.VNum(); break;
    default: return refs;
    }
    for (int i = 0; i < n; ++i)
    {
        if (i >= ctx.currentSelection.GetSize() || !ctx.currentSelection[i]) continue;
        refs.push_back(extract(m, i));
    }
    return refs;
}

// Angle-based match: returns true if `v` is within `maxDeg` of ANY reference.
// For edges, direction is bidirectional: angle to ref OR its negation counts.
bool VectorAngleMatch(const Point3& v, const std::vector<Point3>& refs, float maxDeg, bool bidirectional)
{
    if (refs.empty() || Length(v) < 1e-9f) return false;
    const float maxRad = maxDeg / kRad2Deg;
    for (const Point3& r : refs)
    {
        if (Length(r) < 1e-9f) continue;
    const float d = DotProd(Normalize(v), Normalize(r));
    if (SafeAcos(d) <= maxRad) return true;
    if (bidirectional && SafeAcos(-d) <= maxRad) return true;
    }
    return false;
}

} // anonymous namespace

// ====================================================================================================
// Public compute functions
// ====================================================================================================
void SelectSimilarComputations::ComputeFace(const SelectSimilarContext& ctx, FaceCriterion c, const SimilarParams& p, BitArray& outSel)
{
    MNMesh& m = *ctx.mesh;
    const int nf = m.FNum();
    outSel.SetSize(nf);
    outSel.ClearAll();

    switch (c)
    {
    case FaceCriterion::Material:
    {
        std::set<MtlID> targets;
        for (int i = 0; i < nf; ++i)
        {
            if (i >= ctx.currentSelection.GetSize() || !ctx.currentSelection[i]) continue;
            if (!FaceAlive(m, i)) continue;
            targets.insert(m.f[i].material);
        }
        for (int i = 0; i < nf; ++i)
        {
            if (!FaceAlive(m, i)) continue;
            if (targets.count(m.f[i].material)) outSel.Set(i);
        }
        break;
    }
    case FaceCriterion::FlatSmooth:
    {
        // Match if any smoothing-group bit overlaps with any selected face.
        DWORD mask = 0;
        for (int i = 0; i < nf; ++i)
        {
            if (i >= ctx.currentSelection.GetSize() || !ctx.currentSelection[i]) continue;
            if (!FaceAlive(m, i)) continue;
            mask |= m.f[i].smGroup;
        }
        for (int i = 0; i < nf; ++i)
        {
            if (!FaceAlive(m, i)) continue;
            if ((m.f[i].smGroup & mask) != 0) outSel.Set(i);
        }
        break;
    }
    case FaceCriterion::PolygonSides:
    {
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Face,
                [](MNMesh& mm, int i){ return (float)mm.f[i].deg; }, lo, hi)) return;
        for (int i = 0; i < nf; ++i)
        {
            if (!FaceAlive(m, i)) continue;
            const float v = (float)m.f[i].deg;
            if (ScalarMatch(v, lo, hi, p.compare, 0.0f, 1.0f)) outSel.Set(i);
        }
        break;
    }
    case FaceCriterion::Area:
    case FaceCriterion::Perimeter:
    {
        auto fn = (c == FaceCriterion::Area) ? FaceArea : FacePerimeter;
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Face, fn, lo, hi)) return;
        const float refMag = std::max(std::fabs(lo), std::fabs(hi));
        for (int i = 0; i < nf; ++i)
        {
            if (!FaceAlive(m, i)) continue;
            const float v = fn(m, i);
            if (ScalarMatch(v, lo, hi, p.compare, p.threshold, refMag)) outSel.Set(i);
        }
        break;
    }
    case FaceCriterion::Normal:
    case FaceCriterion::Coplanar:
    {
        auto refs = CollectVectorRefs(ctx, SelLevel::Face, FaceNormal);
        if (refs.empty()) return;
        for (int i = 0; i < nf; ++i)
        {
            if (!FaceAlive(m, i)) continue;
            const Point3 n = FaceNormal(m, i);
            if (VectorAngleMatch(n, refs, p.threshold, /*bidirectional=*/false)) outSel.Set(i);
        }
        break;
    }
    } // switch
}

void SelectSimilarComputations::ComputeEdge(const SelectSimilarContext& ctx, EdgeCriterion c, const SimilarParams& p, BitArray& outSel)
{
    MNMesh& m = *ctx.mesh;
    const int ne = m.ENum();
    outSel.SetSize(ne);
    outSel.ClearAll();

    switch (c)
    {
    case EdgeCriterion::Seam:
    {
        // Binary: select edges whose MN_EDGE_MAP_SEAM flag matches ANY selected edge's.
        bool wantSeam = false;
        bool wantNoSeam = false;
        for (int i = 0; i < ne; ++i)
        {
            if (i >= ctx.currentSelection.GetSize() || !ctx.currentSelection[i]) continue;
            if (!EdgeAlive(m, i)) continue;
            if (m.e[i].GetFlag(MN_EDGE_MAP_SEAM)) wantSeam = true; else wantNoSeam = true;
        }
        for (int i = 0; i < ne; ++i)
        {
            if (!EdgeAlive(m, i)) continue;
            const bool s = m.e[i].GetFlag(MN_EDGE_MAP_SEAM) != 0;
            if ((s && wantSeam) || (!s && wantNoSeam)) outSel.Set(i);
        }
        break;
    }
    case EdgeCriterion::Length:
    {
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Edge, EdgeLengthVal, lo, hi)) return;
        const float refMag = std::max(std::fabs(lo), std::fabs(hi));
        for (int i = 0; i < ne; ++i)
        {
            if (!EdgeAlive(m, i)) continue;
            const float v = EdgeLengthVal(m, i);
            if (ScalarMatch(v, lo, hi, p.compare, p.threshold, refMag)) outSel.Set(i);
        }
        break;
    }
    case EdgeCriterion::Direction:
    {
        auto refs = CollectVectorRefs(ctx, SelLevel::Edge, EdgeDir);
        if (refs.empty()) return;
        for (int i = 0; i < ne; ++i)
        {
            if (!EdgeAlive(m, i)) continue;
            const Point3 d = EdgeDir(m, i);
            // Edges are bidirectional: match direction OR its reverse.
            if (VectorAngleMatch(d, refs, p.threshold, /*bidirectional=*/true)) outSel.Set(i);
        }
        break;
    }
    case EdgeCriterion::FacesAroundEdge:
    {
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Edge,
                [](MNMesh& mm, int i){ return (float)EdgeFaceCount(mm, i); }, lo, hi)) return;
        for (int i = 0; i < ne; ++i)
        {
            if (!EdgeAlive(m, i)) continue;
            const float v = (float)EdgeFaceCount(m, i);
            if (ScalarMatch(v, lo, hi, p.compare, 0.0f, 1.0f)) outSel.Set(i);
        }
        break;
    }
    case EdgeCriterion::FaceAngles:
    {
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Edge, EdgeDihedralDeg, lo, hi)) return;
        // Threshold is in absolute degrees (not a fraction), so refMag=1.0.
        for (int i = 0; i < ne; ++i)
        {
            if (!EdgeAlive(m, i)) continue;
            const float v = EdgeDihedralDeg(m, i);
            if (ScalarMatch(v, lo, hi, p.compare, p.threshold, /*refMag=*/1.0f)) outSel.Set(i);
        }
        break;
    }
    case EdgeCriterion::Crease:
    {
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Edge, EdgeCreaseVal, lo, hi)) return;
        // Crease is already 0..1, threshold is absolute.
        for (int i = 0; i < ne; ++i)
        {
            if (!EdgeAlive(m, i)) continue;
            const float v = EdgeCreaseVal(m, i);
            if (ScalarMatch(v, lo, hi, p.compare, p.threshold, 1.0f)) outSel.Set(i);
        }
        break;
    }
    } // switch
}

void SelectSimilarComputations::ComputeVertex(const SelectSimilarContext& ctx, VertexCriterion c, const SimilarParams& p, BitArray& outSel)
{
    MNMesh& m = *ctx.mesh;
    const int nv = m.VNum();
    outSel.SetSize(nv);
    outSel.ClearAll();

    switch (c)
    {
    case VertexCriterion::Normal:
    {
        auto refs = CollectVectorRefs(ctx, SelLevel::Vertex, VertNormal);
        if (refs.empty()) return;
        for (int i = 0; i < nv; ++i)
        {
            if (!VertAlive(m, i)) continue;
            const Point3 n = VertNormal(m, i);
            if (VectorAngleMatch(n, refs, p.threshold, /*bidirectional=*/false)) outSel.Set(i);
        }
        break;
    }
    case VertexCriterion::AdjacentFaces:
    {
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Vertex,
                [](MNMesh& mm, int i){ return (float)VertAdjacentFaces(mm, i); }, lo, hi)) return;
        for (int i = 0; i < nv; ++i)
        {
            if (!VertAlive(m, i)) continue;
            const float v = (float)VertAdjacentFaces(m, i);
            if (ScalarMatch(v, lo, hi, p.compare, 0.0f, 1.0f)) outSel.Set(i);
        }
        break;
    }
    case VertexCriterion::ConnectingEdges:
    {
        float lo, hi;
        if (!CollectScalarRange(ctx, SelLevel::Vertex,
                [](MNMesh& mm, int i){ return (float)VertConnectingEdges(mm, i); }, lo, hi)) return;
        for (int i = 0; i < nv; ++i)
        {
            if (!VertAlive(m, i)) continue;
            const float v = (float)VertConnectingEdges(m, i);
            if (ScalarMatch(v, lo, hi, p.compare, 0.0f, 1.0f)) outSel.Set(i);
        }
        break;
    }
    } // switch
}

bool SelectSimilarComputations::WantsAdjuster(SelLevel lvl, int critIndex)
{
    switch (lvl)
    {
    case SelLevel::Face:   return NeedsAdjuster(static_cast<FaceCriterion>(critIndex));
    case SelLevel::Edge:   return NeedsAdjuster(static_cast<EdgeCriterion>(critIndex));
    case SelLevel::Vertex: return NeedsAdjuster(static_cast<VertexCriterion>(critIndex));
    default: return false;
    }
}
