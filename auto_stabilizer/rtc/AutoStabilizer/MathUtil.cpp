#include "MathUtil.h"
#include <limits>

namespace mathutil {
  Eigen::Matrix3d orientCoordToAxis(const Eigen::Matrix3d& m, const Eigen::Vector3d& axis, const Eigen::Vector3d& localaxis){
    // axisとlocalaxisはノルムが1, mは回転行列でなければならない.
    // axisとlocalaxisがピッタリ180反対向きの場合、回転方向が定まらないので不安定
    Eigen::AngleAxisd m_ = Eigen::AngleAxisd(m); // Eigen::Matrix3dの空間で積算していると数値誤差によってだんたん回転行列ではなくなってくるので
    Eigen::Vector3d localaxisdir = m_ * localaxis;
    Eigen::Vector3d cross = localaxisdir.cross(axis);
    double dot = std::min(1.0,std::max(-1.0,localaxisdir.dot(axis))); // acosは定義域外のときnanを返す
    if(cross.norm()==0){
      if(dot == -1) return Eigen::Matrix3d(-m);
      else return Eigen::Matrix3d(m_);
    }else{
      double angle = std::acos(dot); // 0~pi
      Eigen::Vector3d axis = cross.normalized(); // include sign
      return Eigen::Matrix3d(Eigen::AngleAxisd(angle, axis) * m_);
    }
  }
  Eigen::Transform<double, 3, Eigen::AffineCompact> orientCoordToAxis(const Eigen::Transform<double, 3, Eigen::AffineCompact>& m, const Eigen::Vector3d& axis, const Eigen::Vector3d& localaxis){
    Eigen::Transform<double, 3, Eigen::AffineCompact> ret = m;
    ret.linear() = mathutil::orientCoordToAxis(ret.linear(), axis, localaxis);
    return ret;
  }
  Eigen::AngleAxisd slerp(const Eigen::AngleAxisd& M1, const Eigen::AngleAxisd& M2, double r){
    // 0 <= r <= 1
    Eigen::AngleAxisd trans = Eigen::AngleAxisd(M1.inverse() * M2);
    return Eigen::AngleAxisd(M1 * Eigen::AngleAxisd(trans.angle() * r, trans.axis()));
  }

  Eigen::Vector3d calcMidPos(const std::vector<Eigen::Vector3d>& coords, const std::vector<double>& weights){
    // coordsとweightsのサイズは同じでなければならない
    double sumWeight = 0.0;
    Eigen::Vector3d midpos = Eigen::Vector3d::Zero();

    for(int i=0;i<coords.size();i++){
      if(weights[i]<=0) continue;
      midpos = ((midpos*sumWeight + coords[i]*weights[i])/(sumWeight+weights[i])).eval();
      sumWeight += weights[i];
    }
    return midpos;
  }
  Eigen::Matrix3d calcMidRot(const std::vector<Eigen::Matrix3d>& coords, const std::vector<double>& weights){
    // coordsとweightsのサイズは同じでなければならない
    double sumWeight = 0.0;
    Eigen::AngleAxisd midrot = Eigen::AngleAxisd::Identity();

    for(int i=0;i<coords.size();i++){
      if(weights[i]<=0) continue;
      midrot = mathutil::slerp(midrot, Eigen::AngleAxisd(coords[i]), weights[i]/(sumWeight+weights[i]));
      //midrot = midrot.slerp(weights[i]/(sumWeight+weights[i]),Eigen::Quaterniond(coords[i])); // quaternionのslerpは、90度回転した姿勢で不自然な遠回り補間をするので使ってはならない
      sumWeight += weights[i];
    }
    return midrot.toRotationMatrix();
  }
  Eigen::Transform<double, 3, Eigen::AffineCompact> calcMidCoords(const std::vector<Eigen::Transform<double, 3, Eigen::AffineCompact>>& coords, const std::vector<double>& weights){
    // coordsとweightsのサイズは同じでなければならない
    double sumWeight = 0.0;
    Eigen::Transform<double, 3, Eigen::AffineCompact> midCoords = Eigen::Transform<double, 3, Eigen::AffineCompact>::Identity();

    for(int i=0;i<coords.size();i++){
      if(weights[i]<=0) continue;
      midCoords.translation() = ((midCoords.translation()*sumWeight + coords[i].translation()*weights[i])/(sumWeight+weights[i])).eval();
      midCoords.linear() = mathutil::slerp(Eigen::AngleAxisd(midCoords.linear()), Eigen::AngleAxisd(coords[i].linear()),(weights[i]/(sumWeight+weights[i]))).toRotationMatrix();
      //midCoords.linear() = Eigen::Quaterniond(midCoords.linear()).slerp(weights[i]/(sumWeight+weights[i]),Eigen::Quaterniond(coords[i].linear())).toRotationMatrix(); // quaternionのslerpは、90度回転した姿勢で不自然な遠回り補間をするので使ってはならない
      sumWeight += weights[i];
    }
    return midCoords;
  }

  Eigen::Matrix3d cross(const Eigen::Vector3d& m){
    Eigen::Matrix3d ret;
    ret <<
        0.0, -m[2],  m[1],
       m[2],   0.0, -m[0],
      -m[1],  m[0],   0.0;
    return ret;
  }

  std::vector<Eigen::Vector3d> calcConvexHull(const std::vector<Eigen::Vector3d>& vertices){
    // Z成分は無視する.
    std::vector<Eigen::Vector3d> tmpVertices(vertices.size());
    for (int i = 0; i < vertices.size(); i++) tmpVertices[i] = Eigen::Vector3d(vertices[i][0],vertices[i][1],0.0);
    std::sort(tmpVertices.begin(), tmpVertices.end(), [](const Eigen::Vector3d& lv, const Eigen::Vector3d& rv){ return lv(0) < rv(0) || (lv(0) == rv(0) && lv(1) < rv(1));});

    std::vector<Eigen::Vector3d> convexHull(2*tmpVertices.size());
    int n_ch = 0;
    for (int i = 0; i < tmpVertices.size(); convexHull[n_ch++] = tmpVertices[i++])
      while (n_ch >= 2 && (convexHull[n_ch-1] - convexHull[n_ch-2]).cross(tmpVertices[i] - convexHull[n_ch-2])[2] <= 0) n_ch--;
    for (int i = tmpVertices.size()-2, j = n_ch+1; i >= 0; convexHull[n_ch++] = tmpVertices[i--])
      while (n_ch >= j && (convexHull[n_ch-1] - convexHull[n_ch-2]).cross(tmpVertices[i] - convexHull[n_ch-2])[2] <= 0) n_ch--;
    convexHull.resize(n_ch-1);
    return convexHull;
  }

  bool isInsideHull(const Eigen::Vector3d& p, const std::vector<Eigen::Vector3d>& hull){
    // Z成分は無視する. hullは半時計回りの凸包
    if(hull.size() == 0) return false;
    else if(hull.size() == 1) return hull[0].head<2>() == p.head<2>();
    else if(hull.size() == 2) {
      Eigen::Vector3d a = hull[0] - p, b = hull[1] - p;
      return (a.cross(b)[2] == 0) && (a.head<2>().dot(b.head<2>()) <= 0);
    }else {
      for (int i = 0; i < hull.size(); i++) {
        Eigen::Vector3d a = hull[i] - p, b = hull[(i+1)%hull.size()] - p;
        if(a.cross(b)[2] < 0) return false;
      }
      return true;
    }
  }

  Eigen::Vector3d calcNearestPointOfHull(const Eigen::Vector3d& p_, const std::vector<Eigen::Vector3d>& hull){
    // Z成分は無視する. hullは半時計回りの凸包
    if(isInsideHull(p_,hull)) return Eigen::Vector3d(p_[0],p_[1],0.0);
    else if(hull.size() == 0) return Eigen::Vector3d(p_[0],p_[1],0.0);
    else if(hull.size() == 1) return Eigen::Vector3d(hull[0][0],hull[0][1],0.0);
    else{
      Eigen::Vector2d p = p_.head<2>();
      double minDistance = std::numeric_limits<double>::max();
      Eigen::Vector2d nearestPoint;
      for (int i = 0; i < hull.size(); i++) {
        Eigen::Vector2d p1 = hull[i].head<2>(), p2 = hull[(i+1)%hull.size()].head<2>();
        double dot = (p2 - p1).dot(p - p1);
        if(dot <= 0) { // p1が近い
          double distance = (p - p1).norm();
          if(distance < minDistance){
            minDistance = distance;
            nearestPoint = p1;
          }
        }else if(dot >= (p2 - p1).squaredNorm()) { // p2が近い
          double distance = (p - p2).norm();
          if(distance < minDistance){
            minDistance = distance;
            nearestPoint = p2;
          }
        }else { // 直線p1 p2におろした垂線の足が近い
          Eigen::Vector2d p1Top2 = (p2 - p1).normalized();
          Eigen::Vector2d p3 = p1 + (p - p1).dot(p1Top2) * p1Top2;
          double distance = (p - p3).norm();
          if(distance < minDistance){
            minDistance = distance;
            nearestPoint = p3;
          }
        }
      }
      return Eigen::Vector3d(nearestPoint[0],nearestPoint[1],0.0);
    }
  }

  Eigen::Vector3d calcInsidePointOfPolygon3D(const Eigen::Vector3d& p, const std::vector<Eigen::Vector3d>& vertices, const Eigen::Vector3d& origin){
    // originから見て、pがverticesの内部に入るようにする. pの高さのXY平面で考える
    if(p[2] == origin[2]) return p;
    Eigen::Transform<double, 3, Eigen::AffineCompact> frame = Eigen::Transform<double, 3, Eigen::AffineCompact>::Identity(); // この座標系のXY平面で考える
    frame.translation() = p;
    Eigen::Transform<double, 3, Eigen::AffineCompact> frameInverse = frame.inverse();

    Eigen::Vector3d projectedOrigin = frameInverse * origin;
    std::vector<Eigen::Vector3d> projectedVertices(vertices.size());
    for(int i=0;i<vertices.size();i++){
      Eigen::Vector3d projectedVertex = frameInverse * vertices[i];
      // originとvertexを結ぶ直線をXY平面へ下ろす
      if(projectedVertex[2] == projectedOrigin[2]) projectedVertices[i] = Eigen::Vector3d(projectedVertex[0],projectedVertex[1],0.0); // 定義できないのでやむをえず
      else projectedVertices[i] = projectedOrigin + (projectedVertex - projectedOrigin) * projectedOrigin[2]/(projectedOrigin[2]-projectedVertex[2]);
    }
    std::vector<Eigen::Vector3d> hull = calcConvexHull(projectedVertices);
    Eigen::Vector3d nearestPoint =  calcNearestPointOfHull(Eigen::Vector3d::Zero(),hull);
    return frame * nearestPoint;
  }
};
