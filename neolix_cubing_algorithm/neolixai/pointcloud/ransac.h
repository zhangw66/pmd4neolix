
#ifndef _RANSAC_HPP_
#define _RANSAC_HPP_

#include"pointcloud.hpp"
#include<opencv2/core/core.hpp>
#include<math.h>
#include<random>
#include<float.h>
namespace  neolix{

/**
 * The  class is used fitness a plane form point  cloud or depth data. The class is mianly
 * based on random sampling consistency algorithm
 * athor:pengcheng
 * Emain:pengcheng@neolix.cn yslrpch@126.com
 * @pengcheng
 **/

class ransac
{
public:

	ransac(pointcloud<double,1,1> *pts);
    ~ransac();
    int  fittingPlane(double p , double w, double threshold);
    const double* getModel();
    unsigned int getLeastAdaptDatanum();
    double getBestError();
    void isFitDepthData(bool isDepth);

private:
	pointcloud<double,1,1> *ptcld;//�۲�����
    double p;//�㷨������ȷ����ĸ���
    double w;//���������ڵ�ı��ʣ�һ������һ����³��ֵ
    double *model;//ģ��
    unsigned int LeastAdaptDataNum;//������ģ�͵��������ݸ���
    unsigned int iters;//��������
    double threshold;//��ֵ
    long long best_points;

    double best_error;//���
    bool ForDepthData ;


};

}
#endif // _RANSAC_HPP_
