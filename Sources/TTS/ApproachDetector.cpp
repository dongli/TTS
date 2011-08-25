#include "ApproachDetector.h"
#include "Constants.h"
#include "Edge.h"
#include "Vertex.h"
#include "MeshManager.h"
#include "FlowManager.h"
#include "PolygonManager.h"
#ifdef DEBUG
#include "DebugTools.h"
#endif

using namespace ApproachDetector;

VertexAgent::VertexAgent()
{
    host = NULL;
    isApproaching = false;
    reinit();
}

VertexAgent::~VertexAgent()
{
}

void VertexAgent::checkin(Vertex *vertex)
{
    host = vertex;
}

void VertexAgent::reinit()
{
    assert(isApproaching == false);
    edge = NULL;
    timeLevel = OldTimeLevel;
    distance = -999.0;
}

void VertexAgent::clean()
{
    if (edge != NULL) {
        edge->detectAgent.remove(host);
        reinit();
    }
}

// -----------------------------------------------------------------------------

EdgeAgent::EdgeAgent()
{
    host = NULL;
    reinit();
}

EdgeAgent::~EdgeAgent()
{
}

void EdgeAgent::checkin(Edge *edge)
{
    host = edge;
}

void EdgeAgent::reinit()
{
}

void EdgeAgent::clean()
{
    VertexPointer *vertexPointer = vertexPointers.front();
    VertexPointer *nextVertexPointer;
    while (vertexPointer != NULL) {
        nextVertexPointer = vertexPointer->next;
        AgentPairs::unpair(vertexPointer->vertex, host);
        vertexPointer = nextVertexPointer;
    }
}

void EdgeAgent::record(Vertex *vertex)
{
    vertexPointers.append();
    vertexPointers.back()->vertex = vertex;
}

void EdgeAgent::remove(Vertex *vertex)
{
    VertexPointer *vertexPointer = vertexPointers.front();
    for (int i = 0; i < vertexPointers.size(); ++i) {
        if (vertexPointer->vertex == vertex) {
            vertexPointers.remove(vertexPointer);
            return;
        }
        vertexPointer = vertexPointer->next;
    }
    REPORT_ERROR("Unfound vertex in approaching vertex list!")
}

void EdgeAgent::handover(Edge *edge)
{
    if (edge == host)
        return;
    Vertex *vertex1 = edge->getEndPoint(FirstPoint);
    Vertex *vertex2 = edge->getEndPoint(SecondPoint);
    const Coordinate &x1n = vertex1->getCoordinate();
    const Coordinate &x2n = vertex2->getCoordinate();
    VertexPointer *vertexPointer = vertexPointers.front();
    VertexPointer *nextVertexPointer;
    while (vertexPointer != NULL) {
        nextVertexPointer = vertexPointer->next;
        Vertex *vertex3 = vertexPointer->vertex;
        if (vertex3 == vertex1 || vertex3 == vertex2) {
            vertexPointer = vertexPointer->next;
            continue;
        }
        const Coordinate &x3n = vertex3->getCoordinate();
        Coordinate x4n;
        double distn;
        if (Sphere::project(x1n, x2n, x3n, x4n, distn)) {
            const Coordinate &x1o = vertex1->getCoordinate(OldTimeLevel);
            const Coordinate &x2o = vertex2->getCoordinate(OldTimeLevel);
            const Coordinate &x3o = vertex3->getCoordinate(OldTimeLevel);
            Coordinate x4o;
            double disto;
            if (Sphere::project(x1o, x2o, x3o, x4o, disto)) {
                OrientStatus orientn = Sphere::orient(x1n, x2n, x3n);
                OrientStatus oriento = Sphere::orient(x1o, x2o, x3o);
#ifdef DEBUG
                assert(orientn != OrientOn);
#endif
                if (orientn == oriento) {
                    if (vertex3->detectAgent.isApproaching)
                        ApproachingVertices::remove(vertex3);
                    AgentPairs::unpair(vertex3, host);
                    if (isApproaching(disto, distn)) {
                        AgentPairs::pair(vertex3, edge, x4o, disto, oriento);
                        ApproachingVertices::record(vertex3);
                        // explain the following line!
                        vertex3->detectAgent.edgePointer =
                        edge->getEdgePointer(orientn);
                    } else {
                        AgentPairs::pair(vertex3, edge, x4n, distn, orientn);
                    }
                } else {
                    REPORT_ERROR("Edge crossing has occurred!")
                }
            }
        }
        vertexPointer = nextVertexPointer;
    }
}

void EdgeAgent::update()
{
    Vertex *vertex1 = host->getEndPoint(FirstPoint);
    Vertex *vertex2 = host->getEndPoint(SecondPoint);
    const Coordinate &x1n = vertex1->getCoordinate();
    const Coordinate &x2n = vertex2->getCoordinate();
    VertexPointer *vertexPointer = vertexPointers.front();
    VertexPointer *nextVertexPointer;
    while (vertexPointer != NULL) {
        nextVertexPointer = vertexPointer->next;
        Vertex *vertex3 = vertexPointer->vertex;
#ifdef DEBUG
        assert(vertex3->detectAgent.edge == host);
#endif
        const Coordinate &x3n = vertex3->getCoordinate();
        OrientStatus orientn = Sphere::orient(x1n, x2n, x3n);
        if (orientn == OrientOn) {
            assert(vertex3 == vertex1 || vertex3 == vertex2);
            vertexPointers.remove(vertexPointer);
            if (vertex3->detectAgent.edge == host)
                ApproachingVertices::remove(vertex3);
            vertex3->detectAgent.reinit();
        } else if (orientn != vertex3->detectAgent.orient) {
            REPORT_ERROR("Edge crossing has occured!")
        } else {
            Coordinate x4n;
            double distn;
            if (Sphere::project(x1n, x2n, x3n, x4n, distn)) {
                const Coordinate &x1o = vertex1->getCoordinate(OldTimeLevel);
                const Coordinate &x2o = vertex2->getCoordinate(OldTimeLevel);
                const Coordinate &x3o = vertex3->getCoordinate(OldTimeLevel);
                Coordinate x4o;
                double disto;
                if (Sphere::project(x1o, x2o, x3o, x4o, disto)) {
                    if (isApproaching(disto, distn)) {
                        vertex3->detectAgent.projection = x4o;
                        vertex3->detectAgent.distance = disto;
                        if (!vertex3->detectAgent.isApproaching)
                            ApproachingVertices::record(vertex3);
                        vertex3->detectAgent.edgePointer =
                        host->getEdgePointer(orientn);
                    } else {
                        vertex3->detectAgent.projection = x4n;
                        vertex3->detectAgent.distance = distn;
                    }
                }
            } else {
                AgentPairs::unpair(vertex3, host);
                REPORT_DEBUG
            }
        }
        vertexPointer = nextVertexPointer;
    }
}

// -----------------------------------------------------------------------------

void AgentPairs::pair(Vertex *vertex, Edge *edge,
                      const Coordinate &projection,
                      double distance, OrientStatus orient)
{
    /*if (vertex->getID() == 339732) {
        DebugTools::watch_vertex(vertex);
        cout << "***** Vertex " << vertex->getID() << " is paired with edge ";
        cout << edge->getID() << "." << endl;
        REPORT_DEBUG
    }*/
#ifdef DEBUG
    assert(vertex->detectAgent.edge == NULL);
    assert(vertex->detectAgent.isApproaching == false);
#endif
    vertex->detectAgent.edge = edge;
    vertex->detectAgent.projection = projection;
    vertex->detectAgent.distance = distance;
    vertex->detectAgent.orient = orient;
    edge->detectAgent.record(vertex);
}

void AgentPairs::unpair(Vertex *vertex, Edge *edge)
{
    /*if (vertex->getID() == 339732) {
        cout << "***** Vertex " << vertex->getID() << " is unpaired with edge ";
        cout << edge->getID() << "." << endl;
        REPORT_DEBUG
    }*/
#ifdef DEBUG
    assert(vertex->detectAgent.edge == edge);
#endif
    vertex->detectAgent.reinit();
    edge->detectAgent.remove(vertex);
}

// -----------------------------------------------------------------------------

List<VertexPointer> ApproachingVertices::vertexPointers;

ApproachingVertices::ApproachingVertices()
{
}

ApproachingVertices::~ApproachingVertices()
{
}

void ApproachingVertices::record(Vertex *vertex)
{
#ifdef DEBUG
    assert(vertex->detectAgent.isApproaching != true);
    VertexPointer *vertexPointer = vertexPointers.front();
    for (int i = 0; i < vertexPointers.size(); ++i) {
        assert(vertexPointer->vertex != vertex);
        vertexPointer = vertexPointer->next;
    }
#endif
    vertexPointers.append();
    vertexPointers.back()->vertex = vertex;
    vertex->detectAgent.isApproaching = true;
}

void ApproachingVertices::record(Vertex *vertex1, Vertex *vertex2)
{
    
#ifdef DEBUG
    assert(vertex1->detectAgent.isApproaching != true);
#endif
    VertexPointer *vertexPointer = vertexPointers.front();
    for (int i = 0; i < vertexPointers.size(); ++i) {
#ifdef DEBUG
        assert(vertexPointer->vertex != vertex1);
#endif
        if (vertexPointer->vertex == vertex2) {
            VertexPointer *newVertexPointer;
            vertexPointers.insert(&newVertexPointer, vertexPointer);
            newVertexPointer->vertex = vertex1;
            vertex1->detectAgent.isApproaching = true;
            return;
        }
        vertexPointer = vertexPointer->next;
    }
}

void ApproachingVertices::remove(Vertex *vertex)
{
#ifdef DEBUG
    assert(vertex->detectAgent.isApproaching == true);
#endif
    VertexPointer *vertexPointer = vertexPointers.front();
    for (int i = 0; i < vertexPointers.size(); ++i) {
        if (vertexPointer->vertex == vertex) {
            vertexPointers.remove(vertexPointer);
            vertex->detectAgent.isApproaching = false;
            return;
        }
        vertexPointer = vertexPointer->next;
    }
    REPORT_ERROR("Unfound vertex in approaching vertex list!")
}

bool ApproachingVertices::isEmpty()
{
    return vertexPointers.size() == 0 ? true : false;
}

void ApproachingVertices::dump()
{
    cout << "***** Approaching vertices list:" << endl;
    cout << "      Number: " << vertexPointers.size() << endl;
    cout << "      Vertex IDs: ";
    VertexPointer *vertexPointer = vertexPointers.front();
    for (int i = 0; i < vertexPointers.size(); ++i) {
        cout << setw(8) << vertexPointer->vertex->getID();
        vertexPointer = vertexPointer->next;
    }
    cout << endl;
}

// -----------------------------------------------------------------------------

bool ApproachDetector::isActive(double distance)
{
    static const double distanceThreshold = 0.5/Rad2Deg*Sphere::radius;
    if (distance < distanceThreshold)
        return true;
    else
        return false;
}

bool ApproachDetector::isApproaching(double oldDistance, double newDistance)
{
    static const double smallDistance = 0.02/Rad2Deg*Sphere::radius;
    double ratio = (oldDistance-newDistance)/oldDistance;
    double ratio0 = approachTrendThreshold(oldDistance);
    if (ratio < ratio0 && newDistance > smallDistance)
        return false;
    else
        return true;
}

double ApproachDetector::approachTrendThreshold(double distance)
{
    static const double D0 = 0.005/Rad2Deg*Sphere::radius;
    static const double D1 = 0.1/Rad2Deg*Sphere::radius;
    static const double P0 = 0.3;
    static const double P1 = 0.8;
    static const double dD = D1-D0;
    static const double dP = P1-P0;

    if (distance > D0 && distance < D1) {
        double t = (distance-D0)/dD;
        return dP*(4.0-3.0*t)*pow(t, 3.0)+P0;
    } else if (distance <= D0) {
        return P0;
    } else {
        return P1;
    }
}

inline void projectOnOldEdge(Vertex *vertex1, Vertex *vertex2, Vertex *vertex3,
                             Coordinate &x4, double &distance)
{
    const Coordinate &x1o = vertex1->getCoordinate(OldTimeLevel);
    const Coordinate &x2o = vertex2->getCoordinate(OldTimeLevel);
    const Coordinate &x3o = vertex3->getCoordinate(OldTimeLevel);
    if (!Sphere::project(x1o, x2o, x3o, x4, distance)) {
        REPORT_ERROR("Vertex is not projected onto the edge at old time step!")
    }
}

inline void handleSmallAngle(Edge *edge1, Vertex *vertex3,
                             EdgePointer *edgePointer1,
                             EdgePointer *edgePointer2, Polygon *polygon)
{
    static const double smallAngle = 5.0/Rad2Deg;
    if (edgePointer1->getAngle() < smallAngle ||
        edgePointer1->next->getAngle() < smallAngle) {
        VertexPointer *vertexPointer = edge1->detectAgent.vertexPointers.front();
        for (int i = 0; i < edge1->detectAgent.vertexPointers.size(); ++i) {
            if (vertexPointer->vertex->detectAgent.isApproaching &&
                vertex3->detectAgent.orient != vertexPointer->vertex->detectAgent.orient) {
                polygon->dump("polygon");
                vertex3->detectAgent.timeLevel = NewTimeLevel;
                ApproachingVertices::dump();
                ApproachingVertices::record(vertex3, vertexPointer->vertex);
                vertex3->detectAgent.edgePointer = edgePointer2;
                ApproachingVertices::dump();
                return;
            }
            vertexPointer = vertexPointer->next;
        }
    }
}

inline void ApproachDetector::detect(MeshManager &meshManager,
                                     const FlowManager &flowManager,
                                     Polygon *polygon)
{
    static const double smallDistance = 0.02/Rad2Deg*Sphere::radius;

    if (TimeManager::getSteps() >= 24 && polygon->getID() == 7529) {
        polygon->dump("polygon");
        REPORT_DEBUG
    }

    EdgePointer *edgePointer1 = polygon->edgePointers.front();
    for (int j = 0; j < polygon->edgePointers.size(); ++j) {
        Edge *edge1 = edgePointer1->edge;
        Vertex *vertex1 = edgePointer1->getEndPoint(FirstPoint);
        Vertex *vertex2 = edgePointer1->getEndPoint(SecondPoint);
        const Coordinate &x1 = vertex1->getCoordinate();
        const Coordinate &x2 = vertex2->getCoordinate();
        // use the PointCounter mesh as the first filter
        int I1, I2, J1, J2;
        if (vertex1->getLocation().i[4] <= vertex2->getLocation().i[4]) {
            I1 = vertex1->getLocation().i[4]-1;
            I2 = vertex2->getLocation().i[4]+1;
        } else {
            I1 = vertex2->getLocation().i[4]-1;
            I2 = vertex1->getLocation().i[4]+1;
        }
        if (vertex1->getLocation().j[4] <= vertex2->getLocation().j[4]) {
            J1 = vertex1->getLocation().j[4]-1;
            J2 = vertex2->getLocation().j[4]+1;
        } else {
            J1 = vertex2->getLocation().j[4]-1;
            J2 = vertex1->getLocation().j[4]+1;
        }
        EdgePointer *edgePointer2 = edgePointer1->next;
        for (int k = 0; k < polygon->edgePointers.size()-2; ++k) {
            Vertex *vertex3 = edgePointer2->getEndPoint(SecondPoint);
            if (vertex3->detectAgent.isApproaching) {
                if (vertex3->detectAgent.edgePointer == edgePointer1) {
                    vertex3->detectAgent.edgePointer = edgePointer2;
                }
                edgePointer2 = edgePointer2->next;
                continue;
            }
            const Location &loc = vertex3->getLocation();
            // *** filter 1
            if ((loc.i[4] >= I1 && loc.i[4] <= I2) &&
                (loc.j[4] >= J1 && loc.j[4] <= J2)) {
                const Coordinate &x3 = vertex3->getCoordinate(NewTimeLevel);
                Coordinate x4;
                double distance;
                // *** filter 2
                // Note: This is a costly operation!
                if (Sphere::project(x1, x2, x3, x4, distance)) {
                    // *** filter 3
                    if (isActive(distance)) {
                        OrientStatus orient;
                        if (edgePointer1->orient == OrientLeft) {
                            orient = Sphere::orient(x1, x2, x3);
                        } else {
                            orient = Sphere::orient(x2, x1, x3);
                        }
                        if (vertex3->detectAgent.edge == NULL) {
                            // vertex3 is not recorded as an approaching vertex
                            // to any edge
                            if (distance <= smallDistance) {
                                // *** APPROACHING
                                // vertex3 is very close to edge1, need to be
                                // handled in the coming splitPolygon procedure
                                // so calculate the old projection
                                projectOnOldEdge(vertex1, vertex2, vertex3, x4, distance);
                                AgentPairs::pair(vertex3, edge1, x4, distance, orient);
                                ApproachingVertices::record(vertex3);
                                vertex3->detectAgent.edgePointer = edgePointer2;
                            } else {
                                // *** NOT APPROACHING
                                // record vertex3 as an approaching vertex to edge3
                                AgentPairs::pair(vertex3, edge1, x4, distance, orient);
                                handleSmallAngle(edge1, vertex3, edgePointer1, edgePointer2, polygon);
                            }
                        } else {
                            if (vertex3->detectAgent.edge == edge1) {
                                if (orient != vertex3->detectAgent.orient) {
                                    REPORT_ERROR("Vertex-edge crossing has occured!")
                                }
                                if (isApproaching(vertex3->detectAgent.distance, distance)) {
                                    // *** APPROACHING
                                    //projectOnOldEdge(vertex1, vertex2, vertex3, x4, distance);
                                    //assert(vertex3->detectAgent.projection == x4);
                                    //vertex3->detectAgent.projection = x4;
                                    //vertex3->detectAgent.distance = distance;
                                    ApproachingVertices::record(vertex3);
                                    vertex3->detectAgent.edgePointer = edgePointer2;
                                } else {
                                    // *** NOT APPROACHING
                                    // update the information for next step
                                    vertex3->detectAgent.projection = x4;
                                    vertex3->detectAgent.distance = distance;
                                    handleSmallAngle(edge1, vertex3, edgePointer1, edgePointer2, polygon);
                                }
                            } else {
                                // vertex3 is moving away from old edge and
                                // moving toward to edge1
                                if (distance < vertex3->detectAgent.distance) {
                                    AgentPairs::unpair(vertex3, vertex3->detectAgent.edge);
                                    if (distance <= smallDistance) {
                                        // *** APPROACHING
                                        projectOnOldEdge(vertex1, vertex2, vertex3, x4, distance);
                                        AgentPairs::pair(vertex3, edge1, x4, distance, orient);
                                        ApproachingVertices::record(vertex3);
                                        vertex3->detectAgent.edgePointer = edgePointer2;
                                    } else {
                                        // *** NOT APPROACHING
                                        AgentPairs::pair(vertex3, edge1, x4, distance, orient);
                                        handleSmallAngle(edge1, vertex3, edgePointer1, edgePointer2, polygon);
                                    }
                                }
                            }
                        }
                    }
                } else if (vertex3->detectAgent.edge == edge1) {
                    // *** NOT APPROACHING
                    // vertex3 is moving away from edge1
                    AgentPairs::unpair(vertex3, edge1);
                }
            }
            edgePointer2 = edgePointer2->next;
        }
        edgePointer1 = edgePointer1->next;
    }
}

void ApproachDetector::detect(MeshManager &meshManager,
                                    const FlowManager &flowManager,
                                    PolygonManager &polygonManager)
{
    Polygon *polygon = polygonManager.polygons.front();
    for (int i = 0; i < polygonManager.polygons.size(); ++i) {
        detect(meshManager, flowManager, polygon);
        polygon = polygon->next;
    }
}