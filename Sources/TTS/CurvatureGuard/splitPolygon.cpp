#ifndef splitPolygon_h
#define splitPolygon_h

/*
 * splitPolygon
 *
 * Description:
 *   Due to the deformation caused by the background flow, the polygons of
 *   tracer will be deformed severely at some time. The computation may even be
 *   disrupted by the wild tangling of edges. Additionally, the incrementally
 *   inserted edges will cost huge amount of memory, which will be impractical
 *   for real applications. Therefore, we should detect the situations when
 *   problems will occur, and fix them.
 *
 *   Potential situations:
 *   (1) Some vertices approach some edges and may across them;
 *
 */

#include "CurvatureGuard.h"
#include "TTS.h"
#include "MeshManager.h"
#include "FlowManager.h"
#include "PolygonManager.h"
#include "ApproachDetector.h"
#include "ApproachingVertices.h"
#include "PotentialCrossDetector.h"
#include "SpecialPolygons.h"
#ifdef DEBUG
#include "DebugTools.h"
#endif

using namespace ApproachDetector;
using namespace PotentialCrossDetector;
using namespace SpecialPolygons;

bool CurvatureGuard::splitPolygon(MeshManager &meshManager,
                                  const FlowManager &flowManager,
                                  PolygonManager &polygonManager)
{
    bool isSplit = false;
    // major internal variables
    // vertex3 is the vertex which is approaching to edge1
    // vertex1 and vertex2 are the end points of the edge1
    // polygon1 is the polygon which will be split
    // polygon2 is the neighbor of polygon1 at edge1
    // polygon3 is the new polygon which is split from polygon1
    Vertex *vertex1, *vertex2, *vertex3;
    Edge *edge1;
    EdgePointer *edgePointer1, *edgePointer2, *edgePointer3;
    Polygon *polygon1, *polygon2, *polygon3, *polygon4;
    // other internal variables
    int i, j;
    EdgePointer *edgePointer, *linkedEdge;
    Projection *projection;
    Vertex *prevVertex = NULL, *nextVertex, vertex;

    TTS::resetTasks();

    while (!ApproachingVertices::isEmpty()) {
        vertex3 = ApproachingVertices::vertices.front();
        // ---------------------------------------------------------------------
        // if the vertex3 is a test point, split its edge
        if (vertex3->getID() == -1) {
            if (splitEdge(meshManager, flowManager, polygonManager,
                          vertex3->getHostEdge(), true))
                vertex3 = polygonManager.vertices.back();
            else {
                projection = vertex3->detectAgent.getActiveProjection();
                projection->setApproach(false);
                if (vertex3->detectAgent.getActiveProjection() == NULL)
                    ApproachingVertices::removeVertex(vertex3);
                continue;
            }
        }
        // ---------------------------------------------------------------------
        projection = vertex3->detectAgent.getActiveProjection();
        // Note: The approaching status of vertex may change during splitPolygon.
        if (projection == NULL) {
            ApproachingVertices::removeVertex(vertex3);
            continue;
        }
        // ---------------------------------------------------------------------
        edge1 = projection->getEdge();
        // ---------------------------------------------------------------------
        // check if there is another approaching vertex whose projection
        // distance is smaller than vertex3
        bool hasAnotherVertex = false;
        list<Vertex *>::const_iterator it = edge1->detectAgent.vertices.begin();
        for (; it != edge1->detectAgent.vertices.end(); ++it) {
            Projection *projection1 = (*it)->detectAgent.getProjection(edge1);
            if (projection1->isApproaching())
                if (projection1->getDistance(NewTimeLevel) <
                    projection->getDistance(NewTimeLevel)) {
                    ApproachingVertices::jumpVertex(vertex3, *it);
                    hasAnotherVertex = true;
                    break;
                }
        }
        if (hasAnotherVertex) continue;
        // ---------------------------------------------------------------------
        edgePointer2 = NULL;
        switch (projection->getOrient()) {
            case OrientLeft:
                polygon1 = edge1->getPolygon(OrientLeft);
                polygon2 = edge1->getPolygon(OrientRight);
                edgePointer1 = edge1->getEdgePointer(OrientLeft);
                break;
            case OrientRight:
                polygon1 = edge1->getPolygon(OrientRight);
                polygon2 = edge1->getPolygon(OrientLeft);
                edgePointer1 = edge1->getEdgePointer(OrientRight);
                break;
            default:
                REPORT_ERROR("Unknown orientation!");
        }
        linkedEdge = vertex3->linkedEdges.front();
        for (i = 0; i < vertex3->linkedEdges.size(); ++i) {
            if (linkedEdge->edge->getPolygon(OrientLeft) == polygon1 &&
                linkedEdge->edge->getEndPoint(SecondPoint) == vertex3) {
                edgePointer2 = linkedEdge->edge->getEdgePointer(OrientLeft);
                break;
            } else if (linkedEdge->edge->getPolygon(OrientRight) == polygon1 &&
                       linkedEdge->edge->getEndPoint(FirstPoint) == vertex3) {
                edgePointer2 = linkedEdge->edge->getEdgePointer(OrientRight);
                break;
            }
            linkedEdge = linkedEdge->next;
        }
#ifdef DEBUG
        if (edgePointer2 == NULL) {
            polygon1->dump("polygon");
        }
        if (edgePointer2 == NULL) {
            // Note: There are possibilities that vertex3 is not a vertex of
            //       polygon1, so we should ignore vertex3.
            bool isReallyCross = false;
            linkedEdge = vertex3->linkedEdges.front();
            for (i = 0; i < vertex3->linkedEdges.size(); ++i) {
                if (linkedEdge->edge->getPolygon(OrientLeft) == polygon1 ||
                    linkedEdge->edge->getPolygon(OrientRight) == polygon1) {
                    isReallyCross = true;
                    break;
                }
                linkedEdge = linkedEdge->next;
            }
            if (!isReallyCross) {
                ApproachDetector::AgentPair::unpair(vertex3, edge1);
                if (vertex3->detectAgent.getActiveProjection() == NULL)
                    ApproachingVertices::recordVertex(vertex3);
                continue;
            }
            Message message;
            message << "Edge-crossing event has occurred!\n";
            message << "polygon1: " << polygon1->getID() << "\n";
            message << "polygon2: " << polygon2->getID() << "\n";
            message << "edge1: " << edge1->getID() << "\n";
            message << "vertex3: " << vertex3->getID() << "\n";
            REPORT_ERROR(message.str());
        }
#endif
        vertex1 = edgePointer1->getEndPoint(FirstPoint);
        vertex2 = edgePointer1->getEndPoint(SecondPoint);
#ifdef DEBUG
        if (edgePointer2->getPolygon(OrientLeft) != polygon1) {
            edgePointer2->getPolygon(OrientLeft)->dump("polygon");
            ostringstream message;
            message << "Edge-crossing event has occurred at polygon ";
            message << edgePointer2->getPolygon(OrientLeft)->getID() << "!" << endl;
            REPORT_ERROR(message.str())
        }
#endif
        // ---------------------------------------------------------------------
#ifdef DEBUG
        SEPERATOR
        ApproachingVertices::dump();
        cout << "***** current approaching vertex ID: ";
        cout << vertex3->getID() << endl;
        cout << "      Polygon ID: "<< polygon1->getID() << endl;
        cout << "      Paired edge ID: " << edge1->getID() << endl;
        cout << "      End point IDs: ";
        cout << setw(8) << edge1->getEndPoint(FirstPoint)->getID();
        cout << setw(8) << edge1->getEndPoint(SecondPoint)->getID();
        cout << endl;
        cout << "      Linked edge ID: ";
        cout << setw(8) << edgePointer2->edge->getID() << endl;
        cout << "      End point IDs: ";
        cout << setw(8) << edgePointer2->getEndPoint(FirstPoint)->getID();
        cout << setw(8) << edgePointer2->getEndPoint(SecondPoint)->getID();
        cout << endl;
        cout << "      Distance: ";
        cout << projection->getDistance(NewTimeLevel) << endl;
        cout << "      Orientation: ";
        if (projection->getOrient() == OrientLeft)
            cout << "left" << endl;
        else if (projection->getOrient() == OrientRight)
            cout << "right" << endl;
        polygon1->dump("polygon");
        DebugTools::assert_consistent_projection(projection);
        assert(edgePointer2->getEndPoint(SecondPoint) == vertex3);
        if (TimeManager::getSteps() == 70 && vertex3->getID() == 249310)
            REPORT_DEBUG;
#endif
        isSplit = true;
        // ---------------------------------------------------------------------
        int mode = ApproachDetector::chooseMode(vertex1, vertex2, vertex3,
                                                projection);
        if (mode == -1) {
            ApproachDetector::AgentPair::unpair(vertex3, edge1);
            if (vertex3->detectAgent.getActiveProjection() == NULL)
                ApproachingVertices::removeVertex(vertex3);
            continue;
        }
        // ---------------------------------------------------------------------
        Vertex *testVertex;
        Location loc;
        OrientStatus orient;
        if (mode == 1) {
            testVertex = vertex1;
        } else if (mode == 2) {
            testVertex = vertex2;
        } else if (mode >= 3) {
            testVertex = &vertex;
            testVertex->setCoordinate(projection->getCoordinate(OldTimeLevel));
            meshManager.checkLocation(testVertex->getCoordinate(), loc);
            testVertex->setLocation(loc);
            TTS::track(meshManager, flowManager, testVertex);
        }
        // ---------------------------------------------------------------------
        if (mode != 3)
            if (detect1(vertex3, testVertex) != NoCross) {
                // when mode is 1/2/4, vertex3 will be eliminated, and this
                // may cause edge-crossing. If this happens, shift to mode 5
                // by making testVertex be vertex3.
                testVertex = vertex3; mode = 5;
            }
        if (mode > 2 && mode != 5)
            if (detect2(testVertex, edge1, polygon2) != NoCross) {
                testVertex->setCoordinate
                (projection->getCoordinate(NewTimeLevel), NewTimeLevel);
                testVertex->setCoordinate
                (projection->getCoordinate(OldTimeLevel), OldTimeLevel);
                meshManager.checkLocation(testVertex->getCoordinate(), loc);
                testVertex->setLocation(loc);
            }
        // ---------------------------------------------------------------------
        ApproachDetector::AgentPair::unpair(vertex3, edge1);
        if (vertex3->detectAgent.getActiveProjection() == NULL)
            ApproachingVertices::removeVertex(vertex3);
        // ---------------------------------------------------------------------
        // splitPolygon may affect the neightbor polygon, so record it for later
        // processing
        polygon4 = NULL;
        if (mode == 1 && edgePointer1 == edgePointer2->next->next)
            polygon4 = edgePointer2->next->getPolygon(OrientRight);
        else if (mode == 2 && edgePointer1 == edgePointer2->prev)
            polygon4 = edgePointer2->getPolygon(OrientRight);
        // ---------------------------------------------------------------------
        int numTracer = static_cast<int>(polygon1->tracers.size());
        double mass[numTracer];
        for (i = 0; i < numTracer; ++i)
            mass[i] = polygon1->tracers[i].getMass();
        // polygon1 and new polygon3 may be degenerated simultaneously, the mass
        // of original polygon1 should be assigned to its neighbors
        int numNeighbor = polygon1->edgePointers.size();
        Polygon *neighbors[numNeighbor];
        edgePointer = polygon1->edgePointers.front();
        if (polygon4 == NULL)
            for (i = 0; i < numNeighbor; ++i) {
                neighbors[i] = edgePointer->getPolygon(OrientRight);
                edgePointer = edgePointer->next;
            }
        else {
            // put polygon4 into the last neighbor
            j = 0;
            for (i = 0; i < numNeighbor; ++i) {
                if (edgePointer->getPolygon(OrientRight) != polygon4)
                    neighbors[j++] = edgePointer->getPolygon(OrientRight);
                edgePointer = edgePointer->next;
            }
            neighbors[j] = polygon4;
        }
        // ---------------------------------------------------------------------
        // create a new polygon
        polygonManager.polygons.append(&polygon3);
        Vertex *newVertex;
        edgePointer3 = edgePointer2->next;
        EdgePointer *endEdgePointer;
        if (mode == 1) {
            endEdgePointer = edgePointer1;
            newVertex = vertex1;
        } else if (mode == 2) {
            endEdgePointer = edgePointer1->next;
            newVertex = vertex2;
        } else {
            endEdgePointer = edgePointer1;
        }
        while (edgePointer3 != endEdgePointer) {
            EdgePointer *edgePointer;
            polygon3->edgePointers.append(&edgePointer);
            edgePointer->replace(edgePointer3);
            edgePointer->edge->setPolygon(edgePointer->orient, polygon3);
            edgePointer3 = edgePointer3->next;
            polygon1->edgePointers.remove(edgePointer3->prev);
        }
        // ---------------------------------------------------------------------
        Edge *newEdge1 = NULL;
        // check if the projection of vertex on the
        // edge 1 is too close to vertex 1 or 2
        // if true, just use the 1 or 2
        // if not, use the new one
        if (mode < 3) {
            polygon3->edgePointers.ring();
        } else {
            polygon3->edgePointers.append(&edgePointer3);
            polygon3->edgePointers.ring();
            // create a new vertex on the edge 1
            if (mode != 5) {
                polygonManager.vertices.append(&newVertex);
                *newVertex = *testVertex;
            } else
                newVertex = testVertex;
            // split the edge pointed by edge 1
            polygonManager.edges.append(&newEdge1);
            newEdge1->linkEndPoint(FirstPoint, vertex1);
            newEdge1->linkEndPoint(SecondPoint, newVertex);
            newEdge1->setPolygon(OrientLeft, polygon3);
            newEdge1->setEdgePointer(OrientLeft, edgePointer3);
            newEdge1->setPolygon(OrientRight, polygon2);
            EdgePointer *edgePointer5, *edgePointer6;
            if (edgePointer1->orient == OrientLeft) {
                edgePointer5 = edgePointer1->edge->getEdgePointer(OrientRight);
            } else {
                edgePointer5 = edgePointer1->edge->getEdgePointer(OrientLeft);
            }
            polygon2->edgePointers.insert(edgePointer5, &edgePointer6);
            newEdge1->setEdgePointer(OrientRight, edgePointer6);
            newEdge1->calcNormVector();
            newEdge1->calcLength();
            Vertex *testPoint = newEdge1->getTestPoint();
            meshManager.checkLocation(testPoint->getCoordinate(), loc);
            testPoint->setLocation(loc);
            TTS::track(meshManager, flowManager, testPoint);
        }
        // ---------------------------------------------------------------------
        // judge whether we need to add another new edge to connect
        // the new vertex and vertex
        Edge *newEdge2 = NULL;
        if (mode == 3) {
            polygonManager.edges.append(&newEdge2);
            newEdge2->linkEndPoint(FirstPoint, vertex3);
            newEdge2->linkEndPoint(SecondPoint, newVertex);
            newEdge2->setPolygon(OrientLeft, polygon1);
            EdgePointer *edgePointer7;
            polygon1->edgePointers.insert(edgePointer2, &edgePointer7);
            newEdge2->setEdgePointer(OrientLeft, edgePointer7);
            newEdge2->setPolygon(OrientRight, polygon3);
            EdgePointer *edgePointer8;
            polygon3->edgePointers.insert(edgePointer3, &edgePointer8);
            newEdge2->setEdgePointer(OrientRight, edgePointer8);
            newEdge2->calcNormVector();
            newEdge2->calcLength();
            Vertex *testPoint = newEdge2->getTestPoint();
            Location loc;
            meshManager.checkLocation(testPoint->getCoordinate(), loc);
            testPoint->setLocation(loc);
            TTS::track(meshManager, flowManager, testPoint);
        }
        // ---------------------------------------------------------------------
        // modify the old edges
        if (newEdge1 != NULL) {
            if (edgePointer1->orient == OrientLeft) {
                edge1->changeEndPoint(FirstPoint, newVertex,
                                      meshManager, flowManager);
            } else {
                edge1->changeEndPoint(SecondPoint, newVertex,
                                      meshManager, flowManager);
            }
            if (newEdge1 != NULL && polygon3 != NULL)
                edge1->detectAgent.handoverVertices(newEdge1);
            edge1->detectAgent.updateVertexProjections();
        }
        if (newEdge2 == NULL) {
            if (mode != 5) {
                vertex3->handoverEdges(newVertex, meshManager,
                                       flowManager, polygonManager);
                polygonManager.vertices.remove(vertex3);
                vertex3 = NULL;
            }
        } else
            vertex3->detectAgent.clean();
        // ---------------------------------------------------------------------
        // handle degenerate polygons
        if (polygon1 != NULL && polygon1->edgePointers.size() == 1) {
            handlePointPolygon(polygonManager, polygon1);
            polygon1 = NULL;
        }
        if (polygon3 != NULL && polygon3->edgePointers.size() == 1) {
            handlePointPolygon(polygonManager, polygon3);
            polygon3 = NULL;
        }
        if (polygon1 != NULL && polygon1->edgePointers.size() == 2) {
            handleLinePolygon(polygonManager, polygon1);
            polygon1 = NULL;
        }
        if (polygon3 != NULL && polygon3->edgePointers.size() == 2) {
            handleLinePolygon(polygonManager, polygon3);
            polygon3 = NULL;
        }
        if (polygon1 != NULL)
            handleSlimPolygon(meshManager, flowManager, polygonManager, polygon1);
        if (polygon3 != NULL)
            handleSlimPolygon(meshManager, flowManager, polygonManager, polygon3);
        if (polygon4 != NULL)
            if (polygon4->edgePointers.size() == 2) {
                Polygon *polygon41 = polygon4->edgePointers.front()->getPolygon(OrientRight);
                Polygon *polygon42 = polygon4->edgePointers.back()->getPolygon(OrientRight);
                for (i = 0; i < numTracer; ++i) {
                    double mass = polygon4->tracers[i].getMass()*0.5;
                    polygon41->tracers[i].addMass(mass);
                    polygon42->tracers[i].addMass(mass);
                }
                numNeighbor--;
                handleLinePolygon(polygonManager, polygon4);
            }
        // ---------------------------------------------------------------------
        TTS::doTask(TTS::UpdateAngle);
        // ---------------------------------------------------------------------
        // assign mass to new parcels or neighbors
        if (polygon1 != NULL && polygon3 != NULL) {
            polygon1->calcArea(); polygon3->calcArea();
            double totalArea = polygon1->getArea()+polygon3->getArea();
            double weight1 = polygon1->getArea()/totalArea;
            double weight3 = polygon3->getArea()/totalArea;
            polygon3->tracers.resize(numTracer);
            for (i = 0; i < numTracer; ++i) {
                polygon1->tracers[i].setMass(mass[i]*weight1);
                polygon3->tracers[i].setMass(mass[i]*weight3);
            }
        } else if (polygon1 == NULL && polygon3 != NULL) {
            polygon3->tracers.resize(numTracer);
            for (i = 0; i < numTracer; ++i)
                polygon3->tracers[i].setMass(mass[i]);
        } else if (polygon1 == NULL && polygon3 == NULL) {
            for (i = 0; i < numTracer; ++i)
                mass[i] /= numNeighbor;
            for (j = 0; j < numNeighbor; ++j)
                for (i = 0; i < numTracer; ++i)
                    neighbors[j]->tracers[i].addMass(mass[i]);
        }
#ifdef DEBUG
        //        static double targetMass = 10.9682466106844085379;
        //        double totalMass = 0.0;
        //        polygon2 = polygonManager.polygons.front();
        //        for (i = 0; i < polygonManager.polygons.size(); ++i) {
        //            totalMass += polygon2->tracers[2].getMass();
        //            polygon2 = polygon2->next;
        //        }
        //        if (fabs(totalMass-targetMass) > 1.0e-9)
        //            REPORT_DEBUG;
#endif
        // ---------------------------------------------------------------------
        // detect the new vertex for approaching
        if (mode >= 3) {
            linkedEdge = newVertex->linkedEdges.front();
            for (i = 0; i < newVertex->linkedEdges.size(); ++i) {
                Edge *edge = linkedEdge->edge;
                if (edge->getEndPoint(FirstPoint) == newVertex)
                    orient = OrientLeft;
                else if (edge->getEndPoint(SecondPoint) == newVertex)
                    orient = OrientRight;
#ifdef DEBUG
                cout << "Detecting polygon ";
                cout << edge->getPolygon(orient)->getID() << endl;
#endif
                ApproachDetector::detect(meshManager, flowManager,
                                         polygonManager,
                                         edge->getPolygon(orient));
                linkedEdge = linkedEdge->next;
            }
#ifdef DEBUG
            cout << "New vertex " << newVertex->getID() << " is linked to ";
            cout << newVertex->linkedEdges.size() << " edges." << endl;
            REPORT_DEBUG
#endif
        }
        // ---------------------------------------------------------------------
        // handle dead loop
        if (prevVertex == NULL) prevVertex = vertex3;
        if (ApproachingVertices::vertices.size() != 0) {
            nextVertex = ApproachingVertices::vertices.front();
            if (prevVertex == nextVertex) {
                nextVertex->detectAgent.getActiveProjection()->setApproach(false);
                if (nextVertex->detectAgent.getActiveProjection() != NULL)
                    ApproachingVertices::removeVertex(nextVertex);
                REPORT_DEBUG;
            }
        }
        prevVertex = vertex3;
    }
    return isSplit;
}

#endif