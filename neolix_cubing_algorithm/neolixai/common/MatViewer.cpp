#include <stdint.h>
#include <stdio.h>
#include "MatViewer.hpp"


void OpencvViewer::__onMouseCallback(int event, int x, int y, int flags, void* ustc)
//void OpencvViewer::__onMouseCallback(int event, int x, int y,  void* ustc)
{
    OpencvViewer* p = (OpencvViewer*)ustc;
    cv::Mat img = p->_img.clone();
    p->onMouseCallback(img, event, cv::Point(x,y));
    if(flags)
        cv::imshow(p->_win.c_str(), img);
    else
        cv::imshow(p->_win.c_str(), img);
}


void OpencvViewer::show(const std::string& win, const cv::Mat& img)
{
    _img = img.clone();
    _win = win;
    cv::imshow(win.c_str(), _img);
    cv::setMouseCallback(win, __onMouseCallback, this);
}




///////////////////////////// DepthViewer ///////////////////////////////////////


std::string DepthViewer::depthStringAtLoc(const cv::Mat& img)
{
    uint16_t val = img.at<uint16_t>(img.rows/2, img.cols/2);
    char str[32];
    #ifdef linux
     snprintf(str,32, "%d", val);
    #endif // linux
    #ifdef WIN32
    sprintf_s(str, "%d", val);
    #endif // WIN32
    return str;
}


void DepthViewer::show(const std::string& win, const cv::Mat& img)
{
    if(img.type() != CV_16U){
        return;
    }

    _img = img.clone();
    cv::Mat colorImg = _render.Compute(img);
    std::string text = "Depth at center: " + depthStringAtLoc(_img);
    drawText(colorImg, text, cv::Point(0,20), 0.5, cv::Scalar(0,255,0), 2);

    drawFixLoc(colorImg);

    OpencvViewer::show(win, colorImg);
}


void DepthViewer::onMouseCallback(cv::Mat& img, int event, const cv::Point pnt)
{
    switch(event){
        case cv::EVENT_LBUTTONDOWN:
            _fixLoc = pnt;
            drawFixLoc(img);
            break;
        case cv::EVENT_MOUSEMOVE:
            // uint16_t val = _img.at<uint16_t>(pnt.y, pnt.x);
            // char str[32];
            // sprintf(str, "Depth at mouse: %d", val);
            // drawText(img, str, cv::Point(0,60), 0.5, cv::Scalar(0,255,0), 2);
            break;
    }
}

void DepthViewer::drawFixLoc(cv::Mat& img)
{
    char str[64];
    #ifdef linux
    snprintf(str,64, "Depth at (%d,%d): %d", _fixLoc.x, _fixLoc.y, _img.at<uint16_t>(_fixLoc));
    #endif // linux
    #ifdef WIN32
    sprintf_s(str, "Depth at (%d,%d): %d", _fixLoc.x, _fixLoc.y, _img.at<uint16_t>(_fixLoc));
    #endif // WIN32
    drawText(img, str, cv::Point(0,40), 0.5, cv::Scalar(0,255,0), 2);
}
