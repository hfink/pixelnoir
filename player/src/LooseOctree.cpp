//Copyright (c) 2010 Heinrich Fink <hf (at) hfink (dot) eu>, 
//                   Thomas Weber <weber (dot) t (at) gmx (dot) at>
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

#include "LooseOctree.h"
#include <math.h>
#include "BoundingVolume.h"
#include <limits>

LooseOctree::Node::Node() {
    for (int ix = 0; ix<2; ++ix)
        for (int iy = 0; iy<2; ++iy)
            for (int iz = 0; iz<2; ++iz)
                children[ix][iy][iz] = NULL;
}

bool LooseOctree::Node::has_children() const {
    bool is_empty = true;

    for (int ix = 0; ix<2; ++ix)
        for (int iy = 0; iy<2; ++iy)
            for (int iz = 0; iz<2; ++iz)
                is_empty &= (children[ix][iy][iz] == NULL);

    return !is_empty;

}

LooseOctree::LooseOctree( float world_size, 
                          const vec3& center, 
                          int max_depth, 
                          StorageType storage_type,
                          bool do_collect_statistics,
                          bool do_collect_debug_info )
    : _world_size(world_size), 
      _max_depth(max_depth),
      _center(center),
      _storage_type(storage_type),
      _do_collect_statistics(do_collect_statistics),
      _do_collect_debug_info(do_collect_debug_info)
{

    if (storage_type == FULL_ARRAY) {

        if (max_depth > 7) {
            std::cerr << "Warning, you are using an extraordinary amount of "
                      << " memory for tree with depth " << max_depth << "."
                      << "Consider using SPARSE_MAP storage type instead." 
                      << std::endl;
        }
        _storage = new ArrayStorage(max_depth);
    } else if (storage_type == SPARSE_MAP) {
        _storage = new MapStorage(max_depth);
    }

    if (max_depth > std::numeric_limits<char>::max()) {
        std::cout << "Max Depth: " << max_depth << " is too high." << std::endl;
        assert(NULL);
    }

    _statistics.nodes_queried = 0;
    _statistics.objects_visible = 0;
    _statistics.storage_size = 0;
    _statistics.nodes_reinserted = 0;

}

LooseOctree::~LooseOctree() {	
    delete _storage;
}

void LooseOctree::reset_statistics() {
    _statistics.nodes_queried = 0;
    _statistics.nodes_reinserted = 0;
    _statistics.objects_visible = 0;
    _statistics.storage_size = 0;
}

void LooseOctree::query(const Frustum& f, QueryResult& query_out) const {

    const Node& root_node = _storage->root_node();

    //Note: default constructors constructs the 
    //root node coords which are 0, 0, 0, 0
    NodeCoords root_node_coords;

    if (_do_collect_statistics) {
        _statistics.nodes_queried = 1; //root node will always be queried
        _statistics.objects_visible = 0;
    }

    if (_do_collect_debug_info) {
        //clear debug info
        _debug_query.clear();
    }

    //TODO-OPT: if we have a very small frustum, check the frustum's
    //size, retrieve the first node where it could fit into, with our
    //formula, and start from there... this might be a tad faster...

    Visibility v = compute_visibility(root_node_coords, f);

    if (v != NOT_VISIBLE)
        query(&root_node, root_node_coords, v, f, query_out);

    if (_do_collect_statistics) {
        _statistics.storage_size = _storage->current_size();
    }

}

void LooseOctree::query( const Node * const n, 
                         const NodeCoords& n_c, 
                         Visibility v, 
                         const Frustum& f, 
                         QueryResult& query_out) const {

    //If a node is null, it means, no children exists, 
    //as guaranteed by our storage...
    if (n == NULL)
        return;

    if (_do_collect_statistics)
        _statistics.nodes_queried++;

    if (v != FULLY_VISIBLE) {
        //There is doubt if we are visible at all, calculate it!
        v = compute_visibility(n_c, f);
        if (v == NOT_VISIBLE)
            return;
    }

    //obviously, this node is visible, collect its
    //geometries, and enter them into query results
    list<const Geometry*>::const_iterator it;
    bool cell_has_visible_geo = false;
    for (it = n->geometries.begin(); it != n->geometries.end(); ++it) {

        //TODO: in here you should cull against the bounding sphere, again
        //Create an AABB of the bounding sphere
        const Sphere& sphere = (*it)->bounding_volume().sphere();

        const vec3 radius_vec(sphere.radius());
        const vec3 aabb_min = sphere.center() - radius_vec;
        const vec3 aabb_max = sphere.center() + radius_vec;

        const AABB aabb(aabb_min, aabb_max);

        if (intersect_aabb_frustum(aabb, f) != OUTSIDE) { 
            query_out[(*it)->material_id()].push_back(*it);
            if (_do_collect_statistics)
                _statistics.objects_visible++;
            cell_has_visible_geo = true;
        }
    }

    if (_do_collect_debug_info && cell_has_visible_geo) {
        vec3 center = calc_node_center(n_c);
        float s = calc_node_spacing(n_c.depth_level);
        //our loose octree has a factor "k" of 2, therefore
        //cube length is twice the cell spacing
        vec3 b_min = center - vec3(s);
        vec3 b_max = center + vec3(s);
        AABB bounding_box = AABB(b_min, b_max);
        _debug_query.push_back(bounding_box);
    }

    for (int ix = 0; ix<2; ++ix) {
        for (int iy = 0; iy<2; ++iy) {
            for (int iz = 0; iz<2; ++iz) {
                NodeCoords child_c = descend(n_c, ix, iy, iz);
                query(n->children[ix][iy][iz], child_c, v, f, query_out);
            }
        }
    }
}

LooseOctree::Visibility LooseOctree::compute_visibility( const NodeCoords& n_c, 
                                                         const Frustum& f ) const {
    
    //extract the bounding box of the current node
    vec3 center = calc_node_center(n_c);
    float s = calc_node_spacing(n_c.depth_level);
    //our loose octree has a factor "k" of 2, therefore
    //cube length is twice the cell spacing
    vec3 b_min = center - vec3(s);
    vec3 b_max = center + vec3(s);
    AABB bounding_box = AABB(b_min, b_max);
    
    TestResult result = intersect_aabb_frustum(bounding_box, f);

    //TODO: would it make more sense to just return the intersection result enum?
    if (result == INTERSECTING)
        return PARTLY_VISIBLE;
    else if (result == INSIDE)
        return FULLY_VISIBLE;
    //is outside
    return NOT_VISIBLE;

}

float LooseOctree::calc_node_spacing(int depth) const {
    return ( _world_size / glm::pow(2.0f, (float)depth) );
}

//Calculates a depth level, where an object with the radius max_radius
//is guaranteed to fit into. This is very conversative measure, it might fit
//into one of the children as well.
//Note: The original publication in Game Programming Gems had a bug, they forget
//the "-1" in their final formula!
int LooseOctree::calc_depth(float max_radius) const {
    return ( glm::min( _max_depth, 
                      (int)glm::floor(glm::log2(_world_size/max_radius)-1)) );
}

//for a grid of side length = world size, with a particular spacing, 
//this method returns its indices
vec3 LooseOctree::calc_indices(const vec3& obj_centroid, float spacing) const {
    float inv_s = 1/spacing;
    vec3 i = glm::floor((obj_centroid-_center+vec3(_world_size*0.5f)) * inv_s );
    return i;
}

//calculate the center of a node
vec3 LooseOctree::calc_node_center(const NodeCoords& n) const {
    float s = calc_node_spacing(n.depth_level);
    vec3 idx(n.x + 0.5f, n.y + 0.5f, n.z + 0.5f);
    return ( s * idx - vec3(_world_size*0.5f) + _center);
}

//calculate if a bounding volume fits into a node or not
bool LooseOctree::fits_inside( const BoundingVolume& b, 
                               const NodeCoords& n) const {

    vec3 node_center = calc_node_center(n);
    
    const Sphere& s_bv = b.sphere();
    float s = calc_node_spacing(n.depth_level);
    vec3 min_extends = node_center - vec3(s);
    vec3 max_extends = node_center + vec3(s);

    bool fits = ( ((s_bv.center().x + s_bv.radius()) <= max_extends.x) &&
                  ((s_bv.center().y + s_bv.radius()) <= max_extends.y) &&
                  ((s_bv.center().z + s_bv.radius()) <= max_extends.z) &&
                  ((s_bv.center().x - s_bv.radius()) >= min_extends.x) &&
                  ((s_bv.center().y - s_bv.radius()) >= min_extends.y) &&
                  ((s_bv.center().z - s_bv.radius()) >= min_extends.z) );

    return fits;
}

LooseOctree::NodeCoords LooseOctree::get_node_coords(const Geometry * geo) const 
{

    NodeCoords node;

    const BoundingVolume& b = geo->bounding_volume();

    //this calculate the depth for the max radius
    int depth = calc_depth(b.sphere().radius());
    float s = calc_node_spacing(depth);

    //calculate the conservative indices
    vec3 idx = calc_indices(b.sphere().center(), s);

    //node query key
    NodeCoords query(depth, (int)idx.x, (int)idx.y, (int)idx.z);

    node = query;

    //get the node candidate
    //(NOTE: if a node with these coordinates does not exist yet, it
    //will automatically created by the storage)
    //Node& candidate = _storage->get_node(query);

    //as suggested by the paper loose octrees, we will now check if
    //the geometry fits into the nearest child node of this candidate
    if (depth != _max_depth) {

        //get the node closest to our bounding volume at a deeper level
        float s_deeper = calc_node_spacing(depth+1);
        vec3 c_idx = calc_indices(b.sphere().center(), s_deeper);
        NodeCoords closest_child_node_coords( depth + 1, 
                                              (int)c_idx.x, 
                                              (int)c_idx.y, 
                                              (int)c_idx.z);
        
        bool does_fit = fits_inside(b, closest_child_node_coords);

        if (does_fit)
            node = closest_child_node_coords;

    }

    return node;
}

void LooseOctree::insert(const Geometry * geo) {

    //check the location where to insert the geometry
    //this is solely based on the geometry's bounding volume, 
    //constant time operation

    NodeCoords query = get_node_coords(geo);

    if (!is_valid(query)) {
        cerr << "Error: Retrieved node coordinates are not valid." << endl;
        cerr << "       Skipping geometry " << geo->get_id() << endl;
        return;
    }

    if (!_node_lookup.insert(std::make_pair(geo, query)).second) {
        std::cerr << "Error: could not insert nodecoord into data structure" 
                  << std::endl;
        assert(NULL);
    }

    //perform the actual insertion in the data storage
    Node* node = _storage->get_node(query);
    node->geometries.push_back(geo);

}

bool LooseOctree::remove(const Geometry * geo) {

    NodeCoordsMap::iterator it = _node_lookup.find(geo);
    if (it == _node_lookup.end()) {
        return false;
    }

    Node* node = _storage->get_node(it->second);
    if (node->geometries.size() > 1) {
        node->geometries.remove(geo);
    } else {
        //seems to be the last geometry in the node, trigger
        //removal
        _storage->remove_node(it->second);
    }

    //finally, also remove this node from the 
    //lookup data structure
    _node_lookup.erase(geo);

    return true;
}

bool LooseOctree::update() {
    NodeCoordsMap::iterator it;
    for (it = _node_lookup.begin(); it != _node_lookup.end(); ++it) {
        if (it->first->transform_node()->has_changed()) {
            //check if we are still in the right place
            NodeCoords nc = get_node_coords(it->first);

            bool is_ok = ( (it->second.depth_level == nc.depth_level) && 
                           (it->second.x == nc.x) && 
                           (it->second.y == nc.y) &&
                           (it->second.z == nc.z) );

            if (!is_ok) {

                if (!is_valid(nc)) {
                    return false;
                }

                //remove, and reinsert
                //note that these are constant time operations

                Node* old_node = _storage->get_node(it->second);
                old_node->geometries.remove(it->first);
                //NOTE: We could move the removal of empty nodes to some 
                //"purge" method that we might call only every 5 seconds or so.
                if ( old_node->geometries.empty() && 
                    (!old_node->has_children())) {
                    _storage->remove_node(it->second);
                }

                //add at new location
                Node* new_node = _storage->get_node(nc);
                new_node->geometries.push_back(it->first);

                it->second = nc;

                if (_do_collect_statistics)
                    _statistics.nodes_reinserted++;
            }               
        }
    }

    return true;
}

void LooseOctree::clear() {
    NodeCoords root_node;
    _storage->remove_node(root_node);
}

/**
 * Implementation of LooseOctree::ArrayStorage
 */
LooseOctree::ArrayStorage::ArrayStorage(int max_depth) :
    _max_depth(max_depth)
{
    //level 0 => 2^0^3 elements
    //level 1 => 2^1^3 elements
    //level 2 => 2^2^3 elements
    //total number of elements can be calculated with a series
    //use maple to derive sum(2^(3*i), i=0..n) into the following formula

    //(we could express into a for-loop as well, but that's not
    //even close as nerdy as this one...)

    unsigned long num_elements = num_elements_at_depth(max_depth);
    _node_array = new Node*[num_elements];

    //initialize all with NULL
    std::fill( &_node_array[0], 
               &_node_array[num_elements], 
               static_cast<Node*>(NULL) );

    //we always initiate the tree with at least one root note
    _node_array[0] = new Node();
}

LooseOctree::ArrayStorage::~ArrayStorage() {
    
    NodeCoords root_node;
    //that removes the root node and all children, which is the
    //the whole tree...
    remove_node(root_node);

    //the root node is never deleted, delete it now
    delete _node_array[0];

    delete[] _node_array;
    _node_array = NULL;
}

const LooseOctree::Node& LooseOctree::ArrayStorage::root_node() const {
    return *_node_array[0];
}

LooseOctree::Node& LooseOctree::ArrayStorage::root_node() {
    return *_node_array[0];
}

unsigned long LooseOctree::ArrayStorage::num_elements_at_depth(int depth) const {
    float s = 1.0f/7.0f;
    unsigned long num_elements = 
                (unsigned long)glm::round( s * glm::pow(8.0f, (float)(depth+1)) - s );
    return num_elements;
}

unsigned long LooseOctree::ArrayStorage::get_address(const NodeCoords& n) const {

    //number of divisions at this depeth
    int divnum = (int)glm::pow(2.0f, (float)n.depth_level);
    //starting index into our array
    unsigned long start_idx = num_elements_at_depth(n.depth_level - 1);
    return (start_idx + n.z*divnum*divnum + n.y*divnum + n.x);

}

bool LooseOctree::ArrayStorage::exists(const NodeCoords& n) const {
    
    if ( (n.depth_level > _max_depth) || (n.depth_level < 0) )
        return false;

    int addr = get_address(n);
    return (_node_array[addr] != NULL);

}

LooseOctree::NodeCoords LooseOctree::ascend(const LooseOctree::NodeCoords& n) {
    
    NodeCoords ascended;
    ascended.depth_level = n.depth_level - 1;
    ascended.x = int(n.x / 2.0f);
    ascended.y = int(n.y / 2.0f);
    ascended.z = int(n.z / 2.0f);

    return ascended;
}

LooseOctree::NodeCoords LooseOctree::descend( const NodeCoords& n, 
                                              int x, int y, int z) {

    NodeCoords descended;
    descended.depth_level = n.depth_level + 1;
    descended.x = n.x * 2 + x;
    descended.y = n.y * 2 + y;
    descended.z = n.z * 2 + z;
    
    return descended;
}


bool LooseOctree::is_valid(const NodeCoords& node) const {

    unsigned int max_index = 
                    static_cast<unsigned int>( pow(2.0f, node.depth_level) );

    bool is_valid = ( (node.depth_level >= 0) &&
                      (node.depth_level <= _max_depth) && 
                      (node.x < max_index) &&          
                      (node.y < max_index) && 
                      (node.z < max_index) );

    return is_valid;

}

bool LooseOctree::ArrayStorage::is_valid(const NodeCoords& node) const {

    unsigned int max_index = 
                    static_cast<unsigned int>( pow(2.0f, node.depth_level) );

    bool is_valid = ( (node.depth_level >= 0) &&
                      (node.depth_level <= _max_depth) && 
                      (node.x < max_index) &&          
                      (node.y < max_index) && 
                      (node.z < max_index) );

    return is_valid;

}

LooseOctree::Node* LooseOctree::ArrayStorage::create_or_get_node(const NodeCoords& n) {
    
    if (!is_valid(n)) {
        std::cerr << "Access to element is invalid" << std::endl;
        std::cerr << "Maybe your octree is too small!" << std::endl;
        return NULL;
    }

    int addr = get_address(n);

    Node* lookup_node = _node_array[addr];
    if (lookup_node != NULL)
        return lookup_node;

    //does not exists, create
    Node* new_node = new Node();
    _node_array[addr] = new_node;

    //create parents, if they do not exist already, and set yourself as a child
    NodeCoords parent_coord = ascend(n);
    //local coords
    int c_idx_x = n.x % 2;
    int c_idx_y = n.y % 2;
    int c_idx_z = n.z % 2;

    //This is the stopping condition of the recursion
    if (parent_coord.depth_level >= 0) {
        Node* parent_node = create_or_get_node(parent_coord);
        //if the place where we insert a newly created node is not null, 
        //our tree is not kept clean... removal always nullifies a parent field.
        assert(parent_node->children[c_idx_x][c_idx_y][c_idx_z] == NULL);
        parent_node->children[c_idx_x][c_idx_y][c_idx_z] = new_node;
    }

    return new_node;
}

LooseOctree::Node* LooseOctree::ArrayStorage::get_node(const NodeCoords& n) {
    return create_or_get_node(n);
}

void LooseOctree::ArrayStorage::remove_downward(Node* node, const NodeCoords& nc) {

    if (node == NULL)
        return;

    unsigned long addr = get_address(nc);

    for (int ix = 0; ix<2; ++ix) {
        for (int iy = 0; iy<2; ++iy) {
            for (int iz = 0; iz<2; ++iz) {
                NodeCoords descended = descend(nc, ix, iy, iz);
                remove_downward(node->children[ix][iy][iz], descended);
                node->children[ix][iy][iz] = NULL;
            }
        }
    }

    if (node != &root_node()) {
        //Nullify array entry
        _node_array[addr] = NULL;
        delete node;
    }

}

void LooseOctree::ArrayStorage::remove_upward(const NodeCoords& n) {

    NodeCoords parent_coord = ascend(n);
    //local coords of this node in parent
    int c_idx_x = n.x % 2;
    int c_idx_y = n.y % 2;
    int c_idx_z = n.z % 2;

    if (parent_coord.depth_level >= 0) {

        //Note that this never creates a node, since every child
        //has to have had a parent already
        Node* parent_node = create_or_get_node(parent_coord);

        parent_node->children[c_idx_x][c_idx_y][c_idx_z] = NULL;

        //if this parent has no childs and currently holds no geometries,
        //remove it
        bool is_empty = parent_node->geometries.empty();

        if ( !parent_node->has_children() && 
             is_empty &&
             (parent_node != &root_node()) ) {
            unsigned long addr = get_address(parent_coord);
            delete parent_node;
            //nullify entry
            _node_array[addr] = NULL;
            remove_upward(parent_coord);
        }
    
    }

}

void LooseOctree::ArrayStorage::remove_node(const NodeCoords& n) {
    
    int addr = get_address(n);
    Node* node = _node_array[addr];
    if (node == NULL)
        return; //nothing to remove

    //remove this node and all children
    remove_downward(node, n);

    //remove this node from the parent's children array, 
    //and remove all parents if they are empty
    remove_upward(n);

    //that's it
}

unsigned long LooseOctree::ArrayStorage::current_size() const {
    //calculate the total size 

    unsigned long size = sizeof(Node**);
    unsigned long num_elements = num_elements_at_depth(_max_depth);
    size += sizeof(Node*) * num_elements;
    for (unsigned long i = 0; i<num_elements; ++i) {
        if (_node_array[i] != NULL) {
            size += sizeof(Node);
        }
    }

    return size;
}

/**
 * Implementation of LooseOctree::MapStorage.
 */

LooseOctree::MapStorage::MapStorage(int max_depth) :
_max_depth(max_depth)
{
    //create the root node
    NodeCoords root_nc;
    Node* root_node = new Node();
    if (!_node_map.insert(NodeMap::value_type(root_nc, root_node)).second) {
        std::cerr << "Could not create root node." << std::endl;
        assert(NULL);
    }
}

LooseOctree::MapStorage::~MapStorage(){
    
    NodeCoords root_nc;

    //that removes the root node and all children, which is the
    //the whole tree...
    remove_node(root_nc);

    //the root node is never deleted, delete it now
    delete _node_map.at(root_nc);
}

bool LooseOctree::MapStorage::is_valid(const NodeCoords& node) const {

    unsigned int max_index = 
                    static_cast<unsigned int>( pow(2.0f, node.depth_level) );

    bool is_valid = ( (node.depth_level >= 0) &&
                      (node.depth_level <= _max_depth) && 
                      (node.x < max_index) &&          
                      (node.y < max_index) && 
                      (node.z < max_index) );

    return is_valid;

}

const LooseOctree::Node& LooseOctree::MapStorage::root_node() const{
    NodeCoords root_nc;
    return *_node_map.at(root_nc);
}

LooseOctree::Node& LooseOctree::MapStorage::root_node(){
    NodeCoords root_nc;
    return *_node_map.at(root_nc);
}

bool LooseOctree::MapStorage::exists(const NodeCoords& n) const{
    return (_node_map.find(n) != _node_map.end());
}

LooseOctree::Node* LooseOctree::MapStorage::create_or_get_node(const NodeCoords& n) 
{

    if (!is_valid(n)) {
        std::cerr << "Access to element is invalid" << std::endl;
        std::cerr << "Maybe your octree is too small!" << std::endl;
        return NULL;
    }

    if (exists(n))
        return _node_map.at(n);
   
    //create new entry
    Node* new_node = new Node();
    _node_map.insert(NodeMap::value_type(n, new_node));
        
    //create parents, if they do not exist already, and set yourself as a child
    NodeCoords parent_coord = ascend(n);
    
    //local coords
    int c_idx_x = n.x % 2;
    int c_idx_y = n.y % 2;
    int c_idx_z = n.z % 2;

    //This is the stopping condition of the recursion
    if (parent_coord.depth_level >= 0) {
        Node* parent_node = create_or_get_node(parent_coord);
        //if the place where we insert a newly created node is not null, 
        //our tree is not kept clean... removal always nullifies a parent field.
        assert(parent_node->children[c_idx_x][c_idx_y][c_idx_z] == NULL);
        parent_node->children[c_idx_x][c_idx_y][c_idx_z] = new_node;
    }

    return new_node;

}

LooseOctree::Node* LooseOctree::MapStorage::get_node(const NodeCoords& n){
    return create_or_get_node(n);
}

void LooseOctree::MapStorage::remove_downward(Node* node, const NodeCoords& n) {

    if (node == NULL)
        return;

    for (int ix = 0; ix<2; ++ix) {
        for (int iy = 0; iy<2; ++iy) {
            for (int iz = 0; iz<2; ++iz) {
                NodeCoords descended = descend(n, ix, iy, iz);
                remove_downward(node->children[ix][iy][iz], descended);
                node->children[ix][iy][iz] = NULL;
            }
        }
    }

    if (node != &root_node()) {
        _node_map.erase(n);
        delete node;
    }

}

void LooseOctree::MapStorage::remove_upward(const NodeCoords& n) {

    NodeCoords parent_coord = ascend(n);
    //local coords of this node in parent
    int c_idx_x = n.x % 2;
    int c_idx_y = n.y % 2;
    int c_idx_z = n.z % 2;

    if (parent_coord.depth_level >= 0) {

        //Note that this never creates a node, since every child
        //has to have had a parent already
        Node* parent_node = create_or_get_node(parent_coord);

        parent_node->children[c_idx_x][c_idx_y][c_idx_z] = NULL;

        //if this parent has no childs and currently holds no geometries,
        //remove it
        bool is_empty = parent_node->geometries.empty();

        if ( !parent_node->has_children() && 
             is_empty &&
             (parent_node != &root_node()) ) {
            delete parent_node;
            //nullify entry
            _node_map.erase(parent_coord);
            remove_upward(parent_coord);
        }
    
    }
}

void LooseOctree::MapStorage::remove_node(const NodeCoords& n){

    if (!exists(n))
        return;

    //remove this node and all children
    Node* node = _node_map.find(n)->second;
    remove_downward(node, n);

    //remove this node from the parent's children array, 
    //and remove all parents if they are empty
    remove_upward(n);

    //that's it
}

unsigned long LooseOctree::MapStorage::current_size() const{
    unsigned long estimated_size = sizeof(NodeMap);
    estimated_size += _node_map.size() * sizeof(Node);
    return estimated_size;
}


