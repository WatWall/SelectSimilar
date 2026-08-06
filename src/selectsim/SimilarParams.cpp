#include "SimilarParams.h"
#include <windows.h>

// ------------------------------------------------------------------------------------------------
// Modifier keys -> selection mode
// ------------------------------------------------------------------------------------------------
SelectionMode GetSelectionModeFromModifiers()
{
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt  = (GetKeyState(VK_MENU)    & 0x8000) != 0;
    if (ctrl) return SelectionMode::Add;
    if (alt)  return SelectionMode::Subtract;
    return SelectionMode::Replace;
}

// ------------------------------------------------------------------------------------------------
// CriterionKind classification
// ------------------------------------------------------------------------------------------------
CriterionKind GetKind(FaceCriterion c)
{
    switch (c)
    {
    case FaceCriterion::Material:     return CriterionKind::ExactMatch;
    case FaceCriterion::FlatSmooth:   return CriterionKind::ExactMatch;
    case FaceCriterion::PolygonSides: return CriterionKind::DiscreteCompare;
    case FaceCriterion::Area:         return CriterionKind::ContinuousRatio;
    case FaceCriterion::Perimeter:    return CriterionKind::ContinuousRatio;
    case FaceCriterion::Normal:       return CriterionKind::ContinuousAngle;
    case FaceCriterion::Coplanar:     return CriterionKind::ContinuousAngle;
    }
    return CriterionKind::ExactMatch;
}

CriterionKind GetKind(EdgeCriterion c)
{
    switch (c)
    {
    case EdgeCriterion::Seam:           return CriterionKind::ExactMatch;
    case EdgeCriterion::Length:         return CriterionKind::ContinuousRatio;
    case EdgeCriterion::Direction:      return CriterionKind::ContinuousAngle;
    case EdgeCriterion::FacesAroundEdge:return CriterionKind::DiscreteCompare;
    case EdgeCriterion::FaceAngles:     return CriterionKind::ContinuousAngle;
    case EdgeCriterion::Crease:         return CriterionKind::ContinuousAbsolute;
    }
    return CriterionKind::ExactMatch;
}

CriterionKind GetKind(VertexCriterion c)
{
    switch (c)
    {
    case VertexCriterion::Normal:         return CriterionKind::ContinuousAngle;
    case VertexCriterion::AdjacentFaces:  return CriterionKind::DiscreteCompare;
    case VertexCriterion::ConnectingEdges:return CriterionKind::DiscreteCompare;
    }
    return CriterionKind::ExactMatch;
}

// ------------------------------------------------------------------------------------------------
// Default thresholds
// ------------------------------------------------------------------------------------------------
float DefaultThreshold(FaceCriterion c)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     return 0.05f;  // 5%
    case CriterionKind::ContinuousAngle:     return 5.0f;   // 5 degrees
    case CriterionKind::ContinuousAbsolute:  return 0.05f;
    default:                                  return 0.0f;
    }
}

float DefaultThreshold(EdgeCriterion c)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     return 0.05f;
    case CriterionKind::ContinuousAngle:     return 5.0f;
    case CriterionKind::ContinuousAbsolute:  return 0.05f;
    default:                                  return 0.0f;
    }
}

float DefaultThreshold(VertexCriterion c)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     return 0.05f;
    case CriterionKind::ContinuousAngle:     return 5.0f;
    case CriterionKind::ContinuousAbsolute:  return 0.05f;
    default:                                  return 0.0f;
    }
}

// ------------------------------------------------------------------------------------------------
// Spinner ranges
// ------------------------------------------------------------------------------------------------
void GetThresholdRange(FaceCriterion c, float& lo, float& hi)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     lo = 0.0f; hi = 1.0f;   break;  // 0-100%
    case CriterionKind::ContinuousAngle:     lo = 0.0f; hi = 180.0f; break;
    case CriterionKind::ContinuousAbsolute:  lo = 0.0f; hi = 1.0f;   break;
    default:                                  lo = 0.0f; hi = 1.0f;   break;
    }
}

void GetThresholdRange(EdgeCriterion c, float& lo, float& hi)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     lo = 0.0f; hi = 1.0f;   break;
    case CriterionKind::ContinuousAngle:     lo = 0.0f; hi = 180.0f; break;
    case CriterionKind::ContinuousAbsolute:  lo = 0.0f; hi = 1.0f;   break;
    default:                                  lo = 0.0f; hi = 1.0f;   break;
    }
}

void GetThresholdRange(VertexCriterion c, float& lo, float& hi)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     lo = 0.0f; hi = 1.0f;   break;
    case CriterionKind::ContinuousAngle:     lo = 0.0f; hi = 180.0f; break;
    case CriterionKind::ContinuousAbsolute:  lo = 0.0f; hi = 1.0f;   break;
    default:                                  lo = 0.0f; hi = 1.0f;   break;
    }
}

// ------------------------------------------------------------------------------------------------
// Unit labels
// ------------------------------------------------------------------------------------------------
const wchar_t* GetThresholdUnit(FaceCriterion c)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     return L"%";
    case CriterionKind::ContinuousAngle:     return L"deg";
    case CriterionKind::ContinuousAbsolute:  return L"";
    default:                                  return L"";
    }
}

const wchar_t* GetThresholdUnit(EdgeCriterion c)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     return L"%";
    case CriterionKind::ContinuousAngle:     return L"deg";
    case CriterionKind::ContinuousAbsolute:  return L"";
    default:                                  return L"";
    }
}

const wchar_t* GetThresholdUnit(VertexCriterion c)
{
    switch (GetKind(c))
    {
    case CriterionKind::ContinuousRatio:     return L"%";
    case CriterionKind::ContinuousAngle:     return L"deg";
    case CriterionKind::ContinuousAbsolute:  return L"";
    default:                                  return L"";
    }
}

// ------------------------------------------------------------------------------------------------
// Display names
// ------------------------------------------------------------------------------------------------
const wchar_t* GetDisplayName(FaceCriterion c)
{
    switch (c)
    {
    case FaceCriterion::Material:     return L"Material";
    case FaceCriterion::FlatSmooth:   return L"Flat/Smooth";
    case FaceCriterion::PolygonSides: return L"Polygon Sides";
    case FaceCriterion::Area:         return L"Area";
    case FaceCriterion::Perimeter:    return L"Perimeter";
    case FaceCriterion::Normal:       return L"Normal";
    case FaceCriterion::Coplanar:     return L"Coplanar";
    }
    return L"";
}

const wchar_t* GetDisplayName(EdgeCriterion c)
{
    switch (c)
    {
    case EdgeCriterion::Seam:           return L"Seam";
    case EdgeCriterion::Length:         return L"Length";
    case EdgeCriterion::Direction:      return L"Direction";
    case EdgeCriterion::FacesAroundEdge:return L"Faces Around Edge";
    case EdgeCriterion::FaceAngles:     return L"Face Angles";
    case EdgeCriterion::Crease:         return L"Crease";
    }
    return L"";
}

const wchar_t* GetDisplayName(VertexCriterion c)
{
    switch (c)
    {
    case VertexCriterion::Normal:         return L"Normal";
    case VertexCriterion::AdjacentFaces:  return L"Adjacent Faces";
    case VertexCriterion::ConnectingEdges:return L"Connecting Edges";
    }
    return L"";
}
