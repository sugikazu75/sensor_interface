#include <takasako_sps/scpi_tcp_client.h>

TakasokoSps::TakasokoSps(ros::NodeHandle nh, ros::NodeHandle nhp): nh_(nh), nhp_(nhp)
{
  nhp_.param("tm_loop_rate", tm_loop_rate_, 10.0);
  nhp_.param("port", port_, 50001);
  nhp_.param("ip_address", ip_address_, std::string("192.168.0.1"));

  //socket

  if((sd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    ROS_ERROR("can not open socket");
    ros::shutdown();
    return;
  }
 
  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(port_);
  addr_.sin_addr.s_addr = inet_addr(ip_address_.c_str());
  connect(sd_, (struct sockaddr *)&addr_, sizeof(struct sockaddr_in));

  //initialize
  if  (write(sd_, "ADDR 1;:ALM:CLEar\n",18)==-1 )
    {
      ROS_ERROR("can not send socket");
      ros::shutdown();
      return;
    }
  memset(read_buf_, 0, sizeof(read_buf_));
  read(sd_, read_buf_, sizeof(read_buf_));
  printf("%s", read_buf_);
  if(!strstr(read_buf_, "OK"))
    {
      ROS_ERROR("can not initialize");
      ros::shutdown();
      return;
    }

  if  (write(sd_, "*IDN?\n",6)==-1 )
    {
      ROS_ERROR("can not send socket");
      ros::shutdown();
      return;
    }
  memset(read_buf_, 0, sizeof(read_buf_));
  read(sd_, read_buf_, sizeof(read_buf_));
  ROS_INFO("initialized: %s", read_buf_);


  power_info_pub_ = nh_.advertise<takasako_sps::PowerInfo>("power_info", 5);
  power_on_sub_ = nh_.subscribe<std_msgs::Empty>("power_on_cmd", 1, &TakasokoSps::powerOnCallback, this, ros::TransportHints().tcpNoDelay());
  power_off_sub_ = nh_.subscribe<std_msgs::Empty>("power_off_cmd", 1, &TakasokoSps::powerOffCallback, this, ros::TransportHints().tcpNoDelay());
  target_voltage_sub_ = nh_.subscribe<std_msgs::Int32>("target_voltage", 1, &TakasokoSps::targetVoltageCallback, this, ros::TransportHints());
  timer_ = nhp_.createTimer(ros::Duration(1.0 / tm_loop_rate_), &TakasokoSps::powerInfoFunction,this);

}


void TakasokoSps::powerOnCallback(const std_msgs::EmptyConstPtr &msg)
{
  if  (write(sd_, "OUTP ON\n" ,8)==-1 )
    {
      ROS_ERROR("can not send socket");
      return;
    }

  memset(read_buf_, 0, sizeof(read_buf_));
  read(sd_, read_buf_, sizeof(read_buf_));

  //bad
  ROS_ERROR("power on!");
  power_flag_ = true;

  /*
  if(strstr(read_buf_, "OK"))
    {
      ROS_ERROR("power on!");
      power_flag_ = true;
    }
  else
    {
      ROS_ERROR("%s", read_buf_);
    }
  */
}

void TakasokoSps::powerOffCallback(const std_msgs::EmptyConstPtr &msg)
{
  if  (write(sd_, "OUTP OFF\n" ,9)==-1 )
    {
      ROS_ERROR("can not send socket");
      return;
    }

  memset(read_buf_, 0, sizeof(read_buf_));
  read(sd_, read_buf_, sizeof(read_buf_));

  //bad
  ROS_ERROR("power off!");
  power_flag_ = false;

  /*
  if(strstr(read_buf_, "OK"))
    {
      ROS_ERROR("power off!");
      power_flag_ = false;
    }
  else
    {
      ROS_ERROR("%s", read_buf_);
    }
  */

}

void TakasokoSps::targetVoltageCallback(const std_msgs::Int32ConstPtr &msg)
{
  int voltage = msg->data;
  std::string cmd = "SOUR VOLT " + std::to_string(voltage) + "\n";
  int length = cmd.size();
  if(write(sd_, cmd.c_str(), length) == -1)
    {
      ROS_ERROR("can not send socket");
      return;
    }
  memset(read_buf_, 0, sizeof(read_buf_));
  read(sd_, read_buf_, sizeof(read_buf_));
}

void TakasokoSps::powerInfoFunction(const ros::TimerEvent & e)
{
  if(!power_flag_) return;


  if  (write(sd_, "MEAS:CURR?\n",11)==-1 )
    {
      ROS_ERROR("can not send socket");
      return;
    }
  memset(read_buf_, 0, sizeof(read_buf_));
  read(sd_, read_buf_, sizeof(read_buf_));

  takasako_sps::PowerInfo power_info;
  float currency;
  std::stringstream ss;
  ss.str(std::string(read_buf_));
  ss >>power_info.currency;

  //ROS_INFO("%f", power_info.currency);

  power_info_pub_.publish(power_info);

}

