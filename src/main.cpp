#include <ros/ros.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/image_encodings.h>
#include <haar_cascade/ImageBoundingBox.h>

using namespace std;
using namespace cv;

string image_topic;
string cascade_path;

class HaasCascadeDetector {

  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  image_transport::Publisher image_pub_;
  ros::Publisher bbox_pub_;
  CascadeClassifier face_descriptor_{cascade_path};
  
public:
  HaasCascadeDetector()
    : it_(nh_) {
    image_sub_ = it_.subscribe(image_topic, 1, &HaasCascadeDetector::cameraCallback, this);
    image_pub_ = it_.advertise("/haar_cascade/image", 1);
    bbox_pub_ = nh_.advertise<haar_cascade::ImageBoundingBox>("/haar_cascade/bounding_box",1000);
  }

  void cameraCallback(const sensor_msgs::ImageConstPtr& msg) {
    
    cv_bridge::CvImagePtr cv_ptr;
    try {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }
    Mat img_gray;
    cvtColor(cv_ptr->image, img_gray, CV_BGR2GRAY );
    equalizeHist(img_gray, img_gray);
    vector<Rect> faces;
    face_descriptor_.detectMultiScale(img_gray, faces, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(30,30), Size(2000,2000));
    haar_cascade::ImageBoundingBox bbox_msg;
    if (faces.size() > 0) {
      for(unsigned i=0; i<faces.size(); i++) {
	      Point center(faces[i].x + faces[i].width*0.5, faces[i].y+faces[i].height*0.5);
	      ellipse(cv_ptr->image,center,Size(faces[i].width*0.5,faces[i].height*0.5), 0, 0, 360, Scalar(0,0,255), 4, 8, 0);
      }
      bbox_msg.center.u = faces[0].x + faces[0].width/2;
      bbox_msg.center.v = faces[0].y + faces[0].height/2;
      bbox_msg.width = faces[0].width;
      bbox_msg.height = faces[0].height;
      bbox_msg.cornerPoints[0].u = faces[0].x;
      bbox_msg.cornerPoints[0].v = faces[0].y;
      bbox_msg.cornerPoints[1].u = faces[0].x + faces[0].width;
      bbox_msg.cornerPoints[1].v = faces[0].y;
      bbox_msg.cornerPoints[2].u = faces[0].x + faces[0].width;
      bbox_msg.cornerPoints[2].v = faces[0].y + faces[0].height;
      bbox_msg.cornerPoints[3].u = faces[0].x;
      bbox_msg.cornerPoints[3].v = faces[0].y + faces[0].height;
    }
    bbox_pub_.publish(bbox_msg);
    image_pub_.publish(cv_ptr->toImageMsg());
  }
};

int main (int argc, char** argv) {
  ros::init(argc, argv, "haar_cascade_detector");
  ros::NodeHandle nh;
  nh.getParam("/camera_topic", image_topic);
  nh.getParam("/cascade_file", cascade_path);
  HaasCascadeDetector ic;
  ros::spin ();
  return 0;
}
