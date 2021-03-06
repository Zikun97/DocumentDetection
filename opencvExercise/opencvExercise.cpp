// opencvExercise.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include <iostream>
#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;

const int maxWidth = 500; //强制规定：缩略图的宽度

bool sortByArea(vector<Point> &v1, vector<Point> &v2) {
	double v1Area = fabs(contourArea(Mat(v1)));
	double v2Area = fabs(contourArea(Mat(v2)));
	return v1Area > v2Area;
}


/*展示图片*/
void ShowImg(String name, Mat showImg) {
	namedWindow(name, WINDOW_NORMAL);
	imshow(name, showImg);
	return;
}


/*缩小图片*/
Mat ResizeImg(Mat srcImg)
{
	if (srcImg.rows < maxWidth) {
		return srcImg;
	}
	int ratio = srcImg.rows / maxWidth;
	int reWidth = srcImg.rows / ratio;
	int reHeight = srcImg.cols / ratio;
	
	Mat resizedImg;
	resize(srcImg, resizedImg, Size(reWidth, reHeight));
	
	return resizedImg;
}

/*预处理图片，包括转为灰度图、自适应直方图均衡化、低通量滤波、canny边缘检测等*/
Mat ProcessImg(Mat srcImg, Size blurSize, int threshold_1) {
	//转为灰度图片
	Mat grayImg;
	cvtColor(srcImg, grayImg, COLOR_BGR2GRAY);
	ShowImg("灰度图", grayImg);
	

	//自适应直方图均衡化
	Mat claheImg;
	Ptr<CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(5);
	clahe->apply(grayImg, claheImg);
	ShowImg("自适应直方图均衡", claheImg);

	//双边滤波
	/*Mat bilateralImg;
	bilateralFilter(grayImg, bilateralImg, -1, 25, 10);
	ShowImg("双边滤波后的图片", bilateralImg);*/
	
	//高斯模糊 
	Mat gaussImg;
	GaussianBlur(grayImg, gaussImg, Size(3, 3), 0);
	ShowImg("模糊后的图片", gaussImg);

	//膨胀操作
	//Mat element = getStructuringElement(MORPH_RECT, Size(2, 2)); //第一个参数MORPH_RECT表示矩形的卷积核，当然还可以选择椭圆形的、交叉型的
	//dilate(gaussImg, gaussImg, element);  //实现过程中发现，适当的膨胀很重要
	//ShowImg("膨胀操作", gaussImg);

	//canny边缘检测
	Mat cannyImg;
	Canny(gaussImg, cannyImg, threshold_1, 200);
	ShowImg("canny边缘检测", cannyImg);

	//霍夫变换  不能用！！！太坑了
	Mat imgCopy = srcImg.clone();
	vector<Vec2f> houghLines;
	HoughLines(cannyImg, houghLines, 1, CV_PI / 180, 130, 0, 0);
	cout << "直线数目：" << houghLines.size() << endl;
	for (size_t i = 0; i < houghLines.size(); ++i) {
		float rho = houghLines[i][0], theta = houghLines[i][1];
		Point pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a * rho, y0 = b * rho;
		pt1.x = cvRound(x0 + 1000 * (-b));
		pt1.y = cvRound(y0 + 1000 * a);
		pt2.x = cvRound(x0 - 1000 * (-b));
		pt2.y = cvRound(y0 - 1000 * a);
		line(imgCopy, pt1, pt2, Scalar(0, 0, 255), 1, LINE_4);
	}
	ShowImg("霍夫变换", imgCopy);

	//threshold


	//直方化处理

	return cannyImg;
}


/*扫描图片，获取边界信息，返回几个点*/
vector<Point> ScanImg(Mat srcImg) {
	Mat resizedImg = ResizeImg(srcImg);
	ShowImg("原图", srcImg);
	ShowImg("缩小图", resizedImg);

	int blurArray[] = { 3, 5, 7};
	
	int thresArray[] = { 10, 50, 100, 150 };
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 4; ++j) {
			Mat processedImg = ProcessImg(resizedImg, Size(blurArray[i], blurArray[i]), thresArray[j]);
			ShowImg("预处理后的图像", processedImg);

			vector<vector<Point> > contours;	//存储轮廓信息
			findContours(processedImg, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

			int len = contours.size();
			if (len == 0)
			{
				continue;
			}
			std::sort(contours.begin(), contours.end(), sortByArea);
			double arcLen = arcLength(contours[0], true);
			for (int k = 0; k < contours[0].size(); ++k) {
				cout << contours[0][k].x << " " << contours[0][k].y << endl;
			}
			vector<Point> outPoints;
			approxPolyDP(contours[0], outPoints, 0.01 * arcLen, true);
			cout << "多边逼近的结果数目h:" << outPoints.size() << endl;

			/***********************单纯测试部分*/
			Mat circleMat = resizedImg.clone();
			for (int n = 0; n < outPoints.size(); ++n)
			{
				circle(circleMat, outPoints[n], 5, Scalar(0, 0, 255));
			}
			ShowImg("找到的点", circleMat);

			//绘制出轮廓图
			for (int k = 0; k < len; ++k) {
			Mat outImg = resizedImg.clone();
				cout << "k:" << k << "   " << contours[k].size() << endl;
				
				drawContours(outImg, contours, k, Scalar(0, 0, 255));
				ShowImg("轮廓图", outImg);				
				waitKey();
			}
			/******测试部分end*****/

			if (outPoints.size() == 4)
			{
				return outPoints;
			}
		}
	}
	
	vector<Point> time;
	return time;
}

/*将rgb转换为lab*/
Mat LabImg(Mat srcImg){
	Mat rgbImg;		//RGB图像
	Mat labImg;		//LAB图像
	float nScale = 1;
	Size sz;
	sz.width = (int)(srcImg.cols * nScale);
	sz.height = (int)(srcImg.rows * nScale);
	rgbImg.create(sz, srcImg.type());
	resize(srcImg, rgbImg, sz);

	cvtColor(srcImg, labImg, COLOR_BGR2YUV);
	vector<Mat> channels;
	split(labImg, channels);
	ShowImg("L", channels.at(0));
	ShowImg("A", channels.at(1));
	ShowImg("B", channels.at(2));

	return channels.at(0);
}

/*使用hsv并用颜色来划分空间*/
Mat devideImg(Mat srcImg) {
	int height = srcImg.rows;
	int width = srcImg.cols;
	
	cout << width / 2 << "  " << height /2 << endl;

	Mat bgr;	//灰度值归一化
	Mat hsv;	
	Mat dst = Mat::zeros(Size(srcImg.cols, srcImg.rows), CV_32FC3);
	Mat mask;	//掩膜
	srcImg.convertTo(bgr, CV_32FC3, 1.0 / 255, 0);
	cvtColor(bgr, hsv, COLOR_BGR2HSV);
	cout << hsv.type() << endl;
	
	cout << hsv.channels() << endl;
	cout << hsv.at<cv::Vec3f>(0, 0)[0] << " " << hsv.at<cv::Vec3f>(0, 0)[1] << " " << hsv.at<cv::Vec3f>(0, 0)[2] << endl;
	//cout << hsv.at<cv::Vec3f>(1512, 2016)[0] << " " << hsv.at<cv::Vec3f>(1512, 2016)[0] << " " << hsv.at<cv::Vec3f>(1512, 2016)[0] << endl;
	cout << hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[0] << " " << hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[1] << " " << hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[2] << endl;
	cout << hsv.at<cv::Vec3f>((long)(height / 2), (int)(width / 2))[0] << " " << hsv.at<cv::Vec3f>((int)(height / 2), (int)(width / 2))[1] << " " << hsv.at<cv::Vec3f>((int)(height / 2), (int)(width / 2))[2] << endl;
	Scalar color_1 = Scalar(hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[0] - 20, hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[1] - 20, hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[2] - 20);
	Scalar color_2 = Scalar(hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[0] + 20, hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[1] + 20, hsv.at<cv::Vec3f>((long)(width / 2), (long)(height / 2))[2] + 20);
	/********获取内部色彩*********/
	double w[] = { 0.25, 0.5, 0.75 };	//宽度权重
	double h[] = { 0.25, 0.5, 0.75 };	//高度权重
	Scalar pColor[9];					//存储9个点的颜色特征
	int tag[9];
	int counter[9] = {0,0};
	int index = 0;
	for (int i = 0; i < 3; ++i) {
		for(int j = 0; j < 3; ++j)
		{
			pColor[index++] = hsv.at<Vec3f>((long)(h[j] * height), (long)(w[i] * width));
		}
	}

	for (int i = 0; i < 9; i++)
	{
		if (i == 0) {
			tag[0] = 0;
			continue;
		}
		for (int j = 0; j < i; ++j)
		{
			if (abs(pColor[i][0] - pColor[j][0]) <= 50 && abs(pColor[i][1] - pColor[j][1]) <= 50 && abs(pColor[i][2] - pColor[j][2]) <= 50)
			{
				tag[i] = j;
				counter[j]++;
				continue;
			}
		}
	}

	index = 0;
	int Max = 0;
	for (int i = 0; i < 9; ++i) {
		if (tag[i] > Max)
		{
			Max = tag[i];
			index = i;
		}
	}

	Scalar maskColor = pColor[index];


	/********获取外部色彩******/



	Scalar colorUp = Scalar(maskColor[0] + 10, maskColor[1] + 10, maskColor[2] + 10);
	Scalar colorDown = Scalar(maskColor[0] - 10, maskColor[1] - 10, maskColor[2] - 10);
	Scalar color = hsv.at<Vec3f>(0, 0);
	cout << color[0] << endl;
	inRange(hsv, colorDown, colorUp, hsv);
	ShowImg("色彩分割", hsv);

	return hsv;

}





int main()
{
	Mat sourceImg = imread("F:\\vsFile\\opencvExercise\\x64\\Debug\\test_img\\9.png");
	if (sourceImg.data == NULL) {
		cout << "图片读取失败！" << endl;
		exit(0);
	}
	ShowImg("真正的原图", sourceImg);

	Mat resizedImg = ResizeImg(sourceImg);
	ShowImg("缩略图1", resizedImg);
	
	ScanImg(sourceImg);

	waitKey();
	return 0;
}

