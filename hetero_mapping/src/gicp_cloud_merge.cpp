// CPP
#include <stdio.h>
#include <iostream>
#include <cmath>
// ROS
#include <ros/ros.h>
#include "sensor_msgs/PointCloud2.h"
// PCL LIBRARY
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/registration/registration.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/gicp.h>
#include <tf_conversions/tf_eigen.h>
#include <tf/transform_broadcaster.h>
#include <pcl/filters/voxel_grid.h> 

using namespace std;
using namespace ros;

boost::shared_ptr<NodeHandle> node_; 
Subscriber pcSubscriber;
Subscriber pcSubscriber1;
Publisher pcPublisher;

Eigen::Matrix4f trans_final = Eigen::Matrix4f::Identity();

pcl::PointCloud<pcl::PointXYZI>::Ptr ugv_cloud          (new pcl::PointCloud<pcl::PointXYZI>);
pcl::PointCloud<pcl::PointXYZI>::Ptr uav_cloud           (new pcl::PointCloud<pcl::PointXYZI>);

bool pc_avaliable = false, pc2_avaliable = false;

void pcCallback_uav(const sensor_msgs::PointCloud2ConstPtr &laserCloudMsg){
  uav_cloud.reset(new pcl::PointCloud<pcl::PointXYZI>);
  pcl::fromROSMsg(*laserCloudMsg, *uav_cloud);
  pc2_avaliable=true;
}

void pcCallback_ugv(const sensor_msgs::PointCloud2ConstPtr &laserCloudMsg){
  ugv_cloud.reset(new pcl::PointCloud<pcl::PointXYZI>);
  pcl::fromROSMsg(*laserCloudMsg, *ugv_cloud);
  pc_avaliable=true;
}



void process(){

    pcl::PointCloud<pcl::PointXYZI> final_cloud;
    pcl::GeneralizedIterativeClosestPoint<pcl::PointXYZI, pcl::PointXYZI> icp; // Generalized
    icp.setMaximumIterations (1000);
    icp.setInputSource(ugv_cloud);
    icp.setInputTarget(uav_cloud);
    icp.align(final_cloud);
    trans_final = icp.getFinalTransformation();
    std::cout << "has converged:" << icp.hasConverged() <<" Score: " << icp.getFitnessScore() <<std::endl;
    pcl::PointCloud<pcl::PointXYZI> total_cloud;
    total_cloud=final_cloud;
    total_cloud+=*uav_cloud;
    sensor_msgs::PointCloud2 cloud_msg1;
    toROSMsg(total_cloud, cloud_msg1);
    cloud_msg1.header.frame_id ="world";
    cloud_msg1.header.stamp = ros::Time::now();
    pcPublisher.publish(cloud_msg1); 
    
    //pcl::transformPointCloud (*uav_cloud, *uav_cloud, trans_final);
    
  







 // pc_avaliable=false;pc2_avaliable=false;
}


int main (int argc, char** argv)
{
  ros::init (argc, argv, "gicp_cloud_merge");
  node_.reset(new NodeHandle());
  pcSubscriber  = node_->subscribe("/uav/octomap_point_cloud_centers",        200, pcCallback_uav);
  pcSubscriber1 = node_->subscribe("/ugv/octomap_point_cloud_centers",   200, pcCallback_ugv);
  pcPublisher = node_->advertise<sensor_msgs::PointCloud2>("total_cloud",1,false);
  
  printf("Starting ... \n" );
  ros::Rate r(1);
  while(node_->ok()){
    if (pc_avaliable && pc2_avaliable)process();
    ros::spinOnce();
    r.sleep();
  }
  return (0);
}