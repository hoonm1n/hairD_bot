#include <ros/ros.h>
#include <std_msgs/String.h>
#include <geometry_msgs/Twist.h>
#include <iostream>
#include <string>
#include <omo_r1_bringup/yolo_check.h>
#include <omo_r1_bringup/navigation_check.h>
#include "std_msgs/Int32.h"
#include <omo_r1_bringup/et_check.h>


class robot_con
{
  public:
    int qt_con= 1;
    std::string qt_con_str = "";                           //1, 2, 3
    std::string nav_con = "before";          //before, proceeding, done
    int flag = 0;                            //0, 1
    int nav_flag = 0;
    std::string fix_con = "close";         //open, close
    int flag_time = 0;
    int flag_val = 0;
    std::string yolo_con = "before";         //yet, detected, done
    std::string yolo_check = "stop";         //stop, start
    int flag_back = 0;
};

robot_con con = robot_con();


// 서비스 핸들러 함수
bool etServiceHandler(omo_r1_bringup::et_check::Request& req, omo_r1_bringup::et_check::Response& res)
{
    ROS_INFO("Service Request: %s, Service Response: %s", req.et_req.c_str(), res.et_res.c_str()); // 로그 출력
    if(con.flag == 0)
    {
      std::cout<<"service in"<<std::endl;
      con.qt_con_str = req.et_req;
      con.qt_con = std::stoi(req.et_req);
      con.flag = 1;
      con.flag_val = 1;
      con.flag_time = 1;

    }

    return true; // 서비스 요청 처리 성공
}


void qtCallback(const std_msgs::String::ConstPtr& msg)
{
  if(con.flag == 0){
    con.qt_con = std::stoi(msg->data.c_str());
    con.flag = 1;
  }
}



void navCallback(const std_msgs::String::ConstPtr& msg)
{
  con.nav_con = msg->data;
  std::cout<<con.nav_con<<std::endl;
  if(con.nav_con == "done"){
 
  }
}

void yoloCallback(const std_msgs::String::ConstPtr& msg)
{
  con.yolo_con = msg->data;
  std::cout<<con.yolo_con<<std::endl;
}

void fixCallback(const std_msgs::String::ConstPtr& msg)
{
  con.fix_con = msg->data;
  std::cout<<con.fix_con<<std::endl;
}


int main(int argc, char** argv)
{
  ros::init(argc, argv, "emptying_hair");
  ros::NodeHandle nh;

  // 비동기 스피너 생성
  ros::AsyncSpinner spinner(2);  // 쓰레드 수를 지정할 수 있음

  // 스피너 시작
  spinner.start();

  ros::ServiceServer service_server = nh.advertiseService("et_service", etServiceHandler);

  ros::Publisher navStart_pub = nh.advertise<std_msgs::Int32>("nav_start_em", 1000);
  ros::Publisher fixStart_pub = nh.advertise<std_msgs::String>("fix_start", 1000);
  ros::Publisher cmd_vel_pub = nh.advertise<geometry_msgs::Twist>("cmd_vel", 10);
  ros::Publisher yoloStart_pub = nh.advertise<std_msgs::String>("yolo_start", 1000);


  ros::Subscriber nav_sub = nh.subscribe("nav_info_em", 1000, navCallback);
  ros::Subscriber qt_sub = nh.subscribe("qt_em", 1000, qtCallback);
  ros::Subscriber fix_sub = nh.subscribe("fix_info", 1000, fixCallback);
  ros::Subscriber yolo_sub = nh.subscribe("yolo_info", 1000, yoloCallback);

  geometry_msgs::Twist cmd_vel;

  ros::Time start_time = ros::Time::now();
  
  ros::Rate loop_rate(10);
  while (ros::ok())
  {
    ros::Time current_time = ros::Time::now();

    if(con.flag == 1){
      if(con.fix_con == "close" and con.flag_val == 1){
        std_msgs::String fix_msg;
        fix_msg.data = "open";
        fixStart_pub.publish(fix_msg);

        while(con.fix_con == "close"){
            std::cout<<"tray open wait "<<std::endl;
            std::cout<<"fix_info: "<<con.fix_con<<std::endl;
            start_time = ros::Time::now();
        }
      }
      if(con.flag_time == 1){
        if((current_time - start_time).toSec()<= 2.0){
          cmd_vel.linear.x = 0.3;
          cmd_vel.angular.z = 0;
          // cmd_vel 메시지 발송
          cmd_vel_pub.publish(cmd_vel);
        }
        else{
          cmd_vel.linear.x = 0.0;
          cmd_vel.angular.z = 0.0;
          // cmd_vel 메시지 발송
          cmd_vel_pub.publish(cmd_vel);
          con.flag_time = 0;
        }
      }

      if (con.nav_con == "before" && con.flag_time == 0){                                                           //before cleaning
        std_msgs::Int32 nav_msg;
        nav_msg.data = con.qt_con;

        navStart_pub.publish(nav_msg);
        con.flag_val = 0;
      }
      if(con.nav_con == "proceeding"){
        start_time = ros::Time::now();
      }
      if (con.nav_con == "done" && con.flag_back == 0){
        if((current_time - start_time).toSec()<= 2.0){
          cmd_vel.linear.x = -0.3;
          cmd_vel.angular.z = 0;
          // cmd_vel 메시지 발송
          cmd_vel_pub.publish(cmd_vel);
        }
        else{
          cmd_vel.linear.x = 0.0;
          cmd_vel.angular.z = 0.0;
          // cmd_vel 메시지 발송
          cmd_vel_pub.publish(cmd_vel);
          con.flag_back = 1;
        }
      }     
      if (con.nav_con == "done" && con.flag_back == 1){
        if(con.yolo_con == "yet"){
          std_msgs::Int32 nav_msg;
          nav_msg.data = 10;
          con.qt_con = 10;
          navStart_pub.publish(nav_msg); 
        
          std_msgs::String yolo_msg;
          yolo_msg.data = "start";
          yoloStart_pub.publish(yolo_msg);
        }
        else if(con.yolo_con == "detected"){
          std_msgs::Int32 nav_msg;
          nav_msg.data = 10;
          con.qt_con = 10;
          navStart_pub.publish(nav_msg); 

          std_msgs::String yolo_msg;
          yolo_msg.data = "start";
          yoloStart_pub.publish(yolo_msg);
        }
        else if(con.yolo_con == "done"){
          std_msgs::String yolo_msg;
          yolo_msg.data = "stop";
          yoloStart_pub.publish(yolo_msg);
          con.yolo_check == "stop";

          std_msgs::Int32 nav_msg;
          nav_msg.data = 0;
          con.qt_con = 0;
          navStart_pub.publish(nav_msg);

          std_msgs::String fix_msg;
          fix_msg.data = "close";
          fixStart_pub.publish(fix_msg);

          while(con.fix_con == "open"){
            
          }
          con.flag = 0; 
          con.nav_flag = 0; 
          con.flag_back = 0;
        }
        else{
          std_msgs::Int32 nav_msg;
          nav_msg.data = 10;
          con.qt_con = 10;
          navStart_pub.publish(nav_msg); 
        }
      }


      // if (con.nav_con == "done"){
      //   std_msgs::Int32 nav_msg;
      //   nav_msg.data = 0;
      //   con.qt_con = 0;
      //   navStart_pub.publish(nav_msg);

      //   std_msgs::String fix_msg;
      //   fix_msg.data = "close";
      //   fixStart_pub.publish(fix_msg);

      //   while(con.fix_con == "open"){
            
      //   }
        
      //   con.flag = 0; 
      //   con.nav_flag = 0; 
      // }
    }
    ros::spinOnce();
    loop_rate.sleep();
    // ros::spinOnce();
    // rate.sleep();
  }

  // 스피너 정지
  spinner.stop();
  return 0;
}
