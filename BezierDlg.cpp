// BezierDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Bezier.h"
#include "BezierDlg.h"
#include <fstream>
#include <iostream>
#include<Common\CommonWin.h>
#include <stdio.h>
#include "atlimage.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "sdm\test_model .h"
#include "FaceReconstruction-master\examples\4dface.h"
#include "ImageLayer.h"

//#include "C:\迅雷下载\FaceReconstruction-master\examples\fit-model.h"

using namespace std;
using namespace cv;
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBezierDlg dialog

CBezierDlg::CBezierDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBezierDlg::IDD, pParent)
{


	m_bMouseMove = false;
	m_leftBtnDown = false;
	m_bKeyOper = false;

	m_bDeleteFile = false;

	m_nowMulti = 1;
	m_points.resize(68);
}

void CBezierDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBezierDlg)
	//}}AFX_DATA_MAP

}

BEGIN_MESSAGE_MAP(CBezierDlg, CDialog)
	//{{AFX_MSG_MAP(CBezierDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()


	//}}AFX_MSG_MAP

	ON_BN_CLICKED(IDC_BTN_SAVE, &CBezierDlg::OnBnClickedBtnSave)
	ON_WM_ERASEBKGND()
	ON_WM_SYSCOMMAND()

	ON_BN_CLICKED(IDC_BTN_BIG, &CBezierDlg::OnBnClickedBtnBig)
	ON_BN_CLICKED(IDC_BTN_ORIGIN, &CBezierDlg::OnBnClickedBtnOrigin)

	
	ON_BN_CLICKED(IDC_BUTTON_IN, &CBezierDlg::OnBnClickedButtonIn)
	ON_BN_CLICKED(IDC_BUTTON_PICLEFT, &CBezierDlg::OnBnClickedButtonPicleft)
	ON_BN_CLICKED(IDC_BUTTON_RIGHT, &CBezierDlg::OnBnClickedButtonRight)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBezierDlg message handlers

BOOL CBezierDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	m_currentMark = -1;

	LoadSdmMode();

	m_dlgMfc.Create(130, this);
	CRect rect;
	GetClientRect(rect);
	m_dlgMfc.MoveWindow(0, 0, 600, rect.Height());


	/*Mat mat(568,637,CV_8UC3);
	FILE * fp = fopen("D:/1.rgb","rb");
	if (fp)
	{
		char szBuf[2048];
		int len = 0;
		int w = 0;
		while ((len = fread(szBuf, 1, sizeof(szBuf), fp)) > 0)
		{
			memcpy(mat.data+w,szBuf,len);
			w += len;
		}
		imwrite("D:/my.bmp",mat);
	}*/
	//m_dlgMfc.SetDlgMain(this);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

/*

	功能：画人脸关键点对应的 移动框
*/
void CBezierDlg::DrawPoint(int start,int end,CDC* pDC)
{
	pDC->MoveTo(m_points[start].x, m_points[start].y);
	for(int i = start; i<=end; i++)
	{
			CPen penStroke(PS_SOLID,1,0x007700);
			CPen *ppenPrevious=pDC->SelectObject(&penStroke);
			pDC->LineTo(m_points[i].x,m_points[i].y);
			pDC->SelectObject(ppenPrevious);

			CPen penStroke1(PS_SOLID,1,0x00FFFF);
		    CPen penStroke2(PS_SOLID,1,0x0000FF);

			CPen *ppenPrevious2;
			if(m_currentMark == i)
			{
				ppenPrevious2 = pDC->SelectObject(&penStroke2);
			}
			else
			{
				ppenPrevious2 = pDC->SelectObject(&penStroke1);
			}
			

			pDC->SetBkMode(TRANSPARENT);
		
			cv::Point2d pt = m_points[i];
			CPoint tmpPts[4];
			int spanD = 6;
			tmpPts[0].x = pt.x - spanD;
			tmpPts[0].y = pt.y - spanD;

			tmpPts[1].x = pt.x + spanD;
			tmpPts[1].y = pt.y - spanD;
			tmpPts[2].x = pt.x + spanD;
			tmpPts[2].y = pt.y + spanD;
			tmpPts[3].x = pt.x - spanD;
			tmpPts[3].y = pt.y + spanD;
		
			
			 pDC->SelectStockObject(NULL_BRUSH);
			 pDC->Ellipse(tmpPts[0].x,tmpPts[0].y,tmpPts[2].x,tmpPts[2].y);

		
			pDC->SelectObject(ppenPrevious2);
	}
}
/*

功能：本绘制主要是  因为在进行微调关键点的时候，防止闪烁， 采用双缓冲区绘制
  绘制 背景待处理图片，然后绘制关键点，绘制到缓冲区，接着整体绘制
  
*/
void CBezierDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

	}
	else
	{
		CDialog::OnPaint();

		CDC * pDC = GetDC();

		if(!m_nowImg.empty() && pDC)
		{
				CImage Image;
				cv::Mat dst = m_nowImg.clone();
				Image.Create(dst.cols,dst.rows,24);
			
				int i;	int j;

				for (i = 0; i < Image.GetHeight(); i++)
				{
					int step = dst.step[0]*i;
					int jump = 0;
					for (j = 0; j < Image.GetWidth(); j++)
					{
						byte *pByte = (byte *)Image.GetPixelAddress(j,i);
						pByte[0] = (unsigned char)dst.data[step+jump+0];
						pByte[1] = (unsigned char)dst.data[step+jump+1];
						pByte[2] = (unsigned char)dst.data[step+jump+2];
		
						jump+=3;
					}
				}


				CDC   MemDC;   //首先定义一个显示设备对象 
				CBitmap   MemBitmap;//定义一个位图对象 

				pDC->SetBkMode(TRANSPARENT);
				//随后建立与屏幕显示兼容的内存显示设备 
				MemDC.CreateCompatibleDC(pDC); 
				//这时还不能绘图，因为没有地方画   ^_^ 
				//下面建立一个与屏幕显示兼容的位图，至于位图的大小嘛，可以用窗口的大小 
				MemBitmap.CreateCompatibleBitmap(pDC,Image.GetWidth(),Image.GetHeight()); 
  
				//将位图选入到内存显示设备中 
				//只有选入了位图的内存显示设备才有地方绘图，画到指定的位图上 
				CBitmap  *pOldBit=MemDC.SelectObject(&MemBitmap); 

				Image.Draw(MemDC.m_hDC,0,0);

				if(m_points.size() > 0) 
				{
					//脸轮廓
					DrawPoint(0,16,&MemDC);
					//左眉毛
					DrawPoint(17,21,&MemDC);
					//右眉毛
					DrawPoint(22,26,&MemDC);
					//鼻梁
					DrawPoint(27,30,&MemDC);
					//鼻下滑线
					DrawPoint(31,35,&MemDC);
					//左眼
					DrawPoint(36,41,&MemDC);
					//右眼
					DrawPoint(42,47,&MemDC);
					//嘴巴外轮廓
					DrawPoint(48,59,&MemDC);

					//嘴巴 内轮廓
					DrawPoint(60,67,&MemDC);
				}
				//将内存中的图拷贝到屏幕上进行显示 
				pDC->BitBlt(0,0,Image.GetWidth(),Image.GetHeight(),&MemDC,0,0,SRCCOPY); 


				MemDC.SelectObject(pOldBit);
				//绘图完成后的清理 
				MemBitmap.DeleteObject(); 
				MemDC.DeleteDC();

				ReleaseDC(pDC);

		}

	}
}

void CBezierDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	m_leftBtnDown = true;
	// TODO: Add your message handler code here and/or call default
	m_currentMark = -1;
		double x, y;
		double t=50;
		for(int i = 0; i < m_points.size(); i++)
		{
			x = m_points[i].x - point.x, y = m_points[i].y - point.y;
			x*=x; y*=y;
			if(x + y < t)
			{	
				m_currentMark = i;	t=x+y;	
			}

		}

		
	
	CDialog::OnLButtonDown(nFlags, point);
	RedrawWindow();
}

void CBezierDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_leftBtnDown = false;
	// TODO: Add your message handler code here and/or call default
	m_bMouseMove = false;
	

	CDialog::OnLButtonUp(nFlags, point);
}

void CBezierDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	if(!m_bMouseMove && m_leftBtnDown)
	{
		m_bMouseMove = true;
	}

	
	// TODO: Add your message handler code here and/or call default
	if(m_currentMark >= 0 && m_leftBtnDown) 
	{
		m_points[m_currentMark].x = point.x;
		m_points[m_currentMark].y = point.y;
		RedrawWindow();
	}
	

	CDialog::OnMouseMove(nFlags, point);
}







void CBezierDlg::DoGetPtsAndDraw(CString fileName)
{
	   m_nowMulti = 1;
	   m_fileName=fileName; //文件名保存在了FilePathName里
		CString ptsFile = m_fileName;
	
		int index = ptsFile.Replace(".jpg",".pts");
		if(index == 0)
		{
			index = ptsFile.Replace(".png",".pts");
			if(index == 0)
			{
				index = ptsFile.Replace(".bmp",".pts");
			}
		}

		ifstream locations(ptsFile.GetBuffer(0), ios_base::in);
		if(locations.is_open() && index)
		{
			m_srcImg = cv::imread(m_fileName.GetBuffer(0));
			string line;
			// The main file contains the references to other files
			while (!locations.eof())
			{ 
				getline(locations, line);

				if(line.compare("{") == 0)
				{
						for(int i=0;i<68;i++)
						{
							locations>>m_points[i].x;
							locations>>m_points[i].y;
						}
				}
			}
		 
			
		}
		else
		{

				//自动生成 关键点
				m_fileName=fileName; //文件名保存在了FilePathName里
				Mat img = imread(m_fileName.GetBuffer(0));

				vector<Point> mainPts = DoDetectMark(img);
				if(!mainPts.empty())
				{
					for(int i=0;i<mainPts.size();i++)
					{
						m_points[i].x = mainPts[i].x;
						m_points[i].y = mainPts[i].y;
					}

					m_srcImg = img;
				}
				else
				{
					if(!m_srcImg.empty())m_srcImg.release();
					if(!m_nowImg.empty())m_nowImg.release();
				}

		}


		if(!m_srcImg.empty())
		{
			m_nowImg = m_srcImg.clone();

			CString strInfo;
			strInfo.Format(" 当前放大倍数 %d倍.可移动上下左右按键微调 ",m_nowMulti);
			SetWindowText("关键点编辑器   "+m_fileName+strInfo);

			Invalidate();
		}
		else
		{ 
			
			   SetWindowText("关键点编辑器    "+m_fileName);
			
			    Mat dst = imread(m_fileName.GetBuffer(0)); 
				CDC * pDC = GetDC();
				if(!dst.empty() && pDC)
				{
					
					CImage Image;
					Image.Create(dst.cols,dst.rows,24);

					int i;
					int j;
					for (i = 0; i < Image.GetHeight(); i++)
					{
						int step = dst.step[0]*i;
						int jump = 0;
						for (j = 0; j < Image.GetWidth(); j++)
						{
							byte *pByte = (byte *)Image.GetPixelAddress(j,i);
							pByte[0] = (unsigned char)dst.data[step+jump+0];
							pByte[1] = (unsigned char)dst.data[step+jump+1];
							pByte[2] = (unsigned char)dst.data[step+jump+2];
		
							jump+=3;
						}
					}

					Image.Draw(pDC->m_hDC,0,0);
					ReleaseDC(pDC);
				}
			  

		}

}
void CBezierDlg::OnBnClickedBtnImport()
{
	// TODO: 在此添加控件通知处理程序代码
	CString FilePathName;
    CFileDialog dlg(TRUE, //TRUE为OPEN对话框，FALSE为SAVE AS对话框
        NULL, 
        NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        (LPCTSTR)_TEXT("pic Files (*.jpg)|*.jpg|(*.bmp)|*.bmp|(*.png)|*.png|All Files (*.*)|*.*||"),
        NULL);
    if(dlg.DoModal()==IDOK)
    {
		DoGetPtsAndDraw(dlg.GetPathName());
		m_dlgMfc.ShowWindow(SW_HIDE);
	}
}


void CBezierDlg::OnBnClickedBtnSave()
{
	// TODO: 在此添加控件通知处理程序代码
	if(!m_srcImg.empty())
	{
		CString ptsFile = m_fileName;
		int index = ptsFile.Replace(".jpg",".pts");
		if(index == 0)
		{
			index = ptsFile.Replace(".png",".pts");
		}
	
		if(index == 0)
		{
			index = ptsFile.Replace(".bmp",".pts");
		}
		ofstream locations(ptsFile.GetBuffer(0), ios_base::out);
		if(!locations.is_open())
		{
			cout << "Couldn't open the model file, aborting" << endl;
			return ;
		}
		string str = "version: 1";
		locations<<str<<"\n";
		str= "n_points:  68";
		locations <<str<<"\n";
		locations<<"{"<<"\n";

		for(int i=0;i<68;i++)
		{
			locations<<(m_points[i].x/m_nowMulti)<<" ";
			locations<<(m_points[i].y/m_nowMulti)<<"\n";
		}
		locations<<"}";
		locations.close();

	

		OnBnClickedButton3d(m_fileName.GetBuffer(0), ptsFile.GetBuffer(0));

		cv::Mat matImg = imread(m_fileName.GetBuffer(0));
		cv::Rect boundRect = boundingRect(m_points);
		int span = boundRect.width*0.3;
		boundRect.x = boundRect.x - span;
		if (boundRect.x < 0) boundRect.x = 0;
		boundRect.width = boundRect.width + 2 * span;
		if (boundRect.width + boundRect.x >= matImg.cols)
			boundRect.width = matImg.cols - boundRect.x;

		boundRect.y = boundRect.y - span;
		if (boundRect.y < 0) boundRect.y = 0;
		boundRect.height = boundRect.height + 2 * span;
		if (boundRect.height + boundRect.y >= matImg.rows)
			boundRect.height = matImg.rows - boundRect.y;

		cv::Mat rectImg = matImg(boundRect).clone();
		DrawThePicPoint(rectImg,1000,540,360);
	}
	

}


BOOL CBezierDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(!m_leftBtnDown)
	{
		if(m_bKeyOper)
		{
			m_bKeyOper = false;return 1;
		}
		else
		{
			return CDialog::OnEraseBkgnd(pDC);
		}
	}
	else
	{
		return 1;
	}
}



void CBezierDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(nID == 61488)
	{
		int cx = GetSystemMetrics(SM_CXSCREEN);
		int cy = GetSystemMetrics(SM_CYSCREEN);
		MoveWindow(0,0,cx,cy);

		GetDlgItem(IDC_BTN_ORIGIN)->MoveWindow(cx-240,5,110,40);
		GetDlgItem(IDC_BTN_BIG)->MoveWindow(cx-120,5,110,40);

		GetDlgItem(IDC_STATIC_PIC1)->MoveWindow(cx-240,55,240,2);
		GetDlgItem(IDC_BTN_SAVE)->MoveWindow(cx-120,60,110,40);
		GetDlgItem(IDC_STATIC_PIC)->MoveWindow(cx-240,115,240,2);


	
		Invalidate();
	}
	CDialog::OnSysCommand(nID, lParam);
}



BOOL CBezierDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	if(pMsg->message == WM_KEYUP)
	{
		
			int span = 1;
			switch (pMsg->wParam)
			{
			case VK_UP:
				{
					if(m_currentMark > 0)
					{
						m_points[m_currentMark].y-=span;
						m_bKeyOper = true;
						RedrawWindow();
					}
				}
				break;
			case VK_DOWN:
				if(m_currentMark > 0)
					{
						m_points[m_currentMark].y+=span;
						m_bKeyOper = true;
						RedrawWindow();
					}
				break;
			case VK_RIGHT:
				if(m_currentMark > 0)
					{
						m_points[m_currentMark].x+=span;
						m_bKeyOper = true;
						RedrawWindow();
					}
				break;
			case VK_LEFT:
				if(m_currentMark > 0)
					{
						m_points[m_currentMark].x-=span;
						m_bKeyOper = true;
						RedrawWindow();
					}
			
				break;
			default:
				break;
			}
			

		
		
	}

	if(pMsg->wParam == VK_UP
				|| pMsg->wParam == VK_DOWN
				|| pMsg->wParam == VK_LEFT
				|| pMsg->wParam == VK_RIGHT)
			{
				if(m_currentMark > 0)
				{
					return 1;
				}
			}

	return CDialog::PreTranslateMessage(pMsg);
}


void CBezierDlg::OnBnClickedBtnBig()
{
	m_nowMulti ++;
	if(!m_srcImg.empty())
	{
		resize(m_srcImg,m_nowImg,Size(m_srcImg.cols*m_nowMulti,m_srcImg.rows*m_nowMulti));
		for(int k = 0;k<m_points.size();++k)
		{
			m_points[k].x = m_points[k].x*m_nowMulti;
			m_points[k].y = m_points[k].y*m_nowMulti;
		}
	}
	RedrawWindow();
}


void CBezierDlg::OnBnClickedBtnOrigin()
{

	if(!m_srcImg.empty())
	{
		m_nowImg = m_srcImg.clone();
		for(int k = 0;k<m_points.size();++k)
		{
			m_points[k].x = m_points[k].x/m_nowMulti;
			m_points[k].y = m_points[k].y/m_nowMulti;
		}
	}

	RedrawWindow();
	m_nowMulti = 1;
}



void CBezierDlg::OnBnClickedButton3d(char * fileName,char * ptsName)
{
	FitMode3d(fileName,ptsName);

	std::vector<string> list;
	//list.push_back("./out.obj");
	list.push_back("./current_merged.obj");

	Mat matSrc = imread("current_merged.isomap.png",-1);
	Mat matDst = imread("out.isomap.png", -1);
	MixLayer(matSrc,matDst);
	imwrite("current_merged.isomap.bmp", matDst);

	m_dlgMfc.ShowNowTestB(list);
}

void CBezierDlg::DrawThePicPoint(cv::Mat clmResult, int posX, int posY, int dstW)
{

	//若使用前不想把原来绘制的图片去掉，可以删去上面那三段
	CDC *pDC = GetDC();

	int biaoWidht = dstW;
	cv::Mat dst;
	if (clmResult.cols > dstW)
	{
		float ratio = clmResult.cols*1.0 / biaoWidht;
		int height = clmResult.rows / ratio;

		resize(clmResult, dst, cv::Size(biaoWidht, height));
	}
	else
	{
		dst = clmResult.clone();
	}




	CImage Image;
	Image.Create(dst.cols, dst.rows, 24);
	int i;
	int j;
	for (i = 0; i < Image.GetHeight(); i++)
	{
		int step = dst.step[0] * i;
		int jump = 0;
		for (j = 0; j < Image.GetWidth(); j++)
		{
			byte *pByte = (byte *)Image.GetPixelAddress(j, i);

			if (dst.channels() != 1)
			{
				pByte[0] = (unsigned char)dst.data[step + jump + 0];
				pByte[1] = (unsigned char)dst.data[step + jump + 1];
				pByte[2] = (unsigned char)dst.data[step + jump + 2];

				jump += 3;
			}
			else
			{
				pByte[0] = (unsigned char)dst.data[step + jump + 0];
				pByte[1] = (unsigned char)dst.data[step + jump + 0];
				pByte[2] = (unsigned char)dst.data[step + jump + 0];

				jump += 1;
			}

		}
	}

	Image.Draw(pDC->m_hDC, posX, posY);
	Image.Destroy();

	ReleaseDC(pDC);
}
void CBezierDlg::OnBnClickedButtonIn()
{

	// TODO: 在此添加控件通知处理程序代码
	CString FilePathName;
	CFileDialog dlg(TRUE, //TRUE为OPEN对话框，FALSE为SAVE AS对话框
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		(LPCTSTR)_TEXT("pic Files (*.jpg)|*.jpg|(*.bmp)|*.bmp|(*.png)|*.png|All Files (*.*)|*.*||"),
		NULL);
	if (dlg.DoModal() == IDOK)
	{
		DoGetPtsAndDraw(dlg.GetPathName());
		m_dlgMfc.ShowWindow(SW_HIDE);
	}
}


void CBezierDlg::OnBnClickedButtonPicleft()
{
	cv::Mat mat = imread("current_merged.isomap.bmp");
	cv::Mat tmpMat;
	flip(mat,tmpMat,1);

	cv::Mat mat4;
	cv::Mat tmpMat4;
	cvtColor(mat,mat4,CV_BGR2BGRA);
	cvtColor(tmpMat,tmpMat4,CV_BGR2BGRA);

	cv::Mat maskMat(mat.rows,mat.cols,CV_8UC1);
	maskMat.setTo(255);

	int span = mat.cols*0.1;
	rectangle(maskMat,cv::Point(mat.cols/2,0),cv::Point(mat.cols-1,mat.rows-1),Scalar(0),-1);
	GaussianBlur(maskMat,maskMat,cv::Size(49,49),0);

	cv::ellipse(maskMat, cv::Point(180, 184), Size(100, 50),360,0,360,  Scalar(0),-1);
	GaussianBlur(maskMat, maskMat, cv::Size(49, 49), 0);

	cv::imwrite("mask.bmp",maskMat);

	for (int i = 0; i < mat.rows; ++i)
	{
		int jump = 0;
		int step = tmpMat4.step[0] * i;
		for (int j = 0; j < mat.cols; ++j)
		{
			tmpMat4.data[step + jump +3] = maskMat.data[maskMat.cols*i + j];
			jump += 4;
		}
		
	}
	cv::imwrite("mat4.bmp", mat4);

	MixLayer(tmpMat4,mat4);
	
	cv::imwrite("current_merged.isomap.bmp",mat4);

	std::vector<string> list;
	list.push_back("./current_merged.obj");
	m_dlgMfc.ShowNowTestB(list);

}


void CBezierDlg::OnBnClickedButtonRight()
{
	cv::Mat mat = imread("current_merged.isomap.bmp");
	cv::Mat tmpMat;
	flip(mat, tmpMat, 1);

	cv::Mat mat4;
	cv::Mat tmpMat4;
	cvtColor(mat, mat4, CV_BGR2BGRA);
	cvtColor(tmpMat, tmpMat4, CV_BGR2BGRA);

	cv::Mat maskMat(mat.rows, mat.cols, CV_8UC1);
	maskMat.setTo(255);

	int span = mat.cols*0.1;
	rectangle(maskMat, cv::Point(0, 0), cv::Point(mat.cols /2, mat.rows - 1), Scalar(0), -1);
	GaussianBlur(maskMat, maskMat, cv::Size(49, 49), 0);

	cv::ellipse(maskMat, cv::Point(340, 184), Size(100, 50), 360, 0, 360, Scalar(0), -1);
	GaussianBlur(maskMat, maskMat, cv::Size(49, 49), 0);

	cv::imwrite("mask.bmp", maskMat);

	for (int i = 0; i < mat.rows; ++i)
	{
		int jump = 0;
		int step = tmpMat4.step[0] * i;
		for (int j = 0; j < mat.cols; ++j)
		{
			tmpMat4.data[step + jump + 3] = maskMat.data[maskMat.cols*i + j];
			jump += 4;
		}

	}
	cv::imwrite("mat4.bmp", mat4);

	MixLayer(tmpMat4, mat4);

	cv::imwrite("current_merged.isomap.bmp", mat4);

	std::vector<string> list;
	list.push_back("./current_merged.obj");
	m_dlgMfc.ShowNowTestB(list);
}
