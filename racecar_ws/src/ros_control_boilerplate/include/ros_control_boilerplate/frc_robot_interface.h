/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2015, University of Colorado, Boulder
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Univ of CO, Boulder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Original Author: Dave Coleman
   Desc:   Example ros_control hardware interface that performs a perfect control loop for
   simulation
*/

#pragma once
#include <thread>
#include <atomic>

// ROS
#include <ros/ros.h>
#include <urdf/model.h>
#include <std_msgs/Bool.h>

// ROS Controls
#include <controller_manager/controller_manager.h>
#include <hardware_interface/joint_command_interface.h>
#include <hardware_interface/robot_hw.h>
#include <talon_interface/talon_command_interface.h>
#include "frc_interfaces/robot_controller_interface.h"
#include "frc_interfaces/pcm_state_interface.h"
#include "frc_interfaces/robot_controller_interface.h"
#include <hardware_interface/imu_sensor_interface.h>

namespace ros_control_boilerplate
{

// Joint used to communicate internally in the hw
// interface(s) by name rather than via some index
// in an array. Typically used for communicating with
// the DS in some way
class DummyJoint
{
	public :
		DummyJoint(const std::string &name, double *address) :
			name_(name), address_(address)
		{
			name_.erase(name.find_last_not_of('_') + 1);
		}
		std::string name_;
		double *address_;
};
#define Dumify(name) ros_control_boilerplate::DummyJoint(#name, &name)

class CustomProfileState
{
	public:
		CustomProfileState()
			: time_start_(ros::Time::now().toSec())
			, iteration_count_(0)
			, points_run_(0)
	{
	}

	double time_start_;
	int iteration_count_;
	int points_run_;
	hardware_interface::CustomProfileStatus status_;
	std::vector<std::vector<hardware_interface::CustomProfilePoint>> saved_points_;
	std::vector<std::vector<double>> saved_times_;
};

/// \brief Hardware interface for a robot
class FRCRobotInterface : public hardware_interface::RobotHW
{
	public:
		/**
		 * \brief Constructor
		 * \param nh - Node handle for topics.
		 * \param urdf - optional pointer to a parsed robot model
		 */
		FRCRobotInterface(ros::NodeHandle &nh, urdf::Model *urdf_model = NULL);

		/** \brief Destructor */
		virtual ~FRCRobotInterface() {}

		/** \brief Initialize the hardware interface */
		virtual void init();

		/** \brief Read the state from the robot hardware. */
		virtual void read(ros::Duration &elapsed_time) = 0;

		/** \brief Write the command to the robot hardware. */
		virtual void write(ros::Duration &elapsed_time) = 0;

		/** \brief Set all members to default values */
		virtual void reset();

		/**
		 * \brief Check (in non-realtime) if given controllers could be started and stopped from the
		 * current state of the RobotHW
		 * with regard to necessary hardware interface switches. Start and stop list are disjoint.
		 * This is just a check, the actual switch is done in doSwitch()
		 */
		virtual bool canSwitch(const std::list<hardware_interface::ControllerInfo> &/*start_list*/,
							   const std::list<hardware_interface::ControllerInfo> &/*stop_list*/) const
		{
			return true;
		}

		/**
		 * \brief Perform (in non-realtime) all necessary hardware interface switches in order to start
		 * and stop the given controllers.
		 * Start and stop list are disjoint. The feasability was checked in canSwitch() beforehand.
		 */
		virtual void doSwitch(const std::list<hardware_interface::ControllerInfo> &/*start_list*/,
							  const std::list<hardware_interface::ControllerInfo> &/*stop_list*/)
		{
		}

		/** \brief Helper for debugging a joint's state */
		virtual void printState();
		std::string printStateHelper();

		/** \brief Helper for debugging a joint's command */
		std::string printCommandHelper();

	protected:
		/** \brief Get the URDF XML from the parameter server */
		virtual void loadURDF(ros::NodeHandle &nh, std::string param_name);
		virtual std::vector<DummyJoint> getDummyJoints(void) { return std::vector<DummyJoint>();}

		// Short name of this class
		std::string name_;

		// Startup and shutdown of the internal node inside a roscpp program
		ros::NodeHandle nh_;

        //Subscriber 
        ros::Subscriber enable_sub_;
        std::atomic<bool> robot_enabled;

		// Hardware interfaces
		hardware_interface::JointStateInterface       joint_state_interface_;
		hardware_interface::TalonStateInterface       talon_state_interface_;
		hardware_interface::PCMStateInterface	      pcm_state_interface_;

		hardware_interface::JointCommandInterface  joint_command_interface_;
		hardware_interface::PositionJointInterface joint_position_interface_;
		hardware_interface::VelocityJointInterface joint_velocity_interface_;
		hardware_interface::EffortJointInterface   joint_effort_interface_;
		hardware_interface::TalonCommandInterface  talon_command_interface_;

		hardware_interface::ImuSensorInterface imu_interface_;

		hardware_interface::RobotControllerStateInterface robot_controller_state_interface_;

        std::vector<CustomProfileState> custom_profile_state_;

		void custom_profile_write(int joint_id);
		void custom_profile_set_talon(hardware_interface::TalonMode mode, double setpoint, double fTerm, int joint_id, int pidSlot, bool zeroPos);

		//void readJointLocalParams(XmlRpc::XmlRpcValue joint_params,
		//						  const bool local,
		//						  const bool saw_local_keyword,
		//						  bool &local_update,
		//						  bool &local_hardware);

		// Configuration
		std::vector<std::string> can_talon_srx_names_;
		std::vector<int>         can_talon_srx_can_ids_;
		std::size_t              num_can_talon_srxs_;

		//I think inverts are worth having on below 3
		std::vector<std::string> digital_input_names_;
		std::vector<int>         digital_input_dio_channels_;
		std::vector<bool>        digital_input_inverts_;
		std::size_t              num_digital_inputs_;

		std::vector<std::string> digital_output_names_;
		std::vector<int>         digital_output_dio_channels_;
		std::vector<bool>        digital_output_inverts_;
		std::size_t              num_digital_outputs_;

		std::vector<std::string> pwm_names_;
		std::vector<int>         pwm_pwm_channels_;
		std::vector<bool>        pwm_inverts_;
		std::size_t              num_pwm_;

		std::vector<std::string> solenoid_names_;
		std::vector<int>         solenoid_ids_;
		std::vector<int>         solenoid_pcms_;
		std::size_t              num_solenoids_;

		std::vector<std::string> double_solenoid_names_;
		std::vector<int>         double_solenoid_forward_ids_;
		std::vector<int>         double_solenoid_reverse_ids_;
		std::vector<int>         double_solenoid_pcms_;
		std::size_t              num_double_solenoids_;

		std::vector<std::string> compressor_names_;
		std::vector<int>         compressor_pcm_ids_;
		std::size_t              num_compressors_;


		std::vector<std::string> rumble_names_;
		std::vector<int>         rumble_ports_;
		std::size_t              num_rumbles_;

		std::vector<std::string> imu_names_;
		std::vector<std::string> imu_frame_ids_;
		std::vector<int>         imu_ids_;
		std::vector<std::string> imu_devices_;

		std::size_t              num_imu_;

		std::vector<std::string> analog_input_names_;
		std::vector<int>         analog_input_analog_channels_;
		std::vector<double>      analog_input_a_;
		std::vector<double>      analog_input_b_;
		std::size_t              num_analog_inputs_;

		std::vector<std::string> dummy_joint_names_;
		std::size_t              num_dummy_joints_;

		std::vector<std::string> ready_signal_names_;
		std::size_t              num_ready_signals_;

		std::vector<std::string> joystick_names_;
		std::vector<int>         joystick_ids_; // pretty sure this is montonic increasing by default?
		std::size_t              num_joysticks_;

		bool run_hal_robot_;
		std::string can_interface_;

		urdf::Model *urdf_model_;

		// Array holding master cached state of hardware
		// resources
		std::vector<hardware_interface::TalonHWState> talon_state_;

		std::vector<double> digital_input_state_;
		std::vector<double> digital_output_state_; //No actual data
		std::vector<double> pwm_state_; //No actual data
		std::vector<double> solenoid_state_;
		std::vector<double> double_solenoid_state_;
		std::vector<double> rumble_state_; //No actual data
		std::vector<double> imu_state_;
		std::vector<double> compressor_state_;
		std::vector<hardware_interface::PCMState> pcm_state_;
		hardware_interface::RobotControllerState robot_controller_state_;

		// Each entry in the vector is an array. That array holds
		// the data returned from one particular imu
		std::vector<std::array<double,4>> imu_orientations_; // x,y,z,w
		std::vector<std::array<double,9>> imu_orientation_covariances_; // [x,y,z] x [x,y,z]
		std::vector<std::array<double,3>> imu_angular_velocities_; //x,y,z
		std::vector<std::array<double,9>> imu_angular_velocity_covariances_;
		std::vector<std::array<double,3>> imu_linear_accelerations_; // x,y,z
		std::vector<std::array<double,9>> imu_linear_acceleration_covariances_;

		std::vector<double> analog_input_state_;
		// Same as above, but for pending commands to be
		// written to the hardware
		std::vector<hardware_interface::TalonHWCommand> talon_command_;
		std::vector<double> digital_output_command_;
		std::vector<double> pwm_command_;
		std::vector<double> solenoid_command_;
		std::vector<double> double_solenoid_command_;
		std::vector<double> rumble_command_;
        std::vector<double> offset_imu_;
		std::vector<double> compressor_command_;

		std::vector<double> dummy_joint_position_;
		std::vector<double> dummy_joint_velocity_;
		std::vector<double> dummy_joint_effort_;
		std::vector<double> dummy_joint_command_;

		std::vector<double> robot_ready_signals_;
		bool                robot_code_ready_;

};  // class

}  // namespace
