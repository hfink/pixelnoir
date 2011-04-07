#ifndef __CB_MESH_MULTI_INDEX_H
#define __CB_MESH_MULTI_INDEX_H

#include "Types.h"

namespace ColladaBakery {

    class MeshMultiIndex {

    public:
        MeshMultiIndex(int size);
        ~MeshMultiIndex();
        UInt get(size_t i) const;
        size_t size() const;

    private:
        UInt* _data;
        const size_t _size;

    };

}

//stuff that we need to store it in a boost::unordered_map
inline bool operator==( const ColladaBakery::MeshMultiIndex& a, 
                        const ColladaBakery::MeshMultiIndex& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i =0; i<a.size(); ++i) {
        if (a.get(i) != b.get(i))
            return false;
    }

    return true;
}

inline std::size_t hash_value(const ColladaBakery::MeshMultiIndex& idx) {
    std::size_t seed = 0;
    for (size_t i = 0; i<idx.size(); ++i) {
        boost::hash_combine(seed, idx.get(i));
    }
    return seed;
}

#endif //__CB_MESH_MULTI_INDEX_H