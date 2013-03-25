/**
 * \file path_planner_options.hpp
 * 
 * This library defines the options available when creating a path-planner.
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date July 2012
 */

/*
 *    Copyright 2012 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAK_PATH_PLANNER_OPTIONS_HPP
#define REAK_PATH_PLANNER_OPTIONS_HPP

namespace ReaK {
  
namespace pp {

/// This flag indicates that the motion-graph should be grown uni-directionally (from start).
const std::size_t UNIDIRECTIONAL_RRT = 0;
/// This flag indicates that the motion-graph should be grown bi-directionally (from start AND goal).
const std::size_t BIDIRECTIONAL_RRT = 1;

/// This flag indicates that the motion-graph should be stored as an adjacency-list graph.
const std::size_t ADJ_LIST_MOTION_GRAPH     = 0;
/// This flag indicates that the motion-graph should be stored as an adjacency-list graph that is overlaid on a dynamic vantage-point tree.
const std::size_t DVP_ADJ_LIST_MOTION_GRAPH = 1;
/// This flag indicates that the motion-graph should be stored as a linked-tree (a tree with links, i.e., like a linked-list).
const std::size_t LINKED_TREE_MOTION_GRAPH  = 2;

/// This flag indicates that the nearest-neighbor queries should be done via a linear search method.
const std::size_t LINEAR_SEARCH_KNN        = 0;
/// This flag indicates that the nearest-neighbor queries should be done via an approximate linear search method (i.e., some partial and randomized method that looks at only a fraction of the points for an approximate KNN set).
const std::size_t APPROX_LINEAR_SEARCH_KNN = 1;
/// This flag indicates that the nearest-neighbor queries should be done via a best-first search through a tree method (this method will be approximate in most cases).
const std::size_t DVP_LINKED_TREE_KNN      = 2;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in breadth-first layout of arity 2 (binary).
const std::size_t DVP_BF2_TREE_KNN         = 3;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in breadth-first layout of arity 4.
const std::size_t DVP_BF4_TREE_KNN         = 4;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in cache-oblivious breadth-first layout of arity 2 (binary).
const std::size_t DVP_COB2_TREE_KNN        = 5;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in cache-oblivious breadth-first layout of arity 4.
const std::size_t DVP_COB4_TREE_KNN        = 6;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in breadth-first layout of arity 2 (binary) which is also shared by the motion-graph.
const std::size_t DVP_ALT_BF2_KNN          = 7;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in breadth-first layout of arity 4 which is also shared by the motion-graph.
const std::size_t DVP_ALT_BF4_KNN          = 8;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in cache-oblivious breadth-first layout of arity 2 (binary) which is also shared by the motion-graph.
const std::size_t DVP_ALT_COB2_KNN         = 9;
/// This flag indicates that the nearest-neighbor queries should be done via a DVP-tree laid out on a contiguous storage in cache-oblivious breadth-first layout of arity 4 which is also shared by the motion-graph.
const std::size_t DVP_ALT_COB4_KNN         = 10;

/// This flag indicates that an eager collision checking policy should be preferred (generally only useful if the bird-flight distance metric is not good as a reflection of the collision-free travel cost).
const std::size_t EAGER_COLLISION_CHECKING = 0;
/// This flag indicates that a lazy collision checking policy should be preferred, meaning that the bird-flight distance will be used tentatively for edge-costs until the edges are considered for inclusion in the optimal path, at which point collision is checked.
const std::size_t LAZY_COLLISION_CHECKING  = 1;

/// This flag indicates that the plain version of an algorithm is to be used.
const std::size_t NOMINAL_PLANNER_ONLY          = 0;
/// This flag indicates that a Voronoi pull (i.e., RRT-style expansion) should be used if the main planning algorithm (usually a more greedy algorithm) gets "stuck". Note, this can be combined with other biasing flags.
const std::size_t PLAN_WITH_VORONOI_PULL        = 0x01;
/// This flag indicates that a narrow-passage push (i.e., PRM-style expansion) should be used if the main planning algorithm (usually a more greedy algorithm) gets "stuck". Note, this can be combined with other biasing flags.
const std::size_t PLAN_WITH_NARROW_PASSAGE_PUSH = 0x02;


};

};

#endif

