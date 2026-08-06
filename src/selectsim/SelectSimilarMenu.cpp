#include "SelectSimilarMenu.h"
#include "SimilarCriteria.h"
#include "SimilarParams.h"

#include <windows.h>
#include <max.h>

namespace {
    // Menu item IDs. We pack (level << 16) | index so a single OnCommand can
    // dispatch from the returned id unambiguously.  Face criteria start at 0x10,
    // Edge at 0x20, Vertex at 0x30 to keep ranges visually distinct.
    enum MenuIds : UINT {
        // Face
        IDM_FACE_MATERIAL     = 0x1001,
        IDM_FACE_FLATSMOOTH   = 0x1002,
        IDM_FACE_POLYSIDES    = 0x1003,
        IDM_FACE_AREA         = 0x1004,
        IDM_FACE_PERIMETER    = 0x1005,
        IDM_FACE_NORMAL       = 0x1006,
        IDM_FACE_COPLANAR     = 0x1007,
        // Edge
        IDM_EDGE_SEAM         = 0x2001,
        IDM_EDGE_LENGTH       = 0x2002,
        IDM_EDGE_DIRECTION    = 0x2003,
        IDM_EDGE_FACESAROUND  = 0x2004,
        IDM_EDGE_FACEANGLES   = 0x2005,
        IDM_EDGE_CREASE       = 0x2006,
        // Vertex
        IDM_VERT_NORMAL       = 0x3001,
        IDM_VERT_ADJFACES     = 0x3002,
        IDM_VERT_CONNECTEDGES = 0x3003,
    };
}

bool ShowCriterionPopup(SelLevel level,
                        FaceCriterion&   outFace,
                        EdgeCriterion&   outEdge,
                        VertexCriterion& outVertex)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return false;

    switch (level)
    {
    case SelLevel::Face:
        AppendMenuW(hMenu, MF_STRING, IDM_FACE_MATERIAL,   L"By Material");
        AppendMenuW(hMenu, MF_STRING, IDM_FACE_FLATSMOOTH, L"By Flat/Smooth (Smoothing Groups)");
        AppendMenuW(hMenu, MF_STRING, IDM_FACE_POLYSIDES,  L"By Polygon Sides");
        AppendMenuW(hMenu, MF_STRING, IDM_FACE_AREA,       L"By Area");
        AppendMenuW(hMenu, MF_STRING, IDM_FACE_PERIMETER,  L"By Perimeter");
        AppendMenuW(hMenu, MF_STRING, IDM_FACE_NORMAL,     L"By Normal");
        AppendMenuW(hMenu, MF_STRING, IDM_FACE_COPLANAR,   L"By Coplanar");
        break;
    case SelLevel::Edge:
        AppendMenuW(hMenu, MF_STRING, IDM_EDGE_SEAM,        L"By Seam");
        AppendMenuW(hMenu, MF_STRING, IDM_EDGE_LENGTH,      L"By Length");
        AppendMenuW(hMenu, MF_STRING, IDM_EDGE_DIRECTION,   L"By Direction");
        AppendMenuW(hMenu, MF_STRING, IDM_EDGE_FACESAROUND, L"By Faces Around Edge");
        AppendMenuW(hMenu, MF_STRING, IDM_EDGE_FACEANGLES,  L"By Face Angles");
        AppendMenuW(hMenu, MF_STRING, IDM_EDGE_CREASE,      L"By Crease");
        break;
    case SelLevel::Vertex:
        AppendMenuW(hMenu, MF_STRING, IDM_VERT_NORMAL,       L"By Normal");
        AppendMenuW(hMenu, MF_STRING, IDM_VERT_ADJFACES,     L"By Adjacent Faces");
        AppendMenuW(hMenu, MF_STRING, IDM_VERT_CONNECTEDGES, L"By Connecting Edges");
        break;
    default:
        DestroyMenu(hMenu);
        return false;
    }

    POINT cur{};
    GetCursorPos(&cur);

    Interface* ip = GetCOREInterface();
    HWND hOwner = ip ? static_cast<HWND>(ip->GetMAXHWnd()) : nullptr;

    UINT flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON;
    const int choice = static_cast<int>(TrackPopupMenuEx(
        hMenu, flags, cur.x, cur.y, hOwner, nullptr));

    DestroyMenu(hMenu);

    switch (choice)
    {
    case IDM_FACE_MATERIAL:     outFace = FaceCriterion::Material;       return true;
    case IDM_FACE_FLATSMOOTH:   outFace = FaceCriterion::FlatSmooth;     return true;
    case IDM_FACE_POLYSIDES:    outFace = FaceCriterion::PolygonSides;   return true;
    case IDM_FACE_AREA:         outFace = FaceCriterion::Area;           return true;
    case IDM_FACE_PERIMETER:    outFace = FaceCriterion::Perimeter;      return true;
    case IDM_FACE_NORMAL:       outFace = FaceCriterion::Normal;         return true;
    case IDM_FACE_COPLANAR:     outFace = FaceCriterion::Coplanar;       return true;

    case IDM_EDGE_SEAM:         outEdge = EdgeCriterion::Seam;             return true;
    case IDM_EDGE_LENGTH:       outEdge = EdgeCriterion::Length;           return true;
    case IDM_EDGE_DIRECTION:    outEdge = EdgeCriterion::Direction;        return true;
    case IDM_EDGE_FACESAROUND:  outEdge = EdgeCriterion::FacesAroundEdge;  return true;
    case IDM_EDGE_FACEANGLES:   outEdge = EdgeCriterion::FaceAngles;       return true;
    case IDM_EDGE_CREASE:       outEdge = EdgeCriterion::Crease;           return true;

    case IDM_VERT_NORMAL:        outVertex = VertexCriterion::Normal;          return true;
    case IDM_VERT_ADJFACES:      outVertex = VertexCriterion::AdjacentFaces;   return true;
    case IDM_VERT_CONNECTEDGES:  outVertex = VertexCriterion::ConnectingEdges; return true;

    default: return false;
    }
}
