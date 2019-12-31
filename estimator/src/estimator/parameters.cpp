/*******************************************************
 * Copyright (C) 2019, Aerial Robotics Group, Hong Kong University of Science and Technology
 *
 * This file is part of VINS.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *******************************************************/

#include "parameters.h"

int MLOAM_RESULT_SAVE;
std::string OUTPUT_FOLDER;
std::string MLOAM_ODOM_PATH;
std::string MLOAM_MAP_PATH;
std::string MLOAM_GT_PATH;
std::string EX_CALIB_RESULT_PATH;

int MULTIPLE_THREAD;

double SOLVER_TIME;
int NUM_ITERATIONS;
int ESTIMATE_EXTRINSIC;
int ESTIMATE_TD;

// LiDAR
int NUM_OF_LASER;
int N_SCANS;

int IDX_REF;

int WINDOW_SIZE;
int OPT_WINDOW_SIZE;

float SCAN_PERIOD;
float DISTANCE_SQ_THRESHOLD;
float NEARBY_SCAN;

// segmentation
int SEGMENT_CLOUD;
int HORIZON_SCAN;
int MIN_CLUSTER_SIZE;
int MIN_LINE_SIZE;
int SEGMENT_VALID_POINT_NUM;
int SEGMENT_VALID_LINE_NUM;
float SEGMENT_THETA;

std::string CLOUD0_TOPIC, CLOUD1_TOPIC;
float LASER_SYNC_THRESHOLD;
double ROI_RANGE;
double ROI_RANGE_MAPPING;

std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond> > QBL;
std::vector<Eigen::Vector3d> TBL;
std::vector<double> TDBL;

int PLANAR_MOVEMENT;

float MIN_MATCH_SQ_DIS;
float MIN_PLANE_DIS;

int MARGINALIZATION_FACTOR;
int POINT_PLANE_FACTOR;
int POINT_EDGE_FACTOR;
int PRIOR_FACTOR;
double PRIOR_FACTOR_POS;
double PRIOR_FACTOR_ROT;
int CHECK_JACOBIAN;

int OPTIMAL_EXTRINSIC;

int EVALUATE_RESIDUAL;

int PCL_VIEWER;
int PCL_VIEWER_NORMAL_RATIO;

int OPTIMAL_ODOMETRY;
int N_CUMU_FEATURE;

double EIG_INITIAL;
double EIG_THRE_CALIB;
int N_CALIB;

Eigen::Matrix<double, 9, 9> XI;
double NORM_THRESHOLD;

float MAP_CORNER_RES;
float MAP_SURF_RES;

template <typename T>
T readParam(ros::NodeHandle &n, std::string name)
{
    T ans;
    if (n.getParam(name, ans))
    {
        ROS_INFO_STREAM("Loaded " << name << ": " << ans);
    }
    else
    {
        ROS_ERROR_STREAM("Failed to load " << name);
        n.shutdown();
    }
    return ans;
}

void readParameters(std::string config_file)
{
    FILE *fh = fopen(config_file.c_str(),"r");
    if(fh == NULL)
    {
        ROS_WARN("config_file dosen't exist; wrong config_file path");
        ROS_BREAK();
        return;
    }
    fclose(fh);

    cv::FileStorage fsSettings(config_file, cv::FileStorage::READ);
    if(!fsSettings.isOpened())
    {
        std::cerr << "ERROR: Wrong path to settings" << std::endl;
    }


    fsSettings["cloud0_topic"] >> CLOUD0_TOPIC;
    fsSettings["cloud1_topic"] >> CLOUD1_TOPIC;

    MULTIPLE_THREAD = fsSettings["multiple_thread"];

    SOLVER_TIME = fsSettings["max_solver_time"];
    NUM_ITERATIONS = fsSettings["max_num_iterations"];

    fsSettings["output_path"] >> OUTPUT_FOLDER;
    MLOAM_RESULT_SAVE = fsSettings["mloam_result_save"];
    MLOAM_ODOM_PATH = OUTPUT_FOLDER + fsSettings["mloam_odom_path"];
	MLOAM_MAP_PATH = OUTPUT_FOLDER + fsSettings["mloam_map_path"];
    MLOAM_GT_PATH = OUTPUT_FOLDER + fsSettings["mloam_gt_path"];
    std::cout << "gt path: " << MLOAM_GT_PATH << std::endl;
    std::cout << "result path: " << MLOAM_ODOM_PATH << ", " << MLOAM_MAP_PATH << std::endl;

    NUM_OF_LASER = fsSettings["num_of_laser"];
    printf("Laser number %d\n", NUM_OF_LASER);
    if(NUM_OF_LASER != 1 && NUM_OF_LASER != 2)
    {
        printf("num_of_cam should be 1 or 2\n");
        assert(0);
    }

    WINDOW_SIZE = fsSettings["window_size"];
    OPT_WINDOW_SIZE = fsSettings["opt_window_size"];
    printf("window_size: %d, opt_window_size: %d\n", WINDOW_SIZE, OPT_WINDOW_SIZE);

    ESTIMATE_EXTRINSIC = fsSettings["estimate_extrinsic"];
    OPTIMAL_EXTRINSIC = fsSettings["optimal_extrinsic"];
    EX_CALIB_RESULT_PATH = OUTPUT_FOLDER + "extrinsic_parameter.txt";
    if (ESTIMATE_EXTRINSIC == 2)
    {
        ROS_WARN("Have no prior about extrinsic param, calibrate extrinsic param");
        for (int i = 0; i < NUM_OF_LASER; i++)
        {
            QBL.push_back(Eigen::Quaterniond::Identity());
            TBL.push_back(Eigen::Vector3d::Zero());
        }
    }
    else
    {
        if (ESTIMATE_EXTRINSIC == 1)
        {
            ROS_WARN("Please optimize extrinsic param around initial guess!");
        }
        if (ESTIMATE_EXTRINSIC == 0)
        {
            ROS_WARN("Fix extrinsic param ");
        }

        cv::Mat cv_T;
        fsSettings["body_T_laser"] >> cv_T;
        for (int i = 0; i < NUM_OF_LASER; i++)
        {
            QBL.push_back(Eigen::Quaterniond(cv_T.ptr<double>(i)[3], cv_T.ptr<double>(i)[0], cv_T.ptr<double>(i)[1], cv_T.ptr<double>(i)[2]));
            TBL.push_back(Eigen::Vector3d(cv_T.ptr<double>(i)[4], cv_T.ptr<double>(i)[5], cv_T.ptr<double>(i)[6]));
        }
    }
    int pn = config_file.find_last_of('/');
    std::string configPath = config_file.substr(0, pn);

    cv::Mat cv_TD;
    fsSettings["td"] >> cv_TD;
    for (int i = 0; i < NUM_OF_LASER; i++) TDBL.push_back(cv_TD.ptr<double>(0)[i]);
    ESTIMATE_TD = fsSettings["estimate_td"];
    if (ESTIMATE_TD)
        ROS_INFO_STREAM("Unsynchronized sensors, online estimate time offset");
    else
        ROS_INFO_STREAM("Synchronized sensors, fix time offset");

    LASER_SYNC_THRESHOLD = fsSettings["laser_sync_threshold"];
    N_SCANS = fsSettings["n_scans"];
    ROI_RANGE = fsSettings["roi_range"];
    ROI_RANGE_MAPPING = fsSettings["roi_range_mapping"];

    SEGMENT_CLOUD = fsSettings["segment_cloud"];
    HORIZON_SCAN = fsSettings["horizon_scan"];
    MIN_CLUSTER_SIZE = fsSettings["min_cluster_size"];
    MIN_LINE_SIZE = fsSettings["min_line_size"];
    SEGMENT_VALID_POINT_NUM = fsSettings["segment_valid_point_num"];
    SEGMENT_VALID_LINE_NUM = fsSettings["segment_valid_line_num"];
    SEGMENT_THETA = fsSettings["segment_theta"];

    IDX_REF = fsSettings["idx_ref"];

    SCAN_PERIOD = fsSettings["scan_period"];
    DISTANCE_SQ_THRESHOLD = fsSettings["distance_sq_threshold"];
    NEARBY_SCAN = fsSettings["nearby_scan"];

    PLANAR_MOVEMENT = fsSettings["planar_movement"];

    MIN_MATCH_SQ_DIS = fsSettings["min_match_sq_dis"];
    MIN_PLANE_DIS = fsSettings["min_plane_dis"];

    MARGINALIZATION_FACTOR = fsSettings["marginalization_factor"];
    POINT_PLANE_FACTOR = fsSettings["point_plane_factor"];
    POINT_EDGE_FACTOR = fsSettings["point_edge_factor"];
    PRIOR_FACTOR = fsSettings["prior_factor"];
    PRIOR_FACTOR_POS = fsSettings["prior_factor_pos"];
    PRIOR_FACTOR_ROT = fsSettings["prior_factor_rot"];
    CHECK_JACOBIAN = fsSettings["check_jacobian"];

    EVALUATE_RESIDUAL = fsSettings["evaluate_residual"];

    PCL_VIEWER = fsSettings["pcl_viewer"];
    PCL_VIEWER_NORMAL_RATIO = fsSettings["pcl_viewer_normal_ratio"];

    OPTIMAL_ODOMETRY = fsSettings["optimal_odometry"];
    N_CUMU_FEATURE = fsSettings["n_cumu_feature"];

    EIG_INITIAL = fsSettings["eig_initial"];
    EIG_THRE_CALIB = fsSettings["eig_thre_calib"];
    N_CALIB = fsSettings["n_calib"];

    // mapping parameter
    cv::Mat cv_xi;
    fsSettings["uncertainty_calib"] >> cv_xi;
    Eigen::Matrix<double, 9, 1> xi_vec; // rotation, translation, point
    xi_vec << cv_xi.ptr<double>(0)[0], cv_xi.ptr<double>(0)[1], cv_xi.ptr<double>(0)[2],
              cv_xi.ptr<double>(0)[3], cv_xi.ptr<double>(0)[4], cv_xi.ptr<double>(0)[5],
              cv_xi.ptr<double>(0)[6], cv_xi.ptr<double>(0)[7], cv_xi.ptr<double>(0)[8];
    XI = xi_vec.asDiagonal();
    std::cout << "initial covariance XI:" << std::endl << XI << std::endl;

    NORM_THRESHOLD = fsSettings["norm_threshold"];

    MAP_CORNER_RES = fsSettings["map_corner_res"];
    MAP_SURF_RES = fsSettings["map_surf_res"];
    std::cout << "map corner resolution:" << MAP_CORNER_RES << ", surf resolutionl: " << MAP_SURF_RES << std::endl;

    fsSettings.release();
}
