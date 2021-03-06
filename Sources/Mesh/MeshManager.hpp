#ifndef MeshManager_h
#define MeshManager_h

#include "Velocity.hpp"
#include "RLLMesh.hpp"
#include "Layers.hpp"
#include "Point.hpp"
#include "PointCounter.hpp"
#include "TimeManager.hpp"

class MeshManager
{
public:
    MeshManager();
    virtual ~MeshManager();

    void setPoleR(double PoleR);

    void init(int numLon, int numLat, double *lon, double *lat);
    void init(int numLon, int numLat, int numLev,
              double *lon, double *lat, double *lev);

    bool hasLayers() const;

    const RLLMesh &getMesh(MeshType type) const { return mesh[type]; }
    const RLLMesh &getMesh(PointCounter::MeshType type) const {
        return pointCounter.mesh[type]; }
    const Layers &getLayers(Layers::LayerType type) const { return layers[type]; }

    void checkLocation(const Coordinate &x, Location &loc, Point *point = NULL);
    void countPoint(Point *point);

    void move(const Coordinate &x0, Coordinate &x1, const Velocity &v,
              Second dt, const Location &loc) const;

    void resetPointCounter() { pointCounter.reset(); }
    int getNumSubLon() { return pointCounter.numSubLon; }
    int getNumSubLat() { return pointCounter.numSubLat; }

private:
    friend class FlowManager;
    friend class MeshAdaptor;
    friend class TTS;

    double PoleR;
    RLLMesh mesh[4];
    Layers layers[2];
    PointCounter pointCounter;
};

#endif
