/** @date 2016
    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2016 Sensics Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <thread>
#include <iostream>

#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/TrackerInterfaceC.h>

#include "org_osvr_Vive_Libre_json.h"

#include "vive.h"

#include <Eigen/Geometry>

static const auto PREFIX = "[OSVR-Vive-Libre] ";


class TrackerDevice {
  public:
    TrackerDevice(OSVR_PluginRegContext ctx) {
        /* init osvr device */

        std::cout << PREFIX << "Init device." << std::endl;
        
        OSVR_DeviceInitOptions opts = osvrDeviceCreateInitOptions(ctx);

        // configure our tracker
        osvrDeviceTrackerConfigure(opts, &m_tracker);

        /// Create the device token with the options
        m_dev.initAsync(ctx, "Tracker", opts);

        /// Send JSON descriptor
        m_dev.sendJsonDescriptor(org_osvr_Vive_Libre_json);

        /// Register update callback
        m_dev.registerUpdateCallback(this);

        /* init openhmd */

        vive = vive_init();

    }

    OSVR_Quaternion openhmd_to_osvr_quaternion(quatf in) {
        OSVR_Quaternion quat;
        quat.data[0] = in.x;
        quat.data[1] = in.y;
        quat.data[2] = in.z;
        quat.data[3] = in.w;
        return quat;
    }

    OSVR_Quaternion eigen_to_osvr_quaternion(Eigen::Quaternionf in) {
        OSVR_Quaternion quat;
        quat.data[0] = in.w();
        quat.data[1] = in.x();
        quat.data[2] = in.y();
        quat.data[3] = in.z();
        return quat;
    }

    OSVR_ReturnCode update() {

        /*
        // Simulate waiting a quarter second for data.
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
        double t = 5.0 * ((double)now.seconds + ((double)now.microseconds / 1000000.0));
        pose.translation.data[0] = std::sin(t) * 0.25;
        pose.translation.data[1] = std::cos(t + 0.5) * 0.25;
        pose.translation.data[2] = std::sin(t + 0.25) * 0.25;
        */

        OSVR_TimeValue now;
        osvrTimeValueGetNow(&now);


        /// Report pose for sensor 0
        OSVR_PoseState pose;
        osvrPose3SetIdentity(&pose);

        pose.rotation = eigen_to_osvr_quaternion(imu_to_pose(vive));
        osvrDeviceTrackerSendPose(m_dev, m_tracker, &pose, 0);

        return OSVR_RETURN_SUCCESS;
    }

  private:
    osvr::pluginkit::DeviceToken m_dev;
    OSVR_TrackerDeviceInterface m_tracker;
    vive_priv* vive;
    //ohmd_device* hmd;
};

class HardwareDetection {
  public:
    HardwareDetection() : m_found(false) {
    
    std::cout << PREFIX << "Detecting VIVE hardware." << std::endl;
    
    }
    OSVR_ReturnCode operator()(OSVR_PluginRegContext ctx) {

        if (m_found) {
            return OSVR_RETURN_SUCCESS;
        }

        /// we always detect device in sample plugin
        m_found = true;

        /// Create our device object
        osvr::pluginkit::registerObjectForDeletion(ctx, new TrackerDevice(ctx));

        return OSVR_RETURN_SUCCESS;
    }

  private:
    bool m_found;
};

OSVR_PLUGIN(org_osvr_Vive_Libre) {

    osvr::pluginkit::PluginContext context(ctx);
    
    std::cout << PREFIX << "Plugin init." << std::endl;

    /// Register a detection callback function object.
    context.registerHardwareDetectCallback(new HardwareDetection());

    return OSVR_RETURN_SUCCESS;
}
