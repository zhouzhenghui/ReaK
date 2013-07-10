/**
 * \file any_sbmp_reporter.hpp
 * 
 * This library defines a type-erasure base-class for sampling-based motion/path planning reporters. 
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date July 2013
 */

/*
 *    Copyright 2013 Sven Mikael Persson
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

#ifndef REAK_ANY_SBMP_REPORTER_HPP
#define REAK_ANY_SBMP_REPORTER_HPP

#include "base/defs.hpp"
#include "base/shared_object.hpp"

#include <boost/config.hpp>
#include <boost/concept_check.hpp>

#include "trajectory_base.hpp"
#include "seq_path_base.hpp"
#include "graph_alg/any_graph.hpp"

#ifndef RK_ENABLE_CXX11_FEATURES
#include <boost/ref.hpp>
#else
#include <functional>
#endif

/** Main namespace for ReaK */
namespace ReaK {

/** Main namespace for ReaK.Path-Planning */
namespace pp {


/**
 * This class can be used as the base for a dynamically polymorphic SBMP/SBPP Reporter 
 * (SBMPReporterConcept and SBPPReporterConcept). This operates on type-erasure via the ReaK::graph::any_graph class.
 * \tparam FreeSpaceType The C-free topology type.
 */
template <typename FreeSpaceType>
class any_sbmp_reporter : public shared_object {
  public:
    typedef any_sbmp_reporter<FreeSpaceType> self;
    typedef typename subspace_traits<FreeSpaceType>::super_space_type SuperSpaceType;
    
#ifndef RK_ENABLE_CXX11_FEATURES
    typedef boost::reference_wrapper< const self > wrapped;
#else
    typedef std::reference_wrapper< const self > wrapped;
#endif
    
  public:
    
    virtual void draw_any_motion_graph(const FreeSpaceType&, const graph::any_graph&) const { };
    virtual void draw_trajectory(const FreeSpaceType&, const shared_ptr< trajectory_base< SuperSpaceType > >&) const { };
    virtual void draw_sequential_path(const FreeSpaceType&, const shared_ptr< seq_path_base< SuperSpaceType > >&) const { };
    
    virtual ~any_sbmp_reporter() { };
    
    /**
     * Draws the entire motion-graph.
     * \tparam MotionGraph The graph structure type representing the motion-graph.
     * \tparam PositionMap The property-map type that can map motion-graph vertex descriptors into point values.
     */
    template <typename MotionGraph, typename PositionMap>
    void draw_motion_graph(const FreeSpaceType& space, const MotionGraph& g, PositionMap) const {
      type_erased_graph<MotionGraph> teg(const_cast<MotionGraph*>(&g));
      this->draw_any_motion_graph(space, teg);
    };
    
    /**
     * Draws the solution trajectory.
     */
    void draw_solution(const FreeSpaceType& space, const shared_ptr< trajectory_base< SuperSpaceType > >& traj) const { 
      this->draw_trajectory(space, traj);
    };
    
    /**
     * Draws the solution trajectory.
     */
    void draw_solution(const FreeSpaceType& space, const shared_ptr< seq_path_base< SuperSpaceType > >& path) const { 
      this->draw_sequential_path(space, path);
    };
  
  
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/
    
    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      shared_object::save(A,shared_object::getStaticObjectType()->TypeVersion());
    };
    
    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      shared_object::load(A,shared_object::getStaticObjectType()->TypeVersion());
    };
    
    RK_RTTI_MAKE_CONCRETE_1BASE(self,0xC2460017,1,"any_sbmp_reporter",shared_object)
    
};





namespace detail {
  
  template <bool IsSteerable, typename FreeSpaceType>
  struct get_sbmp_reporter_any_property_type {
    typedef typename topology_traits<FreeSpaceType>::point_type type;
    static std::string name() { return "vertex_position"; };
  };
  
  template <typename FreeSpaceType>
  struct get_sbmp_reporter_any_property_type<true> {
    typedef typename steerable_space_traits<FreeSpaceType>::steer_record_type type;
    static std::string name() { return "edge_steer_rec"; };
  };
  
};



/**
 * This class can be used to wrap a SBMP Reporter into a dynamically polymorphic SBMP/SBPP Reporter 
 * (SBMPReporterConcept and SBPPReporterConcept). This operates on type-erasure via the ReaK::graph::any_graph class.
 * \tparam FreeSpaceType The C-free topology type.
 * \tparam Reporter The reporter object type to be encapsulated by this type-erasure class.
 */
template <typename FreeSpaceType, typename Reporter>
class type_erased_sbmp_reporter : public any_sbmp_reporter<FreeSpaceType> {
  public:
    typedef any_sbmp_reporter<FreeSpaceType> base_type;
    typedef type_erased_sbmp_reporter<FreeSpaceType, MotionGraph, Reporter> self;
    typedef typename subspace_traits<FreeSpaceType>::super_space_type SuperSpaceType;
    
  protected:
    
    typedef is_steerable_space<FreeSpaceType> has_steering_record;
    
    BOOST_STATIC_CONSTANT(bool, use_steering_record = has_steering_record::type::value);
    
    Reporter reporter;
    
  public:
    
    virtual void draw_any_motion_graph(const FreeSpaceType& space, const graph::any_graph& g) const { 
      typedef get_sbmp_reporter_any_property_type<use_steering_record, FreeSpaceType> PropType;
      reporter.draw_motion_graph(space, g, get< const typename PropType::type& >(PropType::name(), g));
    };
    virtual void draw_trajectory(const FreeSpaceType& space, const shared_ptr< trajectory_base< SuperSpaceType > >& traj) const { 
      reporter.draw_solution(space, traj);
    };
    virtual void draw_sequential_path(const FreeSpaceType& space, const shared_ptr< seq_path_base< SuperSpaceType > >& path) const { 
      reporter.draw_solution(space, path);
    };
    
  public:
    
    type_erased_sbmp_reporter(Reporter aReporter) : reporter(aReporter) { };
    
    virtual ~type_erased_sbmp_reporter() { };
    
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/
    
    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      base_type::save(A,base_type::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(reporter);
    };
    
    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      base_type::load(A,base_type::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(reporter);
    };
    
    RK_RTTI_MAKE_CONCRETE_1BASE(self,0xC2460018,1,"type_erased_sbmp_reporter",base_type)
    
};




/**
 * This class can be used as the base for a dynamically polymorphic SBMP/SBPP Reporter 
 * (SBMPReporterConcept and SBPPReporterConcept). This operates on type-erasure via the ReaK::graph::any_graph class.
 * \tparam FreeSpaceType The C-free topology type.
 */
template <typename FreeSpaceType>
class any_sbmp_reporter_chain : public shared_object {
  public:
    typedef any_sbmp_reporter_chain<FreeSpaceType> self;
    typedef typename subspace_traits<FreeSpaceType>::super_space_type SuperSpaceType;
    
  private:
    
    typedef any_sbmp_reporter<FreeSpaceType> value_type;
    
    typedef ReaK::shared_ptr< value_type > pointer_type;
    
    std::vector< pointer_type > reporters;
    
  public:
    
    /**
     * Add a reporter to this collection of dynamically-dispatched (type-erased) reporters.
     * \tparam Reporter The reporter type to use to report the progress of the path-planning, should model SBMPReporterConcept and SBPPReporterConcept.
     * \param aReporter The reporter object to use to report the progress of the path-planner.
     */
    template <typename Reporter>
    void add_reporter(Reporter aReporter) {
      reporters.push_back(pointer_type(new type_erased_sbmp_reporter<FreeSpaceType, Reporter>(aReporter)));
    };
    
    /**
     * Draws the entire motion-graph.
     * \tparam MotionGraph The graph structure type representing the motion-graph.
     * \tparam PositionMap The property-map type that can map motion-graph vertex descriptors into point values.
     */
    template <typename MotionGraph, typename PositionMap>
    void draw_motion_graph(const FreeSpaceType& space, const MotionGraph& g, PositionMap) const {
      type_erased_graph<MotionGraph> teg(const_cast<MotionGraph*>(&g));
      for(typename std::vector< pointer_type >::const_iterator it = reporters.begin(); it != reporters.end(); ++it)
        (*it)->draw_any_motion_graph(space, teg);
    };
    
    /**
     * Draws the solution trajectory.
     */
    void draw_solution(const FreeSpaceType& space, const shared_ptr< trajectory_base< SuperSpaceType > >& traj) const { 
      for(typename std::vector< pointer_type >::const_iterator it = reporters.begin(); it != reporters.end(); ++it)
        (*it)->draw_trajectory(space, traj);
    };
    
    /**
     * Draws the solution trajectory.
     */
    void draw_solution(const FreeSpaceType& space, const shared_ptr< seq_path_base< SuperSpaceType > >& path) const { 
      for(typename std::vector< pointer_type >::const_iterator it = reporters.begin(); it != reporters.end(); ++it)
        (*it)->draw_sequential_path(space, path);
    };
  
  
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/
    
    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      shared_object::save(A,shared_object::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(reporters);
    };
    
    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      shared_object::load(A,shared_object::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(reporters);
    };
    
    RK_RTTI_MAKE_CONCRETE_1BASE(self,0xC2460019,1,"any_sbmp_reporter_chain",shared_object)
    
};






};

};


#endif

