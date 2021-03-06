/**
 * \file quadrotor_scene.cpp
 *
 * This application 
 *
 * \author Mikael Persson, <mikael.s.persson@gmail.com>
 * \date February 2013
 */


#include "IHAQR_topology.hpp"
#include "MEAQR_topology.hpp"
#include <ReaK/ctrl/ss_systems/quadrotor_system.hpp>

#include <ReaK/ctrl/topologies/se3_random_samplers.hpp>

#include <ReaK/core/serialization/xml_archiver.hpp>

int main(int argc, char ** argv) {
  using namespace ReaK;
  using namespace serialization;
  using namespace pp;
  using namespace ctrl;
  
  typedef IHAQR_topology< quadrotor_system::state_space_type, quadrotor_system, position_only_sampler > IHAQR_space_type;
  typedef MEAQR_topology< quadrotor_system::state_space_type, quadrotor_system, position_only_sampler > MEAQR_space_type;
  
  shared_ptr< quadrotor_system > quad_sys;
  shared_ptr< IHAQR_space_type > quad_space;
  shared_ptr< MEAQR_space_type > quad_MEAQR_space;
  
  
  xml_iarchive file_in("models/quadrotor_spaces.xml");
  
  file_in >> quad_sys >> quad_space >> quad_MEAQR_space;
  
  
  {
//     IHAQR_space_type::point_type p1(make_arithmetic_tuple(make_arithmetic_tuple(
//         vect<double,3>(0.0,0.0,-1.0),
//         vect<double,3>(0.0,0.0,0.0)
//       ),
//       make_arithmetic_tuple(
//         unit_quat<double>(1.0,0.0,0.0,0.0),
//         vect<double,3>(0.0,0.0,0.0)
//       )));
//     IHAQR_space_type::point_type p2(make_arithmetic_tuple(make_arithmetic_tuple(
//         vect<double,3>(0.3,0.0,-1.2),
//         vect<double,3>(0.0,0.0,0.0)
//       ),
//       make_arithmetic_tuple(
//         unit_quat<double>(1.0,0.0,0.0,0.0),
//         vect<double,3>(0.0,0.0,0.0)
//       )));
//     
//     std::cout << " p1 = " << p1.x << std::endl;
//     std::cout << " p2 = " << p2.x << std::endl << std::endl;
//     
//     IHAQR_space_type::point_type p_inter = quad_space->move_position_toward(p1, 1.0, p2);
//     std::cout << " steer 1 = " << p_inter.x << std::endl;
//     p_inter = quad_space->move_position_toward(p1, 1.0, p_inter);
//     std::cout << " steer 2 = " << p_inter.x << std::endl;
//     p_inter = quad_space->move_position_toward(p1, 1.0, p_inter);
//     std::cout << " steer 3 = " << p_inter.x << std::endl;
    
  }; 
  
  {
    for(unsigned int i = 0; i < 10; ++i) {
      MEAQR_space_type::point_type p_rnd = quad_MEAQR_space->random_point();
      std::cout << " p_rnd = " << p_rnd.x << std::endl;
    };
  };
  
  {
    MEAQR_space_type::point_type p1(make_arithmetic_tuple(make_arithmetic_tuple(
        vect<double,3>(0.0,0.0,-1.0),
        vect<double,3>(0.0,0.0,0.0)
      ),
      make_arithmetic_tuple(
        unit_quat<double>(1.0,0.0,0.0,0.0),
        vect<double,3>(0.0,0.0,0.0)
      )));
    MEAQR_space_type::point_type p2(make_arithmetic_tuple(make_arithmetic_tuple(
        vect<double,3>(0.3,0.0,-1.3),
        vect<double,3>(0.0,0.0,0.0)
      ),
      make_arithmetic_tuple(
        unit_quat<double>(1.0,0.0,0.0,0.0),
        vect<double,3>(0.0,0.0,0.0)
      )));
    
    std::cout << " p1 = " << p1.x << std::endl;
    std::cout << " p2 = " << p2.x << std::endl << std::endl;
    
    MEAQR_space_type::point_type p_inter = quad_MEAQR_space->move_position_toward(p1, 0.8, p2);
    p_inter = quad_MEAQR_space->move_position_toward(p1, 1.0, p_inter);
  };
  
  return 0;
};







