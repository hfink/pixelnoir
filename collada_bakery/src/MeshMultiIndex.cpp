#include "MeshMultiIndex.h"
#include <algorithm>
#include <stdexcept>

using namespace ColladaBakery;

MeshMultiIndex::MeshMultiIndex(int size) :
    _size(size)
{
    _data = new UInt[size];

    std::fill(&_data[0], &_data[size], 0);
}

MeshMultiIndex::~MeshMultiIndex() {
    delete [] _data;
}

UInt MeshMultiIndex::get(size_t i) const {
    if (i >= _size)
        throw std::out_of_range("Value is out of range");
   
    return _data[i];
}

size_t MeshMultiIndex::size() const {
    return _size;
}
