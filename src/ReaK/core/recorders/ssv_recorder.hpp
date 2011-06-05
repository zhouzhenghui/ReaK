/**
 * \file ssv_recorder.hpp
 *
 * This library declares the class for data recording to a space-separated-values file. Here, "data" is meant as
 * columns of floating-point records of data, such as simulation results for example.
 *
 * \author Mikael Persson, <mikael.s.persson@gmail.com>
 * \date january 2010
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

#ifndef SSV_RECORDER_HPP
#define SSV_RECORDER_HPP

#include "data_record.hpp"

#include <fstream>

namespace ReaK {

namespace recorder {
  

/**
 * This class handles file IO operations for a space-separated-values data record.
 */
class ssv_recorder : public data_recorder {
  protected:
    virtual void writeRow();
    virtual void writeNames();

    std::ofstream output_file; ///< Holds the ascii file output stream.
  public:

    /**
     * Default constructor.
     */
    ssv_recorder() : data_recorder(),
                     output_file() {
      output_file.setf(std::ios::scientific, std::ios::floatfield);
      output_file.precision(11);
    };

    /**
     * Constructor that opens a file with name aFileName.
     */
    ssv_recorder(const std::string& aFileName) : data_recorder(), output_file(aFileName.c_str()) {
      fileName = aFileName;
      output_file.setf(std::ios::scientific, std::ios::floatfield);
      output_file.precision(11);
    };

    /**
     * Destructor, closes the file.
     */
    virtual ~ssv_recorder() {
      if(output_file.is_open())
        output_file.close();
    };

    virtual void setFileName(const std::string& aFileName);

    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      data_recorder::save(A,data_recorder::getStaticObjectType()->TypeVersion());
    };
    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      data_recorder::load(A,data_recorder::getStaticObjectType()->TypeVersion());
    };

    RK_RTTI_MAKE_CONCRETE_1BASE(ssv_recorder,0x81100002,1,"ssv_recorder",data_recorder)
};




/**
 * This class handles file IO operations for a space-separated-values data extractor.
 */
class ssv_extractor : public data_extractor {
  protected:
    virtual bool readRow();
    virtual bool readNames();
    virtual bool loadFile(const std::string& aFileName);

    std::ifstream input_file; ///< Holds the binary-formatted file output stream.
  public:

    /**
     * Default constructor.
     */
    ssv_extractor() : data_extractor(), input_file() { };

    /**
     * Constructor that opens a file with name aFileName.
     */
    ssv_extractor(const std::string& aFileName) : data_extractor(), input_file(aFileName.c_str()) {
      fileName = aFileName;
      readNames();
    };

    /**
     * Destructor, closes the file.
     */
    virtual ~ssv_extractor() {
      if(input_file.is_open())
        input_file.close();
    };

    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      data_extractor::save(A,data_extractor::getStaticObjectType()->TypeVersion());
    };
    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      data_extractor::load(A,data_extractor::getStaticObjectType()->TypeVersion());
    };

    RK_RTTI_MAKE_CONCRETE_1BASE(ssv_extractor,0x81200002,1,"ssv_extractor",data_extractor)
};



};


};


#endif






