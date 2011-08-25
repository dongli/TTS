/**
 * \file DelaunayTriangle.h
 * \brief Delaunay triangle
 *
 * \author DONG Li
 * \date 2011-01-27
 */

#ifndef _DelaunayTriangle_h_
#define _DelaunayTriangle_h_

#include "List.h"
#include "Point.h"
#include "PointTriangle.h"

class DelaunayTriangle : public ListElement<DelaunayTriangle>
{
public:
    friend class PointTriangle;
    friend class Topology;
    friend class TIP;
    friend class DelaunayDriver;

    DelaunayTriangle();
    virtual ~DelaunayTriangle();
    
    void reinit();
    
    void calcircum();
    
    void dump();

    DelaunayVertex *DVT[3];
    DelaunayTriangle *adjDT[3]; // Adjacent Delaunay triangles
    DelaunayTriangle *subDT[3]; // Subdivided Delaunay triangles
    Point circumcenter;
    TIP tip;
};

class DelaunayTrianglePointer : public ListElement<DelaunayTrianglePointer>
{
public:
    DelaunayTrianglePointer() {
        reinit();
    }
    virtual ~DelaunayTrianglePointer() {}
    
    void reinit() {
        ptr = NULL;
    }
    
    DelaunayTriangle *ptr;
};

#endif