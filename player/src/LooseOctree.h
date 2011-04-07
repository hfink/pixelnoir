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

#ifndef __LOOSE_OCTREE_H
#define __LOOSE_OCTREE_H

#include "common.h"
#include "Geometry.h"
#include "Camera.h"

/**
 * Implements a loose octree as described by U. Thatcher, Game Programming Gems,
 * pp. 444-452. The advantage of this data structure is that node-access based 
 * on an object's extents is constant. Reat Thatcher's for more information.
 *
 * The main purpose of this class is to performance View-Frustum culling, i.e. 
 * this tree can be queried with a Frustum.
 * 
 * Furthermore this tree is separate from its storage back-end. You can choose
 * between the following implementations: 
 * 
 *      SPARSE_MAP:
 *          For sparse trees that host many potentially smaller objects, this
 *          storage performs exceptionally well. Also for most other use cases
 *          this type of storage should be preferred. This storage uses an
 *          unordered map to store nodes.
 *
 *      FULL_ARRAY: 
 *          This storage useses a flat array of pointers for the whole tree. 
 *          While we are able to derive a node's address in constant time
 *          and access nodes directly in the arrays, for a tree depth of greater
 *          than 6 the array size is growing exponentially and is not feasible
 *          to use. The previous storage should be preferred.
 */
class LooseOctree : noncopyable {

public:

    typedef enum {
        FULL_ARRAY,
        SPARSE_MAP
    } StorageType;

    /**
     * The vector is indexed by some prioritization that
     * we formulate. For example, we might use this as the material id.
     * The list of each vector element is roughly sorted along viewers direction 
     * with the earlier entries being closer to the viewer. Note that 
     * this is not strict sorting, however.
     */
    typedef vector<list<const Geometry * > > QueryResult;

    //A list of axis aligned bounding boxes used for debug rendering
    typedef list<AABB > DebugQueryResult;

    //Some statistics which can be collected optionally.
    struct Statistics {
        //The number of visible objects that were identified 
        //during a traversal
        int objects_visible;
        //The number of octree nodes that had to be traversed
        //in order to determine visibility
        int nodes_queried;
        //The number of nodes that had to be re-inserted
        //due to animation updates
        int nodes_reinserted;
        //The current amount of memory (in bytes) that the octree's 
        //storage occupies
        unsigned long storage_size;
    };
    
    /**
     * Creates a new octree.
     * @param world_size The size of the Octree, i.e. the width of the highest
     * level.
     * @param center The coordinates where the Octree should be placed around.
     * @param max_depth The maximum tree depth. Be careful with this parameter
     * if you use FULL_ARRAY as storage type.
     * @param storage_type Denotes which backing storage strategy should be
     * used. SPARSE_MAP is probably the right choice for most cases.
     * @param do_collect_statistics If enabled, the tree keeps track of some
     * statistical data during queries. Enabling this feature might cost you
     * a tiny bit of performance. Use LooseOctree::statistics() to access 
     * this data.
     * @param do_collect_debug_info If enabled, the tree additionally builds a 
     * list of axis aligned bounding boxes representing the non-empty nodes that
     * have been visited dury query-traversal. Use debug_info() to access this
     * data.
     */
    LooseOctree(float world_size, 
                const vec3& center, 
                int max_depth, 
                StorageType storage_type, 
                bool do_collect_statistics = false, 
                bool do_collect_debug_info = false);

    ~LooseOctree();

    /**
     * Queries the Octree with a particular frustum. The results of the query
     * will be written into query_out.
     * @param frustum The frustum used to query the tree hierarchically.
     * @param[out] An out parameter where the results of the query will be 
     * written to. Note that the vector's size of QueryResult must have the 
     * correct size.
     */
    void query(const Frustum& frustum, QueryResult& query_out) const;

    /**
     * Inserts a geometry object into the tree based on its bounding sphere.
     * @param geo The geometry to insert into the tree.
     */
    void insert(const Geometry * geo);

    /**
     * Removes a geometry from this tree.
     * @param geo Geometry to remove.
     * @return TRUE if the geometry was contained in the tree and could be
     * removed, FALSE otherwise.
     */
    bool remove(const Geometry * geo);
    
    /**
     * Checks all contained geometries, if they need to be updated, e.g. when
     * their positions or orientations changed during animation.
     * @return TRUE if updated geometries still fit into the octree, FALSE if
     * a geometry went outside the octree. It's the callers decision if this
     * tree should then be destructed and resized. Geometries that fall outside
     * the octree will be silently exluded from queries.
     */
    bool update();

    /**
     * Removes all elements from the Octree.
     */
    void clear();

    /**
     * Returns some statistical information which is collected during traversal.
     * Note that you must have set the parameter do_collect_statistics to TRUE
     * during construction in order to retrieve meaningful information.
     */
    const Statistics& statistics() const { return _statistics; }

    /**
     * Return a list of axis aligned bounding boxes which represent all non-
     * empty nodes of the last query-traversal. Note that you must have set the 
     * parameter do_collect_debug_info to TRUE during construction of this tree.
     */
    const DebugQueryResult& debug_info() const { return _debug_query; }

    /**
     * Reset statistical data that is not automatically reset on each traversal.
     */
    void reset_statistics();

    /**
     * Returns the storage type of this Octree. The storage is only to be
     * specified during construction of this tree.
     */
    StorageType storage_type() const { return _storage_type; }

    bool has_debug_info() const { return _do_collect_debug_info; }

    float world_size() const { return _world_size; }
    const vec3& center() const { return _center; }

private:

    /**
     * A Node of an Octree holds a list of geometries, and 8 pointers to
     * children.
     * The default constructor initializes all children to NULL.
     */
    struct Node {
        list<const Geometry *> geometries;
        Node* children[2][2][2];
        Node();
        bool has_children() const;
    };

    typedef char DepthIndex;
    typedef unsigned int AxisIndex;

    /**
     * This struct represents a unique coordinate of a node within the octree.
     * A coordinate is represented by its level of depth, and an index for each
     * axis. Note that these indices are global in the sense that they represent
     * one particular node among ALL nodes at this particular level. Note that
     * axis indices have to be in the range of 0 to (2^depth_level -1).
     */
    struct NodeCoords {

        DepthIndex depth_level;
        AxisIndex x;
        AxisIndex y;
        AxisIndex z;

        /**
         * DefaultConstrutor. Creates coordinates representing the root node.
         * (Depth=0, x=0, y=0, z=0)
         */
        NodeCoords() : 
            depth_level(0),x(0),y(0),z(0) {}

        /**
         * Constructs specific coordinates.
         * @param depth_level The level of depth. Zero is at root-level.
         * @param x The X-index at this particular level.
         * @param y The Y-index at this particular level.
         * @param z The Z-index at this particular level.
         */
        NodeCoords(int depth_level, int x, int y, int z) :
            depth_level(static_cast<DepthIndex>(depth_level)),
            x(static_cast<AxisIndex>(x)),
            y(static_cast<AxisIndex>(y)),
            z(static_cast<AxisIndex>(z)) {}

        //euality comparison is required by the hash-adaptors.
        inline friend bool operator==(const NodeCoords& a, const NodeCoords& b)
        {
            return ( a.depth_level == b.depth_level && a.x == b.x && a.y == b.y 
                     && a.z == b.z );
        }

        inline friend bool operator!=(const NodeCoords& a, const NodeCoords& b)
        {
            return !operator==(a, b);
        }

    };

    //Checks if an address is valid
    bool is_valid(const NodeCoords& node) const;

    //Calculates a hash value from on particular NodeCoords. This allows us
    //to use a NodeCoord as key to boost::unordered_map
    friend inline std::size_t hash_value(const NodeCoords& n);

    /**
     * Defines the visibility of cell within in a querying frustum.
     */
    typedef enum {
        NOT_VISIBLE,
        PARTLY_VISIBLE,
        FULLY_VISIBLE
    } Visibility;

    /**
     * Interface for Storages in this tree.
     */
    class Storage {

    public:

        virtual ~Storage() {}
        virtual const Node& root_node() const = 0;
        virtual Node& root_node() = 0;
        virtual bool exists(const NodeCoords& n) const = 0;
        virtual Node* get_node(const NodeCoords& n) = 0;
        
        /**
         * Implementations should remove the specified node, all of its
         * children, and all empty parent-nodes. This is required to keep
         * the tree consistent.
         */
        virtual void remove_node(const NodeCoords& n) = 0;
        virtual unsigned long current_size() const = 0;

    };

    /**
     * Implementation of a storage using one tight array to store pointers
     * to nodes.
     */
    class ArrayStorage : public Storage, noncopyable {
    
    public:

        ArrayStorage(int max_depth);
        virtual ~ArrayStorage();

        virtual const Node& root_node() const;
        virtual Node& root_node();
        virtual bool exists(const NodeCoords& n) const;
        virtual Node* get_node(const NodeCoords& n);
        virtual void remove_node(const NodeCoords& n);
        virtual unsigned long current_size() const;

    private:

        Node* create_or_get_node(const NodeCoords& n);
        unsigned long num_elements_at_depth(int depth) const;
        unsigned long get_address(const NodeCoords& n) const;
        bool is_valid(const NodeCoords& node) const;

        void remove_downward(Node* node, const NodeCoords& nc);
        void remove_upward(const NodeCoords& node);

        int _max_depth;
        Node** _node_array;

    };

    /**
     * Implementation of an octree-storage which uses an unordered_map to store
     * nodes, indexed by their NodeCoords. In most cases, this data structure is
     * more efficient than the ArrayStorage implementation.
     */
    class MapStorage : public Storage, noncopyable {
    
    public:

        MapStorage(int max_depth);
        virtual ~MapStorage();

        virtual const Node& root_node() const;
        virtual Node& root_node();
        virtual bool exists(const NodeCoords& n) const;
        virtual Node* get_node(const NodeCoords& n);
        virtual void remove_node(const NodeCoords& n);
        virtual unsigned long current_size() const;

    private:

        typedef boost::unordered_map<NodeCoords, Node* > NodeMap; 

        Node* create_or_get_node(const NodeCoords& n);
        bool is_valid(const NodeCoords& node) const;

        void remove_downward(Node* node, const NodeCoords& nc);
        void remove_upward(const NodeCoords& node);

        int _max_depth;
        
        NodeMap _node_map;
    };

    /**
     * Actual querying method used to pick out the geometries which are visible
     * with a particular view frustum.
     * @param n The node to visit.
     * @param n_c The coordinates (spatial information) about the node to query.
     * @param v The visibility of the node to visit which had been determined
     * before.
     * @param f The frustum that should be tested against.
     * @param[out] The out_parameter to store the visible Geometries into.
     */
    void query( const Node * const n, 
                const NodeCoords& n_c, 
                Visibility v, 
                const Frustum& f, 
                QueryResult& query_out) const;

    /**
     * Computes the visibility of a node within in a frustum using AABB/Frustum
     * intersection tests.
     */
    Visibility compute_visibility(const NodeCoords& n_c, const Frustum& f) const;

    /**
     * Returns the coordinates of the node where the specified geometry could
     * placed into. Note that this operation only performs simple calculations
     * based on the objects bounding sphere. The returned coordinates could be
     * used to insert the geometry with a complexity of O(1).
     * @param geo The geometry to calculate the node coordinates from.
     * @return The coordinates denoting the exact location in the octree where
     * the specified geometry would fit into.
     */
    NodeCoords get_node_coords(const Geometry * geo) const;

    /**
     * Convenience method. Asends the specified coordinate on one level higher.
     */
    static NodeCoords ascend(const NodeCoords& n);

    /**
     * Convenience method. Descends to one of the child nodes of the specified
     * node.
     * @param n The current node.
     * @param x The current node's local X coordinate to descend into.
     * @param y The current node's local Y coordinate to descend into.
     * @param z The current node's local Z coordinate to descend into.
     * @return The global coordinates of the specified child.
     */
    static NodeCoords descend(const NodeCoords& n, int x, int y, int z);

    /**
     * Calculates the node spacing at a particular level of depth with in the
     * curren tree.
     */
    float calc_node_spacing(int depth) const;

    /**
     * Calculates the depth where an object with the specified maximum radius 
     * could be stored into.
     */
    int calc_depth(float max_radius) const;
    
    /**
     * Calculates the indices for an object centered around obj_centroid, with
     * particular spacing in the tree (which depdends on the target-depth).
     */
    vec3 calc_indices(const vec3& obj_centroid, float spacing) const;

    /**
     * Returns the center of a particular node.
     */
    vec3 calc_node_center(const NodeCoords& n) const;

    /**
     * Checks if a bounding volume fits into a specified cell.
     */
    bool fits_inside(const BoundingVolume& b, const NodeCoords& n) const;

    //member variables
    float _world_size;
    int _max_depth;
    vec3 _center;
    const StorageType _storage_type;
    Storage* _storage;
    const bool _do_collect_statistics;
    const bool _do_collect_debug_info;

    //Statistics and debug info data
    mutable Statistics _statistics;
    mutable DebugQueryResult _debug_query;

    //Cache storage to associate a geometry with its node.
    typedef boost::unordered_map<const Geometry*, NodeCoords > NodeCoordsMap;

    NodeCoordsMap _node_lookup;

};

//Hash function for NodeCoords as used by MapStorage
std::size_t hash_value(const LooseOctree::NodeCoords& n) {
        std::size_t seed = 0;
        boost::hash_combine(seed, n.depth_level);
        boost::hash_combine(seed, n.x);
        boost::hash_combine(seed, n.y);
        boost::hash_combine(seed, n.z);
        return seed;
 }

#endif