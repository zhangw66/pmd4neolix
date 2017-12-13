
#ifndef _RANSAC_HPP_
#define _RANSAC_HPP_

#include"pointcloud.hpp"
#include<opencv2/core/core.hpp>
#include<math.h>
#include<random>
#include<float.h>
namespace  neolix{

class ransac
{
public:

	ransac(pointcloud<double,1,1> *pts);
    ~ransac();
    int  fittingPlane(double p , double w, double threshold);
    const double* getModel();
    unsigned int getLeastAdaptDatanum();
    double getBestError();

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


};

}
#endif // _RANSAC_HPP_
