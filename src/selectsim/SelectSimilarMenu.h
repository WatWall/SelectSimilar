#pragma once

#include "SimilarCriteria.h"

// Shows the criterion popup menu at the cursor and blocks until the user picks.
// Returns true and fills outFace/outEdge/outVertex if a valid pick is made for
// the current level; returns false if the user cancels or the level is invalid.
bool ShowCriterionPopup(SelLevel level,
                        FaceCriterion&   outFace,
                        EdgeCriterion&   outEdge,
                        VertexCriterion& outVertex);
