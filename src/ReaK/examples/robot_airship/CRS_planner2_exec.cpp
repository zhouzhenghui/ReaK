
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

#include "CRS_planner2_impl.hpp"

#include <QMessageBox>

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>

#include "shapes/oi_scene_graph.hpp"
#include "proximity/proxy_query_model.hpp"

#include "mbd_kte/kte_map_chain.hpp"
#include "kte_models/manip_dynamics_model.hpp"

#include "topologies/manip_P3R3R_workspaces.hpp"


#include "path_planning/p2p_planning_query.hpp"

#include "path_planning/rrtstar_manip_planners.hpp"
#include "path_planning/sbastar_manip_planners.hpp"

#if 0
#include "path_planning/rrt_path_planner.hpp"
#include "path_planning/prm_path_planner.hpp"
#include "path_planning/fadprm_path_planner.hpp"
#endif

#include "CRS_planner_data.hpp"

#include "path_planning/frame_tracer_coin3d.hpp"

#include "optimization/optim_exceptions.hpp"

#include "topologies/manip_planning_traits.hpp"

#include "topologies/Ndof_linear_spaces.hpp"
#include "topologies/Ndof_cubic_spaces.hpp"
#include "topologies/Ndof_quintic_spaces.hpp"
#include "topologies/Ndof_svp_spaces.hpp"
#include "topologies/Ndof_sap_spaces.hpp"



template <typename ManipMdlType, typename InterpTag, int Order, typename ManipCSpaceTrajectory>
void CRS_execute_static_planner_impl(const ReaK::kte::chaser_target_data& scene_data, 
                                     const ReaK::pp::planning_option_collection& plan_options,
                                     SoSwitch* sw_motion_graph, SoSwitch* sw_solutions,
                                     bool print_timing, bool print_counter, 
                                     const ReaK::vect_n<double>& jt_start, 
                                     const ReaK::vect_n<double>& jt_desired,
                                     ReaK::shared_ptr< ManipCSpaceTrajectory >& sol_trace) {
  using namespace ReaK;
  using namespace pp;
  
  shared_ptr< ManipMdlType > chaser_concrete_model = rtti::rk_dynamic_ptr_cast<ManipMdlType>(scene_data.chaser_kin_model);
  if( !chaser_concrete_model )
    return;
  
  typedef typename manip_static_workspace< ManipMdlType, Order >::rl_workspace_type static_workspace_type;
  typedef typename manip_pp_traits< ManipMdlType, Order >::rl_jt_space_type rl_jt_space_type;
  typedef typename manip_pp_traits< ManipMdlType, Order >::jt_space_type jt_space_type;
  typedef typename manip_pp_traits< ManipMdlType, Order >::ee_space_type ee_space_type;
  typedef typename manip_DK_map< ManipMdlType, Order >::rl_map_type rlDK_map_type;
  
  typedef typename topology_traits< rl_jt_space_type >::point_type rl_point_type;
  typedef typename topology_traits< jt_space_type >::point_type point_type;
  
  typedef typename subspace_traits<static_workspace_type>::super_space_type static_super_space_type;  // SuperSpaceType
  
  std::size_t workspace_dims = Order * manip_pp_traits< ManipMdlType, Order >::degrees_of_freedom;
  
  shared_ptr< frame_3D<double> > EE_frame = chaser_concrete_model->getDependentFrame3D(0)->mFrame;
  
  shared_ptr< static_workspace_type > workspace = 
    make_manip_static_workspace<Order>(InterpTag(),
      chaser_concrete_model, scene_data.chaser_jt_limits, 
      plan_options.min_travel);
  
  shared_ptr< rl_jt_space_type > jt_space = 
    make_manip_rl_jt_space<Order>(chaser_concrete_model, scene_data.chaser_jt_limits);
  
  shared_ptr< jt_space_type > normal_jt_space = 
    make_manip_jt_space<Order>(chaser_concrete_model, scene_data.chaser_jt_limits);
  
  
  (*workspace) << scene_data.chaser_target_proxy;
  for(std::size_t i = 0; i < scene_data.chaser_env_proxies.size(); ++i)
    (*workspace) << scene_data.chaser_env_proxies[i];
  
  
  rl_point_type start_point, goal_point;
  point_type start_inter, goal_inter;
  
  start_inter = normal_jt_space->origin();
  get<0>(start_inter) = jt_start;
  start_point = scene_data.chaser_jt_limits->map_to_space(start_inter, *normal_jt_space, *jt_space);
  
  goal_inter = normal_jt_space->origin();
  get<0>(goal_inter) = jt_desired;
  goal_point = scene_data.chaser_jt_limits->map_to_space(goal_inter, *normal_jt_space, *jt_space);
  
  
  // Create the reporter chain.
  any_sbmp_reporter_chain< static_workspace_type > report_chain;
  
  // Create the frame tracing reporter.
  typedef frame_tracer_3D< rl_jt_space_type > frame_reporter_type;
  
  frame_reporter_type temp_reporter(
    make_any_model_applicator< rl_jt_space_type >(rlDK_map_type(chaser_concrete_model, scene_data.chaser_jt_limits, normal_jt_space)),
    0.5 * plan_options.min_travel, (sw_motion_graph == NULL));
  
  if((sw_motion_graph == NULL) || (sw_solutions == NULL)) {
    temp_reporter.add_traced_frame(EE_frame);
    report_chain.add_reporter( boost::ref(temp_reporter) );
  };
  
  if( print_counter )
    report_chain.add_reporter( print_sbmp_progress<>() );
  
  if( print_timing )
    report_chain.add_reporter( timing_sbmp_report<>() );
  
  
  
  path_planning_p2p_query< static_workspace_type > pp_query("pp_query", workspace,
    start_point, goal_point, plan_options.max_results);
  
  
  shared_ptr< sample_based_planner< static_workspace_type > > workspace_planner;
  
#if 0
  if( plan_options.planning_algo == 0 ) { // RRT
    
    workspace_planner = shared_ptr< sample_based_planner< static_workspace_type > >(
      new rrt_planner< static_workspace_type >(
        workspace, plan_options.max_vertices, plan_options.prog_interval,
        plan_options.store_policy | plan_options.knn_method,
        plan_options.planning_options,
        0.1, 0.05, report_chain));
    
  } else 
#endif
  if( plan_options.planning_algo == 1 ) { // RRT*
    
    workspace_planner = shared_ptr< sample_based_planner< static_workspace_type > >(
      new rrtstar_planner< static_workspace_type >(
        workspace, plan_options.max_vertices, plan_options.prog_interval,
        plan_options.store_policy | plan_options.knn_method,
        plan_options.planning_options,
        0.1, 0.05, workspace_dims, report_chain));
    
  } else 
#if 0
  if( plan_options.planning_algo == 2 ) { // PRM
    
    workspace_planner = shared_ptr< sample_based_planner< static_workspace_type > >(
      new prm_planner< static_workspace_type >(
        workspace, plan_options.max_vertices, plan_options.prog_interval,
        plan_options.store_policy | plan_options.knn_method,
        plan_options.planning_options,
        0.1, 0.05, plan_options.max_travel, workspace_dims, report_chain));
    
  } else 
#endif
  if( plan_options.planning_algo == 3 ) { // SBA*
    
    shared_ptr< sbastar_planner< static_workspace_type > > tmp(
      new sbastar_planner< static_workspace_type >(
        workspace, plan_options.max_vertices, plan_options.prog_interval,
        plan_options.store_policy | plan_options.knn_method,
        plan_options.planning_options,
        0.1, 0.05, plan_options.max_travel, workspace_dims, report_chain));
    
    tmp->set_initial_density_threshold(0.0);
    tmp->set_initial_relaxation(plan_options.init_relax);
    tmp->set_initial_SA_temperature(plan_options.init_SA_temp);
    
    workspace_planner = tmp;
    
  } 
#if 0
  else if( plan_options.planning_algo == 4 ) { // FADPRM
    
    shared_ptr< fadprm_planner< static_workspace_type > > tmp(
      new fadprm_planner< static_workspace_type >(
        workspace, plan_options.max_vertices, plan_options.prog_interval,
        plan_options.store_policy | plan_options.knn_method,
        0.1, 0.05, plan_options.max_travel, workspace_dims, report_chain));
    
    tmp->set_initial_relaxation(plan_options.init_relax);
    
    workspace_planner = tmp;
    
  }
#endif
  ;
  
  
  if(!workspace_planner)
    return;
  
  pp_query.reset_solution_records();
  workspace_planner->solve_planning_query(pp_query);
  
  shared_ptr< seq_path_base< static_super_space_type > > bestsol_rlpath;
  if(pp_query.solutions.size())
    bestsol_rlpath = pp_query.solutions.begin()->second;
  std::cout << "The shortest distance is: " << pp_query.get_best_solution_distance() << std::endl;
  
  sol_trace.reset();
  if(bestsol_rlpath) {
    sol_trace = shared_ptr<ManipCSpaceTrajectory>(new ManipCSpaceTrajectory());
    typedef typename seq_path_base< static_super_space_type >::point_fraction_iterator PtIter;
    typedef typename spatial_trajectory_traits<ManipCSpaceTrajectory>::point_type TCSpacePointType;
    double t = 0.0;
    for(PtIter it = bestsol_rlpath->begin_fraction_travel(); it != bestsol_rlpath->end_fraction_travel(); it += 0.1, t += 0.1) {
      sol_trace->push_back( TCSpacePointType(t, get<0>(scene_data.chaser_jt_limits->map_to_space(*it, *jt_space, *normal_jt_space))) );
    };
  };
  
  
  // Check the motion-graph separator and solution separators
  //  add them to the switches.
  
  if(sw_motion_graph) {
    SoSeparator* mg_sep = temp_reporter.get_motion_graph_tracer(EE_frame).get_separator();
    if(mg_sep)
      mg_sep->ref();
    
    sw_motion_graph->removeAllChildren();
    if(mg_sep) {
      sw_motion_graph->addChild(mg_sep);
      mg_sep->unref();
    };
  };
  
  if(sw_solutions) {
    SoSeparator* sol_sep = NULL;
    if( temp_reporter.get_solution_count() ) {
      sol_sep = temp_reporter.get_solution_tracer(EE_frame, 0).get_separator();
      if(sol_sep)
        sol_sep->ref();
    };
    
    sw_solutions->removeAllChildren();
    if(sol_sep) {
      sw_solutions->addChild(sol_sep);
      sol_sep->unref();
    };
  };
  
  chaser_concrete_model->setJointPositions( jt_start );
  chaser_concrete_model->doDirectMotion();
  
};









void CRSPlannerGUI::executePlanner() {
  using namespace ReaK;
  using namespace pp;
  
  shared_ptr< frame_3D<double> > EE_frame = scene_data.chaser_kin_model->getDependentFrame3D(0)->mFrame;
  
  vect_n<double> jt_desired(7,0.0);
  if(configs.check_ik_goal->isChecked()) {
    vect_n<double> jt_previous = scene_data.chaser_kin_model->getJointPositions();
    try {
      frame_3D<double> tf = scene_data.target_frame->getFrameRelativeTo(EE_frame);
      EE_frame->addBefore(tf);
      scene_data.chaser_kin_model->doInverseMotion();
      jt_desired = scene_data.chaser_kin_model->getJointPositions();
    } catch( optim::infeasible_problem& e ) { RK_UNUSED(e);
      QMessageBox::critical(this,
                    "Inverse Kinematics Error!",
                    "The target frame cannot be reached! No inverse kinematics solution possible!",
                    QMessageBox::Ok);
      return;
    };
    scene_data.chaser_kin_model->setJointPositions(jt_previous);
    scene_data.chaser_kin_model->doDirectMotion();
  } else {
    std::stringstream ss(configs.custom_goal_edit->text().toStdString());
    ss >> jt_desired;
  };
  
  vect_n<double> jt_start;
  if(configs.check_current_start->isChecked()) {
    jt_start = scene_data.chaser_kin_model->getJointPositions(); 
  } else {
    std::stringstream ss(configs.custom_start_edit->text().toStdString());
    ss >> jt_start;
  };
  
  
  // update the planning options record:
  onConfigsChanged();
  
  
  SoSwitch* sw_motion_graph = NULL;
  if(configs.check_print_graph->isChecked())
    sw_motion_graph = view3d_menu.getDisplayGroup("Motion-Graph",true);
  
  SoSwitch* sw_solutions = NULL;
  if(configs.check_print_best->isChecked())
    sw_solutions = view3d_menu.getDisplayGroup("Solution(s)",true);
  
  bool print_timing  = configs.check_print_timing->isChecked();
  bool print_counter = configs.check_print_counter->isChecked();
  
  
  if((plan_options.space_order == 0) && (plan_options.interp_id == 0)) { 
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, linear_interpolation_tag, 0>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
#if 0
  if((plan_options.space_order == 1) && (plan_options.interp_id == 0)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, linear_interpolation_tag, 1>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
  if((plan_options.space_order == 2) && (plan_options.interp_id == 0)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, linear_interpolation_tag, 2>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
#endif
  if((plan_options.space_order == 1) && (plan_options.interp_id == 1)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, cubic_hermite_interpolation_tag, 1>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
#if 0
  if((plan_options.space_order == 2) && (plan_options.interp_id == 1)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, cubic_hermite_interpolation_tag, 2>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
#endif
  if((plan_options.space_order == 2) && (plan_options.interp_id == 2)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, quintic_hermite_interpolation_tag, 2>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
  if((plan_options.space_order == 1) && (plan_options.interp_id == 3)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, svp_Ndof_interpolation_tag, 1>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
#if 0
  if((plan_options.space_order == 2) && (plan_options.interp_id == 3)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, svp_Ndof_interpolation_tag, 2>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  } else 
#endif
  if((plan_options.space_order == 2) && (plan_options.interp_id == 4)) {
    CRS_execute_static_planner_impl<kte::manip_P3R3R_kinematics, sap_Ndof_interpolation_tag, 2>(scene_data, plan_options,
      sw_motion_graph, sw_solutions, print_timing, print_counter, 
      jt_start, jt_desired, sol_anim.trajectory);
  };
  
  
};




