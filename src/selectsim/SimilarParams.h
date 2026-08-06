#pragma once

#include "SimilarCriteria.h"

// What kind of value a criterion compares on - drives UI and matching logic.
enum class CriterionKind
{
    ExactMatch,         // Material, FlatSmooth, Seam - set membership, no dialog
    DiscreteCompare,    // PolygonSides, FacesAround, AdjacentFaces, ConnectingEdges - compare, no threshold
    ContinuousRatio,    // Length, Area, Perimeter - compare + threshold as % of reference
    ContinuousAngle,    // Normal, Direction, Coplanar, FaceAngles - compare + threshold in degrees
    ContinuousAbsolute, // Crease - compare + threshold as absolute value
};

// All params needed to compute a "select similar" result.
struct SimilarParams
{
    CompareMode   compare    = CompareMode::Equal;
    float         threshold  = 0.05f;  // meaning depends on CriterionKind
};

// --- Per-criterion metadata ---

CriterionKind GetKind(FaceCriterion   c);
CriterionKind GetKind(EdgeCriterion   c);
CriterionKind GetKind(VertexCriterion c);

// Direct-apply criteria (ExactMatch) skip the dialog entirely.
inline bool NeedsAdjuster(FaceCriterion   c) { return GetKind(c) != CriterionKind::ExactMatch; }
inline bool NeedsAdjuster(EdgeCriterion   c) { return GetKind(c) != CriterionKind::ExactMatch; }
inline bool NeedsAdjuster(VertexCriterion c) { return GetKind(c) != CriterionKind::ExactMatch; }

// Whether the threshold control should be shown in the adjuster.
inline bool HasThreshold(FaceCriterion   c) { CriterionKind k = GetKind(c); return k != CriterionKind::ExactMatch && k != CriterionKind::DiscreteCompare; }
inline bool HasThreshold(EdgeCriterion   c) { CriterionKind k = GetKind(c); return k != CriterionKind::ExactMatch && k != CriterionKind::DiscreteCompare; }
inline bool HasThreshold(VertexCriterion c) { CriterionKind k = GetKind(c); return k != CriterionKind::ExactMatch && k != CriterionKind::DiscreteCompare; }

// Default threshold value, depends on kind.
float DefaultThreshold(FaceCriterion   c);
float DefaultThreshold(EdgeCriterion   c);
float DefaultThreshold(VertexCriterion c);

// Min/max for the threshold spinner.
void GetThresholdRange(FaceCriterion   c, float& lo, float& hi);
void GetThresholdRange(EdgeCriterion   c, float& lo, float& hi);
void GetThresholdRange(VertexCriterion c, float& lo, float& hi);

// Unit suffix shown next to the threshold control (e.g. "%", "deg", "").
const wchar_t* GetThresholdUnit(FaceCriterion   c);
const wchar_t* GetThresholdUnit(EdgeCriterion   c);
const wchar_t* GetThresholdUnit(VertexCriterion c);

// Human-readable name (used in dialog title and menu text).
const wchar_t* GetDisplayName(FaceCriterion   c);
const wchar_t* GetDisplayName(EdgeCriterion   c);
const wchar_t* GetDisplayName(VertexCriterion c);
