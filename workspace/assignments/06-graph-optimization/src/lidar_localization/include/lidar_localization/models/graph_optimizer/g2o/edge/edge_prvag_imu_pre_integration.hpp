/*
 * @Description: g2o edge for LIO IMU pre-integration measurement
 * @Author: Ge Yao
 * @Date: 2020-11-29 15:47:49
 */
#ifndef LIDAR_LOCALIZATION_MODELS_GRAPH_OPTIMIZER_G2O_EDGE_EDGE_PRVAG_IMU_PRE_INTEGRATION_HPP_
#define LIDAR_LOCALIZATION_MODELS_GRAPH_OPTIMIZER_G2O_EDGE_EDGE_PRVAG_IMU_PRE_INTEGRATION_HPP_

#include <iostream>

#include <Eigen/Core>
#include <Eigen/Dense>

#include <sophus/so3.hpp>

#include <g2o/types/slam3d/types_slam3d.h>
#include <g2o/types/slam3d_addons/types_slam3d_addons.h>

#include "lidar_localization/models/graph_optimizer/g2o/vertex/vertex_prvag.hpp"

#include <g2o/core/base_binary_edge.h>

typedef Eigen::Matrix<double, 15, 1> Vector15d;

namespace g2o {

class EdgePRVAGIMUPreIntegration : public g2o::BaseBinaryEdge<15, Vector15d, g2o::VertexPRVAG, g2o::VertexPRVAG> {
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    static const int INDEX_P = 0;
    static const int INDEX_R = 3;
    static const int INDEX_V = 6;
    static const int INDEX_A = 9;
    static const int INDEX_G = 12;

	EdgePRVAGIMUPreIntegration()
	 : g2o::BaseBinaryEdge<15, Vector15d, g2o::VertexPRVAG, g2o::VertexPRVAG>() {
	 }

	virtual void computeError() override {
        const g2o::VertexPRVAG* v0 = static_cast<const g2o::VertexPRVAG*>(_vertices[0]);
        const g2o::VertexPRVAG* v1 = static_cast<const g2o::VertexPRVAG*>(_vertices[1]);

		const Eigen::Vector3d &pos_i = v0->estimate().pos;
		const Sophus::SO3d    &ori_i = v0->estimate().ori;
		const Eigen::Vector3d &vel_i = v0->estimate().vel;
		const Eigen::Vector3d &b_a_i = v0->estimate().b_a;
		const Eigen::Vector3d &b_g_i = v0->estimate().b_g;

		const Eigen::Vector3d &pos_j = v1->estimate().pos;
		const Sophus::SO3d    &ori_j = v1->estimate().ori;
		const Eigen::Vector3d &vel_j = v1->estimate().vel;
		const Eigen::Vector3d &b_a_j = v1->estimate().b_a;
		const Eigen::Vector3d &b_g_j = v1->estimate().b_g;

		const Eigen::Vector3d &alpha_ij = _measurement.block<3, 1>(INDEX_P, 0);
		const Eigen::Vector3d &theta_ij = _measurement.block<3, 1>(INDEX_R, 0);
		const Eigen::Vector3d  &beta_ij = _measurement.block<3, 1>(INDEX_V, 0);

		_error.block(INDEX_P, 0, 3, 1) = ori_i.inverse() * (pos_j - pos_i - (vel_i - 0.50 * g_ * T_) * T_) - alpha_ij;

		auto r = (Sophus::SO3d::exp(theta_ij).inverse()*ori_i.inverse()*ori_j).log();
		for (size_t i = 0; i < 3; ++i) 
			_error(INDEX_R + i, 0) = r[i];

		_error.block(INDEX_V, 0, 3, 1) = ori_i.inverse() * (vel_j - vel_i + g_ * T_); - beta_ij;
		_error.block(INDEX_A, 0, 3, 1) = b_a_j - b_a_i;
		_error.block(INDEX_G, 0, 3, 1) = b_g_j - b_g_i;
    }

	void setT(const double &T) {
		T_ = T;
	}

	void setGravitiy(const Eigen::Vector3d &g) {
		g_ = g;
	}

    virtual void setMeasurement(const Vector15d& m) override {
		_measurement = m;
	}

	virtual bool read(std::istream& is) override {
		double T;
		is >> T;

		Eigen::Vector3d g;
		is >> g.x() >> g.y() >> g.z();
		
		Vector15d v;
		is >> v(INDEX_P + 0) >> v(INDEX_P + 1) >> v(INDEX_P + 2)
		   >> v(INDEX_R + 0) >> v(INDEX_R + 1) >> v(INDEX_R + 2)
		   >> v(INDEX_V + 0) >> v(INDEX_V + 1) >> v(INDEX_V + 2)
		   >> v(INDEX_A + 0) >> v(INDEX_A + 1) >> v(INDEX_A + 2)
		   >> v(INDEX_G + 0) >> v(INDEX_G + 1) >> v(INDEX_G + 2);

		setT(T);
		setGravitiy(g);
    	setMeasurement(v);

		for (int i = 0; i < information().rows(); ++i) {
			for (int j = i; j < information().cols(); ++j) {
				is >> information()(i, j);
				// update cross-diagonal element:
				if (i != j) {
					information()(j, i) = information()(i, j);
				}
			}
		}

		return true;
	}

	virtual bool write(std::ostream& os) const override {
    	Vector15d v = _measurement;
		
		os << T_ << " ";

		os << g_.x() << " " << g_.y() << " " << g_.z() << " ";

		os << v(INDEX_P + 0) << " " <<v(INDEX_P + 1) << " " << v(INDEX_P + 2) << " "
		   << v(INDEX_R + 0) << " " <<v(INDEX_R + 1) << " " << v(INDEX_R + 2) << " "
		   << v(INDEX_V + 0) << " " <<v(INDEX_A + 1) << " " << v(INDEX_A + 2) << " "
		   << v(INDEX_A + 0) << " " <<v(INDEX_V + 1) << " " << v(INDEX_V + 2) << " "
		   << v(INDEX_G + 0) << " " <<v(INDEX_G + 1) << " " << v(INDEX_G + 2) << " ";

		for (int i = 0; i < information().rows(); ++i)
			for (int j = i; j < information().cols(); ++j)
				os << " " << information()(i, j);

		return os.good();
	}

private:
	double T_ = 0.0;
	Eigen::Vector3d g_ = Eigen::Vector3d::Zero();
};

} // namespace g2o

#endif // LIDAR_LOCALIZATION_MODELS_GRAPH_OPTIMIZER_G2O_EDGE_EDGE_PRVAG_IMU_PRE_INTEGRATION_HPP_
