#ifndef AutoStabilizer_MathUtil_H
#define AutoStabilizer_MathUtil_H

#include <Eigen/Eigen>

namespace mathutil {
  // axisとlocalaxisはノルムが1, mは回転行列でなければならない.
  // axisとlocalaxisがピッタリ180反対向きの場合、回転方向が定まらないので不安定
  Eigen::Matrix3d orientCoordToAxis(const Eigen::Matrix3d& m, const Eigen::Vector3d& axis, const Eigen::Vector3d& localaxis = Eigen::Vector3d::UnitZ());

  Eigen::Transform<double, 3, Eigen::AffineCompact> orientCoordToAxis(const Eigen::Transform<double, 3, Eigen::AffineCompact>& m, const Eigen::Vector3d& axis, const Eigen::Vector3d& localaxis = Eigen::Vector3d::UnitZ());

  // coordsとweightsのサイズは同じでなければならない
  Eigen::Vector3d calcMidPos(const std::vector<Eigen::Vector3d>& coords, const std::vector<double>& weights);

  // coordsとweightsのサイズは同じでなければならない
  Eigen::Matrix3d calcMidRot(const std::vector<Eigen::Matrix3d>& coords, const std::vector<double>& weights);

  // coordsとweightsのサイズは同じでなければならない
  Eigen::Transform<double, 3, Eigen::AffineCompact> calcMidCoords(const std::vector<Eigen::Transform<double, 3, Eigen::AffineCompact>>& coords, const std::vector<double>& weights);

  template<typename T>
  inline T clamp(const T& value, const T& limit_value) {
    return std::max(-limit_value, std::min(limit_value, value));
  }
  template<typename T>
  inline T clamp(const T& value, const T& llimit_value, const T& ulimit_value) {
    return std::max(llimit_value, std::min(ulimit_value, value));
  }
  template<typename Derived>
  inline typename Derived::PlainObject clampMatrix(const Eigen::MatrixBase<Derived>& value, const Eigen::MatrixBase<Derived>& limit_value) {
    return value.array().max(-limit_value.array()).min(limit_value.array());
  }
  template<typename Derived>
  inline typename Derived::PlainObject clampMatrix(const Eigen::MatrixBase<Derived>& value, const Eigen::MatrixBase<Derived>& llimit_value, const Eigen::MatrixBase<Derived>& ulimit_value) {
    return value.array().max(llimit_value.array()).min(ulimit_value.array());
  }

  Eigen::Matrix3d cross(const Eigen::Vector3d& m);

  // Z成分は無視する.
  std::vector<Eigen::Vector3d> calcConvexHull(const std::vector<Eigen::Vector3d>& vertices);

  // Z成分は無視する. hullは半時計回りの凸包
  bool isInsideHull(const Eigen::Vector3d& p, const std::vector<Eigen::Vector3d>& hull);

  // Z成分は無視する. hullは半時計回りの凸包
  Eigen::Vector3d calcNearestPointOfHull(const Eigen::Vector3d& p_, const std::vector<Eigen::Vector3d>& hull);

  // originから見て、pがverticesの内部に入るようにする. pの高さのXY平面で考える
  Eigen::Vector3d calcInsidePointOfPolygon3D(const Eigen::Vector3d& p, const std::vector<Eigen::Vector3d>& vertices, const Eigen::Vector3d& origin);
};


#endif
