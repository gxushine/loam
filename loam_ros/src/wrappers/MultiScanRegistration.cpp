// Copyright 2013, Ji Zhang, Carnegie Mellon University
// Further contributions copyright (c) 2016, Southwest Research Institute
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is an implementation of the algorithm described in the following paper:
//   J. Zhang and S. Singh. LOAM: Lidar Odometry and Mapping in Real-time.
//     Robotics: Science and Systems Conference (RSS). Berkeley, CA, July 2014.

#include "loam_velodyne/MultiScanRegistration.h"
#include "loam_utils/math_utils.h"

#include <pcl_conversions/pcl_conversions.h>


namespace loam {


bool MultiScanRegistration::setup(ros::NodeHandle& node,
                                  ros::NodeHandle& privateNode)
{
  if (!ScanRegistration::setup(node, privateNode)) {
    return false;
  }

  const char* module_name = "MultiScanRegistration:";

  float vAngleMin, vAngleMax;
  int nScanRings;

  // fetch scan matching params
  if (node.getParam("/loam/registration/lidar_model", _params.lidarModel)) {
    if (_params.lidarModel == "VLP-16") {
      _scanMapper = MultiScanMapper::Velodyne_VLP_16();
    } else if (_params.lidarModel == "HDL-32") {
      _scanMapper = MultiScanMapper::Velodyne_HDL_32();
    } else if (_params.lidarModel == "HDL-64E") {
      _scanMapper = MultiScanMapper::Velodyne_HDL_64E();
    } else {
      ROS_ERROR("%s: Invalid lidar parameter: %s (only \"VLP-16\", \"HDL-32\" and \"HDL-64E\" are supported)", module_name, _params.lidarModel.c_str());
      return false;
    }
    ROS_INFO("%s: Set  %s  scan mapper.", module_name, _params.lidarModel.c_str());

  } else if (node.getParam("/loam/registration/min_vertical_angle", vAngleMin) &&
      node.getParam("/loam/registration/max_vertical_angle", vAngleMax) &&
      node.getParam("/loam/registration/n_scan_rings", nScanRings)) {
    if (vAngleMin >= vAngleMax) {
      ROS_ERROR("%s: Invalid vertical range (min >= max)", module_name);
      return false;
    } else if (nScanRings < 2) {
      ROS_ERROR("%s: Invalid number of scan rings (n < 2)", module_name);
      return false;
    }
    _scanMapper.set(vAngleMin, vAngleMax, nScanRings);
    ROS_INFO("%s: Set linear scan mapper from %g to %g degrees with %d scan rings.", module_name, vAngleMin, vAngleMax, nScanRings);

  }
  else {
    ROS_ERROR("%s: Invalid scan registration parameters", module_name);
    ROS_ERROR("%s: Default VLP-16 registration model will be used", module_name);
  }
  
  // subscribe to input cloud topic
  _subLaserCloud = node.subscribe<sensor_msgs::PointCloud2>
      ("/multi_scan_points", 2, &MultiScanRegistration::handleCloudMessage, this);

  return true;
}



void MultiScanRegistration::handleCloudMessage(const sensor_msgs::PointCloud2ConstPtr &laserCloudMsg)
{
  if (_systemDelay > 0) {
    _systemDelay--;
    return;
  }

  // fetch new input cloud
  pcl::PointCloud<pcl::PointXYZ> laserCloudIn;
  pcl::fromROSMsg(*laserCloudMsg, laserCloudIn);

  process(laserCloudIn, laserCloudMsg->header.stamp);
}

} // end namespace loam