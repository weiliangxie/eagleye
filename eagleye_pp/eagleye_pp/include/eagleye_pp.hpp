// Copyright (c) 2022, Map IV, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the Map IV, Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <ros/ros.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <boost/foreach.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <yaml-cpp/yaml.h>

class eagleye_pp
{
  private:
 
  // Frequently used variables
  std::string outputpath_;
  std::size_t data_length_;

  // For inital azimuth calculation
  std::vector<bool>flag_GNSS_;
  
  // Data estimated by eagleye
  std::vector<rtklib_msgs::RtklibNav> rtklib_nav_;
  std::vector<sensor_msgs::Imu> imu_;
  std::vector<geometry_msgs::TwistStamped> velocity_;
  std::vector<nmea_msgs::Sentence> nmea_sentence_;
  std::vector<nmea_msgs::Gpgga> gga_;
  std::vector<nmea_msgs::Gprmc> rmc_;

  struct EagleyeStates
  {
    std::vector<bool> flag_reliability_buffer;
    std::vector<double> enu_smoothing_trajectory_east;
    std::vector<double> smoothing_trajectory_status;
    std::vector<double> llh_smoothing_trajectory_lat;
    std::vector<eagleye_msgs::VelocityScaleFactor> velocity_scale_factor;
    std::vector<eagleye_msgs::Distance> distance;
    std::vector<eagleye_msgs::Heading> heading_1st; 
    std::vector<eagleye_msgs::Heading> heading_interpolate_1st;
    std::vector<eagleye_msgs::Heading> heading_2nd;
    std::vector<eagleye_msgs::Heading> heading_interpolate_2nd;
    std::vector<eagleye_msgs::Heading> heading_3rd;
    std::vector<eagleye_msgs::Heading> heading_interpolate_3rd;
    std::vector<eagleye_msgs::YawrateOffset> yawrate_offset_stop;
    std::vector<eagleye_msgs::YawrateOffset> yawrate_offset_1st;
    std::vector<eagleye_msgs::YawrateOffset> yawrate_offset_2nd;
    std::vector<eagleye_msgs::SlipAngle> slip_angle;
    std::vector<eagleye_msgs::Height> height;
    std::vector<eagleye_msgs::Pitching> pitching;
    std::vector<eagleye_msgs::AccXOffset> acc_x_offset;
    std::vector<eagleye_msgs::AccXScaleFactor> acc_x_scale_factor;
    std::vector<eagleye_msgs::Position> enu_relative_pos;
    std::vector<geometry_msgs::Vector3Stamped> enu_vel;
    std::vector<eagleye_msgs::Position> enu_absolute_pos;
    std::vector<eagleye_msgs::Position> enu_absolute_pos_interpolate; 
    std::vector<eagleye_msgs::Position> gnss_smooth_pos_enu;
    std::vector<eagleye_msgs::Rolling> rolling;
    std::vector<sensor_msgs::NavSatFix> eagleye_fix;
    std::vector<geometry_msgs::TwistStamped> eagleye_twist;
  };

  EagleyeStates eagleye_state_forward_, eagleye_state_backward_;
  std::vector<double> enu_smoothing_trajectory_east_, enu_smoothing_trajectory_north_, enu_smoothing_trajectory_height_;
  std::vector<double> smoothing_trajectory_status_;
  std::vector<double> llh_smoothing_trajectory_lat_,llh_smoothing_trajectory_lon_,llh_smoothing_trajectory_hei_;

  // Data retrieved from .yaml file
  double interval_plot_;
  double interval_line_;
  // Distance interval in the kml file to output eagleye estimation results [m]

  bool output_log_ = true; 
  bool timestamp_sort_ = true;
  int convert_height_num_;

  bool output_kml_eagleye_forward_plot_;  
  bool output_kml_eagleye_backward_plot_;
  bool output_kml_eagleye_pp_plot_;
  bool output_kml_rtklib_plot_;
  bool output_kml_gnss_plot_;
  bool output_kml_eagleye_forward_line_;
  bool output_kml_eagleye_backward_line_;
  bool output_kml_eagleye_pp_line_;

  std::string use_gnss_mode_;
  bool use_nmea_downsample_;
  double nmea_downsample_freq_;

  struct HeadingParameter heading_parameter_;
  struct HeadingInterpolateParameter heading_interpolate_parameter_;
  struct YawrateOffsetStopParameter yawrate_offset_stop_parameter_;
  struct YawrateOffsetParameter yawrate_offset_1st_parameter_;
  struct YawrateOffsetParameter yawrate_offset_2nd_parameter_;
  struct SlipangleParameter slip_angle_parameter_;
  struct TrajectoryParameter trajectory_parameter_;
  struct VelocityScaleFactorParameter velocity_scale_factor_parameter_;
  struct PositionParameter position_parameter_;
  struct PositionInterpolateParameter position_interpolate_parameter_;
  struct SmoothingParameter smoothing_parameter_;
  struct HeightParameter height_parameter_;
  struct RollingParameter rolling_parameter_;

  // Function declaration
  public:

  eagleye_pp();//Constructor

  void setOutputPath(std::string arg_output_path);
  void setParam(YAML::Node arg_conf, std::string *arg_twist_topic, std::string *arg_imu_topic, std::string *arg_rtklib_nav_topic, std::string *arg_nmea_sentence_topic);
  void setDataLength();
  std::size_t getDataLength();
  std::string getUseGNSSMode();
  std::vector<rtklib_msgs::RtklibNav> getRtklibNavVector();
  bool getUseBackward();
  bool getUseCombination();


  void syncTimestamp(bool arg_nmea_data_flag, rosbag::View& arg_in_view);
  void estimatingEagleye(bool arg_forward_flag);

  // Function to calculate the initial azimuth
  void setGPSTime(double arg_GPSTime[]);
  void calcMissPositiveFIX(double arg_TH_POSMAX, double arg_GPSTime[]);
  void calcPickDR(double arg_GPSTime[], bool *arg_flag_SMRaw_2D, std::vector<int> &arg_index_DRs, std::vector<int> &arg_index_DRe);
  void calcInitialHeading(double arg_GPSTime[], bool arg_flag_SMRaw_2D[], std::vector<int> arg_index_DRs, std::vector<int> arg_index_DRe);
  void smoothingDeadReckoning();

  void smoothingTrajectory();
  void convertHeight();

  // output function
  void writePointKMLOneWay(std::stringstream* eagleye_plot, std::string name, const EagleyeStates& eagleye_state, int i);

  void writePointKML(bool arg_use_rtk_navsatfix_topic,std::string* arg_s_eagleye_line,std::string* arg_s_eagleye_back_line, std::string* arg_s_eagleye_pp_line);
  void addHeaderLineKML(std::ofstream* output_line_kml_file);
  void addLineStyleInKML(std::ofstream* output_line_kml_file, std::string style_id, std::string color);
  void addPlacemarkInKML(std::ofstream* output_line_kml_file,std::string name, int visibility,std::string coordinates);
  void updateLineKML(std::stringstream* eagleye_line,double llh[3], eagleye_msgs::Distance distance, double * driving_distance_last);
  void writeLineKML(bool arg_use_rtk_navsatfix_topic,std::string* arg_s_eagleye_line,std::string* arg_s_eagleye_back_line, std::string* arg_s_eagleye_pp_line);

  void writeSimpleCSV();

  void writeDetailCSVOneWay(std::ofstream* output_log_csv_file, const EagleyeStates& eagleye_state);
  void writeDetailCSV();
};
