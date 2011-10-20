/**
 * \file so3_topologies.hpp
 * 
 * This library provides classes that define topologies on SO(3) (3D rotation). A quaternion-topology is 
 * a simple metric-space where the points are unit quaternion values. Higher-order differential spaces 
 * in SO(3) are just normal vector-spaces.
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date October 2011
 */

/*
 *    Copyright 2011 Sven Mikael Persson
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

#ifndef REAK_SO3_TOPOLOGIES_HPP
#define REAK_SO3_TOPOLOGIES_HPP


#include "base/defs.hpp"

#include <boost/config.hpp> // For BOOST_STATIC_CONSTANT

#include "vect_distance_metrics.hpp"

#include "differentiable_space.hpp"
#include "hyperball_topology.hpp"

#include <cmath>
#include "base/named_object.hpp"

#include "kinetostatics/quat_alg.hpp"

namespace ReaK {

namespace pp {

/**
 * This class implements a quaternion-topology. Because quaternions are constrained on the unit hyper-sphere,
 * this topology does indeed model the MetricSpaceConcept (with random generation of quaternions) although it 
 * is not bounded per se.
 * \tparam T The value-type for the topology.
 */
template <typename T>
class quaternion_topology : public named_object
{
  public:
    typedef quaternion_topology<T> self;
    
    typedef unit_quat<T> point_type;
    typedef vect<T,3> point_difference_type;
    
    BOOST_STATIC_CONSTANT(std::size_t, dimensions = 3);
    
    /**
     * Default constructor.
     */
    quaternion_topology(const std::string& aName = "quaternion_topology") : named_object() {
      setName(aName);
    };
    
    /**
     * Returns the distance between two points.
     */
    double distance(const point_type& a, const point_type& b) const 
    {
      return ReaK::norm(this->difference(b,a);
    }
    
    /**
     * Returns the norm of the difference between two points.
     */
    double norm(const point_difference_type& delta) const {
      return ReaK::norm(delta);
    }
    
    /**
     * Generates a random point in the space, uniformly distributed.
     */
    point_type random_point() const {
      boost::variate_generator< pp::global_rng_type&, boost::normal_distribution<typename vect_traits<point_difference_type>::value_type> > var_rnd(pp::get_global_rng(), boost::normal_distribution<typename vect_traits<point_difference_type>::value_type>());
      return point_type(var_rnd(),var_rnd(),var_rnd(),var_rnd());
      //According to most sources, normalizing a vector of Normal-distributed components will yield a uniform 
      // distribution of the components on the unit hyper-sphere. N.B., the normalization happens in the 
      // constructor of the unit-quaternion (when constructed from four values).
    };


    /**
     * Returns a point which is at a fraction between two points a to b. This function uses SLERP.
     */
    point_type move_position_toward(const point_type& a, double fraction, const point_type& b) const 
    {
      return unit(a * pow( conj(a) * b, fraction ));
    };

    /**
     * Returns the difference between two points (analogous to a - b, but implemented in SO(3) Lie algebra).
     */
    point_difference_type difference(const point_type& a, const point_type& b) const {
      return T(2.0) * log(conj(b) * a);
    };

    /**
     * Returns the addition of a point-difference to a point.
     */
    point_type adjust(const point_type& a, const point_difference_type& delta) const {
      return a * exp( 0.5 * delta );
    };

    /**
     * Returns the origin of the space (the lower-limit).
     */
    point_type origin() const {
      return point_type(); //this just creates the "no-rotation" quaternion.
    };

    
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/
    
    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      ReaK::named_object::save(A,named_object::getStaticObjectType()->TypeVersion());
    };

    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      ReaK::named_object::load(A,named_object::getStaticObjectType()->TypeVersion());
    };

    RK_RTTI_MAKE_CONCRETE_1BASE(self,0xC240000C,1,"quaternion_topology",named_object)
    
};

/**
 * This class implements an angular velocity topology (for SO(3)). The angular velocities are constrained
 * to within a hyper-ball of a given maximum radius (max angular speed), this topology models 
 * the MetricSpaceConcept, and is bounded spherically (models BoundedSpaceConcept and SphereBoundedSpaceConcept).
 * \tparam T The value-type for the topology.
 */
template <typename T>
class ang_velocity_3D_topology : public hyperball_topology< vect<T,3>, mat<T, mat_structure::identity> > {
  public:
    typedef ang_velocity_3D_topology<T> self;
    typedef hyperball_topology< vect<T,3>, mat<T, mat_structure::identity> > base;
    
    /**
     * Default constructor.
     * \param aMaxAngSpeed The maximum (scalar) angular velocity that bounds this hyper-ball topology.
     */
    ang_velocity_3D_topology(const std::string& aName = "ang_velocity_3D_topology",
                             double aMaxAngSpeed = 1.0) : 
		             base(aName, 
			          vect<T,3>(), 
			          aMaxAngSpeed, 
			          mat<T, mat_structure::identity>(3)) { };
    
     
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/
    
    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      base::save(A,base::getStaticObjectType()->TypeVersion());
    };

    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      base::load(A,base::getStaticObjectType()->TypeVersion());
    };

    RK_RTTI_MAKE_CONCRETE_1BASE(self,0xC240000D,1,"ang_velocity_3D_topology",base)
    
};


/**
 * This class implements an angular acceleration topology (for SO(3)). The angular accelerations are constrained
 * to within a hyper-ball of a given maximum radius (max angular acceleration), this topology models 
 * the MetricSpaceConcept, and is bounded spherically (models BoundedSpaceConcept and SphereBoundedSpaceConcept).
 * \tparam T The value-type for the topology.
 */
template <typename T>
class ang_accel_3D_topology : public hyperball_topology< vect<T,3>, mat<T, mat_structure::identity> > {
  public:
    typedef ang_accel_3D_topology<T> self;
    typedef hyperball_topology< vect<T,3>, mat<T, mat_structure::identity> > base;
    
    /**
     * Default constructor.
     * \param aMaxAngSpeed The maximum (scalar) angular acceleration that bounds this hyper-ball topology.
     */
    ang_accel_3D_topology(const std::string& aName = "ang_accel_3D_topology",
                          double aMaxAngAcc = 1.0) : 
		          base(aName, 
			       vect<T,3>(), 
			       aMaxAngAcc, 
			       mat<T, mat_structure::identity>(3)) { };
    
     
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/
    
    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      base::save(A,base::getStaticObjectType()->TypeVersion());
    };

    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      base::load(A,base::getStaticObjectType()->TypeVersion());
    };

    RK_RTTI_MAKE_CONCRETE_1BASE(self,0xC240000E,1,"ang_accel_3D_topology",base)
    
};

/**
 * This meta-function defines the type for a 0th order SO(3) topology (a zero-differentiable space).
 * \tparam T The value type for the topology.
 * \tparam DistanceMetric The distance metric to apply to the tuple.
 */
template <typename T, typename DistanceMetric = euclidean_tuple_distance>
struct so3_0th_order_topology {
  typedef differentiable_space< time_topology, arithmetic_tuple< quaternion_topology<T> >, DistanceMetric > type;
};

/**
 * This meta-function defines the type for a 1st order SO(3) topology (a once-differentiable space).
 * \tparam T The value type for the topology.
 * \tparam DistanceMetric The distance metric to apply to the tuple.
 */
template <typename T, typename DistanceMetric = euclidean_tuple_distance>
struct so3_1st_order_topology {
  typedef differentiable_space< time_topology, arithmetic_tuple< quaternion_topology<T>, ang_velocity_3D_topology<T> >, DistanceMetric > type;
};

/**
 * This meta-function defines the type for a 2nd order SO(3) topology (a twice-differentiable space).
 * \tparam T The value type for the topology.
 * \tparam DistanceMetric The distance metric to apply to the tuple.
 */
template <typename T, typename DistanceMetric = euclidean_tuple_distance>
struct so3_2nd_order_topology {
  typedef differentiable_space< time_topology, arithmetic_tuple< quaternion_topology<T>, ang_velocity_3D_topology<T>, ang_accel_3D_topology<T> >, DistanceMetric > type;
};


};

};

#endif








