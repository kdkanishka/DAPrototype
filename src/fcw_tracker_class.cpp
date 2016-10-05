#include "fcw_tracker_class.h"
#include <iostream>
#include <math.h> 
#include <chrono>
#include <deque>
#include <sys/time.h>

FcwTracker::FcwTracker( double distanceoffset, int samplestoaverage ) :
	kdistanceoffset_{ distanceoffset },
	ksamplestoaverage_{ samplestoaverage }
{
	//Fill with some data to prevent exceptions
	distances_.push_back(0.0);
	distances_.push_back(0.0);
	velocities_.push_back(0.0);
	velocities_.push_back(0.0);
	accelerations_.push_back(0.0);
	accelerations_.push_back(0.0);
}

void FcwTracker::Update( double distance, double speed )
{
	//Update time
	sampletime_ = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now() - lastsampletime_).count();
    lastsampletime_ = std::chrono::high_resolution_clock::now();
	
	//Calculate speed
	distances_.push_back(distance - kdistanceoffset_);
	velocities_.push_back((1000000.0 * (distances_[1] - distances_[0]))/sampletime_);
	
	//Calculate acceleration
	accelerations_.push_back((1000000.0 * (velocities_[1] - velocities_[0]))/sampletime_);
	
	//Limit queues
	if (distances_.size() > ksamplestoaverage_) distances_.pop_front();
	if (velocities_.size() > ksamplestoaverage_) velocities_.pop_front();
	if (accelerations_.size() > ksamplestoaverage_) accelerations_.pop_front();
	
	//Average queues
	followingdistance_ = Average(distances_);
	velocity_ = Average(velocities_);
	acceleration_ = Average(accelerations_);
	
	//Calculate time to impact with kinematic equation
		//t = (-v - sqrt(v^2 -2*a*d))/a
	timetocollision_ = (-velocity_ - sqrt (velocity_*velocity_ -
		2*acceleration_*followingdistance_))/acceleration_;		
	
	//Check for NaN or invalid result
	if ((timetocollision_ != timetocollision_) ||
		(timetocollision_ < 0.0)) timetocollision_ = 100.0;
	
	//Calcualte following time -> Speed is in mph!
	followingtime_ = (speed * 0.44704) / followingdistance_;
	
}

double FcwTracker::Average ( std::deque<double> deque )
{
	double average{ 0.0 };
	if (deque.size() != 0) {
		for ( double value : deque ) {
			average += value;
		}
		average /= deque.size();
	}
	return average;
}

FcwTracker::~FcwTracker()
{

}