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

#include <QtGui>
#include <QtOpenGL>
#include <QTime>
#include <QMessageBox>

#include "OS_GL_Wrapper.h"
#include "math.h"
#include "worldview.h"
#include "b9layoutprojectdata.h"
#include "b9modelinstance.h"
#include "b9supportstructure.h"
#include "geometricfunctions.h"
#include "sliceset.h"
#include "slice.h"


#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif


WorldView::WorldView(QWidget *parent, B9Layout* main) : QGLWidget(parent)
{
	pMain = main;

    xRot = 315.0;
	yRot = 0.0;
    zRot = 45.0;

    xRotTarget = 315.0;
    yRotTarget = 0.0;
    zRotTarget = 45.0;

    currViewAngle = "FREE";

    camdist = 350;
    camdistTarget = 350;

    deltaTime = 0.0;//每帧毫秒。
    lastFrameTime = QTime::currentTime().msec();

    pan = QVector3D(0,0,0);//在构建中心位置设置一个托盘。
    panTarget = QVector3D(0,0,0);

    revolvePoint = QVector3D(0,0,0);

    cursorPos3D = QVector3D(0,0,0);
    cursorNormal3D = QVector3D(0,0,0);
    cursorPreDragPos3D = QVector3D(0,0,0);
    cursorPostDragPos3D = QVector3D(0,0,0);

    perspective = true;
    hideNonActiveSupports = false;

    //工具/键
    currtool = "move";
	shiftdown = false;
    controldown = false;
	dragdown = false;
	pandown = 0;

    buildsizex = pMain->ProjectData()->GetBuildSpace().x();
    buildsizey = pMain->ProjectData()->GetBuildSpace().y();
    buildsizez = pMain->ProjectData()->GetBuildSpace().z();

    //视觉栅栏
    fencesOn[0] = false;
    fencesOn[1] = false;
    fencesOn[2] = false;
    fencesOn[3] = false;

    pVisSlice = NULL;

	pDrawTimer = new QTimer();
	connect(pDrawTimer, SIGNAL(timeout()), this, SLOT(UpdateTick()));
    pDrawTimer->start(16.66);//期望值60fps。

	setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
}

WorldView::~WorldView()
{
	delete pDrawTimer;
}

static void qNormalizeAngle(float &angle)
{
    while (angle < 0)
        angle += 360;
    while (angle >= 360 )
        angle -= 360;
}


///////////////////////////////////////
//Public Slots
///////////////////////////////////////
void WorldView::setXRotation(float angle)
{
    qNormalizeAngle(angle);

	if(angle < 180)
	{
		return;
	}

    if (angle != xRotTarget)
	{
		
        xRotTarget = angle;
        emit xRotationChanged(angle);
    }
}
void WorldView::setYRotation(float angle)
{
    //qNormalizeAngle(angle);
    if (angle != yRotTarget) {
        yRotTarget = angle;
        emit yRotationChanged(angle);
        
    }
}
void WorldView::setZRotation(float angle)
{
    //qNormalizeAngle(angle);
    if (angle != zRotTarget) {
        zRotTarget = angle;
        emit zRotationChanged(angle);
    }
}
void WorldView::CenterView()
{
    panTarget = QVector3D(0,pMain->ProjectData()->GetBuildSpace().z()/2.0,0);//在构建中心位置设置一个托盘。
    camdistTarget = 250;

}

void WorldView::TopView()
{
    qNormalizeAngle(xRotTarget);
    qNormalizeAngle(xRot);
    qNormalizeAngle(zRot);
    xRotTarget = 360;
    zRotTarget = 0.1f;
    currViewAngle = "TOP";
}
void WorldView::RightView()
{
    qNormalizeAngle(xRotTarget);
    qNormalizeAngle(xRot);
    qNormalizeAngle(zRot);
    xRotTarget = 180.0 + 90;
    zRotTarget = 180 + 90.0;
    currViewAngle = "RIGHT";
}
void WorldView::FrontView()
{
    qNormalizeAngle(xRotTarget);
    qNormalizeAngle(xRot);
    qNormalizeAngle(zRot);
    xRotTarget = 180+90;
    zRotTarget = 0.1f;
    currViewAngle = "FRONT";
}
void WorldView::BackView()
{
    qNormalizeAngle(xRotTarget);
    qNormalizeAngle(xRot);
    qNormalizeAngle(zRot);
    xRotTarget = 180+90;
    zRotTarget = 180.0f;
    currViewAngle = "BACK";
}
void WorldView::LeftView()
{
    qNormalizeAngle(xRotTarget);
    qNormalizeAngle(xRot);
    qNormalizeAngle(zRot);
    xRotTarget = 180+90;
    zRotTarget = 180.0 - 90;
    currViewAngle = "LEFT";
}
void WorldView::BottomView()
{
    qNormalizeAngle(xRotTarget);
    qNormalizeAngle(xRot);
    qNormalizeAngle(zRot);
    xRotTarget = 180;
    zRotTarget = 0.1f;
    currViewAngle = "BOTTOM";
}

void WorldView::SetRevolvePoint(QVector3D point)
{
    revolvePointTarget = point;
}
void WorldView::SetPan(QVector3D pan)
{
    panTarget = pan;
}
void WorldView::SetZoom(double zoom)
{
    camdistTarget = zoom;
}

//计时器调用 - 绘制视界界并更新很酷的过渡视觉效果
void WorldView::UpdateTick()
{
    float dt = QTime::currentTime().msec() - lastFrameTime;
    bool anyFenceOn;

    if(dt > 0) deltaTime = dt;

    lastFrameTime = QTime::currentTime().msec();
    //float timeScaleFactor = deltaTime/33.333;


    buildsizex += ((pMain->ProjectData()->GetBuildSpace().x() - buildsizex)/2);
    buildsizey += ((pMain->ProjectData()->GetBuildSpace().y() - buildsizey)/2);
    buildsizez += ((pMain->ProjectData()->GetBuildSpace().z() - buildsizez)/2);

    anyFenceOn = (fencesOn[0] || fencesOn[1] || fencesOn[2] || fencesOn[3]);
    if(anyFenceOn)
        fenceAlpha += 0.01f;
    else
        fenceAlpha = 0;

    if(fenceAlpha >= 0.3)
        fenceAlpha = 0.3f;

    if(xRot < 200 && xRot >= 180)
    {
        supportAlpha = ((xRot-180)/20.0);
        if(supportAlpha < 0.1)
            supportAlpha = 0.1f;
    }
    else
        supportAlpha = 1.0;

    UpdatePlasmaFence();


    //总是正常化各个角度0-360
    //qNormalizeAngle(xRotTarget);
    qNormalizeAngle(yRotTarget);
    qNormalizeAngle(zRotTarget);
    qNormalizeAngle(xRot);
    qNormalizeAngle(yRot);
    qNormalizeAngle(zRot);


    if((zRotTarget - zRot) < -180)
        zRotTarget = zRotTarget + 360;

    if((zRot - zRotTarget) < -180)
        zRot = zRot + 360;

    xRot = xRot + ((xRotTarget - xRot)/5.0);
    yRot = yRot + ((yRotTarget - yRot)/5.0);
    zRot = zRot + ((zRotTarget - zRot)/5.0);


    camdist = camdist + (camdistTarget - camdist)/5.0;

    pan = pan + (panTarget - pan)/2.0;

    revolvePoint = revolvePoint + (revolvePointTarget - revolvePoint)/2.0;


    //更新可视切片
    QTimer::singleShot(0,this,SLOT(UpdateVisSlice()));





    glDraw();//实际的绘制（连同翻转内部缓冲区）
}
void WorldView::SetTool(QString tool)
{
	currtool = tool;
}
QString WorldView::GetTool()
{
     return currtool;
}

void WorldView::ExitToolAction()
{
    dragdown = false;
    shiftdown = false;
    controldown = false;
}


////////////////////////////////////////////////////////
//Public Gets
//////////////////////////////////////////////////////////////


QVector3D WorldView::GetPan()
{
    return panTarget;
}

float WorldView::GetZoom()
{
    return camdistTarget;
}
QVector3D WorldView::GetRotation()
{
    return QVector3D(xRotTarget,yRotTarget,zRotTarget);
}




//Private
void WorldView::initializeGL()
{

    qglClearColor(QColor(0,100,100));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);
    glLineWidth(0.5);
    glEnable(GL_LINE_SMOOTH);

    glEnable ( GL_COLOR_MATERIAL );
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);


    glDisable(GL_MULTISAMPLE);
    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    static GLfloat lightPosition0[4] = { 0.0, 0.0, 100.0, 1.0 };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition0);


}

void WorldView::paintGL()
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ResetCamera(false);


    if(pMain->XRayEnabled())
    {
        DrawBuildArea();
        ResetCamera(true);
    }

    DrawMisc();

    DrawInstances();

    //绘制切片
    DrawVisualSlice();

    if(!pMain->XRayEnabled())
        DrawBuildArea();
}

void WorldView::resizeGL(int width, int height)
{
	 glViewport(0,0,width,height); 
}


void WorldView::ResetCamera(bool xrayon)
{
    double nearClip;
    //perpective/ortho/xray stuff!
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();//恢复默认矩阵。
    if(perspective)
    {
        if(pMain->XRayEnabled() && pMain->SupportModeInst() && xrayon)
        {
            nearClip = camdist -
            (pMain->SupportModeInst()->GetMaxBound() - pMain->SupportModeInst()->GetMinBound()).length()*(1-pMain->XRayPercentage())
            +(pMain->SupportModeInst()->GetMaxBound() - pMain->SupportModeInst()->GetMinBound()).length()*0.5;
        }
        else
            nearClip = 1;

        gluPerspective(30,double(width())/height(),nearClip,5500);
    }
    else
    {
        float aspRatio = float(this->width())/this->height();
        if(pMain->XRayEnabled() && pMain->SupportModeInst() && xrayon)
        {
            nearClip = 200-
            (pMain->SupportModeInst()->GetMaxBound() - pMain->SupportModeInst()->GetMinBound()).length()*(1-pMain->XRayPercentage())
            +(pMain->SupportModeInst()->GetMaxBound() - pMain->SupportModeInst()->GetMinBound()).length()*0.5;
        }
        else
            nearClip = 0.1;

        glOrtho(-(camdist/120.0)*aspRatio*102.4/2,
                (camdist/120.0)*aspRatio*102.4/2,
                -(camdist/120.0)*102.4/2,
                (camdist/120.0)*102.4/2,
                nearClip,
                5500);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if(perspective)
        glTranslatef(0.0, 0.0, -camdist);//退一步量
    else
        glTranslatef(0.0, 0.0, -200.0);//退一步量

    glTranslatef(pan.x(),-pan.y(),0);

    glRotatef(xRot, 1.0, 0.0, 0.0);
    glRotatef(yRot, 0.0, 1.0, 0.0);
    glRotatef(zRot, 0.0, 0.0, 1.0);
    glTranslatef(-revolvePoint.x(),-revolvePoint.y(),-revolvePoint.z());


}

void WorldView::DrawMisc()
{
    //显示光标位置
    if(currtool == "SUPPORTADD")
    {
        glDisable(GL_LIGHTING);
        glPushMatrix();
        if(cursorNormal3D.z() <= 0)
        glColor3f(0.0f,1.0f,0.0f);
        else
        glColor3f(1.0f,0.0f,0.0f);
            glBegin(GL_LINES);
                glVertex3f( cursorPos3D.x(), cursorPos3D.y(), cursorPos3D.z());
                glVertex3f( cursorPos3D.x() + cursorNormal3D.x(), cursorPos3D.y() + cursorNormal3D.y(), cursorPos3D.z() + cursorNormal3D.z());
            glEnd();
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }
}


void WorldView::DrawInstances()
{
    unsigned int i;
    B9ModelInstance* pInst;

	glEnable(GL_DEPTH_TEST);

    //如果是支撑模态下，我们只想渲染支撑模态实例
    if(pMain->SupportModeInst() != NULL)
    {
        glColor3f(0.3f,0.3f,0.3f);
        glDisable(GL_CULL_FACE);
        pMain->SupportModeInst()->RenderGL(true);
        glEnable(GL_CULL_FACE);


        glEnable(GL_BLEND);
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        if(hideNonActiveSupports && pMain->HidingSupports()){
            glDisable(GL_LIGHTING);
            glColor3f(0.0f,1.0f,0.0f);
            pMain->SupportModeInst()->RenderSupportsTipsGL();
            glEnable(GL_LIGHTING);
            pMain->SupportModeInst()->renderSupportGL(pMain->GetSelectedSupports()->at(0),false,1.0,supportAlpha);
        }
        else
            pMain->SupportModeInst()->RenderSupportsGL(false,1.0,supportAlpha);
        glEnable(GL_CULL_FACE);
        glPopAttrib();
        glDisable(GL_BLEND);

    }
    else
    {
        for(i = 0; i < pMain->GetAllInstances().size();i++)
        {
            pInst = pMain->GetAllInstances()[i];
            pInst->RenderGL();
            glColor3f(pInst->visualcolor.redF()*0.5,
                    pInst->visualcolor.greenF()*0.5,
                    pInst->visualcolor.blueF()*0.5);

            pInst->RenderSupportsGL(true,1.0,1.0);

        }
    }

}

void  WorldView::DrawVisualSlice()
{
    if(pVisSlice == NULL)
        return;

    if(supportAlpha < 0.5)
        glDisable(GL_DEPTH_TEST);

    glDisable(GL_LIGHTING);

    glPushMatrix();
    glTranslatef(0.0,0.0,pVisSlice->realAltitude);
    glColor3f(0.5,0.5,0.5);
    pVisSlice->RenderOutlines();


    glPopMatrix();

    if(supportAlpha < 0.5)
        glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void WorldView::DrawBuildArea()
{
	glPushMatrix();
    glColor3f(0.5f,0.5f,0.5f);
	glTranslatef(-buildsizex/2, -buildsizey/2, 0);

    glDisable(GL_LIGHTING);
		//4 vertical lines
		glBegin(GL_LINES); 
			glVertex3d( 0, 0, 0);
			glVertex3d( 0, 0, buildsizez); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( buildsizex, 0,0);
			glVertex3d( buildsizex, 0,buildsizez); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( 0, buildsizey,0);
			glVertex3d( 0, buildsizey,buildsizez); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( buildsizex, buildsizey,0);
			glVertex3d( buildsizex, buildsizey,buildsizez); 
		glEnd();

		//4 Top lines
		glBegin(GL_LINES); 
			glVertex3d( 0, 0, buildsizez);
			glVertex3d( buildsizex, 0, buildsizez); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( 0, 0, buildsizez);
			glVertex3d( 0, buildsizey, buildsizez); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( buildsizex, buildsizey, buildsizez);
			glVertex3d( 0, buildsizey, buildsizez); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( buildsizex, buildsizey, buildsizez);
			glVertex3d( buildsizex, 0, buildsizez); 
		glEnd();
        //4底线
		glBegin(GL_LINES); 
			glVertex3d( 0, 0, 0);
			glVertex3d( buildsizex, 0,0); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( 0, 0, 0);
			glVertex3d( 0, buildsizey, 0); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( buildsizex, buildsizey, 0);
			glVertex3d( 0, buildsizey, 0); 
		glEnd();
		glBegin(GL_LINES); 
			glVertex3d( buildsizex, buildsizey, 0);
			glVertex3d( buildsizex, 0, 0); 
		glEnd();
		
		
        //绘制红色坐标以提示
        glColor3f(1.0f,0.0f,0.0f);
		glBegin(GL_LINES); 
                glVertex3d( 0, 0, buildsizez);
				glVertex3d( 0, 0, buildsizez + 10); 
		glEnd();
		glBegin(GL_LINES); 
                glVertex3d( buildsizex, 0, 0);
				glVertex3d( buildsizex + 10, 0, 0); 
		glEnd();
		glBegin(GL_LINES); 
                glVertex3d( 0, buildsizey, 0);
				glVertex3d( 0, buildsizey + 10, 0); 
		glEnd();
        glColor3f(1.0f,1.0f,1.0f);


        glEnable(GL_LIGHTING);
        glEnable(GL_BLEND);

        //顶部的矩形
        if(fencesOn[4] && !pMain->SupportModeInst())
            glColor4f(1.0f,0.0f,0.0f,0.5f);
        else
            glColor4f(0.0f,0.0f,0.7f,1.0f);

		glNormal3f(0,0,1);
			glBegin(GL_TRIANGLES);                    
                glVertex3f( buildsizex, 0, 0);
				glVertex3f( buildsizex, buildsizey, 0);     
				glVertex3f( 0, buildsizey, 0);     
			glEnd();
		
			glBegin(GL_TRIANGLES);                    
				glVertex3f( buildsizex, 0, 0);     
				glVertex3f( 0, buildsizey, 0);     
				glVertex3f( 0, 0, 0);     
			glEnd();

		
        //底部的矩形

        if(!pMain->SupportModeInst())
        {

            glNormal3f(0,0,-1);
            if(fencesOn[4])
                glColor4f(1.0f,0.0f,0.0f,0.5f);
            else
                glColor4f(0.0f,0.0f,1.0f,0.3f);

                glBegin(GL_TRIANGLES);//-0.1 is to hide z-fighting
                    glVertex3f( 0, buildsizey, -0.1f);
                    glVertex3f( buildsizex, buildsizey, -0.1f);
                    glVertex3f( buildsizex, 0, -0.1f);
                glEnd();

                glBegin(GL_TRIANGLES);
                    glVertex3f( 0, 0, -0.1f);
                    glVertex3f( 0, buildsizey, -0.1f);
                    glVertex3f( buildsizex, 0, -0.1f);
                glEnd();
            glColor4f(1.0f,1.0f,1.0f,1.0f);

        }

        //绘制等离子栅栏！
        if(!pMain->SupportModeInst())
        {
            glColor4f(fenceAlpha*2,0.0f,0.0f,fenceAlpha);
            glDisable(GL_CULL_FACE);
            glDisable(GL_LIGHTING);

            //X射线+ 光栅
            if(fencesOn[0])
            {
                glNormal3f(1,0,0);
                glBegin(GL_QUADS);
                    glVertex3f( buildsizex, 0, 0);
                    glVertex3f( buildsizex, buildsizey, 0);
                    glVertex3f( buildsizex, buildsizey, buildsizez);
                    glVertex3f( buildsizex, 0, buildsizez);
                glEnd();
            }
            //X- 光栅
            if(fencesOn[1])
            {
                glNormal3f(1,0,0);
                glBegin(GL_QUADS);
                    glVertex3f( 0, 0, 0);
                    glVertex3f( 0, buildsizey, 0);
                    glVertex3f( 0, buildsizey, buildsizez);
                    glVertex3f( 0, 0, buildsizez);
                glEnd();
            }
            //Y+
            if(fencesOn[2])
            {
                glNormal3f(1,0,0);
                glBegin(GL_QUADS);
                    glVertex3f( 0, buildsizey, 0);
                    glVertex3f( buildsizex, buildsizey, 0);
                    glVertex3f( buildsizex, buildsizey, buildsizez);
                    glVertex3f( 0, buildsizey, buildsizez);
                glEnd();
            }
            //Y-
            if(fencesOn[3])
            {
                glNormal3f(1,0,0);
                glBegin(GL_QUADS);
                    glVertex3f( 0, 0, 0);
                    glVertex3f( buildsizex, 0, 0);
                    glVertex3f( buildsizex, 0, buildsizez);
                    glVertex3f( 0, 0, buildsizez);
                glEnd();
            }

            glEnable(GL_LIGHTING);
            glEnable(GL_CULL_FACE);
        }
        glDisable(GL_BLEND);
	glPopMatrix();
 }


 void WorldView::UpdateSelectedBounds()
 {
     unsigned int i;
     pMain->setCursor(Qt::WaitCursor);

	 for(i=0;i<pMain->GetSelectedInstances().size();i++)
	 {
		 pMain->GetSelectedInstances()[i]->UpdateBounds();
	 }
     pMain->setCursor(Qt::ArrowCursor);
 }

 //检查对着全部光栅的所有实例。
 void WorldView::UpdatePlasmaFence()
 {
     unsigned int i;
     B9ModelInstance* inst;

     fencesOn[0] = false;
     fencesOn[1] = false;
     fencesOn[2] = false;
     fencesOn[3] = false;
     fencesOn[4] = false;

     for(i = 0; i < pMain->GetAllInstances().size(); i++)
     {
         inst = pMain->GetAllInstances()[i];
         if(inst->GetMaxBound().x() > pMain->ProjectData()->GetBuildSpace().x()*0.5)
             fencesOn[0] = true;
         if(inst->GetMinBound().x() < -pMain->ProjectData()->GetBuildSpace().x()*0.5)
             fencesOn[1] = true;
         if(inst->GetMaxBound().y() > pMain->ProjectData()->GetBuildSpace().y()*0.5)
             fencesOn[2] = true;
         if(inst->GetMinBound().y() < -pMain->ProjectData()->GetBuildSpace().y()*0.5)
             fencesOn[3] = true;
         //接下来要生成表
         if(inst->GetMinBound().z() < -0.01)
             fencesOn[4] = true;
     }
 }



 //鼠标交互
void WorldView::mousePressEvent(QMouseEvent *event)
{
     mouseLastPos = event->pos();
     mouseDownInitialPos = event->pos();

	 if(event->button() == Qt::MiddleButton)
	 {
		pandown = true;
	 }
	 if(event->button() == Qt::LeftButton)
	 {
		 dragdown = true;
         OnToolInitialAction(currtool, event);
     }
}

void WorldView::mouseMoveEvent(QMouseEvent *event)
{
    mouseDeltaPos.setX(event->x() - mouseLastPos.x());
    mouseDeltaPos.setY(event->y() - mouseLastPos.y());
    mouseLastPos = event->pos();

    if((event->buttons() & Qt::RightButton) && !shiftdown) {
        setXRotation(xRotTarget + 0.5 * mouseDeltaPos.y());
        setZRotation(zRotTarget + 0.5 * mouseDeltaPos.x());
        //我们旋转视图，以便切换视图模式！
        currViewAngle = "FREE";
    }

    if(pandown || (event->buttons() & Qt::RightButton && shiftdown))
    {
        panTarget += QVector3D(mouseDeltaPos.x()/20.0,mouseDeltaPos.y()/20.0,0);
    }

    if(dragdown && (event->buttons() & Qt::LeftButton))
    {
        OnToolDragAction(currtool,event);
    }

    if(!dragdown && !pandown && !shiftdown
            && !(event->buttons() & Qt::LeftButton) && !(event->buttons() & Qt::RightButton))
        OnToolHoverMove(currtool,event);


}
void WorldView::mouseReleaseEvent(QMouseEvent *event)
{

	if(event->button() == Qt::MiddleButton)
	{
		pandown = false;
	}
	if(event->button() == Qt::LeftButton)
	{
        dragdown = false;
        OnToolReleaseAction(currtool, event);
	}
}


void WorldView::wheelEvent(QWheelEvent *event)
{
    camdistTarget -= event->delta()/8.0;
    //变焦范围
    if(perspective)
    {
        if(camdistTarget <= 10.0)
        {
            camdistTarget = 10.0;
        }
        if(camdistTarget >= 350.00)
            camdistTarget = 350.00;
    }
    else
    {
        if(camdistTarget <= 5.0)
        {
            camdistTarget = 5.0;
        }
        if(camdistTarget >= 350.00)
            camdistTarget = 350.00;


    }
}

 //键盘交互
void WorldView::keyPressEvent( QKeyEvent * event )
{
	QWidget::keyPressEvent(event);
    if(event->key() == Qt::Key_Shift)
    {
        shiftdown = true;
    }
    if(event->key() == Qt::Key_Control)
    {
        controldown = true;
    }
}
void WorldView::keyReleaseEvent( QKeyEvent * event )
{
    if(event->key() == Qt::Key_Shift)
    {
        shiftdown = false;
    }
    if(event->key() == Qt::Key_Control)
    {
        controldown = false;
    }
}

void WorldView::OnToolInitialAction(QString tool, QMouseEvent* event)
{
    B9ModelInstance* clickedInst = NULL;
    Triangle3D* pTopTri = NULL;
    Triangle3D* pBottomTri = NULL;
    B9SupportStructure* addedSupport;
    B9SupportStructure* pSup;
    bool hitGround;



    if(currtool == "MODELSELECT" ||
            currtool == "MODELMOVE" ||
            currtool == "MODELSPIN" ||
            currtool == "MODELSCALE" ||
            currtool == "MODELORIENTATE")
    {
        Update3DCursor(event->pos());

        pMain->DeSelectAll();
        clickedInst = SelectInstanceByScreen(event->pos());

        if(clickedInst)
        {
            PreDragInstanceOffset = cursorPos3D - clickedInst->GetPos();

            float theta = qAtan2((cursorPos3D.y() - clickedInst->GetPos().y())
                               ,
                               (cursorPos3D.x() - clickedInst->GetPos().x()));

            theta = theta*(180/M_PI);

            PreDragRotationOffsetData.setX(theta);
            PreDragRotationOffsetData.setY(clickedInst->GetRot().z());
        }
    }

    if(currtool == "SUPPORTADD")//用户点击添加支撑按钮后。
    {

        bool s;
        unsigned int tri;

        if(!pMain->SupportModeInst())
            return;

        tri = GetPickTriangleIndx(pMain->SupportModeInst(),event->pos(),s);

        if(s)
        {

            QVector3D topPos = GetPickOnTriangle(event->pos(),pMain->SupportModeInst(),tri);
            pTopTri = pMain->SupportModeInst()->triList[tri];

            if(pTopTri->normal.z() <= 0)
            {
                addedSupport = pMain->SupportModeInst()->AddSupport(QVector3D(),QVector3D());
                pMain->FillSupportList();

                addedSupport->SetTopPoint(topPos);
                addedSupport->SetTopNormal(pTopTri->normal*-1.0);

                //“钻”从上顶点的位置下来，看看那里的垂直线命中。
                QVector3D basePos = GetDrillingHit(addedSupport->GetTopPivot(),pMain->SupportModeInst(), hitGround, pBottomTri);
                if(hitGround)
                {
                    addedSupport->SetIsGrounded(true);
                }
                else
                {
                    addedSupport->SetBottomPoint(basePos);
                    addedSupport->SetBottomNormal(pBottomTri->normal*-1.0);
                    addedSupport->SetIsGrounded(false);
                }
                //现在支撑已经创建好了 - 但我们希望在用户设置加载默认的形状和半径
                QSettings appSettings;
                appSettings.beginGroup("USERSUPPORTPARAMS");
                appSettings.beginGroup("SUPPORT_TOP");
                    addedSupport->SetTopAttachShape(appSettings.value("ATTACHSHAPE",addedSupport->GetTopAttachShape()).toString());
                    addedSupport->SetTopLength(appSettings.value("LENGTH",addedSupport->GetTopLength()).toDouble());
                    addedSupport->SetTopPenetration(appSettings.value("PENETRATION",addedSupport->GetTopPenetration()).toDouble());
                    addedSupport->SetTopRadius(appSettings.value("RADIUS",addedSupport->GetTopRadius()).toDouble());
                    addedSupport->SetTopAngleFactor(appSettings.value("ANGLEFACTOR",addedSupport->GetTopAngleFactor()).toDouble());
                appSettings.endGroup();
                appSettings.beginGroup("SUPPORT_MID");
                    addedSupport->SetMidAttachShape(appSettings.value("ATTACHSHAPE",addedSupport->GetMidAttachShape()).toString());
                    addedSupport->SetMidRadius(appSettings.value("RADIUS",addedSupport->GetMidLength()).toDouble());
                appSettings.endGroup();


                if(addedSupport->GetIsGrounded())
                {
                    appSettings.beginGroup("SUPPORT_BOTTOM_GROUNDED");
                    addedSupport->SetBottomAttachShape(appSettings.value("ATTACHSHAPE",addedSupport->GetTopAttachShape()).toString());
                        addedSupport->SetBottomLength(appSettings.value("LENGTH",addedSupport->GetTopLength()).toDouble());
                        addedSupport->SetBottomPenetration(appSettings.value("PENETRATION",addedSupport->GetTopPenetration()).toDouble());
                        addedSupport->SetBottomRadius(appSettings.value("RADIUS",addedSupport->GetTopRadius()).toDouble());
                        addedSupport->SetBottomAngleFactor(appSettings.value("ANGLEFACTOR",addedSupport->GetTopAngleFactor()).toDouble());
                    appSettings.endGroup();
                    addedSupport->SetBottomPoint(addedSupport->GetTopPivot());
                }
                else
                {
                    appSettings.beginGroup("SUPPORT_BOTTOM_NONGROUNDED");
                        addedSupport->SetBottomAttachShape(appSettings.value("ATTACHSHAPE",addedSupport->GetTopAttachShape()).toString());
                        addedSupport->SetBottomLength(appSettings.value("LENGTH",addedSupport->GetTopLength()).toDouble());
                        addedSupport->SetBottomPenetration(appSettings.value("PENETRATION",addedSupport->GetTopPenetration()).toDouble());
                        //addedSupport->SetBottomRadius(appSettings.value("RADIUS",addedSupport->GetTopRadius()).toDouble());
                        addedSupport->SetBottomAngleFactor(appSettings.value("ANGLEFACTOR",addedSupport->GetTopAngleFactor()).toDouble());
                    appSettings.endGroup();
                    //new support grounded on instance take que from for radius......
                    appSettings.beginGroup("SUPPORT_TOP");
                    addedSupport->SetBottomRadius(appSettings.value("RADIUS",addedSupport->GetBottomRadius()).toDouble());
                    appSettings.endGroup();
                }
                appSettings.endGroup();
            }
        }
    }

    if(currtool == "SUPPORTMODIFY")
    {
        if(!shiftdown && !controldown)
            pMain->DeSelectAllSupports();

        pSup = GetSupportByScreen(event->pos());
        if(pSup != NULL)
        {

            pMain->SelectSupport(pSup);
            //同时确认我们点击了哪些部分 ("TOP", "MID", "BOTTOM")
            toolStringMemory = GetSupportSectionByScreen(event->pos(),pSup);

            toolVectorOffsetMemory = QVector3D();
            if(toolStringMemory == "BOTTOM" && pSup->GetIsGrounded())
            {   Update3DCursor(event->pos());//update against floor.
                toolVectorOffsetMemory = cursorPos3D - pMain->SupportModeInst()->GetPos() - pSup->GetBottomPoint();
            }

            toolSupportMemory.CopyFrom(pSup);//记录支撑设定。

            pMain->UpdateSupportList();
        }
    }

    if(currtool == "SUPPORTDELETE")
    {
        pMain->DeSelectAllSupports();

        pSup = GetSupportByScreen(event->pos());
        if(pSup != NULL)
        {
            pMain->DeleteSupport(pSup);
        }
    }
}

void WorldView::OnToolDragAction(QString tool, QMouseEvent* event)
{
    unsigned int i;
    B9ModelInstance* pInst;
    B9SupportStructure* pSup;
    QSettings appSettings;


    if(currtool == "MODELMOVE")
    {
        Update3DCursor(event->pos());

        for(i = 0; i < pMain->GetSelectedInstances().size(); i++)
        {
            if(shiftdown)
            {
                pMain->GetSelectedInstances()[i]->Move(QVector3D(0,0,-mouseDeltaPos.y()*0.1));
            }
            else
            {
                if(currViewAngle == "FREE" || currViewAngle == "TOP" || currViewAngle == "BOTTOM")
                {
                    pMain->GetSelectedInstances()[i]->SetX(cursorPos3D.x() - PreDragInstanceOffset.x());
                    pMain->GetSelectedInstances()[i]->SetY(cursorPos3D.y() - PreDragInstanceOffset.y());
                }
                else if(currViewAngle == "FRONT" || currViewAngle == "BACK")
                {
                    pMain->GetSelectedInstances()[i]->SetX(cursorPos3D.x() - PreDragInstanceOffset.x());
                    pMain->GetSelectedInstances()[i]->SetZ(cursorPos3D.z() - PreDragInstanceOffset.z());
                }
                else if(currViewAngle == "RIGHT" || currViewAngle == "LEFT")
                {
                    pMain->GetSelectedInstances()[i]->SetY(cursorPos3D.y() - PreDragInstanceOffset.y());
                    pMain->GetSelectedInstances()[i]->SetZ(cursorPos3D.z() - PreDragInstanceOffset.z());
                }
            }
            pMain->UpdateInterface();
        }

    }
    if(currtool == "MODELSPIN")
    {
        Update3DCursor(event->pos());
            for(unsigned int i = 0; i < pMain->GetSelectedInstances().size(); i++)
            {
                pInst = pMain->GetSelectedInstances()[i];

                if(currViewAngle == "FREE" || currViewAngle == "TOP")
                {
                    float theta = qAtan2((cursorPos3D.y() - pInst->GetPos().y())
                                         ,
                                         (cursorPos3D.x()-pInst->GetPos().x()));
                    theta = theta*(180/M_PI);
                    pInst->SetRot(QVector3D(pInst->GetRot().x(),pInst->GetRot().y(),
                                            PreDragRotationOffsetData.y() + (theta - PreDragRotationOffsetData.x())));
                }
                else
                    pInst->Rotate(QVector3D(0,0,mouseDeltaPos.x()*0.2));
            }
            pMain->UpdateInterface();


    }
    if(currtool == "MODELORIENTATE")
    {

            for(unsigned int i = 0; i < pMain->GetSelectedInstances().size(); i++)
            {
                pInst = pMain->GetSelectedInstances()[i];
                pInst->Rotate(QVector3D(mouseDeltaPos.y()*0.2,mouseDeltaPos.x()*0.2,0));
            }
            pMain->UpdateInterface();


    }
    if(currtool == "MODELSCALE")
    {
        for(unsigned int i = 0; i<pMain->GetSelectedInstances().size(); i++)
        {
            //pMain->GetSelectedInstances()[i]->Scale(QVector3D(dx*0.01,dx*0.01,dx*0.01));
            QVector3D cScale = pMain->GetSelectedInstances()[i]->GetScale();

            pMain->GetSelectedInstances()[i]->SetScale(cScale*(1+mouseDeltaPos.x()/50.0));
        }
        pMain->UpdateInterface();

    }
    if(currtool == "SUPPORTMODIFY")
    {
        pInst = pMain->SupportModeInst();
        if(!pInst)
            return;

        if(!pMain->GetSelectedSupports()->size())
            return;

        if(pMain->GetSelectedSupports()->size() > 1)
            pMain->SelectOnly(pMain->GetSelectedSupports()->at(pMain->GetSelectedSupports()->size()-1));


        bool isAgainstInst;
        hideNonActiveSupports = true;

        pSup = pMain->GetSelectedSupports()->at(0);

        //检查我们是否需要渲染警告
        if(!pSup->IsPrintable())
            pSup->EnableErrorGlow();
        else
            pSup->DisableErrorGlow();

        if(toolStringMemory == "BOTTOM" )
        {
            if(controldown)
            {
                pSup->SetBottomLength(pSup->GetBottomLength() - mouseDeltaPos.y()/30.0);
                if(pSup->GetBottomLength() <= 0.2)
                    pSup->SetBottomLength(0.2);
            }
            else if(shiftdown)
            {
                //鼠标基于底部法线旋转。
                if(!toolSupportMemory.GetIsGrounded())
                {
                    B9ModelInstance* pInst = pMain->SupportModeInst();
                    QVector3D instPos = pInst->GetPos();
                    QVector3D normCur;
                    QVector3D delta = pSup->GetBottomPoint() - (Get3DCursorOnScreen(event->pos(),pSup->GetBottomPoint()+instPos,2.0)
                                                                - instPos);
                    delta.normalize();
                    normCur = delta;
                    pSup->SetBottomNormal(normCur);
                }
            }
            else
            {
                Update3DCursor(event->pos(),pInst,isAgainstInst);
                if(isAgainstInst)
                    pSup->SetBottomPoint(cursorPos3D - pInst->GetPos());//返回局部坐标
                else
                {
                     pSup->SetBottomPoint(cursorPos3D - pInst->GetPos() - toolVectorOffsetMemory);//移动到地板上
                     //以一定偏移量
                }
                pSup->SetBottomNormal(-cursorNormal3D);
                pSup->SetBottomAngleFactor(1.0);

                if(isAgainstInst)
                {

                    pSup->SetIsGrounded(false);
                    if(toolSupportMemory.GetIsGrounded())//追踪支撑是否着地
                    {
                        appSettings.beginGroup("USERSUPPORTPARAMS");
                        appSettings.beginGroup("SUPPORT_BOTTOM_NONGROUNDED");
                        pSup->SetBottomAttachShape(appSettings.value("ATTACHSHAPE").toString());
                        pSup->SetBottomRadius(appSettings.value("RADIUS").toDouble());
                        pSup->SetBottomLength(appSettings.value("LENGTH").toDouble());
                        pSup->SetBottomPenetration(appSettings.value("PENETRATION").toDouble());
                        appSettings.endGroup();
                        appSettings.endGroup();
                    }
                    else
                    {
                        pSup->SetBottomAttachShape(toolSupportMemory.GetBottomAttachShape());
                        pSup->SetBottomRadius(toolSupportMemory.GetBottomRadius());
                        pSup->SetBottomLength(toolSupportMemory.GetBottomLength());
                        pSup->SetBottomPenetration(toolSupportMemory.GetBottomPenetration());
                    }

                }
                else
                {
                    pSup->SetIsGrounded(true);
                    if(!toolSupportMemory.GetIsGrounded())//追踪支撑是否着地
                    {
                        appSettings.beginGroup("USERSUPPORTPARAMS");
                        appSettings.beginGroup("SUPPORT_BOTTOM_GROUNDED");
                        pSup->SetBottomAttachShape(appSettings.value("ATTACHSHAPE").toString());
                        pSup->SetBottomRadius(appSettings.value("RADIUS").toDouble());
                        pSup->SetBottomLength(appSettings.value("LENGTH").toDouble());
                        pSup->SetBottomPenetration(appSettings.value("PENETRATION").toDouble());
                        appSettings.endGroup();
                        appSettings.endGroup();

                    }
                    else
                    {
                        pSup->SetBottomAttachShape(toolSupportMemory.GetBottomAttachShape());
                        pSup->SetBottomRadius(toolSupportMemory.GetBottomRadius());
                        pSup->SetBottomLength(toolSupportMemory.GetBottomLength());
                        pSup->SetBottomPenetration(toolSupportMemory.GetBottomPenetration());
                    }
                }
            }
        }
        else if(toolStringMemory == "TOP" || toolStringMemory == "MID")
        {
            if(controldown && toolStringMemory == "TOP")//顶部的长度比例
            {
                pSup->SetTopLength(pSup->GetTopLength() + mouseDeltaPos.y()/30.0);
                if(pSup->GetTopLength() <= 0.2)
                    pSup->SetTopLength(0.2);
            }
            else if(controldown && toolStringMemory == "MID")//半径比例
            {
                double rt = pSup->GetTopRadius() + mouseDeltaPos.x()/80.0;
                if(rt <= 0.05)
                    rt = 0.05;
                pSup->SetTopRadius(rt);

                double rm = pSup->GetMidRadius() + mouseDeltaPos.x()/80.0;
                if(rm <= 0.05)
                    rm = 0.05;
                pSup->SetMidRadius(rm);

                if(!pSup->GetIsGrounded())
                {
                    double rb = pSup->GetBottomRadius() + mouseDeltaPos.x()/80.0;
                    if(rb <= 0.05)
                        rb = 0.05;
                    pSup->SetBottomRadius(rb);
                }

            }
            else if(shiftdown)
            {
                //鼠标基于顶部正转。
                B9ModelInstance* pInst = pMain->SupportModeInst();
                QVector3D instPos = pInst->GetPos();
                QVector3D normCur;
                QVector3D delta = pSup->GetTopPoint() - (Get3DCursorOnScreen(event->pos(),pSup->GetTopPoint()+instPos,2.0)
                                                            - instPos);
                delta.normalize();
                normCur = delta;
                pSup->SetTopNormal(normCur);

            }
            else
            {
                Update3DCursor(event->pos(),pInst,isAgainstInst);
                pSup->SetTopPoint(cursorPos3D - pInst->GetPos());//返回局部坐标
                pSup->SetTopNormal(-cursorNormal3D);
                pSup->SetTopAngleFactor(1.0);

                //如果我们开始了垂直平移，移动底座连同顶部
                if(toolSupportMemory.IsVertical() && toolSupportMemory.GetIsGrounded())
                {
                    pSup->SetBottomPoint(QVector3D(pSup->GetTopPivot().x(),
                                                   pSup->GetTopPivot().y(),
                                                   pSup->GetBottomPoint().z()));

                }
            }
        }
    }
}

void WorldView::OnToolReleaseAction(QString tool, QMouseEvent* event)
{
    B9SupportStructure* pSup;


    hideNonActiveSupports = false;
    pMain->UpdateInterface();
    if(currtool == "MODELSPIN" || currtool == "MODELORIENTATE" || currtool == "MODELSCALE")
    {
        UpdateSelectedBounds();
    }
    if(currtool == "MODELMOVE")
    {

    }
    if(currtool == "SUPPORTMODIFY")
    {
        if(pMain->GetSelectedSupports()->size() == 1)
        {
            pSup = pMain->GetSelectedSupports()->at(0);

            if(!pSup->IsPrintable())//如果我们在错误的地方建立了支撑
            {

               pSup->CopyFrom(&toolSupportMemory);//还原
            }
            else
            {

            }
            pSup->DisableErrorGlow();
        }
    }
}

void WorldView::OnToolHoverMove(QString tool, QMouseEvent* event)
{
    B9ModelInstance* pInst;
    if(tool == "SUPPORTADD" || tool == "SUPPORTMODIFY")
    {
       if(!pMain->SupportModeInst())
            return;
       pInst = pMain->SupportModeInst();
       Update3DCursor(event->pos(),pInst);

    }
}



////////////////////////////////////////////////////////////////////
//Private Functions
////////////////////////////////////////////////////////////////////


B9ModelInstance* WorldView::SelectInstanceByScreen(QPoint pos)
{
   unsigned int i;
   unsigned char pixel[3];
   //clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glDisable(GL_LIGHTING);

   for(i = 0; i < pMain->GetAllInstances().size();i++)
   {
       pMain->GetAllInstances()[i]->RenderPickGL();
   }

   glReadPixels(pos.x(), this->height() - pos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_LIGHTING);

   //现在我们有颜色，对每一个实例进行比较
   for(i = 0; i < pMain->GetAllInstances().size();i++)
   {
       B9ModelInstance* inst = pMain->GetAllInstances()[i];

       if((pixel[0] == inst->pickcolor[0]) && (pixel[1] == inst->pickcolor[1]) && (pixel[2] == inst->pickcolor[2]))
       {

           pMain->Select(inst);
           return inst;
       }
   }

   return NULL;
}

B9SupportStructure* WorldView::GetSupportByScreen(QPoint pos)
{
    unsigned int i;
    unsigned char pixel[3];
    B9ModelInstance* pInst;
    B9SupportStructure* pSup;

    pInst = pMain->SupportModeInst();
    if(pInst == NULL)
        return NULL;


    //清除屏幕
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);

    //我们需要渲染模型过于所以它能够关闭隐藏支撑
    pInst->RenderGL(true);

    for(i = 0; i < pInst->GetSupports().size(); i++)
    {
        pSup = pInst->GetSupports()[i];
        pSup->RenderPickGL(supportAlpha > 0.99 || !pSup->GetIsGrounded(),true);
    }

    glReadPixels(pos.x(), this->height() - pos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_LIGHTING);

    //现在我们有颜色，对每一个实例进行比较
    for(i = 0; i < pInst->GetSupports().size();i++)
    {
        pSup = pInst->GetSupports()[i];

        if((pixel[0] == pSup->pickcolor[0]) && (pixel[1] == pSup->pickcolor[1]) && (pixel[2] == pSup->pickcolor[2]))
        {
            return pSup;
        }
    }
    return NULL;
}

QString WorldView::GetSupportSectionByScreen(QPoint pos, B9SupportStructure* sup)
{
    unsigned char pixel[3];

    //清除屏幕
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);

    sup->RenderPartPickGL(supportAlpha > 0.99 || !sup->GetIsGrounded(),true);//renders all 3 sections in different colors.

    glReadPixels(pos.x(), this->height() - pos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_LIGHTING);

    if(pixel[0] > 200)
        return "TOP";

    if(pixel[1] > 200)
        return "MID";

    if(pixel[2] > 200)
        return "BOTTOM";

    //else
    return "";
}

//返回鼠标位置的全局坐标系中，平面围绕提示。
//从外部看是否有许多平移平面朝向观察者。
QVector3D WorldView::Get3DCursorOnScreen(QPoint screenPos, QVector3D hintPos, double standoff)
{

    unsigned char pixel[3];//拾色器像素
    QVector3D firstPassPos;
    QVector3D finalPos;

    //清屏，并设置背景纯绿色。
    qglClearColor(QColor(0,255,0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);//不剔除，所以我们可以从后面跟踪
    glPushMatrix();


    //电脑支架容器
    QVector3D standoffv(0,0,1);
    RotateVector(standoffv,-xRot,QVector3D(1,0,0));
    RotateVector(standoffv,-yRot,QVector3D(0,1,0));
    RotateVector(standoffv,-zRot,QVector3D(0,0,1));

    standoffv = standoffv*standoff;

    hintPos += standoffv;

    glTranslatef(hintPos.x(),hintPos.y(),hintPos.z());

    //旋转面对摄像头并提示移动位置
    glRotatef(-zRot,0.0,0.0,1.0);
    glRotatef(-yRot,0.0,1.0,0.0);
    glRotatef(-xRot,1.0,0.0,0.0);


    //在第一个过程中，我们将呈现一个非常大的四方体
    //即生成区域的10倍大
    //渲染着色坐标表
    //255,0,0-------------255,0,255
    //| |
    //| |
    //| |
    //0,0,0----------------0,0,255
    //
    //Ÿ坐标映射到红色通道
    //x坐标映射到蓝色通道
    glNormal3f(0,0,1.0f);
    glBegin(GL_QUADS);
        glColor3f(0.0f,0.0f,0.0f);
        glVertex3f( -buildsizex*5, -buildsizey*5, 0);
        glColor3f(0.0f,0.0f,1.0f);
        glVertex3f( buildsizex*5, -buildsizey*5, 0);
        glColor3f(1.0f,0.0f,1.0f);
        glVertex3f( buildsizex*5, buildsizey*5, 0);
        glColor3f(1.0f,0.0f,0.0f);
        glVertex3f( -buildsizex*5, buildsizey*5, 0);
    glEnd();

    glReadPixels(screenPos.x(), this->height() - screenPos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    //让色彩映射到第一遍轨迹中的位置
    firstPassPos.setX((float(pixel[2])/255.0)*buildsizex*10 - buildsizex*5);
    firstPassPos.setY((float(pixel[0])/255.0)*buildsizey*10 - buildsizey*5);


    //现在我们已经准备好根据第一遍轨迹的位置，开始第二遍
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_QUADS);
        glColor3f(0.0f,0.0f,0.0f);
        glVertex3f( firstPassPos.x() - buildsizex*2*0.039063*0.5, firstPassPos.y() - buildsizey*2*0.039063*0.5, 0);
        glColor3f(0.0f,0.0f,1.0f);
        glVertex3f( firstPassPos.x() + buildsizex*2*0.039063*0.5, firstPassPos.y() - buildsizey*2*0.039063*0.5, 0);
        glColor3f(1.0f,0.0f,1.0f);
        glVertex3f( firstPassPos.x() + buildsizex*2*0.039063*0.5, firstPassPos.y() + buildsizey*2*0.039063*0.5, 0);
        glColor3f(1.0f,0.0f,0.0f);
        glVertex3f( firstPassPos.x() - buildsizex*2*0.039063*0.5, firstPassPos.y() + buildsizey*2*0.039063*0.5, 0);
    glEnd();

    //重新读第二遍的像素
    glReadPixels(screenPos.x(), this->height() - screenPos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    glPopMatrix();

    //将OpenGL的东西还原到我们建立它的时候...
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);

    qglClearColor(QColor(0,100,100));


    //如果我们打纯绿色 - 我们已经打了明确的颜色（容器是未知数）
    if(pixel[1] > 200)
        return QVector3D(0,0,0);

    //让的颜色映射到现实世界
    finalPos.setX((float(pixel[2])/255.0)*buildsizex*2*0.039063 - buildsizex*2*0.039063*0.5 + firstPassPos.x());
    finalPos.setY((float(pixel[0])/255.0)*buildsizey*2*0.039063 - buildsizey*2*0.039063*0.5 + firstPassPos.y());
    finalPos.setZ(0.0);

    //带来实际空间。
    RotateVector(finalPos,-xRot,QVector3D(1,0,0));
    RotateVector(finalPos,-yRot,QVector3D(0,1,0));
    RotateVector(finalPos,-zRot,QVector3D(0,0,1));

    finalPos += hintPos;

    return finalPos;
}


//看不见的用户 - 做了两遍渲染的基础上
//跟踪反对当前的视角给出的平面鼠标
//如果给出一个实例 - 这将跟踪对整个模型
void WorldView::Update3DCursor(QPoint pos)
{
    bool b;
    Update3DCursor(pos, NULL, b);
}
void WorldView::Update3DCursor(QPoint pos, B9ModelInstance* trackInst)
{
    bool b;
    Update3DCursor(pos, trackInst, b);
}
void WorldView::Update3DCursor(QPoint pos, B9ModelInstance* trackInst, bool &isAgainstInst)
{
    unsigned char pixel[3];//拾色器像素
    QVector3D firstPassPos;
    bool hitInstSuccess;
    unsigned int hitTri;

    isAgainstInst = false;

    //开始追踪踪平面前 - 查看我们是否给出实例提示
    if(trackInst != NULL)
    {

        hitTri = GetPickTriangleIndx(trackInst,pos,hitInstSuccess);
        if( hitInstSuccess )
        {
            isAgainstInst = true;
            cursorPos3D = GetPickOnTriangle(pos,trackInst,hitTri) + trackInst->GetPos();
            cursorNormal3D = trackInst->triList[hitTri]->normal;

            return;
        }
    }

    // 清屏，并设置背景纯绿色
    qglClearColor(QColor(0,255,0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);//不剔除，所以我们可以从后面跟踪
    glPushMatrix();
    //根据当前视角 - 我们可能要高亮画布
    //垂直或旋转
    if(currViewAngle == "FREE" || currViewAngle == "TOP" || currViewAngle == "BOTTOM")
        glRotatef(0.0f,0.0f,0.0f,1.0f);//no rotating - put on ground
    else if(currViewAngle == "FRONT" || currViewAngle == "BACK"){
        glRotatef(-90.0f,1.0f,0.0f,0.0f);
    }
    else if(currViewAngle == "RIGHT" || currViewAngle == "LEFT")
    {
        glRotatef(-90.0f,0.0f,0.0f,1.0f);
        glRotatef(-90.0f,1.0f,0.0f,0.0f);
    }

    //在第一个过程中，我们将呈现一个非常大的四方体
    //即生成区域的10倍大
    //渲染着色坐标表
    //255,0,0-------------255,0,255
    //| |
    //| |
    //| |
    //0,0,0----------------0,0,255
    //
    //Ÿ坐标映射到红色通道
    //x坐标映射到蓝色通道

    glNormal3f(0,0,1.0f);
    glBegin(GL_QUADS);
        glColor3f(0.0f,0.0f,0.0f);
        glVertex3f( -buildsizex*5, -buildsizey*5, 0);
        glColor3f(0.0f,0.0f,1.0f);
        glVertex3f( buildsizex*5, -buildsizey*5, 0);
        glColor3f(1.0f,0.0f,1.0f);
        glVertex3f( buildsizex*5, buildsizey*5, 0);
        glColor3f(1.0f,0.0f,0.0f);
        glVertex3f( -buildsizex*5, buildsizey*5, 0);
    glEnd();

    glReadPixels(pos.x(), this->height() - pos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    //让色彩映射到第一遍轨迹中的位置
    firstPassPos.setX((float(pixel[2])/255.0)*buildsizex*10 - buildsizex*5);
    firstPassPos.setY((float(pixel[0])/255.0)*buildsizey*10 - buildsizey*5);


    //现在我们已经准备好根据第一遍轨迹的位置，开始第二遍
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_QUADS);
        glColor3f(0.0f,0.0f,0.0f);
        glVertex3f( firstPassPos.x() - buildsizex*2*0.039063*0.5, firstPassPos.y() - buildsizey*2*0.039063*0.5, 0);
        glColor3f(0.0f,0.0f,1.0f);
        glVertex3f( firstPassPos.x() + buildsizex*2*0.039063*0.5, firstPassPos.y() - buildsizey*2*0.039063*0.5, 0);
        glColor3f(1.0f,0.0f,1.0f);
        glVertex3f( firstPassPos.x() + buildsizex*2*0.039063*0.5, firstPassPos.y() + buildsizey*2*0.039063*0.5, 0);
        glColor3f(1.0f,0.0f,0.0f);
        glVertex3f( firstPassPos.x() - buildsizex*2*0.039063*0.5, firstPassPos.y() + buildsizey*2*0.039063*0.5, 0);
    glEnd();
    //重新读第二遍的像素
    glReadPixels(pos.x(), this->height() - pos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    glPopMatrix();

    //将OpenGL的东西还原到我们建立它的时候...
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);

    qglClearColor(QColor(0,100,100));


    //如果我们打纯绿色 - 我们已经提示过清除色彩，不想做任何改变
    if(pixel[1] > 200)
        return;


    //将颜色映射到现实世界
    if(currViewAngle == "FREE" || currViewAngle == "TOP" || currViewAngle == "BOTTOM"){
        cursorPos3D.setX((float(pixel[2])/255.0)*buildsizex*2*0.039063 - buildsizex*2*0.039063*0.5 + firstPassPos.x());
        cursorPos3D.setY((float(pixel[0])/255.0)*buildsizey*2*0.039063 - buildsizey*2*0.039063*0.5 + firstPassPos.y());
        cursorPos3D.setZ(0.0);
        cursorNormal3D = QVector3D(0,0,1);
        cursorPosOnTrackCanvas.setX(cursorPos3D.x());
        cursorPosOnTrackCanvas.setY(cursorPos3D.y());
    }
    else if(currViewAngle == "FRONT" || currViewAngle == "BACK"){
        cursorPos3D.setX((float(pixel[2])/256.0)*buildsizex*2*0.039063 - buildsizex*2*0.039063*0.5 + firstPassPos.x());
        cursorPos3D.setY(0.0);
        cursorPos3D.setZ(-((float(pixel[0])/256.0)*buildsizey*2*0.039063 - buildsizey*2*0.039063*0.5 + firstPassPos.y()));
        cursorNormal3D = QVector3D(0,1,0);
        cursorPosOnTrackCanvas.setX(cursorPos3D.x());
        cursorPosOnTrackCanvas.setY(cursorPos3D.z());
    }
    else if(currViewAngle == "RIGHT" || currViewAngle == "LEFT")
    {
        cursorPos3D.setX(0.0);
        cursorPos3D.setY(-((float(pixel[2])/255.0)*buildsizex*2*0.039063 - buildsizex*2*0.039063*0.5 + firstPassPos.x()));
        cursorPos3D.setZ(-((float(pixel[0])/255.0)*buildsizey*2*0.039063 - buildsizey*2*0.039063*0.5 + firstPassPos.y()));
        cursorNormal3D = QVector3D(1,0,0);
        cursorPosOnTrackCanvas.setX(cursorPos3D.y());
        cursorPosOnTrackCanvas.setY(cursorPos3D.z());
    }
}

unsigned int WorldView::GetPickTriangleIndx(B9ModelInstance* inst, QPoint pos, bool &success)
{
    unsigned char pixel[3];

    if(inst == NULL)
    {
        success = false;
        return 0;
    }

    //清除屏幕
    qglClearColor(QColor(0,0,0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);

    inst->RenderTrianglePickGL();

    glReadPixels(pos.x(), this->height() - pos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    qglClearColor(QColor(0,100,100));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);


    //现在我们有了颜色，我们应该知道它是什么三角形的指标。
    //如果颜色是0,0,0，那么我们错过了模型
    unsigned int result = pixel[0]*1 + pixel[1]*256 + pixel[2]*65536;

    if(!result)
    {
        success = false;
        return 0;
    }
    else
    {
        if((result - 1) < inst->triList.size())//确保指数处于有效范围内！
        {                                //以防万一..
            success = true;
            return result - 1;//获得实际的指数
        }
        else
        {
            success = false;
            return 0;
        }

    }
}
//返回到实例中心相对位置！
QVector3D WorldView::GetPickOnTriangle(QPoint pos, B9ModelInstance* inst, unsigned int triIndx)
{
    unsigned char pixel[3];

    //清除屏幕
    qglClearColor(QColor(128,128,128));// 因此，如果是偶然，我们错过了将中心边界
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);

    inst->RenderSingleTrianglePickGL(triIndx);

    glReadPixels(pos.x(), this->height() - pos.y(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    double xPercent = float(pixel[0])/256.0;
    double yPercent = float(pixel[1])/256.0;
    double zPercent = float(pixel[2])/256.0;
    QVector3D Slew(xPercent,yPercent,zPercent);

    QVector3D minBound = inst->triList[triIndx]->minBound;
    QVector3D maxBound = inst->triList[triIndx]->maxBound;

    QVector3D globalPos = minBound + (maxBound - minBound)*Slew;


    QVector3D localPos = globalPos - inst->GetPos();


    qglClearColor(QColor(0,100,100));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);

    return localPos;

}

//返回实例上的局部位置
QVector3D WorldView::GetDrillingHit(QVector3D localBeginPosition, B9ModelInstance *inst, bool &hitground, Triangle3D *&pTri)
{
    unsigned int hitTri;
    const float viewRad = 1.0;
    const float nearPlane = 0.5;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //让成立了邻视图朝下从一开始的位置。
    glOrtho((inst->GetPos().x() + localBeginPosition.x()) - viewRad,
            (inst->GetPos().x() + localBeginPosition.x()) + viewRad,
            (inst->GetPos().y() + localBeginPosition.y()) - viewRad,
            (inst->GetPos().y() + localBeginPosition.y()) + viewRad,
            nearPlane,
            (inst->GetMaxBound().z() - inst->GetMinBound().z()));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //移动“相机”到合适的位置。
    glTranslatef(0.0f,
                 0.0f,
                 -(inst->GetPos().z() + localBeginPosition.z()));

    bool s;
    hitTri = GetPickTriangleIndx(inst, QPoint(this->width()/2,this->height()/2), s);
    if(!s)
    {
        hitground = true;
        pTri = NULL;
        return QVector3D(0,0,0);
    }

    hitground = false;
    pTri = inst->triList[hitTri];
    return GetPickOnTriangle(QPoint(this->width()/2,this->height()/2),inst,hitTri);
}




void WorldView::UpdateVisSlice()
{

    B9ModelInstance* pInst;
    if((currtool != "SUPPORTADD" && currtool != "SUPPORTMODIFY") || !pMain->ContourAidEnabled())
    {
        if(pVisSlice)
            delete pVisSlice;
        pVisSlice = NULL;
        return;
    }
    if(pMain->SupportModeInst() == NULL)
        return;



    pInst = pMain->SupportModeInst();


    if(pVisSlice != NULL)
    {
        delete pVisSlice;
    }

    pVisSlice = new Slice(cursorPos3D.z(),0);
    pVisSlice->GenerateSegments(pInst);



}

