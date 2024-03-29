#include "FusionEKF.h"
#include <iostream>
#include "Eigen/Dense"
#include "tools.h"

using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::cout;
using std::endl;
using std::vector;

/**
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;
  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
              0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
              0, 0.0009, 0,
              0, 0, 0.09;

  // measurement matrix - laser
  H_laser_ << 1.0, 0, 0, 0,
              0, 1.0, 0, 0;  

  // measurement matrix - lidar is defined later by calling Jocobian in tools.cpp                      

  
  // state covariance matrix P
  ekf_.P_ = MatrixXd(4, 4);
  ekf_.P_ << 1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 1000, 0,
             0, 0, 0, 1000;

          
  // the initial transition matrix F
  ekf_.F_ = MatrixXd(4, 4);
  ekf_.F_ << 1, 0, 1, 0,
             0, 1, 0, 1,
             0, 0, 1, 0,
             0, 0, 0, 1;


}

/**
 * Destructor.
 */
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
  /**
   * Initialization
   */
  if (!is_initialized_) {
    // Initialize the state ekf_.x_ with the first measurement.
    
    ekf_.x_ = VectorXd(4);
    std::cout << "Initialized with first measurement " << std::endl;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      // Convert radar from polar to cartesian coordinates and initialize state.
      double rho = measurement_pack.raw_measurements_[0];
      double phi = measurement_pack.raw_measurements_[1];
      // double rho_dot = measurement_pack.raw_measurements_[2];
      ekf_.x_ << rho * cos(phi), // x, y, vx, vy
                 rho * sin(phi), 
                 0.0,  //a radar measurement does not contain enough information to determine the state variable velocities. 
                 0.0;  // no need for rho_dot * cos(phi), rho_dot * sin(phi)  
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      // Initialize state.
      ekf_.x_ << measurement_pack.raw_measurements_[0], 
                 measurement_pack.raw_measurements_[1], 
                 0.0, 
                 0.0; // x, y, vx, vy
    }
    previous_timestamp_ = measurement_pack.timestamp_;
    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /******************************************************
   * Prediction
   * Update the state transition matrix F according to the new elapsed time.
   * Time is measured in seconds.
   * Update the process noise covariance matrix.
   * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
  double dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;
  previous_timestamp_ = measurement_pack.timestamp_;
    
  double dt_2 = dt * dt;
  double dt_3 = dt_2 * dt;
  double dt_4 = dt_3 * dt;

  // Modify the F matrix so that the time is integrated
  ekf_.F_(0, 2) = dt;
  ekf_.F_(1, 3) = dt;
  
  const double noise_ax = 9.0;
  const double noise_ay = 9.0; 
  // noise_ax = 9.0;
  // noise_ay = 9.0;
  // set the process covariance matrix Q

  ekf_.Q_ = MatrixXd(4, 4);
  ekf_.Q_ <<  dt_4/4 * noise_ax, 0.0,                 dt_3/2 * noise_ax, 0.0,
              0.0,                 dt_4/4 * noise_ay, 0.0,                 dt_3/2 * noise_ay,
              dt_3/2*noise_ax,   0.0,                 dt_2 * noise_ax,   0.0,
              0.0,                 dt_3/2 * noise_ay, 0.0,                 dt_2 * noise_ay;
  ekf_.Predict();

  /*****************************************************
   * Update
   * - Use the sensor type to perform the update step.
   * - Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates
    Hj_ = tools.CalculateJacobian(ekf_.x_);
    ekf_.H_ = Hj_;
    ekf_.R_ = R_radar_;
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser updates
    ekf_.H_ = H_laser_;
    ekf_.R_ = R_laser_;
    ekf_.Update(measurement_pack.raw_measurements_);
  } 

  // print the output
  std::cout << "----------------------------" << std::endl;
  std::cout << "x_ = " << std::endl
            << ekf_.x_ << std::endl;
  std::cout << "P_ = " << std::endl
            << ekf_.P_ << std::endl;
}
