#ifndef B9VERTICALTRICONTAINER_H
#define B9VERTICALTRICONTAINER_H

#include <vector>
#include "triangle3d.h"

#define TRICONTAINER_PLAY 0.001

class B9VerticalTriContainer
{
public:
    B9VerticalTriContainer();

    double maxZ;
    double minZ;

    B9VerticalTriContainer* upContainer;
    B9VerticalTriContainer* downContainer;

    std::vector<Triangle3D*> tris;


    bool TriangleFits(Triangle3D* tri)
    {
        //测试三角数据是否有任何部分在此容器中。
        if((tri->maxBound.z() >= (minZ - TRICONTAINER_PLAY))
          &&
          (tri->minBound.z() <= (maxZ + TRICONTAINER_PLAY)))

           return true;

        return false;
    }

};

#endif // B9VERTICALTRICONTAINER_H
