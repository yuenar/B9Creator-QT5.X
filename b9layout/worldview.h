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

#ifndef WORLDVIEW_H
#define WORLDVIEW_H

#include <QGLWidget>
#include "b9layout.h"
#include "b9supportstructure.h"
#include <QPoint>
class Slice;
class B9ModelInstance;
class B9SupportStructure;
class MainWindow;
class B9Layout;
class Triangle3D;
class WorldView : public QGLWidget
{
    Q_OBJECT
 public:
     WorldView(QWidget *parent, B9Layout* main);
     ~WorldView();
     QTimer* pDrawTimer; //刷新3D场景

     bool shiftdown; //（公有类）从而使主窗口可以轻易改变这些值
     bool controldown;
 public slots:


	void CenterView();
    void TopView();
    void RightView();
    void FrontView();
    void BackView();
    void LeftView();
    void BottomView();
    void SetPerpective(bool persp){perspective = persp;}
    void SetRevolvePoint(QVector3D point);
    void SetPan(QVector3D pan);
    void SetZoom(double zoom);
    void setXRotation(float angle);
    void setYRotation(float angle);
    void setZRotation(float angle);


    void UpdatePlasmaFence();//检查是否有任何实例越界（超过了构建区域）并显示栅栏。

    void UpdateVisSlice();

    void UpdateTick();// 设置更新计时器每过1/60秒后调用此方法，然后刷新OpenGL屏幕
	void SetTool(QString tool);
    QString GetTool();
    void ExitToolAction();//当鼠标移动时，工具不可用。

 public://获取

    QVector3D GetPan();
    float GetZoom();
    QVector3D GetRotation();

public: //事件
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent * event );
    void keyReleaseEvent(QKeyEvent * event );

    //工具代码
    void OnToolInitialAction(QString tool, QMouseEvent* event);
    void OnToolDragAction(QString tool, QMouseEvent* event);
    void OnToolReleaseAction(QString tool, QMouseEvent* event);
    void OnToolHoverMove(QString tool, QMouseEvent* event);
    QString toolStringMemory;
    QString toolStringMemory2;
    QVector3D toolVectorMemory1;
    QVector3D toolVectorMemory2;
    QVector3D toolVectorOffsetMemory;
    double toolDoubleMemory1;
    double toolDoubleMemory2;
    double toolDoubleMemory3;
    B9SupportStructure toolSupportMemory;
    bool toolBoolMemory1;


 signals:

    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);

 private://函数
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);


    void ResetCamera(bool xrayon = false);
    void DrawMisc();//绘制填充...
    void DrawInstances(); //绘制所有实例!
    void DrawVisualSlice();//绘制可视切片指标。
    void DrawBuildArea();//绘制生成区域的边界。
	

    B9ModelInstance* SelectInstanceByScreen(QPoint pos);//在屏幕中找出对象的坐标,
    B9SupportStructure* GetSupportByScreen(QPoint pos);//返回指向对象的指针。
    QString GetSupportSectionByScreen(QPoint pos, B9SupportStructure* sup);

    void Update3DCursor(QPoint pos);
    void Update3DCursor(QPoint pos, B9ModelInstance *trackInst);
    void Update3DCursor(QPoint pos, B9ModelInstance *trackInst, bool &isAgainstInst);
    QVector3D Get3DCursorOnScreen(QPoint pos, QVector3D hintPos, double standoff);

    unsigned int GetPickTriangleIndx(B9ModelInstance *inst, QPoint pos, bool &success);//返回用户点击任何模型的三角数据，若返回即成功
    QVector3D GetPickOnTriangle(QPoint pos, B9ModelInstance *inst, unsigned int triIndx);
    QVector3D GetDrillingHit(QVector3D localBeginPosition, B9ModelInstance *inst, bool &hitground, Triangle3D *&pTri);




    void UpdateSelectedBounds();


 private: //成员
    //可视变量
    float xRot;
    float yRot;
    float zRot;
    float xRotTarget;//目标值的平稳过渡。
    float yRotTarget;
    float zRotTarget;

    float camdist;
    float camdistTarget;

    QString currViewAngle;//上下左右前后6个视角

    bool perspective;
    bool hideNonActiveSupports;

    QVector3D pan;
    QVector3D panTarget;
    QVector3D revolvePoint;
    QVector3D revolvePointTarget;

    float deltaTime;//帧速率和时差变量
    float lastFrameTime;


    QVector3D cursorPos3D;//真正的3D坐标
    QVector3D cursorNormal3D;//
    QVector2D cursorPosOnTrackCanvas;//类似于像素线，但在视界中真实可见
    QVector3D cursorPreDragPos3D;
    QVector3D cursorPostDragPos3D;//TODO尚未实现
    QVector3D PreDragInstanceOffset;
    QVector3D PreDragRotationOffsetData;//不是一个真正的坐标只是信息。

    //工具/键值
	QString currtool;
	bool pandown;
	bool dragdown;

    //可视建筑面积，使用项目应设置为实际大小
	float buildsizex;
	float buildsizey;
	float buildsizez;

    //填充视觉栅栏
    bool fencesOn[5];
    float fenceAlpha;
    float supportAlpha;

    //鼠标线
    QVector2D mouseDeltaPos;
    QPoint mouseDownInitialPos;
    QPoint mouseLastPos;

    //可视切片如下
    Slice* pVisSlice;

    //布局窗口指针.
    B9Layout* pMain;
 };

 #endif
