/*************************************************************************************
//
//  LICENSE INFORMATION
//
//  BCreator(tm)
//  Software for the control of the 3D Printer, "B9Creator"(tm)
//
//  Copyright 2011-2012 B9Creations, LLC
//  B9Creations(tm) and B9Creator(tm) are trademarks of B9Creations, LLC
//
//  This file is part of B9Creator
//
//    B9Creator is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    B9Creator is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with B9Creator .  If not, see <http://www.gnu.org/licenses/>.
//
//  The above copyright notice and this permission notice shall be
//    included in all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
//    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
*************************************************************************************/

#ifndef B9PROJECTOR_H
#define B9PROJECTOR_H

#include <QWidget>
#include <QHideEvent>
#include <QImage>
#include <QColor>
#include <QByteArray>
#include "crushbitmap.h"

class B9Projector : public QWidget
{
	Q_OBJECT

public:
    B9Projector(bool bPrintWindow, QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowMinMaxButtonsHint|Qt::Window|Qt::WindowCloseButtonHint);
    ~B9Projector();

public slots:
    void showProjector(int x, int y, int w, int h);	// 显示自己，当我们发出信号

    void hideCursor(){setCursor(Qt::BlankCursor);}	//隐藏鼠标指针当发出信号

    void setShowGrid(bool bShow);					// 设置bShow为true，如果网格即将绘制
    void setStatusMsg(QString status);				// 要显示设置状态到消息
    void setCPJ(CrushedPrintJob *pCPJ);				//设置指向CMB的指针显示，如果为空则返回NULL
    bool clearTimedPixels(int iLevel);              //根据级别（0〜255），我们清除与TOVER数组值的所有像素<iLevel
    void createToverMap(int iRadius);
    void setXoff(int xOff){m_xOffset = xOff;drawAll();} //X补偿量的层图像
    void setYoff(int yOff){m_yOffset = yOff;drawAll();} //Y补偿量的层图像
    void createNormalizedMask(double XYPS=0.1, double dZ = 257.0, double dOhMM = 91.088); //当我们显示或调整时调用

signals:
    void eventHiding();             // 发出信号到父类，告知我们正在隐藏
    void hideProjector();			// 发出信号到父类，请求将我们隐藏

    void keyReleased (int iKey);	// 发出信号，即一个键被按下和释放
    void newGeometry (int iScreenNumber, QRect geoRect);

private:
    void keyReleaseEvent(QKeyEvent * pEvent);		//处理按键释放事件
    void mouseReleaseEvent(QMouseEvent * pEvent);	// 处理鼠标按钮释放事件
    void mouseMoveEvent(QMouseEvent * pEvent);		// 处理鼠标移动事件
    void paintEvent (QPaintEvent * pEvent);			//处理画板事件
    void resizeEvent ( QResizeEvent * event );      //处理调整大小事件

    void hideEvent(QHideEvent *event);
    void drawAll();			// 刷新整个屏幕
    void blankProjector();	// 填充黑色到整个背景，将覆盖以前的图象数据
    void drawGrid();		// 绘制使用mGridColor栅格图案
    void drawStatusMsg();	//绘制当前状态信息到投影屏幕上
    void drawCBM();			// draws the current CBM pointed to by mpCBM, returns if mpCBM is null

    void createToverMap0();
    void createToverMap1();
    void createToverMap2();
    void createToverMap3();
	
    bool m_bIsPrintWindow;  // 如果我们锁定到全屏窗口时显示，设置为真
    bool m_bGrid;			// 如果为真，网格会被绘制
	CrushedPrintJob* mpCPJ;	// CPJ to inflate CBM from
    QImage mImage;
    QImage mCurSliceImage;  // 当前标准化的切片，可能具有一些或所有像素被清零
    int m_iLevel;
    QImage m_NormalizedMask;
	QString mStatusMsg;
	int m_xOffset, m_yOffset;
    QByteArray m_vToverMap;

};

#endif // B9PROJECTOR_H
