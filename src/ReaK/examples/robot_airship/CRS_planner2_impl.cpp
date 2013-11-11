
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

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QMainWindow>
#include <QDir>

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/sensors/SoTimerSensor.h>  // for SoTimerSensor

#include "shapes/oi_scene_graph.hpp"
#include "proximity/proxy_query_model.hpp"

#include "serialization/archiver_factory.hpp"

#include "CRS_planner_data.hpp"

#include "optimization/optim_exceptions.hpp"

#include "base/chrono_incl.hpp"



using namespace ReaK;




static QString last_used_path;





void CRSPlannerGUI_animate_bestsol_trajectory(void* pv, SoSensor*) {
  CRSPlannerGUI* p = static_cast<CRSPlannerGUI*>(pv);
  
  static shared_ptr< CRS_sol_anim_data::trajectory_type > manip_traj     = p->sol_anim.trajectory;
  static CRS_sol_anim_data::trajectory_type::point_time_iterator cur_pit = manip_traj->begin_time_travel();
  static ReaKaux::chrono::high_resolution_clock::time_point animation_start = ReaKaux::chrono::high_resolution_clock::now();
  
  if(!manip_traj) {
    manip_traj = p->sol_anim.trajectory;
    cur_pit = manip_traj->begin_time_travel();
    animation_start = ReaKaux::chrono::high_resolution_clock::now();
  };
  if( (p->sol_anim.enabled) && ( (*cur_pit).time < manip_traj->get_end_time() ) ) {
    if( (*cur_pit).time <= 0.001 * (ReaKaux::chrono::duration_cast<ReaKaux::chrono::milliseconds>(ReaKaux::chrono::high_resolution_clock::now() - animation_start)).count() ) {
      cur_pit += 0.1;
      p->scene_data.chaser_kin_model->setJointPositions(vect_n<double>((*cur_pit).pt));
      p->scene_data.chaser_kin_model->doDirectMotion();
    };
  } else {
    p->sol_anim.animation_timer->unschedule();
    animation_start = ReaKaux::chrono::high_resolution_clock::now();
    manip_traj.reset();
  };
};

void CRSPlannerGUI::startSolutionAnimation() {
  
  if( configs.check_trajectory->isChecked() ) {
    startCompleteAnimation();
    return;
  };
  
  if( !sol_anim.trajectory || !scene_data.chaser_kin_model ) {
    QMessageBox::critical(this,
                  "Animation Error!",
                  "The best-solution trajectory is missing (not loaded or erroneous)! Cannot animate chaser!",
                  QMessageBox::Ok);
    return;
  };
  sol_anim.enabled = true;
  sol_anim.animation_timer->schedule();
};


void CRSPlannerGUI::stopSolutionAnimation() {
  
  if( configs.check_trajectory->isChecked() ) {
    stopCompleteAnimation();
    return;
  };
  
  sol_anim.enabled = false;
};





void CRSPlannerGUI_animate_target_trajectory(void* pv, SoSensor*) {
  CRSPlannerGUI* p = static_cast<CRSPlannerGUI*>(pv);
  
  static shared_ptr< CRS_target_anim_data::trajectory_type > target_traj    = p->target_anim.trajectory;
  static CRS_target_anim_data::trajectory_type::point_time_iterator cur_pit = target_traj->begin_time_travel();
  static ReaKaux::chrono::high_resolution_clock::time_point animation_start = ReaKaux::chrono::high_resolution_clock::now();
  
  if(!target_traj) {
    target_traj = p->target_anim.trajectory;
    cur_pit = target_traj->begin_time_travel();
    animation_start = ReaKaux::chrono::high_resolution_clock::now();
  };
  if( (p->target_anim.enabled) && ( (*cur_pit).time < target_traj->get_end_time() ) ) {
    if( (*cur_pit).time <= 0.001 * (ReaKaux::chrono::duration_cast<ReaKaux::chrono::milliseconds>(ReaKaux::chrono::high_resolution_clock::now() - animation_start)).count() ) {
      cur_pit += 0.1;
      *(p->scene_data.target_kin_model->getFrame3D(0)) = get_frame_3D((*cur_pit).pt); 
      p->scene_data.target_kin_model->doDirectMotion();
    };
  } else {
    p->target_anim.animation_timer->unschedule();
    animation_start = ReaKaux::chrono::high_resolution_clock::now();
    target_traj.reset();
  };
};

void CRSPlannerGUI::startTargetAnimation() {
  if( !target_anim.trajectory || !scene_data.target_kin_model ) {
    QMessageBox::critical(this,
                  "Animation Error!",
                  "The target trajectory is missing (not loaded or erroneous)! Cannot animate target!",
                  QMessageBox::Ok);
    return;
  };
  target_anim.enabled = true;
  target_anim.animation_timer->schedule();
};

void CRSPlannerGUI::stopTargetAnimation() {
  target_anim.enabled = false;
};

void CRSPlannerGUI::loadTargetTrajectory() {
  
  QString fileName = QFileDialog::getOpenFileName(
    this, 
    tr("Open Target Trajectory..."),
    last_used_path,
    tr("SE(3) Trajectories (*.rkx *.rkb *.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  QFileInfo fileInf(fileName);
  
  last_used_path = fileInf.absolutePath();
  
  *(serialization::open_iarchive(fileName.toStdString()))
    & RK_SERIAL_LOAD_WITH_ALIAS("se3_trajectory", target_anim.trajectory);
  
  if( target_anim.trajectory ) {
    configs.traj_filename_edit->setText(fileInf.baseName());
  } else {
    configs.traj_filename_edit->setText(tr("ERROR!"));
  };
};




void CRSPlannerGUI::startCompleteAnimation() {
  
  if( !sol_anim.trajectory || !target_anim.trajectory || !scene_data.chaser_kin_model || !scene_data.target_kin_model ) {
    QMessageBox::critical(this,
                  "Animation Error!",
                  "One of the trajectories is missing (not loaded or erroneous)! Cannot animate chaser and target!",
                  QMessageBox::Ok);
    return;
  };
  sol_anim.enabled = true;
  target_anim.enabled = true;
  sol_anim.animation_timer->schedule();
  target_anim.animation_timer->schedule();
};

void CRSPlannerGUI::stopCompleteAnimation() {
  sol_anim.enabled = false;
  target_anim.enabled = false;
};




CRSPlannerGUI::CRSPlannerGUI( QWidget * parent, Qt::WindowFlags flags ) : QMainWindow(parent,flags),
                                                                          configs(), view3d_menu(this) {
  setupUi(this);
  
  configs.setupUi(this->config_dock->widget());
  connect(configs.actionStart_Robot, SIGNAL(triggered()), this, SLOT(startSolutionAnimation()));
  connect(configs.actionStopRobot, SIGNAL(triggered()), this, SLOT(stopSolutionAnimation()));
  connect(configs.actionAnimateTarget, SIGNAL(triggered()), this, SLOT(startTargetAnimation()));
  connect(configs.actionStopTargetAnimation, SIGNAL(triggered()), this, SLOT(stopTargetAnimation()));
  connect(configs.actionExecutePlanner, SIGNAL(triggered()), this, SLOT(executePlanner()));
  connect(configs.actionJointChange, SIGNAL(triggered()), this, SLOT(onJointChange()));
  connect(configs.actionTargetChange, SIGNAL(triggered()), this, SLOT(onTargetChange()));
  
  connect(configs.actionLoadTargetTrajectory, SIGNAL(triggered()), this, SLOT(loadTargetTrajectory()));
  
  connect(configs.actionUpdateAvailOptions, SIGNAL(triggered()), this, SLOT(onUpdateAvailableOptions()));
  
  connect(actionLoad_Positions, SIGNAL(triggered()), this, SLOT(loadPositions()));
  connect(actionSave_Positions, SIGNAL(triggered()), this, SLOT(savePositions()));
  connect(actionLoad_Planner, SIGNAL(triggered()), this, SLOT(loadPlannerConfig()));
  connect(actionSave_Planner, SIGNAL(triggered()), this, SLOT(savePlannerConfig()));
  
  
  SoQt::init(this->centralwidget);
  
  menubar->addMenu(&view3d_menu);
  view3d_menu.setViewer(new SoQtExaminerViewer(this->centralwidget));
  
  view3d_menu.getGeometryGroup("Chaser Geometry",true);
  view3d_menu.getGeometryGroup("Chaser KTE Chain",false);
  view3d_menu.getGeometryGroup("Target Geometry",true);
  view3d_menu.getGeometryGroup("Environment",true);
  view3d_menu.getDisplayGroup("Motion-Graph",true);
  view3d_menu.getDisplayGroup("Solution(s)",true);
  
  sol_anim.animation_timer    = new SoTimerSensor(CRSPlannerGUI_animate_bestsol_trajectory, this);
  target_anim.animation_timer = new SoTimerSensor(CRSPlannerGUI_animate_target_trajectory, this);
  
  plan_options.space_order = 0;
  plan_options.interp_id = 0;
  plan_options.min_travel = 0.1;
  plan_options.max_travel = 1.0;
  plan_options.planning_algo = 0;
  plan_options.max_vertices = 2000;
  plan_options.prog_interval = 500;
  plan_options.max_results = 50;
  plan_options.planning_options = 0;
  plan_options.store_policy = 0;
  plan_options.knn_method = 2;
  plan_options.init_SA_temp = 0.0;
  plan_options.init_relax = 5.0;
  plan_options.start_delay = 20.0;
  updateConfigs();
  
};


CRSPlannerGUI::~CRSPlannerGUI() {
  
  delete target_anim.animation_timer;
  delete sol_anim.animation_timer;
  
  view3d_menu.setViewer(NULL);
  SoQt::done();
  
};



void CRSPlannerGUI::onConfigsChanged() {
  
  // joint-space parameters:
  plan_options.space_order = configs.order_selection->currentIndex();
  plan_options.interp_id = configs.interp_selection->currentIndex();
  
  plan_options.min_travel = configs.min_interval_spinbox->value();
  plan_options.max_travel = configs.max_interval_spinbox->value();
  
  
  // planner parameters:
  plan_options.planning_algo = configs.planning_algo_selection->currentIndex();
  
  plan_options.max_vertices = configs.maxvertices_spinbox->value();
  plan_options.prog_interval = configs.progress_interval_spinbox->value();
  plan_options.max_results = configs.maxsolutions_spinbox->value();
  
  plan_options.planning_options = pp::UNIDIRECTIONAL_PLANNING;
  
  if(configs.check_bidir->isChecked())
    plan_options.planning_options |= pp::BIDIRECTIONAL_PLANNING;
  
  if(configs.check_lazy_collision->isChecked())
    plan_options.planning_options |= pp::LAZY_COLLISION_CHECKING;
  
  plan_options.init_SA_temp = -1.0;
  if(configs.check_voronoi_pull->isChecked()) {
    plan_options.planning_options |= pp::PLAN_WITH_VORONOI_PULL;
    plan_options.init_SA_temp = configs.init_sa_temp_spinbox->value();
    if(plan_options.init_SA_temp < 1e-6)
      plan_options.init_SA_temp = -1.0;
  };
  
  plan_options.init_relax = 0.0;
  if(configs.check_anytime_heuristic->isChecked()) {
    plan_options.planning_options |= pp::PLAN_WITH_ANYTIME_HEURISTIC;
    plan_options.init_relax = configs.init_relax_spinbox->value();
  };
  
  if(configs.check_bnb->isChecked())
    plan_options.planning_options |= pp::USE_BRANCH_AND_BOUND_PRUNING_FLAG;
  
  plan_options.start_delay = 0.0;
  if(configs.check_trajectory->isChecked())
    plan_options.start_delay = configs.start_delay_spinbox->value();
  
  plan_options.store_policy = pp::ADJ_LIST_MOTION_GRAPH;
  if(configs.graph_storage_selection->currentIndex())
    plan_options.store_policy = pp::DVP_ADJ_LIST_MOTION_GRAPH;
  
  plan_options.knn_method = pp::LINEAR_SEARCH_KNN;
  switch(configs.KNN_method_selection->currentIndex()) {
    case 1:
      plan_options.knn_method = pp::DVP_BF2_TREE_KNN;
      break;
    case 2:
      plan_options.knn_method = pp::DVP_BF4_TREE_KNN;
      break;
    case 3:
      plan_options.knn_method = pp::DVP_COB2_TREE_KNN;
      break;
    case 4:
      plan_options.knn_method = pp::DVP_COB4_TREE_KNN;
      break;
    default:
      break;
  };
  
};


void CRSPlannerGUI::updateConfigs() {
  configs.order_selection->setCurrentIndex(plan_options.space_order);
  configs.interp_selection->setCurrentIndex(plan_options.interp_id);
  
  configs.min_interval_spinbox->setValue(plan_options.min_travel);
  configs.max_interval_spinbox->setValue(plan_options.max_travel);
  
  // planner parameters:
  configs.planning_algo_selection->setCurrentIndex(plan_options.planning_algo);
  
  configs.maxvertices_spinbox->setValue(plan_options.max_vertices);
  configs.progress_interval_spinbox->setValue(plan_options.prog_interval);
  configs.maxsolutions_spinbox->setValue(plan_options.max_results);
  
  if(plan_options.planning_options & pp::BIDIRECTIONAL_PLANNING)
    configs.check_bidir->setChecked(true);
  else
    configs.check_bidir->setChecked(false);
  
  
  if(plan_options.planning_options & pp::LAZY_COLLISION_CHECKING)
    configs.check_lazy_collision->setChecked(true);
  else
    configs.check_lazy_collision->setChecked(false);
  
  
  if(plan_options.planning_options & pp::PLAN_WITH_VORONOI_PULL) {
    configs.init_sa_temp_spinbox->setValue(plan_options.init_SA_temp);
    configs.check_voronoi_pull->setChecked(true);
  } else {
    configs.init_sa_temp_spinbox->setValue(0.0);
    configs.check_voronoi_pull->setChecked(false);
  };
  
  if(plan_options.planning_options & pp::PLAN_WITH_ANYTIME_HEURISTIC) {
    configs.init_relax_spinbox->setValue(plan_options.init_relax);
    configs.check_anytime_heuristic->setChecked(true);
  } else {
    configs.init_relax_spinbox->setValue(0.0);
    configs.check_anytime_heuristic->setChecked(false);
  };
  
  if(plan_options.planning_options & pp::USE_BRANCH_AND_BOUND_PRUNING_FLAG)
    configs.check_bnb->setChecked(true);
  else
    configs.check_bnb->setChecked(false);
  
  configs.start_delay_spinbox->setValue(plan_options.start_delay);
  
  
  if( plan_options.store_policy == pp::DVP_ADJ_LIST_MOTION_GRAPH ) {
    configs.graph_storage_selection->setCurrentIndex(1);
  } else {
    configs.graph_storage_selection->setCurrentIndex(0);
  };
  
  switch(plan_options.knn_method) {
    case pp::DVP_BF2_TREE_KNN:
      configs.KNN_method_selection->setCurrentIndex(1);
      break;
    case pp::DVP_BF4_TREE_KNN:
      configs.KNN_method_selection->setCurrentIndex(2);
      break;
    case pp::DVP_COB2_TREE_KNN:
      configs.KNN_method_selection->setCurrentIndex(3);
      break;
    case pp::DVP_COB4_TREE_KNN:
      configs.KNN_method_selection->setCurrentIndex(4);
      break;
    default:
      configs.KNN_method_selection->setCurrentIndex(0);
      break;
  };
  
  onUpdateAvailableOptions();
  
};



void CRSPlannerGUI::onUpdateAvailableOptions() {
  int plan_alg = configs.planning_algo_selection->currentIndex();
  
  switch(plan_alg) {
    case 1:  // RRT*
      configs.check_lazy_collision->setEnabled(false);
      configs.check_lazy_collision->setChecked(true);
      break;
    case 3:  // SBA*
      configs.check_lazy_collision->setEnabled(true);
      configs.check_lazy_collision->setChecked(true);
      break;
    case 0:  // RRT
    case 2:  // PRM
    case 4:  // FADPRM
    default:
      configs.check_lazy_collision->setEnabled(false);
      configs.check_lazy_collision->setChecked(false);
      break;
  };
  
  switch(plan_alg) {
    case 0:  // RRT
      configs.check_bidir->setEnabled(true);
      break;
    case 1:  // RRT*
    case 2:  // PRM
    case 3:  // SBA*
    case 4:  // FADPRM
    default:
      configs.check_bidir->setEnabled(false);
      configs.check_bidir->setChecked(false);
      break;
  };
  
  switch(plan_alg) {
    case 3:  // SBA*
      configs.check_voronoi_pull->setEnabled(true);
      configs.check_anytime_heuristic->setEnabled(true);
      break;
    case 0:  // RRT
    case 1:  // RRT*
    case 2:  // PRM
    case 4:  // FADPRM
    default:
      configs.check_voronoi_pull->setEnabled(false);
      configs.check_voronoi_pull->setChecked(false);
      configs.check_anytime_heuristic->setEnabled(false);
      configs.check_anytime_heuristic->setChecked(false);
      break;
  };
  
  switch(plan_alg) {
    case 1:  // RRT*
    case 3:  // SBA*
      configs.check_bnb->setEnabled(true);
      break;
    case 0:  // RRT
    case 2:  // PRM
    case 4:  // FADPRM
    default:
      configs.check_bnb->setEnabled(false);
      configs.check_bnb->setChecked(false);
      break;
  };
  
};




void CRSPlannerGUI::savePositions() {
  QString fileName = QFileDialog::getSaveFileName(
    this, tr("Save Positions Record..."), last_used_path,
    tr("Robot-Airship Positions Record (*.rapos.rkx *.rapos.rkb *.rapos.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  last_used_path = QFileInfo(fileName).absolutePath();
  
  vect<double,7> robot_joint_positions;
  robot_joint_positions[0] = double(configs.track_pos->value()) * 0.001;
  robot_joint_positions[1] = double(configs.joint1_pos->value()) * 0.001;
  robot_joint_positions[2] = double(configs.joint2_pos->value()) * 0.001;
  robot_joint_positions[3] = double(configs.joint3_pos->value()) * 0.001;
  robot_joint_positions[4] = double(configs.joint4_pos->value()) * 0.001;
  robot_joint_positions[5] = double(configs.joint5_pos->value()) * 0.001;
  robot_joint_positions[6] = double(configs.joint6_pos->value()) * 0.001;
  
  vect<double,3> airship_position;
  airship_position[0] = double(configs.target_x->value()) * 0.001;
  airship_position[1] = double(configs.target_y->value()) * 0.001;
  airship_position[2] = double(configs.target_z->value()) * 0.001;
  
  euler_angles_TB<double> ea;
  ea.yaw() = double(configs.target_yaw->value()) * 0.001;
  ea.pitch() = double(configs.target_pitch->value()) * 0.001;
  ea.roll() = double(configs.target_roll->value()) * 0.001;
  quaternion<double> airship_quaternion = ea.getQuaternion();
  
  
  try {
    *(serialization::open_oarchive(fileName.toStdString()))
      & RK_SERIAL_SAVE_WITH_NAME(robot_joint_positions)
      & RK_SERIAL_SAVE_WITH_NAME(airship_position)
      & RK_SERIAL_SAVE_WITH_NAME(airship_quaternion);
  } catch(...) {
    QMessageBox::information(this,
                "File Type Not Supported!",
                "Sorry, this file-type is not supported!",
                QMessageBox::Ok);
    return;
  };
  
};

void CRSPlannerGUI::loadPositions() {
  QString fileName = QFileDialog::getOpenFileName(
    this, 
    tr("Open Positions Record..."),
    last_used_path,
    tr("Robot-Airship Positions Record (*.rapos.rkx *.rapos.rkb *.rapos.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  last_used_path = QFileInfo(fileName).absolutePath();
  
  vect<double,7> robot_joint_positions;
  vect<double,3> airship_position;
  quaternion<double> airship_quaternion;
  
  try {
    *(serialization::open_iarchive(fileName.toStdString()))
      & RK_SERIAL_LOAD_WITH_NAME(robot_joint_positions)
      & RK_SERIAL_LOAD_WITH_NAME(airship_position)
      & RK_SERIAL_LOAD_WITH_NAME(airship_quaternion);
  } catch(...) {
    QMessageBox::information(this,
                "File Type Not Supported!",
                "Sorry, this file-type is not supported!",
                QMessageBox::Ok);
    return;
  };
  
  configs.track_pos->setValue(int(1000.0 * robot_joint_positions[0]));
  configs.joint1_pos->setValue(int(1000.0 * robot_joint_positions[1]));
  configs.joint2_pos->setValue(int(1000.0 * robot_joint_positions[2]));
  configs.joint3_pos->setValue(int(1000.0 * robot_joint_positions[3]));
  configs.joint4_pos->setValue(int(1000.0 * robot_joint_positions[4]));
  configs.joint5_pos->setValue(int(1000.0 * robot_joint_positions[5]));
  configs.joint6_pos->setValue(int(1000.0 * robot_joint_positions[6])); 
  //onJointChange();
  
  configs.target_x->setValue(int(1000.0 * airship_position[0]));
  configs.target_y->setValue(int(1000.0 * airship_position[1]));
  configs.target_z->setValue(int(1000.0 * airship_position[2]));
  euler_angles_TB<double> ea = euler_angles_TB<double>(airship_quaternion);
  configs.target_yaw->setValue(int(1000.0 * ea.yaw()));
  configs.target_pitch->setValue(int(1000.0 * ea.pitch()));
  configs.target_roll->setValue(int(1000.0 * ea.roll()));
  //onTargetChange();
  
};

void CRSPlannerGUI::savePlannerConfig() {
  QString fileName = QFileDialog::getSaveFileName(
    this, tr("Save Planner Configurations..."), last_used_path,
    tr("Robot-Airship Planner Configurations (*.raplan.rkx *.raplan.rkb *.raplan.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  last_used_path = QFileInfo(fileName).absolutePath();
  
  onConfigsChanged();
  
  try {
    (*serialization::open_oarchive(fileName.toStdString())) << plan_options;
  } catch(...) {
    QMessageBox::information(this,
                "File Type Not Supported!",
                "Sorry, this file-type is not supported!",
                QMessageBox::Ok);
    return;
  };
  
};

void CRSPlannerGUI::loadPlannerConfig() {
  QString fileName = QFileDialog::getOpenFileName(
    this, tr("Open Planner Configurations..."), last_used_path,
    tr("Robot-Airship Planner Configurations (*.raplan.rkx *.raplan.rkb *.raplan.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  last_used_path = QFileInfo(fileName).absolutePath();
  
  try {
    (*serialization::open_iarchive(fileName.toStdString())) >> plan_options;
  } catch(...) {
    QMessageBox::information(this,
                "File Type Not Supported!",
                "Sorry, this file-type is not supported!",
                QMessageBox::Ok);
    return;
  };
  
  updateConfigs();
};





void CRSPlannerGUI::onJointChange() {
  if( !scene_data.chaser_kin_model )
    return;
  
  scene_data.chaser_kin_model->setJointPositions(
    vect_n<double>(
      double(configs.track_pos->value()) * 0.001,
      double(configs.joint1_pos->value()) * 0.001,
      double(configs.joint2_pos->value()) * 0.001,
      double(configs.joint3_pos->value()) * 0.001,
      double(configs.joint4_pos->value()) * 0.001,
      double(configs.joint5_pos->value()) * 0.001,
      double(configs.joint6_pos->value()) * 0.001
    )
  );
  scene_data.chaser_kin_model->doDirectMotion();
};

  
void CRSPlannerGUI::loadChaserModel() {
  
  QString fileName = QFileDialog::getOpenFileName(
    this, tr("Open Chaser Kinematic Model..."), last_used_path,
    tr("Chaser Kinematic Model (*.model.rkx *.model.rkb *.model.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  last_used_path = QFileInfo(fileName).absolutePath();
  
  try {
    scene_data.load_chaser(fileName.toStdString());  // "models/CRS_A465.model.rkx"
    
    shared_ptr<geom::oi_scene_graph> psg = view3d_menu.getGeometryGroup("Chaser Geometry");
    psg->clearAll();
    (*psg) << (*scene_data.chaser_geom_model);
    
    shared_ptr<geom::oi_scene_graph> psg_kte = view3d_menu.getGeometryGroup("Chaser KTE Chain");
    psg_kte->clearAll();
    psg_kte->setCharacteristicLength( psg->computeCharacteristicLength() );
    (*psg_kte) << (*scene_data.chaser_kin_model->getKTEChain());
    
    onJointChange();
    
  } catch(...) {
    QMessageBox::information(this,
                "File Type Not Supported!",
                "Sorry, this file-type is not supported!",
                QMessageBox::Ok);
    return;
  };
  
};


void CRSPlannerGUI::onTargetChange() {
  if( !scene_data.target_kin_model )
    return;
  
  shared_ptr< frame_3D<double> > target_state = scene_data.target_kin_model->getFrame3D(0);
  target_state->Position = vect<double,3>(
    double(configs.target_x->value()) * 0.001, 
    double(configs.target_y->value()) * 0.001, 
    double(configs.target_z->value()) * 0.001);
  target_state->Quat = 
    quaternion<double>::zrot(double(configs.target_yaw->value()) * 0.001) * 
    quaternion<double>::yrot(double(configs.target_pitch->value()) * 0.001) * 
    quaternion<double>::xrot(double(configs.target_roll->value()) * 0.001);
  scene_data.target_kin_model->doDirectMotion();
  
  
  if( scene_data.chaser_kin_model && configs.check_enable_ik->isChecked()) {
    try {
      frame_3D<double> tf = scene_data.target_frame->getFrameRelativeTo(scene_data.chaser_kin_model->getDependentFrame3D(0)->mFrame);
      scene_data.chaser_kin_model->getDependentFrame3D(0)->mFrame->addBefore(tf);
      scene_data.chaser_kin_model->doInverseMotion();
    } catch( optim::infeasible_problem& e ) { RK_UNUSED(e); };
    scene_data.chaser_kin_model->doDirectMotion();
  };
  
};


void CRSPlannerGUI::loadTargetModel() {
  
  
  QString fileName = QFileDialog::getOpenFileName(
    this, tr("Open Target Model..."), last_used_path,
    tr("Target Model (*.model.rkx *.model.rkb *.model.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  last_used_path = QFileInfo(fileName).absolutePath();
  
  try {
    scene_data.load_target(fileName.toStdString());  // "models/airship3D.model.rkx"
    
    shared_ptr<geom::oi_scene_graph> psg = view3d_menu.getGeometryGroup("Target Geometry");
    psg->clearAll();
    (*psg) << (*scene_data.target_geom_model);
    
    onTargetChange();
    
  } catch(...) {
    QMessageBox::information(this,
                "File Type Not Supported!",
                "Sorry, this file-type is not supported!",
                QMessageBox::Ok);
    return;
  };
  
};

void CRSPlannerGUI::loadEnvironmentGeometry() {
  
  QString fileName = QFileDialog::getOpenFileName(
    this, tr("Open Environment Geometry..."), last_used_path,
    tr("Environment Geometry (*.geom.rkx *.geom.rkb *.geom.pbuf)"));
  
  if( fileName == tr("") )
    return;
  
  last_used_path = QFileInfo(fileName).absolutePath();
  
  try {
    scene_data.load_environment(fileName.toStdString());  // "models/MD148_lab.geom.rkx"
    
    shared_ptr<geom::oi_scene_graph> psg = view3d_menu.getGeometryGroup("Environment");
    psg->clearAll();
    for(std::size_t i = 0; i < scene_data.env_geom_models.size(); ++i)
      (*psg) << (*(scene_data.env_geom_models[i]));
    
  } catch(...) {
    QMessageBox::information(this,
                "File Type Not Supported!",
                "Sorry, this file-type is not supported!",
                QMessageBox::Ok);
    return;
  };
  
};

void CRSPlannerGUI::clearEnvironmentGeometries() {
  view3d_menu.getGeometryGroup("Environment")->clearAll();
  scene_data.clear_environment();
};


int main(int argc, char** argv) {
  QApplication app(argc,argv);
  CRSPlannerGUI window;
  window.show();
  // Pop up the main window.
  SoQt::show(&window);
  // Loop until exit.
  SoQt::mainLoop();
  
  return 0;
  //return app.exec();
};








