#pragma once

#include <max.h>
#include "SimilarCriteria.h"
#include "SimilarParams.h"
#include "SelectSimilarEngine.h"

// Pure selection-computation API.  These functions do NOT touch the modifier's
// state - they only read the (read-only) cached MNMesh and produce a new
// BitArray that the caller may write live or wrap in undo as it sees fit.

namespace SelectSimilarComputations
{
    void ComputeFace  (const SelectSimilarContext& ctx, FaceCriterion   c, const SimilarParams& p, BitArray& outSel);
    void ComputeEdge  (const SelectSimilarContext& ctx, EdgeCriterion   c, const SimilarParams& p, BitArray& outSel);
    void ComputeVertex(const SelectSimilarContext& ctx, VertexCriterion c, const SimilarParams& p, BitArray& outSel);

    // Returns false for ExactMatch criteria (Material, FlatSmooth, Seam) since
    // they produce sensible results with default Equal compare; for everything
    // else returns true to signal the caller that the adjuster dialog applies.
    // (Mirrors NeedsAdjuster but kept here as a single dispatch.)
    bool WantsAdjuster(SelLevel lvl, int critIndex);
}
