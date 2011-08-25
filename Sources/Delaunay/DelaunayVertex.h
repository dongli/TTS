/**
 * \file DelaunayVertex.h
 * \brief Delaunay vertex
 *
 * \author DONG Li
 * \date 2011-01-27
 */

#ifndef _DelaunayVertex_h_
#define _DelaunayVertex_h_

#include "List.h"
#include "Point.h"
#include "Topology.h"
#include "PointTriangle.h"

class DelaunayVertex : public ListElement<DelaunayVertex>
{
public:
    friend class PointTriangle;
    friend class TIP;
    friend class FakeVertices;
    friend class DelaunayDriver;

    DelaunayVertex();
    virtual ~DelaunayVertex();
    
    void
    reinit();

    Point *point;
    bool inserted;
    Topology topology;
    PIT pit;
};

class DelaunayVertexPointer : public ListElement<DelaunayVertexPointer>
{
public:
    DelaunayVertexPointer() {
        reinit();
    }
    virtual ~DelaunayVertexPointer() {}
    
    void
    reinit() {
        ptr = NULL;
    }
    
    DelaunayVertex *ptr;
};

#endif