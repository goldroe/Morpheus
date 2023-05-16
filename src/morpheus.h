#ifndef MORPHEUS_H
#define MORPHEUS_H

#include <vector>
#include "ext/HandmadeMath.h"


struct OBJ_Model {
    std::vector<uint32> indices;
    std::vector<HMM_Vec4> vertices;
    std::vector<HMM_Vec3> normals;
    std::vector<HMM_Vec3> texcoords;
};

#endif
