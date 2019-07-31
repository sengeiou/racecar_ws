#!/bin/bash

git submodule foreach git checkout master

#Clones usb cam driver from external github repository and overrides everything
#in the local repository except launch files
echo "Installing usb cam driver"
cd ../racecar_ws/src/adi_usb_cam_kinetic || exit
git clone https://github.com/ros-drivers/usb_cam.git
cd usb_cam || exit
rm -R launch
cd ..
mv usb_cam/* .
rm -R -f usb_cam


cd ../zed-ros-wrapper || exit
chmod 777 ZED_SDK_JTX2_JP3.2_v2.7.1.run
./ZED_SDK_JTX2_JP3.2_v2.7.1.run
cd cfg/ || exit
chmod 777 Zed.cfg


#Clones IMU driver from external github repository and overrides everything
#in the local repository except launch files
echo "Installing Analog Devices IMU driver"
cd ../../adi_driver || exit
git clone https://github.com/tork-a/adi_driver.git
cd adi_driver || exit
rm -R launch
cd ..
mv adi_driver/* .
rm -R -f adi_driver

SDK="PicoZenseSDK_aarch64_linux"
#SDK="PicoZenseSDK_Ubuntu16.04_DCAM710"
cd ../adi_tof_kinetic/dependencies/$SDK  || exit
chmod 777 install.sh
./install.sh


cd ../../../../..  || exit
echo "Lidar Permission for Sensors"
sudo chmod a+rw /dev/ttyUSB0

echo "USB and IMU dependencies"
sudo apt-get -y install \
    ros-melodic-urdf-geometry-parser ros-melodic-four-wheel-steering-msgs ros-melodic-imu-sensor-controller ros-melodic-camera-info-manager ros-melodic-image-transport libbullet-dev libsdl-image1.2-dev ros-melodic-geometry2 ros-melodic-navigation ros-melodic-imu-filter-madgwick ros-melodic-roslint ros-melodic-joy ros-melodic-teleop-twist-joy ros-melodic-teleop-twist-keyboard ros-melodic-laser-proc ros-melodic-rgbd-launch ros-melodic-depthimage-to-laserscan ros-melodic-rosserial-arduino ros-melodic-rosserial-python ros-melodic-rosserial-server ros-melodic-rosserial-client ros-melodic-rosserial-msgs ros-melodic-amcl ros-melodic-map-server ros-melodic-move-base ros-melodic-urdf ros-melodic-xacro ros-melodic-compressed-image-transport ros-melodic-rqt-image-view ros-melodic-navigation ros-melodic-interactive-markers ros-melodic-ackermann-msgs ros-melodic-serial
echo "Install Complete"
