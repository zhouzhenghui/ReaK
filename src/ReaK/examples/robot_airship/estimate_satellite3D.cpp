
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


#include "satellite_invar_models.hpp"

#include "serialization/xml_archiver.hpp"
#include "recorders/ssv_recorder.hpp"


#include "ctrl_sys/kalman_filter.hpp"
#include "ctrl_sys/kalman_bucy_filter.hpp"
#include "ctrl_sys/invariant_kalman_filter.hpp"
#include "ctrl_sys/invariant_kalman_bucy_filter.hpp"
#include "ctrl_sys/unscented_kalman_filter.hpp"

#include "ctrl_sys/gaussian_belief_state.hpp"
#include "ctrl_sys/covariance_matrix.hpp"

#include "integrators/fixed_step_integrators.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

#include <boost/random/linear_congruential.hpp>
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>


#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;




struct sat3D_measurement_point {
  ReaK::vect_n<double> pose;
  ReaK::vect_n<double> gyro;
  ReaK::vect_n<double> IMU_a_m;
  ReaK::vect_n<double> u;
};


int main(int argc, char** argv) {
  using namespace ReaK;
  
  
  po::options_description generic_options("Generic options");
  generic_options.add_options()
    ("help,h", "produce this help message.")
  ;
  
  po::options_description io_options("I/O options");
  io_options.add_options()
    ("measurements,m", po::value< std::string >()->default_value("sim_results/satellite3D/output_record.ssv"), "specify the filename for the satellite's initial conditions (default is 'models/satellite3D_init.rkx')")
    ("init,i", po::value< std::string >()->default_value("models/satellite3D_init.rkx"), "specify the filename for the satellite's initial conditions, only used when Monte-Carlo simulations are done (default is 'models/satellite3D_init.rkx')")
    ("inertia,I", po::value< std::string >()->default_value("models/satellite3D_inertia.rkx"), "specify the filename for the satellite's inertial data (default is 'models/satellite3D_inertia.rkx')")
    ("Q-matrix,Q", po::value< std::string >()->default_value("models/satellite3D_Q.rkx"), "specify the filename for the satellite's input disturbance covariance matrix (default is 'models/satellite3D_Q.rkx')")
    ("R-matrix,R", po::value< std::string >()->default_value("models/satellite3D_R.rkx"), "specify the filename for the satellite's measurement noise covariance matrix (default is 'models/satellite3D_R.rkx')")
    ("R-added,A", "specify the filename for the satellite's artificial measurement noise covariance matrix")
    ("IMU-config", po::value< std::string >()->default_value("models/satellite3D_IMU_config.rkx"), "specify the filename for the satellite's IMU configuration data, specifying its placement on the satellite and the inertial / magnetic-field frame it is relative to (default is 'models/satellite3D_IMU_config.rkx')")
    ("output,o", po::value< std::string >()->default_value("est_results/satellite3D/output_record"), "specify the filename stem (without extension) for the output of the results (default is 'sim_results/satellite3D/output_record')")
  ;
  
  po::options_description sim_options("Simulation options");
  sim_options.add_options()
    ("start-time,s", po::value< double >()->default_value(0.0), "start time of the estimation (default is 0.0)")
    ("end-time,e", po::value< double >()->default_value(1.0), "end time of the estimation (default is 1.0)")
    ("time-step,t", po::value< double >()->default_value(0.01), "time-step in the measurement files (default is 0.01)")
    ("skips,S", po::value< unsigned int >()->default_value(1), "number of time-step skips between estimations (default is 1, i.e., one estimation point per measurement point)")
  ;
  
  po::options_description sim_options("Monte-Carlo options");
  sim_options.add_options()
    ("monte-carlo", "if set, will perform a Monte-Carlo set of randomized runs to gather estimation performance statistics")
    ("mc-runs", po::value< unsigned int >()->default_value(1000), "number of Monte-Carlo runs to perform (default is 1000)")
    ("min-skips", po::value< unsigned int >()->default_value(1), "minimum number of time-step skips between estimations when generating a series of Monte-Carlo statistics (default is 1, i.e., one estimation point per measurement point)")
    ("max-skips", po::value< unsigned int >()->default_value(1), "maximum number of time-step skips between estimations when generating a series of Monte-Carlo statistics (default is 1, i.e., one estimation point per measurement point)")
  ;
  
  po::options_description model_options("Modeling options");
  sim_options.add_options()
    ("gyro", "if set, a set of gyros is added to the model (angular velocity measurements). This requires the 'R-matrix' file to contain a 9x9 matrix.")
    ("IMU", "if set, a set of gyros is added to the model (angular velocity, magnetic field, and accelerometer measurements).\
 This requires the 'R-matrix' file to contain a 15x15 matrix. This option also automatically implies the 'midpoint' option.\
 This option will trigger the use of the 'IMU-config' file to obtain the information necessary about the IMU and the Earth's inertial frame.")
    ("mekf", "if set, results for the multiplicative extended Kalman filter (MEKF) will be generated.")
    ("iekf", "if set, results for the invariant extended Kalman filter (IEKF) will be generated.")
    ("imkf", "if set, results for the invariant momentum-tracking Kalman filter (IMKF) will be generated.")
    ("imkfv2", "if set, results for the invariant midpoint Kalman filter (IMKFv2) will be generated.")
  ;
  
  po::options_description output_options("Output options (at least one must be set)");
  output_options.add_options()
    ("xml,x", "if set, output results in XML format (rkx)")
    ("protobuf,p", "if set, output results in protobuf format (pbuf)")
    ("binary,b", "if set, output results in binary format (rkb)")
    ("ssv", "if set, output resulting trajectories as time-series in space-separated-values files (ssv) (easily loadable in matlab / octave / excel)")
  ;
  
  po::options_description cmdline_options;
  cmdline_options.add(generic_options).add(io_options).add(sim_options).add(model_options).add(output_options);
  
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
  po::notify(vm);
  
  
  
  std::string output_path_name = vm["output"].as<std::string>();
  std::string output_stem_name = output_path_name;
  if(output_stem_name[output_stem_name.size()-1] == '/')
    output_stem_name += "output_record";
  else {
    std::size_t p = output_path_name.find_last_of('/');
    if(p == std::string::npos)
      output_path_name = "";
    else
      output_path_name.erase(p);
  };
  while(output_path_name[output_path_name.length()-1] == '/') 
    output_path_name.erase(output_path_name.length()-1, 1);
  
  if(!output_path_name.empty())
    fs::create_directory(output_path_name.c_str());
  
  
  double start_time = vm["start-time"].as<double>();
  double end_time   = vm["end-time"].as<double>();
  double time_step  = vm["time-step"].as<double>();
  unsigned int skips  = vm["skips"].as<unsigned int>();
  
  unsigned int mc_runs    = vm["mc-runs"].as<unsigned int>();
  unsigned int min_skips  = vm["min-skips"].as<unsigned int>();
  unsigned int max_skips  = vm["max-skips"].as<unsigned int>();
  
  
  boost::variate_generator< boost::minstd_rand, boost::normal_distribution<double> > var_rnd(boost::minstd_rand(static_cast<unsigned int>(time(NULL))), boost::normal_distribution<double>());
  
  
  /* measurement file */
  std::string measurements_filename;
  if( ! vm.count("monte-carlo") ) {
    measurements_filename = vm["measurements"].as<std::string>();
    
    if( ! fs::exists( fs::path( measurements_filename ) ) ) {
      std::cout << "Measurements file does not exist!" << std::endl;
      return 3;
    };
  };
  
  
  /* initial states */
  frame_3D<double> initial_motion;
  if( vm.count("monte-carlo") ) {
    try {
      
      std::string init_filename = vm["init"].as<std::string>();
      
      if( ! fs::exists( fs::path( init_filename ) ) ) {
        std::cout << "Initial-conditions file does not exist!" << std::endl;
        return 3;
      };
      
      *(serialization::open_iarchive(init_filename))
        & RK_SERIAL_LOAD_WITH_NAME(initial_motion);
      
    } catch(...) {
      RK_ERROR("An exception occurred during the loading of the initial conditions!");
      return 11;
    };
  };
  
  
  /* inertial data */
  double mass = 1.0;
  ReaK::mat<double,ReaK::mat_structure::symmetric> inertia_tensor(1.0, 0.0, 0.0, 1.0, 0.0, 1.0);
  try {
    
    std::string inertia_filename = vm["inertia"].as<std::string>();
    
    if( ! fs::exists( fs::path( inertia_filename ) ) ) {
      std::cout << "Inertial-information file does not exist!" << std::endl;
      return 4;
    };
    
    *(serialization::open_iarchive(inertia_filename))
      & RK_SERIAL_LOAD_WITH_NAME(mass)
      & RK_SERIAL_LOAD_WITH_NAME(inertia_tensor);
    
  } catch(...) {
    RK_ERROR("An exception occurred during the loading of the initial conditions!");
    return 12;
  };
  
  
  /* input disturbance */
  mat<double,mat_structure::diagonal> input_disturbance(6,true);
  try {
    
    std::string Qu_filename = vm["Q-matrix"].as<std::string>();
    
    if( ! fs::exists( fs::path( Qu_filename ) ) ) {
      std::cout << "Input disturbance covariance matrix file does not exist!" << std::endl;
      return 5;
    };
    
    *(serialization::open_iarchive(Qu_filename))
      & RK_SERIAL_LOAD_WITH_NAME(input_disturbance);
    
  } catch(...) {
    RK_ERROR("An exception occurred during the loading of the input disturbance covariance matrix!");
    return 13;
  };
  
  
  /* measurement noise */
  std::size_t m_noise_size = 6;
  if( vm.count("gyro") )
    m_noise_size = 9;
  if( vm.count("IMU") )
    m_noise_size = 15;
  mat<double,mat_structure::diagonal> measurement_noise(m_noise_size,true);
  try {
    std::string R_filename  = vm["R-matrix"].as<std::string>();
    
    if( ! fs::exists( fs::path( R_filename ) ) ) {
      std::cout << "Measurement noise covariance matrix file does not exist!" << std::endl;
      return 6;
    };
    
    *(serialization::open_iarchive(R_filename))
      & RK_SERIAL_LOAD_WITH_NAME(measurement_noise);
    
  } catch(...) {
    RK_ERROR("An exception occurred during the loading of the measurement noise covariance matrix!");
    return 14;
  };
  double Rq0 = (measurement_noise(3,3) + measurement_noise(4,4) + measurement_noise(5,5)) / 12.0;
  
  
  /* artificial measurement noise */
  mat<double,mat_structure::diagonal> artificial_noise(m_noise_size,0.0);
  if( vm.count("R-added") ) {
    try {
      std::string R_added_filename  = vm["R-added"].as<std::string>();
      
      if( ! fs::exists( fs::path( R_added_filename ) ) ) {
        std::cout << "Artificial noise covariance matrix file does not exist!" << std::endl;
        return 6;
      };
      
      *(serialization::open_iarchive(R_filename))
        & RK_SERIAL_LOAD_WITH_NAME(artificial_noise);
      
    } catch(...) {
      RK_ERROR("An exception occurred during the loading of the artificial measurement noise covariance matrix!");
      return 3;
    };
  };
  double RAq0 = (artificial_noise(3,3) + artificial_noise(4,4) + artificial_noise(5,5)) / 12.0;
  
  
  /* IMU configuration data */
  unit_quat<double> IMU_orientation;
  vect<double,3> IMU_location;
  unit_quat<double> earth_orientation;
  vect<double,3> mag_field_direction(1.0,0.0,0.0);
  if( vm.count("IMU") ) {
    try {
      
      std::string IMUconf_filename  = vm["IMU-config"].as<std::string>();
      
      if( ! fs::exists( fs::path( IMUconf_filename ) ) ) {
        std::cout << "IMU configuration data file does not exist!" << std::endl;
        return 6;
      };
      
      *(serialization::open_iarchive(IMUconf_filename))
        & RK_SERIAL_LOAD_WITH_NAME(IMU_orientation)
        & RK_SERIAL_LOAD_WITH_NAME(IMU_location)
        & RK_SERIAL_LOAD_WITH_NAME(earth_orientation)
        & RK_SERIAL_LOAD_WITH_NAME(mag_field_direction);
      
    } catch(...) {
      RK_ERROR("An exception occurred during the loading of the measurement noise covariance matrix!");
      return 14;
    };
  };
  
  
  
  std::list< std::pair< double, sat3D_measurement_point > > measurements;
  std::list< std::pair< double, sat3D_measurement_point > > measurements_noisy;
  std::list< std::pair< double, vect_n<double> > > ground_truth;
  try {
    recorder::ssv_extractor measurements_file(measurements_filename);
    unsigned int j = 0;
    while(true) {
      double t;
      measurements_file >> t;
      std::vector<double> meas;
      try {
        while(true) {
          double dummy;
          measurements_file >> dummy;
          meas.push_back(dummy);
        };
      } catch(recorder::out_of_bounds&) { };
      measurements_file >> recorder::data_extractor::end_value_row;
      
      if(j == 0) {
        sat3D_measurement_point meas_actual, meas_noisy;
        
        /* read off the position-orientation measurements. */
        if(meas.size() < 7) {
          RK_ERROR("The measurement file does not even contain the position and quaternion measurements!");
          return 4;
        };
        meas_actual.pose = vect_n<double>(meas.begin(), meas.begin() + 7);
        meas_noisy.pose = meas_actual.pose;
        if( vm.count("R-added") ) {
          meas_noisy.pose += vect_n<double>(
            var_rnd() * sqrt(artificial_noise(0,0)),
            var_rnd() * sqrt(artificial_noise(1,1)),
            var_rnd() * sqrt(artificial_noise(2,2)),
            var_rnd() * sqrt(RAq0),
            var_rnd() * sqrt(0.25 * artificial_noise(3,3)),
            var_rnd() * sqrt(0.25 * artificial_noise(4,4)),
            var_rnd() * sqrt(0.25 * artificial_noise(5,5))
          );
        };
        meas.erase(meas.begin(), meas.begin() + 7);
        
        /* read off the IMU/gyro angular velocity measurements. */
        if( vm.count("gyro") || vm.count("IMU") ) {
          if(meas.size() < 3) {
            RK_ERROR("The measurement file does not contain the angular velocity measurements!");
            return 4;
          };
          meas_actual.gyro = vect_n<double>(meas.begin(), meas.begin() + 3);
          meas_noisy.gyro  = meas_actual.gyro;
          if( vm.count("R-added") && (artificial_noise.get_row_count() >= 9) ) {
            meas_noisy.gyro += vect_n<double>(
              var_rnd() * sqrt(artificial_noise(6,6)),
              var_rnd() * sqrt(artificial_noise(7,7)),
              var_rnd() * sqrt(artificial_noise(8,8))
            );
          };
          meas.erase(meas.begin(), meas.begin() + 3);
        };
        
        /* read off the IMU accel-mag measurements. */
        if( vm.count("IMU") ) {
          if(meas.size() < 6) {
            RK_ERROR("The measurement file does not contain the accelerometer and magnetometer measurements!");
            return 4;
          };
          meas_actual.IMU_a_m = vect_n<double>(meas.begin(), meas.begin() + 6);
          meas_noisy.IMU_a_m  = meas_actual.IMU_a_m;
          if( vm.count("R-added") && (artificial_noise.get_row_count() >= 15) ) {
            meas_noisy.IMU_a_m += vect_n<double>(
              var_rnd() * sqrt(artificial_noise( 9, 9)),
              var_rnd() * sqrt(artificial_noise(10,10)),
              var_rnd() * sqrt(artificial_noise(11,11)),
              var_rnd() * sqrt(artificial_noise(12,12)),
              var_rnd() * sqrt(artificial_noise(13,13)),
              var_rnd() * sqrt(artificial_noise(14,14))
            );
          };
          meas.erase(meas.begin(), meas.begin() + 6);
        };
        
        /* read off the input vector. */
        if(meas.size() < 6) {
          RK_ERROR("The measurement file does not contain the input force-torque vector measurements!");
          return 4;
        };
        meas_actual.u = vect_n<double>(meas.begin(), meas.begin() + 6);
        meas_noisy.u  = meas_actual.u;
        meas.erase(meas.begin(), meas.begin() + 6);
        
        /* now, the meas_actual and meas_noisy are fully formed. */
        measurements.push_back( std::make_pair(t, meas_actual) );
        measurements_noisy.push_back( std::make_pair(t, meas_noisy) );
        
        /* check if the file contains a ground-truth: */
        if(meas.size() >= 13) {
          ground_truth.push_back( std::make_pair(t, vect_n<double>(meas.begin(), meas.begin() + 13)) );
          meas.erase(meas.begin(), meas.begin() + 13);
        };
        
      };
      j = (j+1) % skips;
    };
  } catch(recorder::out_of_bounds&) {
    RK_ERROR("The measurement file does not appear to have the required number of columns!");
    return 4;
  } catch(recorder::end_of_record&) { };
  
  
  // Create the set of satellite3D systems:
  
  // linearized systems: (still multiplicative, since the state-space takes care of state-vector operations)
  ctrl::satellite3D_lin_dt_system sat3D_lin(
    "satellite3D_lin", mass, inertia_tensor, time_step);
  
  ctrl::satellite3D_gyro_lin_dt_system sat3D_lin_gyro(
    "satellite3D_lin_with_gyros", mass, inertia_tensor, time_step);
  
  
  // invariant systems:
  ctrl::satellite3D_inv_dt_system sat3D_inv(
    "satellite3D_inv", mass, inertia_tensor, time_step);
  
  ctrl::satellite3D_gyro_inv_dt_system sat3D_inv_gyro(
    "satellite3D_inv_with_gyros", mass, inertia_tensor, time_step);
  
  
  // invariant-momemtum-tracking systems (order = 1):
  ctrl::satellite3D_imdt_sys sat3D_invmom(
    "satellite3D_invmom", mass, inertia_tensor, time_step);
  
  ctrl::satellite3D_gyro_imdt_sys sat3D_invmom_gyro(
    "satellite3D_invmom_with_gyros", mass, inertia_tensor, time_step);
  
  ctrl::satellite3D_IMU_imdt_sys sat3D_invmom_IMU(
    "satellite3D_invmom_with_IMU", mass, inertia_tensor, time_step,
    IMU_orientation, IMU_location, earth_orientation, mag_field_direction);
  
  
  // invariant-momemtum-tracking systems (order = 2 (midpoint)):
  ctrl::satellite3D_imdt_sys sat3D_invmid(
    "satellite3D_invmid", mass, inertia_tensor, time_step, 2);
  
  ctrl::satellite3D_gyro_imdt_sys sat3D_invmid_gyro(
    "satellite3D_invmid_with_gyros", mass, inertia_tensor, time_step, 2);
  
  ctrl::satellite3D_IMU_imdt_sys sat3D_invmid_IMU(
    "satellite3D_invmid_with_IMU", mass, inertia_tensor, time_step,
    IMU_orientation, IMU_location, earth_orientation, mag_field_direction, 2);
  
  
  typedef ctrl::satellite3D_lin_dt_system::state_space_type      sat3D_state_space_type;
  typedef ctrl::satellite3D_lin_dt_system::point_type            sat3D_state_type;
  typedef ctrl::satellite3D_lin_dt_system::point_difference_type sat3D_state_diff_type;
  typedef ctrl::satellite3D_lin_dt_system::input_type            sat3D_input_type;
  typedef ctrl::satellite3D_lin_dt_system::output_type           sat3D_output_type;
  
  typedef pp::temporal_space<sat3D_state_space_type, pp::time_poisson_topology, pp::time_distance_only> sat3D_temp_space_type;
  typedef pp::topology_traits< sat3D_temp_space_type >::point_type temp_point_type;
  
  sat3D_temp_space_type sat_space(
    "satellite3D_temporal_space",
    sat3D_state_space_type(),
    pp::time_poisson_topology("satellite3D_time_space", time_step, (end_time - start_time) * 0.5));
  
//   typedef pp::discrete_point_trajectory< sat3D_temp_space_type > sat_traj_type;
  
  
  typedef ctrl::covariance_matrix< vect_n<double> > cov_type;
  typedef cov_type::matrix_type cov_matrix_type;
  typedef ctrl::gaussian_belief_state< sat3D_state_type,  cov_type > sat3D_state_belief_type;
  typedef ctrl::gaussian_belief_state< sat3D_input_type,  cov_type > sat3D_input_belief_type;
  typedef ctrl::gaussian_belief_state< sat3D_output_type, cov_type > sat3D_output_belief_type;
  
  
  sat3D_state_type x_init;
  set_position(x_init, vect<double,3>(0.0, 0.0, 0.0));
  set_velocity(x_init, vect<double,3>(0.0, 0.0, 0.0));
  set_quaternion(x_init, quaternion<double>());
  set_ang_velocity(x_init, vect<double,3>(0.0, 0.0, 0.0));
  
  sat3D_state_belief_type b_init(x_init, 
                                 cov_type(cov_matrix_type(mat<double,mat_structure::diagonal>(13,10.0))));
  
  sat3D_input_belief_type  b_u(sat3D_input_type(vect_n<double>(6, 0.0)),  
                               cov_type(cov_matrix_type(input_disturbance)));
  
  sat3D_output_belief_type b_z(sat3D_output_type(vect_n<double>(6, 0.0)), 
                               cov_type(cov_matrix_type(measurement_noise + artificial_noise)));
  
  boost::posix_time::ptime t1;
  boost::posix_time::time_duration dt[4];
  
  
  
  std::cout << "Running Extended Kalman Filter..." << std::endl;
  {
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b = b_init;
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_u(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               ctrl::covariance_matrix< vect_n<double> >(Qu));
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_z(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               Rcov);
  recorder::ssv_recorder results(result_filename + "_ekf.ssv");
  results << "time" << "pos_x" << "pos_y" << "pos_z" << "q0" << "q1" << "q2" << "q3" 
                    << "ep_x"  << "ep_y"  << "ep_z"  << "ea_x" << "ea_y" << "ea_z"
                    << "P_xx" << "P_yy" << "P_zz" << "P_aax" << "P_aay" << "P_aaz" << recorder::data_recorder::end_name_row;
  t1 = boost::posix_time::microsec_clock::local_time();
  std::list< std::pair< double, vect_n<double> > >::iterator it_orig = measurements.begin();
  for(std::list< std::pair< double, vect_n<double> > >::iterator it = measurements_noisy.begin(); it != measurements_noisy.end(); ++it, ++it_orig) {
     
    b_z.set_mean_state(it->second);
    ctrl::kalman_filter_step(mdl_lin_dt,mdl_state_space,b,b_u,b_z,it->first);
    
    vect_n<double> b_mean = b.get_mean_state();
    quaternion<double> q_mean(vect<double,4>(b_mean[3],b_mean[4],b_mean[5],b_mean[6]));
    b_mean[3] = q_mean[0]; b_mean[4] = q_mean[1]; b_mean[5] = q_mean[2]; b_mean[6] = q_mean[3];
    b.set_mean_state(b_mean);
    
    quaternion<double> q_orig(vect<double,4>(it_orig->second[0],it_orig->second[1],it_orig->second[2],it_orig->second[3]));
    axis_angle<double> aa_diff = axis_angle<double>(invert(q_mean) * q_orig);
    
    const vect_n<double>& x_mean = b.get_mean_state();
    results << it->first << x_mean[0] << x_mean[1] << x_mean[2] 
                         << x_mean[3] << x_mean[4] << x_mean[5] << x_mean[6]
                         << (x_mean[0] - it_orig->second[4])
                         << (x_mean[1] - it_orig->second[5])
                         << (x_mean[2] - it_orig->second[6])
                         << (aa_diff.angle() * aa_diff.axis()[0])
                         << (aa_diff.angle() * aa_diff.axis()[1])
                         << (aa_diff.angle() * aa_diff.axis()[2])
                         << b.get_covariance().get_matrix()(0,0)
                         << b.get_covariance().get_matrix()(1,1)
                         << b.get_covariance().get_matrix()(2,2)
                         << 4 * b.get_covariance().get_matrix()(4,4)
                         << 4 * b.get_covariance().get_matrix()(5,5)
                         << 4 * b.get_covariance().get_matrix()(6,6)
                         << recorder::data_recorder::end_value_row;
    
  };
  results << recorder::data_recorder::flush;
  dt[0] = boost::posix_time::microsec_clock::local_time() - t1;
  };
  std::cout << "Done." << std::endl;

  
  

  
  std::cout << "Running Invariant Extended Kalman Filter..." << std::endl;
  {
    
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > 
    b(b_init.get_mean_state(),
      ctrl::covariance_matrix< vect_n<double> >(ctrl::covariance_matrix< vect_n<double> >::matrix_type(mat<double,mat_structure::diagonal>(12,10.0))));
  
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_u(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               ctrl::covariance_matrix< vect_n<double> >(Qu));
  
  mat<double,mat_structure::diagonal> R_inv(6);
  R_inv(0,0) = R(0,0); R_inv(1,1) = R(1,1); R_inv(2,2) = R(2,2);
  R_inv(3,3) = 4*R(4,4); R_inv(4,4) = 4*R(5,5); R_inv(5,5) = 4*R(6,6);
  ctrl::covariance_matrix< vect_n<double> > Rcovinv = ctrl::covariance_matrix< vect_n<double> >(ctrl::covariance_matrix< vect_n<double> >::matrix_type(R_inv));
  
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_z(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               Rcovinv);
  
  recorder::ssv_recorder results(result_filename + "_iekf.ssv");
  results << "time" << "pos_x" << "pos_y" << "pos_z" << "q0" << "q1" << "q2" << "q3" 
                    << "ep_x"  << "ep_y"  << "ep_z"  << "ea_x" << "ea_y" << "ea_z"
                    << "P_xx" << "P_yy" << "P_zz" << "P_aax" << "P_aay" << "P_aaz" << recorder::data_recorder::end_name_row;
  t1 = boost::posix_time::microsec_clock::local_time();
  std::list< std::pair< double, vect_n<double> > >::iterator it_orig = measurements.begin();
  for(std::list< std::pair< double, vect_n<double> > >::iterator it = measurements_noisy.begin(); it != measurements_noisy.end(); ++it, ++it_orig) {
    
    b_z.set_mean_state(it->second);
    ctrl::invariant_kalman_filter_step(mdl_inv_dt,mdl_state_space,b,b_u,b_z,it->first);
    
    vect_n<double> b_mean = b.get_mean_state();
    quaternion<double> q_mean(vect<double,4>(b_mean[3],b_mean[4],b_mean[5],b_mean[6]));
    b_mean[3] = q_mean[0]; b_mean[4] = q_mean[1]; b_mean[5] = q_mean[2]; b_mean[6] = q_mean[3];
    b.set_mean_state(b_mean);
    
    quaternion<double> q_orig(vect<double,4>(it_orig->second[0],it_orig->second[1],it_orig->second[2],it_orig->second[3]));
    axis_angle<double> aa_diff = axis_angle<double>(invert(q_mean) * q_orig);
    
    const vect_n<double>& x_mean = b.get_mean_state();
    results << it->first << x_mean[0] << x_mean[1] << x_mean[2] 
                         << x_mean[3] << x_mean[4] << x_mean[5] << x_mean[6]
                         << (x_mean[0] - it_orig->second[4])
                         << (x_mean[1] - it_orig->second[5])
                         << (x_mean[2] - it_orig->second[6])
                         << (aa_diff.angle() * aa_diff.axis()[0])
                         << (aa_diff.angle() * aa_diff.axis()[1])
                         << (aa_diff.angle() * aa_diff.axis()[2])
                         << b.get_covariance().get_matrix()(0,0)
                         << b.get_covariance().get_matrix()(1,1)
                         << b.get_covariance().get_matrix()(2,2)
                         << b.get_covariance().get_matrix()(3,3)
                         << b.get_covariance().get_matrix()(4,4)
                         << b.get_covariance().get_matrix()(5,5)
                         << recorder::data_recorder::end_value_row;
    
  };
  results << recorder::data_recorder::flush;
  dt[1] = boost::posix_time::microsec_clock::local_time() - t1;
  };
  std::cout << "Done." << std::endl;

  
  
  

  
  std::cout << "Running Invariant-Momentum Kalman Filter..." << std::endl;
  {
    
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > 
    b(b_init.get_mean_state(),
      ctrl::covariance_matrix< vect_n<double> >(ctrl::covariance_matrix< vect_n<double> >::matrix_type(mat<double,mat_structure::diagonal>(12,10.0))));
  
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_u(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               ctrl::covariance_matrix< vect_n<double> >(Qu));
  
  mat<double,mat_structure::diagonal> R_inv(6);
  R_inv(0,0) = R(0,0); R_inv(1,1) = R(1,1); R_inv(2,2) = R(2,2);
  R_inv(3,3) = 4*R(4,4); R_inv(4,4) = 4*R(5,5); R_inv(5,5) = 4*R(6,6);
  ctrl::covariance_matrix< vect_n<double> > Rcovinv = ctrl::covariance_matrix< vect_n<double> >(ctrl::covariance_matrix< vect_n<double> >::matrix_type(R_inv));
  
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_z(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               Rcovinv);
  
  recorder::ssv_recorder results(result_filename + "_imkf.ssv");
  results << "time" << "pos_x" << "pos_y" << "pos_z" << "q0" << "q1" << "q2" << "q3" 
                    << "ep_x"  << "ep_y"  << "ep_z"  << "ea_x" << "ea_y" << "ea_z"
                    << "P_xx" << "P_yy" << "P_zz" << "P_aax" << "P_aay" << "P_aaz" << recorder::data_recorder::end_name_row;
  t1 = boost::posix_time::microsec_clock::local_time();
  std::list< std::pair< double, vect_n<double> > >::iterator it_orig = measurements.begin();
  for(std::list< std::pair< double, vect_n<double> > >::iterator it = measurements_noisy.begin(); it != measurements_noisy.end(); ++it, ++it_orig) {
    
    b_z.set_mean_state(it->second);
    ctrl::invariant_kalman_filter_step(mdl_inv_mom_dt,mdl_state_space,b,b_u,b_z,it->first);
    
    vect_n<double> b_mean = b.get_mean_state();
    quaternion<double> q_mean(vect<double,4>(b_mean[3],b_mean[4],b_mean[5],b_mean[6]));
    b_mean[3] = q_mean[0]; b_mean[4] = q_mean[1]; b_mean[5] = q_mean[2]; b_mean[6] = q_mean[3];
    b.set_mean_state(b_mean);
    
    quaternion<double> q_orig(vect<double,4>(it_orig->second[0],it_orig->second[1],it_orig->second[2],it_orig->second[3]));
    axis_angle<double> aa_diff = axis_angle<double>(invert(q_mean) * q_orig);
    
    const vect_n<double>& x_mean = b.get_mean_state();
    results << it->first << x_mean[0] << x_mean[1] << x_mean[2] 
                         << x_mean[3] << x_mean[4] << x_mean[5] << x_mean[6] 
                         << (x_mean[0] - it_orig->second[4])
                         << (x_mean[1] - it_orig->second[5])
                         << (x_mean[2] - it_orig->second[6])
                         << (aa_diff.angle() * aa_diff.axis()[0])
                         << (aa_diff.angle() * aa_diff.axis()[1])
                         << (aa_diff.angle() * aa_diff.axis()[2])
                         << b.get_covariance().get_matrix()(0,0)
                         << b.get_covariance().get_matrix()(1,1)
                         << b.get_covariance().get_matrix()(2,2)
                         << b.get_covariance().get_matrix()(3,3)
                         << b.get_covariance().get_matrix()(4,4)
                         << b.get_covariance().get_matrix()(5,5)
                         << recorder::data_recorder::end_value_row;
    
  };
  results << recorder::data_recorder::flush;
  dt[2] = boost::posix_time::microsec_clock::local_time() - t1;
  };
  std::cout << "Done." << std::endl;
  
  
  
  

  
  std::cout << "Running Invariant-Midpoint Kalman Filter..." << std::endl;
  {
    
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > 
    b(b_init.get_mean_state(),
      ctrl::covariance_matrix< vect_n<double> >(ctrl::covariance_matrix< vect_n<double> >::matrix_type(mat<double,mat_structure::diagonal>(12,10.0))));
  
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_u(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               ctrl::covariance_matrix< vect_n<double> >(Qu));
  
  mat<double,mat_structure::diagonal> R_inv(6);
  R_inv(0,0) = R(0,0); R_inv(1,1) = R(1,1); R_inv(2,2) = R(2,2);
  R_inv(3,3) = 4*R(4,4); R_inv(4,4) = 4*R(5,5); R_inv(5,5) = 4*R(6,6);
  ctrl::covariance_matrix< vect_n<double> > Rcovinv = ctrl::covariance_matrix< vect_n<double> >(ctrl::covariance_matrix< vect_n<double> >::matrix_type(R_inv));
  
  ctrl::gaussian_belief_state< vect_n<double>, ctrl::covariance_matrix< vect_n<double> > > b_z(vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0,0.0), 
                                                                                               Rcovinv);
  
  recorder::ssv_recorder results(result_filename + "_imkfv2.ssv");
  results << "time" << "pos_x" << "pos_y" << "pos_z" << "q0" << "q1" << "q2" << "q3" 
                    << "ep_x"  << "ep_y"  << "ep_z"  << "ea_x" << "ea_y" << "ea_z"
                    << "P_xx" << "P_yy" << "P_zz" << "P_aax" << "P_aay" << "P_aaz" << recorder::data_recorder::end_name_row;
  t1 = boost::posix_time::microsec_clock::local_time();
  std::list< std::pair< double, vect_n<double> > >::iterator it_orig = measurements.begin();
  for(std::list< std::pair< double, vect_n<double> > >::iterator it = measurements_noisy.begin(); it != measurements_noisy.end(); ++it, ++it_orig) {
    
    b_z.set_mean_state(it->second);
    ctrl::invariant_kalman_filter_step(mdl_inv_mid_dt,mdl_state_space,b,b_u,b_z,it->first);
    
    vect_n<double> b_mean = b.get_mean_state();
    quaternion<double> q_mean(vect<double,4>(b_mean[3],b_mean[4],b_mean[5],b_mean[6]));
    b_mean[3] = q_mean[0]; b_mean[4] = q_mean[1]; b_mean[5] = q_mean[2]; b_mean[6] = q_mean[3];
    b.set_mean_state(b_mean);
    
    quaternion<double> q_orig(vect<double,4>(it_orig->second[0],it_orig->second[1],it_orig->second[2],it_orig->second[3]));
    axis_angle<double> aa_diff = axis_angle<double>(invert(q_mean) * q_orig);
    
    const vect_n<double>& x_mean = b.get_mean_state();
    results << it->first << x_mean[0] << x_mean[1] << x_mean[2] 
                         << x_mean[3] << x_mean[4] << x_mean[5] << x_mean[6] 
                         << (x_mean[0] - it_orig->second[4])
                         << (x_mean[1] - it_orig->second[5])
                         << (x_mean[2] - it_orig->second[6])
                         << (aa_diff.angle() * aa_diff.axis()[0])
                         << (aa_diff.angle() * aa_diff.axis()[1])
                         << (aa_diff.angle() * aa_diff.axis()[2])
                         << b.get_covariance().get_matrix()(0,0)
                         << b.get_covariance().get_matrix()(1,1)
                         << b.get_covariance().get_matrix()(2,2)
                         << b.get_covariance().get_matrix()(3,3)
                         << b.get_covariance().get_matrix()(4,4)
                         << b.get_covariance().get_matrix()(5,5)
                         << recorder::data_recorder::end_value_row;
    
  };
  results << recorder::data_recorder::flush;
  dt[3] = boost::posix_time::microsec_clock::local_time() - t1;
  };
  std::cout << "Done." << std::endl;
  
  
  
  {
  recorder::ssv_recorder results(result_filename + "_times.ssv");
  results << "step_count" << "ekf(ms)" << "iekf(ms)" << "imkfv1(ms)" << "imkfv2(ms)" << recorder::data_recorder::end_name_row;
  results << double(measurements_noisy.size()) << double(dt[0].total_milliseconds())
                                               << double(dt[1].total_milliseconds())
                                               << double(dt[2].total_milliseconds())
                                               << double(dt[3].total_milliseconds())
                                               << recorder::data_recorder::end_value_row << recorder::data_recorder::flush;
  };
  
  
};





#if 0
  std::cout << "Running Kalman-Bucy Filter..." << std::endl;
  {
  ctrl::gaussian_belief_state< ctrl::covariance_matrix<double> > b = b_init;
  recorder::ssv_recorder results(result_filename + "_kbf.ssv");
  results << "time" << "pos_x" << "pos_y" << "pos_z" << "q0" << "q1" << "q2" << "q3" << recorder::data_recorder::end_name_row;
  t1 = boost::posix_time::microsec_clock::local_time();
  for(std::list< std::pair< double, vect_n<double> > >::iterator it = measurements.begin(); it != measurements.end(); ++it) {
    ctrl::airship3D_lin_system::matrixA_type A;
    ctrl::airship3D_lin_system::matrixB_type B;
    ctrl::airship3D_lin_system::matrixC_type C;
    ctrl::airship3D_lin_system::matrixD_type D;
    mdl_lin.get_linear_blocks(A,B,C,D,it->first,b.get_mean_state(),vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0));
    ctrl::covariance_matrix<double> Qcov(ctrl::covariance_matrix<double>::matrix_type( B * Qu * transpose(B) ));
    
    ctrl::kalman_bucy_filter_step(mdl_lin,integ,b,vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0),it->second,Qcov,Rcov,time_step,it->first);
    
    const vect_n<double>& x_mean = b.get_mean_state();
    results << it->first << x_mean[0] << x_mean[1] << x_mean[2] 
                         << x_mean[3] << x_mean[4] << x_mean[5] << x_mean[6] << recorder::data_recorder::end_value_row;
    
  };
  results << recorder::data_recorder::flush;
  dt[0] = boost::posix_time::microsec_clock::local_time() - t1;
  };
  std::cout << "Done." << std::endl;
#endif
  
#if 0
  integ.setStepSize(0.0001 * time_step);
  std::cout << "Running Invariant Kalman-Bucy Filter..." << std::endl;
  {
  
  ctrl::gaussian_belief_state< ctrl::covariance_matrix<double> > 
    b(b_init.get_mean_state(),
      ctrl::covariance_matrix<double>(ctrl::covariance_matrix<double>::matrix_type(mat<double,mat_structure::diagonal>(12,10.0))));
    
  ctrl::covariance_matrix<double> RcovInvar = ctrl::covariance_matrix<double>(ctrl::covariance_matrix<double>::matrix_type( mat_const_sub_sym_block< mat<double,mat_structure::diagonal> >(R,6,0) ));
  recorder::ssv_recorder results(result_filename + "_ikbf.ssv");
  results << "time" << "pos_x" << "pos_y" << "pos_z" << "q0" << "q1" << "q2" << "q3" << recorder::data_recorder::end_name_row;
  t1 = boost::posix_time::microsec_clock::local_time();
  for(std::list< std::pair< double, vect_n<double> > >::iterator it = measurements.begin(); it != measurements.end(); ++it) {
    ctrl::airship3D_inv_system::matrixA_type A;
    ctrl::airship3D_inv_system::matrixB_type B;
    ctrl::airship3D_inv_system::matrixC_type C;
    ctrl::airship3D_inv_system::matrixD_type D;
    mdl_inv.get_linear_blocks(A,B,C,D,it->first,b.get_mean_state(),vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0));
    ctrl::covariance_matrix<double> Qcov(ctrl::covariance_matrix<double>::matrix_type( B * Qu * transpose(B) ));
    
    ctrl::invariant_kalman_bucy_filter_step(mdl_inv,integ,b,vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0),it->second,Qcov,RcovInvar,time_step,it->first);
    
    vect_n<double> b_mean = b.get_mean_state();
    quaternion<double> q_mean(vect<double,4>(b_mean[3],b_mean[4],b_mean[5],b_mean[6]));
    b_mean[3] = q_mean[0]; b_mean[4] = q_mean[1]; b_mean[5] = q_mean[2]; b_mean[6] = q_mean[3];
    b.set_mean_state(b_mean);
    
    const vect_n<double>& x_mean = b.get_mean_state();
    results << it->first << x_mean[0] << x_mean[1] << x_mean[2] 
                         << x_mean[3] << x_mean[4] << x_mean[5] << x_mean[6] << recorder::data_recorder::end_value_row;

  };
  results << recorder::data_recorder::flush;
  dt[1] = boost::posix_time::microsec_clock::local_time() - t1;
  };
  std::cout << "Done." << std::endl;
#endif
  
#if 0
  std::cout << "Running Unscented Kalman Filter..." << std::endl;
  {
  ctrl::gaussian_belief_state< ctrl::covariance_matrix<double> > b = b_init;
  recorder::ssv_recorder results(result_filename + "_ukf.ssv");
  results << "time" << "pos_x" << "pos_y" << "pos_z" << "q0" << "q1" << "q2" << "q3" << recorder::data_recorder::end_name_row;
  t1 = boost::posix_time::microsec_clock::local_time();
  for(std::list< std::pair< double, vect_n<double> > >::iterator it = measurements.begin(); it != measurements.end(); ++it) {
    ctrl::airship3D_lin_dt_system::matrixA_type A;
    ctrl::airship3D_lin_dt_system::matrixB_type B;
    ctrl::airship3D_lin_dt_system::matrixC_type C;
    ctrl::airship3D_lin_dt_system::matrixD_type D;
    mdl_lin_dt.get_linear_blocks(A,B,C,D,it->first,b.get_mean_state(),vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0));
    ctrl::covariance_matrix<double> Qcov(ctrl::covariance_matrix<double>::matrix_type( B * Qu * transpose(B) ));
    
    try {
      ctrl::unscented_kalman_filter_step(mdl_lin_dt,b,vect_n<double>(0.0,0.0,0.0,0.0,0.0,0.0),it->second,Qcov,Rcov,it->first);
    } catch(singularity_error& e) {
      RK_ERROR("The Unscented Kalman filtering was interupted by a singularity, at time " << it->first << " with message: " << e.what());
      break;
    } catch(std::exception& e) {
      RK_ERROR("The Unscented Kalman filtering was interupted by an exception, at time " << it->first << " with message: " << e.what());   
      break;
    };
    
    vect_n<double> b_mean = b.get_mean_state();
    quaternion<double> q_mean(vect<double,4>(b_mean[3],b_mean[4],b_mean[5],b_mean[6]));
    b_mean[3] = q_mean[0]; b_mean[4] = q_mean[1]; b_mean[5] = q_mean[2]; b_mean[6] = q_mean[3];
    b.set_mean_state(b_mean);
    
    const vect_n<double>& x_mean = b.get_mean_state();
    results << it->first << x_mean[0] << x_mean[1] << x_mean[2] 
                         << x_mean[3] << x_mean[4] << x_mean[5] << x_mean[6] << recorder::data_recorder::end_value_row;
    
  };
  results << recorder::data_recorder::flush;
  dt[3] = boost::posix_time::microsec_clock::local_time() - t1;
  };
  std::cout << "Done." << std::endl;
#endif




