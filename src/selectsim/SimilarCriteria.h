#pragma once

// Sub-object level we are operating on.
// Border collapses to Edge for selection purposes; Element collapses to Face.
enum class SelLevel
{
    None = 0,
    Vertex,
    Edge,      // also covers Border
    Face,      // also covers Element
};

// Compare direction, mirrors Blender's "Compare" dropdown.
enum class CompareMode
{
    Equal,
    Greater,
    Less,
};

// Face-level criteria (7)
enum class FaceCriterion
{
    Material,       // direct apply (set membership)
    FlatSmooth,     // direct apply (smoothing-group overlap)
    PolygonSides,   // adjuster, compare-only (discrete)
    Area,           // adjuster, % threshold (continuous)
    Perimeter,      // adjuster, % threshold (continuous)
    Normal,         // adjuster, degrees threshold (continuous angle)
    Coplanar,       // adjuster, degrees threshold (continuous angle)
};

// Edge-level criteria (6 - Sharpness skipped, identical to Crease in Max)
enum class EdgeCriterion
{
    Seam,             // direct apply (binary UV-seam flag)
    Length,           // adjuster, % threshold (continuous)
    Direction,        // adjuster, degrees threshold (continuous angle)
    FacesAroundEdge,  // adjuster, compare-only (discrete)
    FaceAngles,       // adjuster, degrees threshold (continuous angle, dihedral)
    Crease,           // adjuster, absolute threshold (continuous 0..1)
};

// Vertex-level criteria (3 - Vertex Groups skipped, no Max equivalent)
enum class VertexCriterion
{
    Normal,          // adjuster, degrees threshold (continuous angle)
    AdjacentFaces,   // adjuster, compare-only (discrete)
    ConnectingEdges, // adjuster, compare-only (discrete)
};
