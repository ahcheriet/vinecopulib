/*
Copyright 2016 Thibault Vatter, Thomas Nagler

This file is part of vinecopulib.

vinecopulib is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

vinecopulib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vinecopulib.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "include/structselect_tools.hpp"

namespace structselect_tools {
    
    //! Create base tree of the vine
    //! 
    //!  The base tree is a star on d + 1 variables, where the conditioning
    //!  set of each edge consists of a single number. When building the next
    //!  tree, the edges become vertices. Because the base graph was a star
    //!  all edges are allowed by the proximity condition, and the edges will
    //!  have a conditioning set consisting of the two vertex indices. This
    //!  will be the first actual tree of the vine.
    //!  
    //!  @param data nxd matrix of copula data.
    //!  @return A VineTree object containing the base graph.
    VineTree make_base_tree(const MatXd& data) {
        int d = data.cols();
        VineTree base_tree(d);
        // a star connects the root node (d) with all other nodes
        for (int target = 0; target < d; ++target) {
            // add edge and extract edge iterator
            auto e = add_edge(d, target, base_tree).first;
            
            // add edge data & info
            base_tree[e].hfunc1 = data.col(boost::target(e, base_tree));

            std::vector<int> conditioning(1);
            conditioning[0] = boost::target(e, base_tree);
            base_tree[e].conditioning = conditioning;

            std::vector<int> conditioned(0);
            conditioned.reserve(d - 2);
            base_tree[e].conditioned = conditioned;
        }

        return base_tree;
    }
    
    //! Build next tree of the vine
    //! 
    //! The next tree is found the following way:
    //!     1. Edges of the previous tree become edges in the new tree.
    //!     2. All edges allowed by the proximity condition are added to the new
    //!        graph.
    //!     3. Collapse the new graph to a maximum spanning tree for edge weight
    //!        |tau|.
    //! 
    //! @param prev_tree tree T_{k}.
    //! @param tree T_{k+1}.
    VineTree build_next_tree(VineTree& prev_tree) 
    {
        auto new_tree = edges_as_vertices(prev_tree);
        add_allowed_edges(new_tree);
    
        return new_tree;
    }
    
    //! Convert edge set into vertex set of a new graph
    //! 
    //! Further information about the structure is passed along:
    //!     - conditioning/conditioned set,
    //!     - indices of vertices connected by the edge in the previous tree.
    //! 
    //! @param tree T_{k}.
    //! @param A edge-less graph of vertices, each representing one edge of the
    //! previous tree.
    VineTree edges_as_vertices(const VineTree& prev_tree) {
        // start with full graph
        int d = num_edges(prev_tree);
        VineTree new_tree(d);
        
        // cut & paste information from previous tree
        int i = 0;
        for (auto e : boost::edges(prev_tree)) {
            new_tree[i].hfunc1 = prev_tree[e].hfunc1;
            new_tree[i].hfunc2 = prev_tree[e].hfunc2;
            new_tree[i].conditioning = prev_tree[e].conditioning;
            new_tree[i].conditioned = prev_tree[e].conditioned;
            new_tree[i].prev_edge_indices.reserve(2);
            new_tree[i].prev_edge_indices.push_back(boost::source(e, prev_tree));
            new_tree[i].prev_edge_indices.push_back(boost::target(e, prev_tree));
            ++i;
        }
        
        return new_tree;
    }
    
    //! Add edges allowed by the proximity condition
    //! 
    //! Also calculates the Kendall's tau for the edge and sets  edge weights
    //! to 1-|tau| so that the minimum spanning tree algorithm aximizes sum of 
    //! |tau|.
    //! 
    //! @param tree tree of a vine.
    void add_allowed_edges(VineTree& vine_tree) {
        for (auto v0 : boost::vertices(vine_tree)) {
            for (unsigned int v1 = 0; v1 < v0; ++v1) {
                // check proximity condition: common neighbor in previous tree
                // (-1 means 'no common neighbor')
                if (find_common_neighbor(v0, v1, vine_tree) > -1) {
                    auto pc_data = get_pc_data(v0, v1, vine_tree);
                    // edge weight is 1 - |tau| 
                    // -> minimum spanning tree maximizes sum of |tau|
                    auto w = 1.0 - std::fabs(pairwise_ktau(pc_data));
                    boost::add_edge(v0, v1, w, vine_tree);
                }            
            }
        }
    }
    
    // Find common neighbor in previous tree
    // 
    // @param v0,v1 vertices in the tree.
    // @param tree the current tree.
    // @param Gives the index of the vertex in the previous tree that was 
    // shared by e0, e1, the edge representations of v0, v1.
    int find_common_neighbor(int v0, int v1, const VineTree& tree) {
        auto ei0 = tree[v0].prev_edge_indices;
        auto ei1 = tree[v1].prev_edge_indices;
        auto ei_common = intersect(ei0, ei1);
        
        if (ei_common.size() == 0) 
            return -1; 
        else 
            return ei_common[0];
    }
    
    // Extract pair copula pseudo-observations from h-functions
    // 
    // @param v0,v1 vertex indices.
    // @param tree a vine tree.
    // @return The pseudo-observations for the pair coula, extracted from
    // the h-functions calculated in the previous tree.
    MatXd get_pc_data(int v0, int v1, const VineTree& tree) {
        int ei_common = find_common_neighbor(v0, v1, tree);
        int pos0 = find_position(ei_common, tree[v0].prev_edge_indices);
        int pos1 = find_position(ei_common, tree[v1].prev_edge_indices);
        
        int n = std::max(tree[v0].hfunc1.size(), tree[v0].hfunc2.size());
        MatXd pc_data(n, 2);
        
        if (pos0 == 0) {
            pc_data.col(0) = tree[v0].hfunc1;
        } else {
            pc_data.col(0) = tree[v0].hfunc2;
        }
        if (pos1 == 0) {
            pc_data.col(1) = tree[v1].hfunc1;
        } else {
            pc_data.col(1) = tree[v1].hfunc2;
        }
        
        return pc_data;
    }  
    
}
