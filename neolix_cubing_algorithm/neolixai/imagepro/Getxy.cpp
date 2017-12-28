#include<opencv2/opencv.hpp>
#include"../driverdecorate/base.h"
#include "Utils.h"
namespace neolix{

    extern cv::Mat g_cammer_intrinic;
void Getxy(const float PixLength,const float PixWidth,short distance,float &Length,float &Width)
{

	double a = g_cammer_intrinic.at<double>(0,0);
	double b = g_cammer_intrinic.at<double>(1,0);

	Length = PixLength * (1/b) *  distance / 10;
	Width = PixWidth * (1/a) *  distance / 10;
	if(Length<Width)
	{
		float temp = Length;
		Length = Width;
		Width = temp;
	}
}

void Getxyz2(cv::vector<cv::Point2f> point,short distance,float &Length,float &Width)
{
	////����yml�ļ���ȡ�ڲ���
	//cv::FileStorage fs2("intrinsics.yml", cv::FileStorage::READ);
	//cv::Mat data;

	//fs2["M1"] >> data;

	//float fu=data.at<double>(0,0);//��ȡfx��fy
	//float fv=data.at<double>(1,1);
	//float u0=data.at<double>(0,2);
	//float v0=data.at<double>(1,2);
	//float fu = NEICAN_FU;
	//float fv = NEICAN_FV;
	//float u0 = NEICAN_U0;
	//float v0 = NEICAN_V0;
	double fu = g_cammer_intrinic.at<double>(0,0);//fu
	double fv = g_cammer_intrinic.at<double>(1,0);//fv
	double u0 = g_cammer_intrinic.at<double>(2,0);//u0
	double v0 = g_cammer_intrinic.at<double>(3,0);//v0
	float a[2];
	cv::vector<cv::Point3f> point3f;
	point3f.resize(3);
	cv::vector<cv::Point2f>::iterator iter = point.begin();
	for(int j=0;j<=2;j++)
	{

			//������ά����
			point3f[j].x = distance*(iter->x-u0)/fu;
			point3f[j].y = distance*(iter->y-v0)/fv;
			point3f[j].z = distance;

		iter++;
	}

	//�����
	for(size_t j=0;j<=1;j++)
	{
		float x1 = point3f[j].x - point3f[j+1].x;
		float y1 = point3f[j].y - point3f[j+1].y;
		float temp = x1*x1 + y1*y1;
		a[j] = sqrt(temp);
	}
	if(a[0]>a[1])
	{
		Length = a[0];
		Width = a[1];
	}
	else
	{
		Length= a[1];
		Width = a[0];
	}
}


}
