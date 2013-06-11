#ifndef DETECTOR_HPP
#define DETECTOR_HPP

// ROS
#include <sensor_msgs/image_encodings.h>
#include <camera_calibration_parsers/parse_yml.h>
#include <cv_bridge/cv_bridge.h>
// OPENCV
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>
#include <opencv2/objdetect/objdetect.hpp>
// MY INCLUDES
#include "laserLib.hpp"
#include "projectTools.hpp"

#define HOG_THRESHOLD -10000

class detector {
protected:

	/// Local node handle, used to get the file parameters, subscribers, publishers etc.
	ros::NodeHandle nh;

	/// The camera info is loaded from the file "yaml/camera_calib.yaml"
	sensor_msgs::CameraInfo cInfo;

	/// Contains the laser clusters, annotation, features, cogs, annotation, if it should be fused etc.
	upm::ClusteredScan scanData;

	/// Used to listen the transform between the laser and the camera.
	tf::TransformListener tf_listener_;
	tf::StampedTransform transform;

	///  When the image was captured.
	ros::Time acquisition_time;

	/// Curent cluster and scan number
	int clusterNo;
	int scanNo;

	/// Crop box corners
	cv::Point upleft, downright;

	/// Crop box size
	int boxSize;

	/// Point projected to pixel coordinates.
	cv::Point2d prPixel;

	/// Object used to do the low lever segmentation and feature extraction from the scan.
	laserLib laserProcessor;

	///  A pointer to the opencv converted image.
	cv_bridge::CvImagePtr cv_ptr;

	// DETECTION DEPARTMENT
	//
	//

	// Vectors where the probabilities of the detectors are stored
	std::vector<float> laserProb;
	// If there is no fusion the probability is 0.5
	std::vector<float> cameraProb;
	std::vector<float> fusionProb;

	// Vectors where the class of the detector are stored
	std::vector<int> laserClass;
	std::vector<int> cameraClass;
	std::vector<int> fusionClass;

	// The adaboost detector for the laser
	CvBoost::CvBoost boost;
	// The laser feature matrix
	cv::Mat lFeatures;

	// The HoG detector for the image
    cv::HOGDescriptor hog;

    /// Mat where the temporary crop will be saved
    Mat crop;

    // Vectors to hold the class and the probability of the ROI
    std::vector<Rect> hogFound;
    std::vector<double> hogPred;

	/// Does the laser segmentation, feature extraction etc into scanClusters
	void getLaser(const sensor_msgs::LaserScan::ConstPtr &lScan);

	/// Brings sensor_msgs::Image to an opencv accesible pointer.
	void getImage(const sensor_msgs::Image::ConstPtr &image);

	/// Returns the transform between image and lScan
	void getTF(const sensor_msgs::Image::ConstPtr &image, const sensor_msgs::LaserScan::ConstPtr &lScan);

	/// Does the rest of the laser processing, find projected and fused segments
	/// Used directly scanClusters
	/// Must be run after getTF and getImage
	void extractLaser();

	/// Detects if there is a pedestrian in the cluster and or ROI
	/// Gives the probabilities and the class output of each detector
	void detectPedestrian();

	/// Finds the class and the probability for a given sample of laser features
	void detectLaser(std_msgs::Float32MultiArray &features);

	/// Finds the class and the probability for a given crop of the image
	void detectCamera(geometry_msgs::Point32 &cog);

	void saveDetection() {};

public:
	detector();
	~detector() {};

	/** Extracts all the info from an imaga/laserscan pair
	 *	First it segments the laser scan and finds the cog for each cluster.
	 *	Then translates each cog into the corresponding pixel values.
	 *	Crops the ROI from the image.
	 *
	 * @param image Image message
	 * @param lScan LaserScan message
	 */
	void extractData(const sensor_msgs::Image::ConstPtr &image,
			const sensor_msgs::LaserScan::ConstPtr &lScan);

	void setClusters(upm::ClusteredScan cs);
	upm::ClusteredScan getClusters();

};
#endif