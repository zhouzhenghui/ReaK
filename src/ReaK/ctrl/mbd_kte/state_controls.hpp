
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

#ifndef STATE_CONTROLS_HPP
#define STATE_CONTROLS_HPP

#include "math/kinetostatics.hpp"
#include "kte_system_input.hpp"
#include "kte_map.hpp"

namespace ReaK {

namespace kte {


class position_control_gen : public kte_map, public system_input {
  private:
    boost::shared_ptr< gen_coord<double> > mAnchor; 
    double mPosDesired;
    
  public:
    double& getPosDesired() { return mPosDesired; };
    double getPosDesired() const { return mPosDesired; };
    
    virtual unsigned int getInputCount() const { return 1; };
    virtual double& getInput(unsigned int i) { return mPosDesired; };
    virtual double getInput(unsigned int i) const { return mPosDesired; };
    
    /**
     * Default constructor.
     */
    position_control_gen(const std::string& aName = "") : kte_map(aName), mAnchor(), mPosDesired(0.0) { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    position_control_gen(const std::string& aName,
			 boost::shared_ptr< ReaK::gen_coord<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mPosDesired(0.0) { };

    /**
     * Default destructor.
     */
    virtual ~position_control_gen() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->q = mPosDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(position_control_gen,0xC2100043,1,"position_control_gen",kte_map,system_input)

};



class position_control_2D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_2D<double> > mAnchor; 
    vect<double,2> mPosDesired;
    
  public:
    vect<double,2>& getPosDesired() { return mPosDesired; };
    const vect<double,2>& getPosDesired() const { return mPosDesired; };
    
    virtual unsigned int getInputCount() const { return 2; };
    virtual double& getInput(unsigned int i) { 
      if(i < 2)
        return mPosDesired[i]; 
      else
	return mPosDesired[0];
    };
    virtual double getInput(unsigned int i) const { 
      if(i < 2)
        return mPosDesired[i]; 
      else
	return mPosDesired[0];
    };
    
    /**
     * Default constructor.
     */
    position_control_2D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mPosDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    position_control_2D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_2D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mPosDesired() { };

    /**
     * Default destructor.
     */
    virtual ~position_control_2D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->Position = mPosDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(position_control_2D,0xC2100044,1,"position_control_2D",kte_map,system_input)

};



class position_control_3D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_3D<double> > mAnchor; 
    vect<double,3> mPosDesired;
    
  public:
    vect<double,3>& getPosDesired() { return mPosDesired; };
    const vect<double,3>& getPosMeasure() const { return mPosDesired; };
    
    virtual unsigned int getInputCount() const { return 3; };
    virtual double& getInput(unsigned int i) { 
      if(i < 3)
        return mPosDesired[i]; 
      else
	return mPosDesired[0];
    };
    virtual double getInput(unsigned int i) const { 
      if(i < 3)
        return mPosDesired[i]; 
      else
	return mPosDesired[0];
    };
    
    /**
     * Default constructor.
     */
    position_control_3D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mPosDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    position_control_3D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_3D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mPosDesired() { };

    /**
     * Default destructor.
     */
    virtual ~position_control_3D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->Position = mPosDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(position_control_3D,0xC2100045,1,"position_control_3D",kte_map,system_input)

};



class rotation_control_2D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_2D<double> > mAnchor; 
    double mAngleDesired;
    
  public:
    double& getAngleDesired() { return mAngleDesired; };
    double getAngleDesired() const { return mAngleDesired; };
    
    virtual unsigned int getInputCount() const { return 1; };
    virtual double& getInput(unsigned int) { return mAngleDesired; };
    virtual double getInput(unsigned int) const { return mAngleDesired; };
    
    /**
     * Default constructor.
     */
    rotation_control_2D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mAngleDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    rotation_control_2D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_2D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mAngleDesired() { };

    /**
     * Default destructor.
     */
    virtual ~rotation_measure_2D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->Rotation.getAngle() = mAngleDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(rotation_control_2D,0xC2100046,1,"rotation_control_2D",kte_map,system_input)

};




class rotation_control_3D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_3D<double> > mAnchor; 
    quaternion<double> mQuatDesired;
    
  public:
    quaternion<double>& getQuatDesired() { return mQuatDesired; };
    const quaternion<double>& getQuatDesired() const { return mQuatDesired; };
    
    virtual unsigned int getInputCount() const { return 4; };
    virtual double& getInput(unsigned int i) { 
      if(i < 4)
        return mQuatDesired[i]; 
      else
	return mQuatDesired[0];
    };
    virtual double getInput(unsigned int i) const { 
      if(i < 4)
        return mQuatDesired[i]; 
      else
	return mQuatDesired[0];
    };
    
    /**
     * Default constructor.
     */
    rotation_measure_3D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mQuatDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    rotation_measure_3D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_3D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mQuatDesired() { };

    /**
     * Default destructor.
     */
    virtual ~rotation_measure_3D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->Quat = mQuatDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(rotation_control_3D,0xC2100047,1,"rotation_control_3D",kte_map,system_input)

};



class velocity_control_gen : public kte_map, public system_input {
  private:
    boost::shared_ptr< gen_coord<double> > mAnchor; 
    double mVelDesired;
    
  public:
    double& getVelDesired() { return mVelDesired; };
    double getVelDesired() const { return mVelDesired; };
    
    virtual unsigned int getInputCount() const { return 1; };
    virtual double& getInput(unsigned int) { return mVelDesired; };
    virtual double getInput(unsigned int) const { return mVelDesired; };
    
    /**
     * Default constructor.
     */
    velocity_control_gen(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mVelDesired(0.0) { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    velocity_control_gen(const std::string& aName,
			 boost::shared_ptr< ReaK::gen_coord<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mVelDesired(0.0) { };

    /**
     * Default destructor.
     */
    virtual ~velocity_control_gen() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->q_dot = mVelDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(velocity_control_gen,0xC2100048,1,"velocity_control_gen",kte_map,system_input)

};



class velocity_control_2D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_2D<double> > mAnchor; 
    vect<double,2> mVelDesired;
    
  public:
    vect<double,2>& getVelDesired() { return mVelDesired; };
    const vect<double,2>& getVelDesired() const { return mVelDesired; };
    
    virtual unsigned int getInputCount() const { return 2; };
    virtual double& getInput(unsigned int i) { 
      if(i < 2)
        return mVelDesired[i]; 
      else
	return mVelDesired[0];
    };
    virtual double getInput(unsigned int i) const { 
      if(i < 2)
        return mVelDesired[i]; 
      else
	return mVelDesired[0];
    };
    
    /**
     * Default constructor.
     */
    velocity_control_2D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mVelDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    velocity_control_2D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_2D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mVelDesired() { };

    /**
     * Default destructor.
     */
    virtual ~velocity_control_2D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->Velocity = mVelDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(velocity_control_2D,0xC2100049,1,"velocity_control_2D",kte_map,system_input)

};



class velocity_control_3D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_3D<double> > mAnchor; 
    vect<double,3> mVelDesired;
    
  public:
    vect<double,3>& getVelDesired() { return mVelDesired; };
    const vect<double,3>& getVelDesired() const { return mVelDesired; };
    
    virtual unsigned int getInputCount() const { return 3; };
    virtual double& getInput(unsigned int i) { 
      if(i < 3)
        return mVelDesired[i]; 
      else
	return mVelDesired[0];
    };
    virtual double getInput(unsigned int i) const { 
      if(i < 3)
        return mVelDesired[i]; 
      else
	return mVelDesired[0];
    };
    
    /**
     * Default constructor.
     */
    velocity_control_3D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mVelDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    velocity_control_3D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_3D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mVelDesired() { };

    /**
     * Default destructor.
     */
    virtual ~velocity_control_3D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->Velocity = mVelDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(velocity_control_3D,0xC210004A,1,"velocity_control_3D",kte_map,system_input)

};



class ang_velocity_control_2D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_2D<double> > mAnchor; 
    vect<double,2> mAngVelDesired;
    
  public:
    vect<double,2>& getAngVelDesired() { return mAngVelDesired; };
    const vect<double,2>& getAngVelDesired() const { return mAngVelDesired; };
    
    virtual unsigned int getInputCount() const { return 2; };
    virtual double& getInput(unsigned int i) { 
      if(i < 2)
        return mAngVelDesired[i]; 
      else
	return mAngVelDesired[0];
    };
    virtual double getInput(unsigned int i) const { 
      if(i < 2)
        return mAngVelDesired[i]; 
      else
	return mAngVelDesired[0];
    };
    
    /**
     * Default constructor.
     */
    ang_velocity_control_2D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mAngVelDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    ang_velocity_control_2D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_2D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mAngVelDesired() { };

    /**
     * Default destructor.
     */
    virtual ~ang_velocity_control_2D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->AngVelocity = mAngVelDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(ang_velocity_control_2D,0xC210004B,1,"ang_velocity_control_2D",kte_map,system_input)

};



class ang_velocity_control_3D : public kte_map, public system_input {
  private:
    boost::shared_ptr< frame_3D<double> > mAnchor; 
    vect<double,3> mAngVelDesired;
    
  public:
    vect<double,3>& getAngVelDesired() { return mAngVelDesired; };
    const vect<double,3>& getAngVelDesired() const { return mAngVelDesired; };
    
    virtual unsigned int getInputCount() const { return 3; };
    virtual double& getInput(unsigned int i) {
      if(i < 3)
        return mAngVelDesired[i]; 
      else
	return mAngVelDesired[0];
    };
    virtual double getInput(unsigned int i) const { 
      if(i < 3)
        return mAngVelDesired[i]; 
      else
	return mAngVelDesired[0];
    };
    
    /**
     * Default constructor.
     */
    ang_velocity_control_3D(const std::string& aName = "") : kte_map(aName), system_input(aName), mAnchor(), mAngVelDesired() { };

    /**
     * Parametrized constructor.
     * \param aName name of the KTE model.
     * \param aAnchor the coordinate from which position is measured.
     */
    ang_velocity_control_3D(const std::string& aName,
			 boost::shared_ptr< ReaK::frame_3D<double> > aAnchor) :
			 kte_map(aName),
			 system_input(aName),
			 mAnchor(aAnchor),
			 mAngVelDesired() { };

    /**
     * Default destructor.
     */
    virtual ~ang_velocity_control_3D() { };
    
    virtual void doMotion(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) {
      if(mAnchor)
	mAnchor->AngVelocity = mAngVelDesired;
    };

    virtual void doForce(kte_pass_flag aFlag = nothing, boost::shared_ptr<frame_storage> aStorage = boost::shared_ptr<frame_storage>()) { };

    virtual void clearForce() { };
    
    
    virtual void RK_CALL save(ReaK::serialization::oarchive& A, unsigned int) const {
      kte_map::save(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::save(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(mAnchor);
    };

    virtual void RK_CALL load(ReaK::serialization::iarchive& A, unsigned int) {
      kte_map::load(A,kte_map::getStaticObjectType()->TypeVersion());
      system_input::load(A,system_input::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(mAnchor);
    };

    RK_RTTI_MAKE_CONCRETE_2BASE(ang_velocity_control_3D,0xC210004C,1,"ang_velocity_control_3D",kte_map,system_input)

};





};

};

#endif









