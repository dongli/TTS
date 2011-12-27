#ifndef guard_h
#define guard_h

#include "MeshManager.h"
#include "FlowManager.h"
#include "PolygonManager.h"
#include "TimeManager.h"
#include "ApproachDetector.h"
#include "CurvatureGuard.h"
#include "TTS.h"
#include "CommonTasks.h"
#ifdef DEBUG_TTS
#include "DebugTools.h"
#endif

void CurvatureGuard::guard(MeshManager &meshManager,
                           const FlowManager &flowManager,
                           PolygonManager &polygonManager)
{
    bool flag = false;
    
    // -------------------------------------------------------------------------
    // some operations at the first step
    if (TimeManager::isFirstStep()) {
        // check the location of each edge's test point
        Edge *edge = polygonManager.edges.front();
        for (int i = 0; i < polygonManager.edges.size(); ++i) {
            Vertex *testPoint = edge->getTestPoint();
            Location loc;
            meshManager.checkLocation(testPoint->getCoordinate(), loc);
            testPoint->setLocation(loc);
            edge = edge->next;
        }
    }
    
    // -------------------------------------------------------------------------
    // advect test points
    Edge *edge = polygonManager.edges.front();
    for (int i = 0; i < polygonManager.edges.size(); ++i) {
        TTS::track(meshManager, flowManager, edge->getTestPoint());
        edge = edge->next;
    }
#ifdef DEBUG_TTS
    DebugTools::dump_watchers();
#endif

    // -------------------------------------------------------------------------
    if (splitEdges(meshManager, flowManager, polygonManager)) flag = true;
#ifdef DEBUG_TTS
    DebugTools::dump_watchers();
#endif

    // -------------------------------------------------------------------------
    ApproachDetector::detectPolygons(meshManager, flowManager, polygonManager);
#ifdef DEBUG_TTS
    DebugTools::dump_watchers();
#endif

    // -------------------------------------------------------------------------
    if (mergeEdges(meshManager, flowManager, polygonManager)) flag = true;
#ifdef DEBUG_TTS
    DebugTools::dump_watchers();
#endif

    // -------------------------------------------------------------------------
    ApproachDetector::detectPolygons(meshManager, flowManager, polygonManager);
#ifdef DEBUG_TTS
    DebugTools::dump_watchers();
#endif

    // -------------------------------------------------------------------------
    if (splitPolygons(meshManager, flowManager, polygonManager)) flag = true;
#ifdef DEBUG_TTS
    DebugTools::dump_watchers();
#endif

    // -------------------------------------------------------------------------
    ApproachDetector::reset(polygonManager);

    CommonTasks::resetTasks();

#ifndef DEBUG_TTS
    // -------------------------------------------------------------------------
    // reindex the vertices and edges for outputting
    if (flag) {
        polygonManager.vertices.reindex();
        polygonManager.edges.reindex();
    }
#endif
}

#endif
