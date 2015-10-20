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

#include "b9modelinstance.h"
#include "geometricfunctions.h"
#include "b9layoutprojectdata.h"
#include "b9supportstructure.h"
#include "triangle3d.h"
#include "OS_Wrapper_Functions.h"
#include <QtOpenGL>

unsigned char B9ModelInstance::gColorID[3] = {0,0,0};
QColor B9ModelInstance::selectedcolor = QColor(0,200,200);


///////////////////////////////////
//Public
///////////////////////////////////
B9ModelInstance::B9ModelInstance(ModelData* parent)
{
	pData = parent;
	pSliceSet = new SliceSet(this);
	listItem = new QListWidgetItem();
	parent->pMain->AddTagToModelList(listItem);


	pos = QVector3D(0,0,0);
	rot = QVector3D(0,0,0);
	scale = QVector3D(1,1,1);

    isFlipped = false;

	maxbound = parent->maxbound;
	minbound = parent->minbound;
	
    //选项
	isselected = false;

    //支撑
    isInSupportMode = false;
    supportCounter = 1;//start with support 1, basePlate gets 0
    basePlateSupport = NULL;

    //颜色
    normalcolor = QColor(125,125,150);
	currcolor = normalcolor;
	

    //采集
	pickcolor[0] = gColorID[0];
    pickcolor[1] = gColorID[1];
    pickcolor[2] = gColorID[2];
 
    gColorID[0]++;
    if(gColorID[0] >= 255)
    {
        gColorID[0] = 0;
        gColorID[1]++;
        if(gColorID[1] >= 255)
        {
                gColorID[1] = 0;
                gColorID[2]++;
        }
	}
	RestOnBuildSpace();
}
B9ModelInstance::~B9ModelInstance()
{
    //将实例从父对象的列表中移除
    unsigned int i;
    std::vector<B9ModelInstance*> temp;
	for(i=0;i<pData->instList.size();i++)
	{
		if(pData->instList[i] != this)
		{
			temp.push_back(pData->instList[i]);
		}
	}
	pData->instList.clear();
	pData->instList = temp;
	
	pData = NULL;
	delete listItem;
	delete pSliceSet;
	UnBakeGeometry();

    //清空任何支撑结构
    for(i = 0; i < supportStructureList.size(); i++)
    {
        delete supportStructureList[i];
    }
    delete basePlateSupport;
    //清空任何显示列表
    FreeTriPickDispLists();
}

void B9ModelInstance::SetTag(QString tag)
{
	listItem->setText(tag);
}

//translation
void B9ModelInstance::RestOnBuildSpace()
{
    SetZ(pos.z() - minbound.z() );
}
void B9ModelInstance::SetPos(QVector3D pos)
{
    QVector3D oldPos = this->pos;

    if(OnPosChangeRequest(pos - oldPos) == false)
        return;

    //更新边界坐标。
    maxbound += pos - oldPos;
    minbound += pos - oldPos;

    this->pos = pos;

    //if(pos != oldPos)
        OnPosChanged(pos - oldPos);

}
void B9ModelInstance::SetX(double x)
{
	SetPos(QVector3D(x,pos.y(),pos.z()));
}
void B9ModelInstance::SetY(double y)
{
	SetPos(QVector3D(pos.x(),y,pos.z()));
}
void B9ModelInstance::SetZ(double z)
{
    SetPos(QVector3D(pos.x(),pos.y(),z));
}

void B9ModelInstance::SetScale(QVector3D scale)
{
    CorrectScale(scale);

    if(OnScaleChangeRequest(scale - this->scale) == false)
        return;

    if(scale != this->scale)
        OnScaleChanged( scale - this->scale);


	this->scale = scale;
    CorrectScale(this->scale);
}

void B9ModelInstance::SetScaleX(double x)
{
	SetScale(QVector3D(x,scale.y(),scale.z()));
}
void B9ModelInstance::SetScaleY(double y)
{
	SetScale(QVector3D(scale.x(),y,scale.z()));
}
void B9ModelInstance::SetScaleZ(double z)
{
	SetScale(QVector3D(scale.x(),scale.y(),z));
}

void B9ModelInstance::SetRot(QVector3D r)
{
    CorrectRot(r);

    if(OnRotationChangeRequest(r - this->rot) == false)
        return;

    if(r != this->rot)
        OnRotChanged(r - this->rot);

    if((r.x() != this->rot.x()) || (r.y() != this->rot.y()))
        OnRotXOrYChanged(r - this->rot);

    if((r.x() == this->rot.x()) && (r.y() == this->rot.y()) && (r.z() != this->rot.z()))
        OnRotZChangedOnly(r - this->rot);

    this->rot = r;
}



void B9ModelInstance::SetFlipped(int flipped)
{
    //ealy out
    if(flipped != int(isFlipped))
    {
        if(supportStructureList.size())
        {
            if(this->rot.x() || this->rot.y())
            {
                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText("Mirroring will break all supports");
                msgBox.setInformativeText("Are you sure you want to Mirror?");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                int ret = msgBox.exec();

                if(ret == QMessageBox::No)
                {
                    pData->pMain->ExitToolAction();//解决鼠标的问题
                    pData->pMain->UpdateInterface();
                    return;
                }
                else
                {
                    //终止支撑
                    RemoveAllSupports();
                    pData->pMain->ExitToolAction();//解决鼠标的问题
                }
            }
            else//我们没有任何X或Y旋转，所以我们应该能够反映支撑
            {
                FlipSupports();
            }
        }
    }


    isFlipped = flipped;

    if(flipped)
    {
        if(!pData->flippedDispLists.size())
        {
            pData->FormFlippedDisplayLists();
        }
    }


}

//人工边界设定(无承重)
//警告 - 仅使用这个值作为移动的结果。
void B9ModelInstance::SetBounds(QVector3D newmax, QVector3D newmin)
{
    maxbound = newmax;
    minbound = newmin;
}


//预移动检测回调
bool B9ModelInstance::OnPosChangeRequest(QVector3D deltaPos)
{


    return true;
}

bool B9ModelInstance::OnScaleChangeRequest(QVector3D deltaScale)
{

    return true;

}

bool B9ModelInstance::OnRotationChangeRequest(QVector3D deltaRot)
{

    if((deltaRot.x() || deltaRot.y()) && supportStructureList.size())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Orientating will break all supports");
        msgBox.setInformativeText("Are you sure you want to orientate?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();

        if(ret == QMessageBox::No)
        {
            pData->pMain->ExitToolAction();//解决鼠标的问题
            return false;
        }
        else
        {
            //终止支撑
            RemoveAllSupports();
            pData->pMain->ExitToolAction();//解决鼠标的问题
        }
    }

    return true;

}



//移动后回调
void B9ModelInstance::OnPosChanged(QVector3D deltaPos)
{
    unsigned int s;
    double maxDepth = -99999.0;
    double depth;
    double buildAreaMaxX,buildAreaMaxY;


    UpdateSupports();

    //倒立模型支撑检测
    for(s = 0; s < supportStructureList.size(); s++)
    {
        if(supportStructureList[s]->IsConstricted(depth) && supportStructureList[s]->GetIsGrounded())
        {
            if(depth > maxDepth)
                maxDepth = depth;
        }
    }
    if((maxDepth > 0))
    {
        Move(QVector3D(0,0,maxDepth+0.01));
    }

    if(GetBasePlate())
    {
        if(minbound.z() < GetBasePlate()->GetBottomLength())
        {
            Move(QVector3D(0,0,GetBasePlate()->GetBottomLength() - minbound.z()));
        }
    }


        //限制运动当构建表完全关闭
        //允许势必延长了构建表的分有一个实例大小
    buildAreaMaxX = pData->pMain->ProjectData()->GetBuildSpace().x()/2.0 + (maxbound.x() - minbound.x())*0.5;
    buildAreaMaxY = pData->pMain->ProjectData()->GetBuildSpace().y()/2.0 + (maxbound.y() - minbound.y())*0.5;


    if(minbound.x() > buildAreaMaxX)
    {
        Move(QVector3D(-(minbound.x() - buildAreaMaxX)-0.01,0,0));
    }
    if(maxbound.x() < -buildAreaMaxX)
    {
        Move(QVector3D((-buildAreaMaxX - maxbound.x())+0.01,0,0));
    }
    if(minbound.y() > buildAreaMaxY)
    {
        Move(QVector3D(0,-(minbound.y() - buildAreaMaxY)-0.01,0));
    }
    if(maxbound.y() < -buildAreaMaxY)
    {
        Move(QVector3D(0,(-buildAreaMaxY - maxbound.y())+0.01,0));
    }



    pData->pMain->ProjectData()->UpdateZSpace();
}

void B9ModelInstance::OnScaleChanged(QVector3D deltaScale)
{
    unsigned int s;
    double maxDepth = -99999.0;
    double depth;

    ScaleSupportPositionsAroundCenter(scale, scale + deltaScale);

    //缩放操作后，若有底部支撑则上移模型
    for(s = 0; s < GetSupports().size(); s++)
    {
        if(GetSupports()[s]->IsConstricted(depth) && GetSupports()[s]->GetIsGrounded())
        {
            if(depth > maxDepth)
                maxDepth = depth;
        }
    }

    if((maxDepth > 0))
    {
        Move(QVector3D(0,0,maxDepth+0.01));
    }

    if(GetBasePlate())
    {
        if(minbound.z() < GetBasePlate()->GetBottomLength())
        {
            Move(QVector3D(0,0,GetBasePlate()->GetBottomLength() - minbound.z()));
        }
    }

}

void B9ModelInstance::OnRotChanged(QVector3D deltaRot)
{

}

void B9ModelInstance::OnRotXOrYChanged(QVector3D deltaRot)
{

}
void B9ModelInstance::OnRotZChangedOnly(QVector3D deltaRot)
{
    //旋转所有支撑增量。
    RotateSupports(deltaRot);
}




	
//增量
void B9ModelInstance::Scale(QVector3D scalar)
{
    SetScale(QVector3D(scale.x() + scalar.x(),scale.y() + scalar.y(),scale.z() + scalar.z()));
}
void B9ModelInstance::Move(QVector3D translation)
{
	SetPos(QVector3D(pos.x() + translation.x(),pos.y() + translation.y(),pos.z() + translation.z()));
}
void B9ModelInstance::Rotate(QVector3D rotation)
{
	SetRot(QVector3D(rot.x() + rotation.x(),rot.y() + rotation.y(),rot.z() + rotation.z()));
}


//获取
QVector3D B9ModelInstance::GetPos()
{
	return pos;
}
QVector3D B9ModelInstance::GetRot()
{
	return rot;
}

QVector3D B9ModelInstance::GetScale()
{
	return scale;
}
QVector3D B9ModelInstance::GetMaxBound()
{
	return maxbound;
}

QVector3D B9ModelInstance::GetMinBound()
{
	return minbound;
}

bool B9ModelInstance::GetFlipped()
{
    return isFlipped;
}

//当承重后便被调用
QVector3D B9ModelInstance::GetTriangleLocalPosition(unsigned int tri)
{

    return(triList[tri]->maxBound + triList[tri]->minBound)*0.5 - GetPos();
}




//选项
void B9ModelInstance::SetSelected(bool sel)
{
	isselected = sel;
	
	if(sel)
	{
		currcolor = selectedcolor;
	}
	else
	{
		currcolor = normalcolor;
	}
	listItem->setSelected(sel);
}

//支撑

bool B9ModelInstance::IsInSupportMode()
{
    return isInSupportMode;
}

void B9ModelInstance::SetInSupportMode(bool s)
{
    isInSupportMode = s;
}

std::vector<B9SupportStructure*> B9ModelInstance::GetSupports()
{
    return supportStructureList;
}

B9SupportStructure* B9ModelInstance::GetBasePlate()
{
    return basePlateSupport;

}
void B9ModelInstance::EnableBasePlate()
{
    if(basePlateSupport != NULL)
    {
        delete basePlateSupport;
        basePlateSupport = NULL;
    }

    basePlateSupport = new B9SupportStructure(this);
    basePlateSupport->SetIsGrounded(true);
    basePlateSupport->SetTopAttachShape("");
    basePlateSupport->SetMidAttachShape("");
    basePlateSupport->SetBottomAttachShape("Cylinder");
    basePlateSupport->SetBottomPenetration(0.0);
}

void B9ModelInstance::DisableBasePlate()
{
    if(basePlateSupport != NULL)
    {
        delete basePlateSupport;
        basePlateSupport = NULL;
    }
}

B9SupportStructure* B9ModelInstance::AddSupport(QVector3D localTopPosition, QVector3D localBottomPosition)
{
    B9SupportStructure* newSup = new B9SupportStructure(this);
    newSup->AssignPickId();
    newSup->SetBottomPoint(localBottomPosition);
    newSup->SetTopPoint(localTopPosition);
    newSup->SupportNumber = supportCounter;
    supportStructureList.push_back(newSup);
    supportCounter++;

    return newSup;
}
void B9ModelInstance::RemoveSupport(B9SupportStructure* sup)
{
    unsigned int s;
    std::vector<B9SupportStructure*> keepers;
    for(s = 0; s < supportStructureList.size(); s++)
    {
        if(supportStructureList[s] == sup)
        {
            delete sup;
        }
        else
            keepers.push_back(supportStructureList[s]);
    }
    supportStructureList = keepers;
}
void B9ModelInstance::RemoveAllSupports()
{
    unsigned int s;
    for(s = 0; s < supportStructureList.size(); s++)
    {
        delete supportStructureList[s];
    }

    supportStructureList.clear();
    supportCounter = 1;
}
void B9ModelInstance::UpdateSupports()
{
    unsigned int s;
    for(s = 0; s < supportStructureList.size(); s++)
    {
        supportStructureList[s]->ForceUpdate();
    }

    if(basePlateSupport != NULL)
    {

        basePlateSupport->ForceUpdate();
    }
}

void B9ModelInstance::RotateSupports(QVector3D deltaRot)
{
    //我们需要按度数来旋转的所有支撑点。
    unsigned int s;
    B9SupportStructure* cSprt;
    QVector3D oldTopLocalPosition;
    QVector3D oldBottomLocalPosition;

    for(s = 0; s < supportStructureList.size(); s++)
    {
        cSprt = supportStructureList[s];

        cSprt->Rotate(deltaRot);

    }

}

void B9ModelInstance::ScaleSupportPositionsAroundCenter(QVector3D newScale, QVector3D oldScale)
{
    //支撑的缩放其实是改变它的位置
    unsigned int s;
    B9SupportStructure* cSprt;
    QVector3D oldTopLocalPosition;
    QVector3D oldBottomLocalPosition;
    QVector3D newTopLocalPosition;
    QVector3D newBottomLocalPosition;

    QVector3D moveFactor;
    moveFactor = oldScale*QVector3D(1/newScale.x(),1/newScale.y(),1/newScale.z());

    for(s = 0; s < supportStructureList.size(); s++)
    {
        cSprt = supportStructureList[s];

        oldTopLocalPosition = cSprt->GetTopPoint();
        oldBottomLocalPosition = cSprt->GetBottomPoint();

        //移动旧位置到局部模型旋转区域。
        RotateVector(oldTopLocalPosition,-this->rot.z(),QVector3D(0,0,1));
        RotateVector(oldTopLocalPosition,-this->rot.x(),QVector3D(1,0,0));
        RotateVector(oldTopLocalPosition,-this->rot.y(),QVector3D(0,1,0));
        RotateVector(oldBottomLocalPosition,-this->rot.z(),QVector3D(0,0,1));
        RotateVector(oldBottomLocalPosition,-this->rot.x(),QVector3D(1,0,0));
        RotateVector(oldBottomLocalPosition,-this->rot.y(),QVector3D(0,1,0));


        //do the resulting translation

        newTopLocalPosition = oldTopLocalPosition*moveFactor;
        newBottomLocalPosition = oldBottomLocalPosition*moveFactor;

        //回到全局区域
        RotateVector(newTopLocalPosition,this->rot.y(),QVector3D(0,1,0));
        RotateVector(newTopLocalPosition,this->rot.x(),QVector3D(1,0,0));
        RotateVector(newTopLocalPosition,this->rot.z(),QVector3D(0,0,1));
        RotateVector(newBottomLocalPosition,this->rot.y(),QVector3D(0,1,0));
        RotateVector(newBottomLocalPosition,this->rot.x(),QVector3D(1,0,0));
        RotateVector(newBottomLocalPosition,this->rot.z(),QVector3D(0,0,1));

        cSprt->SetTopPoint(newTopLocalPosition);
        cSprt->SetBottomPoint(newBottomLocalPosition);

    }

    //此外，我们要处理的底板和缩放半径
    if(GetBasePlate())
    {
        GetBasePlate()->SetBottomRadius(GetBasePlate()->GetBottomRadius()*moveFactor.x());
    }
}

//镜面所有支撑基于Z轴旋转
//不会影响x或y轴旋转。
void B9ModelInstance::FlipSupports()
{
    unsigned int s;
    B9SupportStructure* cSprt;
    QVector3D oldTopLocalPosition;
    QVector3D oldBottomLocalPosition;
    QVector3D newTopLocalPosition;
    QVector3D newBottomLocalPosition;
    QVector3D TopNorm;
    QVector3D BottomNorm;


    QVector3D mirrorVec(0,1,0);
    RotateVector(mirrorVec,this->rot.z(),QVector3D(0,0,1));
    QVector3D flipVec(0,1,0);
    RotateVector(flipVec,this->rot.z()-90,QVector3D(0,0,1));

    double side;
    double distToPlane;

    for(s = 0; s < supportStructureList.size(); s++)
    {
        cSprt = supportStructureList[s];

        //顶部
        oldTopLocalPosition = cSprt->GetTopPoint();
        distToPlane = QVector3D().distanceToLine(QVector2D(oldTopLocalPosition),mirrorVec);
        side = QVector3D::crossProduct(mirrorVec,QVector2D(oldTopLocalPosition).normalized()).z();
        if(side < 0) side = -1.0;
        else side = 1.0;
        newTopLocalPosition = oldTopLocalPosition + side*2.0*distToPlane*flipVec;

        //底部
        oldBottomLocalPosition = cSprt->GetBottomPoint();
        distToPlane = QVector3D().distanceToLine(QVector2D(oldBottomLocalPosition),mirrorVec);
        side = QVector3D::crossProduct(mirrorVec,QVector2D(oldBottomLocalPosition).normalized()).z();
        if(side < 0) side = -1.0;
        else side = 1.0;
        newBottomLocalPosition = oldBottomLocalPosition + side*2.0*distToPlane*flipVec;


        //顶部法线
        TopNorm = cSprt->GetTopNormal();
        distToPlane = QVector3D().distanceToLine(QVector2D(TopNorm),mirrorVec);
        side = QVector3D::crossProduct(mirrorVec,QVector2D(TopNorm).normalized()).z();
        if(side < 0) side = -1.0;
        else side = 1.0;
        TopNorm = TopNorm + side*distToPlane*2.0*flipVec;
        TopNorm.normalize();


        //底部法线
        BottomNorm = cSprt->GetBottomNormal();
        distToPlane = QVector3D().distanceToLine(QVector2D(BottomNorm),mirrorVec);
        side = QVector3D::crossProduct(mirrorVec,QVector2D(BottomNorm).normalized()).z();
        if(side < 0) side = -1.0;
        else side = 1.0;
        BottomNorm = BottomNorm + side*distToPlane*2.0*flipVec;
        BottomNorm.normalize();

        //  设置新的数值
        cSprt->SetTopPoint(newTopLocalPosition);
        cSprt->SetBottomPoint(newBottomLocalPosition);

        cSprt->SetTopNormal(TopNorm);
        cSprt->SetBottomNormal(BottomNorm);
    }
}



void B9ModelInstance::ShowSupports()
{
    unsigned int s;
    for(s = 0; s < supportStructureList.size(); s++)
    {
        supportStructureList[s]->SetVisible(true);
    }
}

void B9ModelInstance::HideSupports()
{
    unsigned int s;
    for(s = 0; s < supportStructureList.size(); s++)
    {
        supportStructureList[s]->SetVisible(false);
    }
}









//绘制
void B9ModelInstance::RenderGL(bool disableColor)
{
    unsigned int ls;


    //做一个流畅的视觉过渡。
	visualcolor.setRedF(visualcolor.redF() + (currcolor.redF() - visualcolor.redF())/2.0);
	visualcolor.setGreenF(visualcolor.greenF() + (currcolor.greenF() - visualcolor.greenF())/2.0);
	visualcolor.setBlueF(visualcolor.blueF() + (currcolor.blueF() - visualcolor.blueF())/2.0);

	glPushMatrix();
            if(!disableColor)
            glColor3f(visualcolor.redF(),visualcolor.greenF(),visualcolor.blueF());

                glTranslatef(pos.x(),pos.y(),pos.z());
                    //整体Z轴翻转
                    glRotatef(rot.z(), 0.0, 0.0, 1.0);
                    glRotatef(rot.x(), 1.0, 0.0, 0.0);
                    glRotatef(rot.y(), 0.0, 1.0, 0.0);

                        glScalef(scale.x(),scale.y(),scale.z());
							
                        if(isFlipped)//选择使用哪个显示列表
                            for(ls = 0; ls < pData->flippedDispLists.size(); ls++)
                                glCallList(pData->flippedDispLists[ls]);
                        else
                        {
                            for(ls = 0; ls < pData->normDispLists.size(); ls++)
                                glCallList(pData->normDispLists[ls]);
                        }


	glPopMatrix();


}
void B9ModelInstance::RenderSupportsGL(bool solidColor, float topAlpha, float bottomAlpha)
{
    //绘制所有支撑
    //尽管支撑定义为实例的全球中心偏移量
    //我们要用到gltranslate.
    unsigned int s;
    glPushMatrix();
        glTranslatef(pos.x(),pos.y(),pos.z());
        for(s = 0; s < supportStructureList.size(); s++)
        {

            supportStructureList[s]->RenderUpper(solidColor, topAlpha);
        }
        for(s = 0; s < supportStructureList.size(); s++)
        {

            supportStructureList[s]->RenderLower(solidColor, bottomAlpha);
        }
        if(basePlateSupport) basePlateSupport->RenderLower(solidColor, bottomAlpha);
    glPopMatrix();
}

void B9ModelInstance::renderSupportGL(B9SupportStructure* pSup, bool solidColor, float topAlpha, float bottomAlpha)
{
    glPushMatrix();
        glTranslatef(pos.x(),pos.y(),pos.z());

            pSup->RenderUpper(solidColor, topAlpha);
            pSup->RenderLower(solidColor, bottomAlpha);

    glPopMatrix();
}


//renders just lines for support tips.
void B9ModelInstance::RenderSupportsTipsGL()
{
    unsigned int s;
    B9SupportStructure* pSup;
    glPushMatrix();
        glTranslatef(pos.x(),pos.y(),pos.z());
        for(s = 0; s < supportStructureList.size(); s++)
        {
            pSup = supportStructureList[s];
            glBegin(GL_LINES);

                glVertex3f(pSup->GetTopPoint().x(),pSup->GetTopPoint().y(),pSup->GetTopPoint().z());
                glVertex3f(pSup->GetTopPivot().x(),pSup->GetTopPivot().y(),pSup->GetTopPivot().z());

                glVertex3f(pSup->GetTopPivot().x(),pSup->GetTopPivot().y(),pSup->GetTopPivot().z());
                glVertex3f(pSup->GetBottomPivot().x(),pSup->GetBottomPivot().y(),pSup->GetBottomPivot().z());


                glVertex3f(pSup->GetBottomPivot().x(),pSup->GetBottomPivot().y(),pSup->GetBottomPivot().z());
                glVertex3f(pSup->GetBottomPoint().x(),pSup->GetBottomPoint().y(),pSup->GetBottomPoint().z());

            glEnd();
        }

    glPopMatrix();
}



void B9ModelInstance::RenderPickGL()
{
    unsigned int ls,s;

	glPushMatrix();
		glColor3f(pickcolor[0]/255.0,pickcolor[1]/255.0,pickcolor[2]/255.0);
			glTranslatef(pos.x(),pos.y(),pos.z());
			
                //全局z轴翻转
                glRotatef(rot.z(), 0.0, 0.0, 1.0);
                glRotatef(rot.x(), 1.0, 0.0, 0.0);
                glRotatef(rot.y(), 0.0, 1.0, 0.0);
					glScalef(scale.x(),scale.y(),scale.z());
						
                    if(isFlipped)//选择要使用的显示列表
                        for(ls = 0; ls < pData->flippedDispLists.size(); ls++)
                            glCallList(pData->flippedDispLists[ls]);
                    else
                    {
                        for(ls = 0; ls < pData->normDispLists.size(); ls++)
                            glCallList(pData->normDispLists[ls]);
                    }
	glPopMatrix();
    glPushMatrix();
        glColor3f(pickcolor[0]/255.0,pickcolor[1]/255.0,pickcolor[2]/255.0);
        glTranslatef(pos.x(),pos.y(),pos.z());
        //glRotatef(rot.z(), 0.0, 0.0, 1.0);
        for(s = 0; s < supportStructureList.size(); s++)
        {
            supportStructureList[s]->RenderUpper(true, 1.0);
            supportStructureList[s]->RenderLower(true, 1.0);
        }
        if(basePlateSupport) basePlateSupport->RenderLower(true, 1.0);
    glPopMatrix();
}

//这功能会慢些，因为它用不到显示列表。
//在溢出前总共有1600多万三角数据...
//假设实例被承重，并使用承重三角数据;
//减去最后的三角数据值，如果跳过承重三角数据列表中最后的那个三角数据
//(对支撑模态下有用)
bool B9ModelInstance::FormTriPickDispLists()
{
    unsigned char r = 1;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned int l;
    unsigned int t;
    const unsigned int listSize = 10000;//定义每个列表存放10000三角数据。
    unsigned int tSeamCount = 0;

    //首先清空现有显示列表
    for( l = 0; l < triPickDispLists.size(); l++)
    {
        glDeleteLists(triPickDispLists[l],1);
    }

    triPickDispLists.push_back(glGenLists(1));

    glNewList(triPickDispLists.at(triPickDispLists.size()-1),GL_COMPILE);
    glBegin(GL_TRIANGLES);//使用三角数据绘制
    for(t = 0; t < triList.size(); t++)//遍历所有三角数据
    {
        glColor3ub(r,g,b);
        glNormal3f( triList[t]->normal.x(),triList[t]->normal.y(),triList[t]->normal.z());//法线

        glVertex3f( triList[t]->vertex[0].x(), triList[t]->vertex[0].y(), triList[t]->vertex[0].z());
        glVertex3f( triList[t]->vertex[1].x(), triList[t]->vertex[1].y(), triList[t]->vertex[1].z());
        glVertex3f( triList[t]->vertex[2].x(), triList[t]->vertex[2].y(), triList[t]->vertex[2].z());

        //inc颜色计算器
        r++;
        if(!r)
        {
            g++;
            if(!g)
            {
                b++;
                if(!b)
                    qDebug() << "WARNING! Triangle Pick Overflow!";
            }
        }

        //make a new seam.
        if(tSeamCount >= listSize)
        {
            glEnd();
            glEndList();
            triPickDispLists.push_back(glGenLists(1));

            glNewList(triPickDispLists.at(triPickDispLists.size()-1),GL_COMPILE);
            glBegin(GL_TRIANGLES);// 使用三角数据绘制
            tSeamCount = 0;
        }
        tSeamCount++;
    }
    glEnd();
    glEndList();

    //稍后检查错误
    if(glGetError())
    {
       for( l = 0; l < triPickDispLists.size(); l++)
       {
            glDeleteLists(triPickDispLists[l],1);
       }
       triPickDispLists.clear();
       qDebug() << "TriPick Display List creation failed for Instance " << this;
       qDebug() << "Using Procedural rendering.";

       return false;
    }


    qDebug() << triPickDispLists.size() << "TriPick Display Lists created for Instance " << this;

    return true;
}
void B9ModelInstance::FreeTriPickDispLists()
{
    unsigned int l;
    for( l = 0; l < triPickDispLists.size(); l++)
    {
         glDeleteLists(triPickDispLists[l],1);
    }
    triPickDispLists.clear();
}

void B9ModelInstance::RenderTrianglePickGL()
{
    unsigned char r = 1;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned int t;
    unsigned int i;

    if(triPickDispLists.size())
    {
        for(i = 0; i < triPickDispLists.size(); i++)
            glCallList(triPickDispLists[i]);
    }
    else//同时放慢程序渲染
    {
        glBegin(GL_TRIANGLES);// 使用三角数据绘制
        for(t = 0; t < triList.size(); t++)//遍历所有三角数据
        {
            glColor3ub(r,g,b);
            glNormal3f( triList[t]->normal.x(),triList[t]->normal.y(),triList[t]->normal.z());//normals

            glVertex3f( triList[t]->vertex[0].x(), triList[t]->vertex[0].y(), triList[t]->vertex[0].z());
            glVertex3f( triList[t]->vertex[1].x(), triList[t]->vertex[1].y(), triList[t]->vertex[1].z());
            glVertex3f( triList[t]->vertex[2].x(), triList[t]->vertex[2].y(), triList[t]->vertex[2].z());

            r++;
            if(!r)
            {
                g++;
                if(!g)
                {
                    b++;
                    if(!b)
                        qDebug() << "WARNING! Triangle Pick Overflow!";
                }
            }
        }
        glEnd();
    }
}


//渲染位于内部边框顶点的三角数据用特定颜色。
void B9ModelInstance::RenderSingleTrianglePickGL(unsigned int triIndx)
{

    float vert0X = triList[triIndx]->vertex[0].x();
    float vert0Y = triList[triIndx]->vertex[0].y();
    float vert0Z = triList[triIndx]->vertex[0].z();

    float vert1X = triList[triIndx]->vertex[1].x();
    float vert1Y = triList[triIndx]->vertex[1].y();
    float vert1Z = triList[triIndx]->vertex[1].z();

    float vert2X = triList[triIndx]->vertex[2].x();
    float vert2Y = triList[triIndx]->vertex[2].y();
    float vert2Z = triList[triIndx]->vertex[2].z();

    QVector3D maxbound = triList[triIndx]->maxBound;
    QVector3D minbound = triList[triIndx]->minBound;




    float vert0XPercent = (vert0X - minbound.x())/(maxbound.x() - minbound.x());
    float vert0YPercent = (vert0Y - minbound.y())/(maxbound.y() - minbound.y());
    float vert0ZPercent = (vert0Z - minbound.z())/(maxbound.z() - minbound.z());

    float vert1XPercent = (vert1X - minbound.x())/(maxbound.x() - minbound.x());
    float vert1YPercent = (vert1Y - minbound.y())/(maxbound.y() - minbound.y());
    float vert1ZPercent = (vert1Z - minbound.z())/(maxbound.z() - minbound.z());

    float vert2XPercent = (vert2X - minbound.x())/(maxbound.x() - minbound.x());
    float vert2YPercent = (vert2Y - minbound.y())/(maxbound.y() - minbound.y());
    float vert2ZPercent = (vert2Z - minbound.z())/(maxbound.z() - minbound.z());


    glBegin(GL_TRIANGLES);// 使用三角数据绘制
    if(isFlipped)
    {
        glNormal3f( triList[triIndx]->normal.x(),triList[triIndx]->normal.y(),triList[triIndx]->normal.z());//法线

        glColor3f(vert2XPercent,vert2YPercent,vert2ZPercent);
        glVertex3f( triList[triIndx]->vertex[2].x(),
                    triList[triIndx]->vertex[2].y(),
                    triList[triIndx]->vertex[2].z());


        glColor3f(vert1XPercent,vert1YPercent,vert1ZPercent);
        glVertex3f( triList[triIndx]->vertex[1].x(),
                    triList[triIndx]->vertex[1].y(),
                    triList[triIndx]->vertex[1].z());

        glColor3f(vert0XPercent,vert0YPercent,vert0ZPercent);
        glVertex3f( triList[triIndx]->vertex[0].x(),
                    triList[triIndx]->vertex[0].y(),
                    triList[triIndx]->vertex[0].z());

    }
    else
    {
        glNormal3f( triList[triIndx]->normal.x(),triList[triIndx]->normal.y(),triList[triIndx]->normal.z());//normals

        glColor3f(vert0XPercent,vert0YPercent,vert0ZPercent);
        glVertex3f( triList[triIndx]->vertex[0].x(),
                    triList[triIndx]->vertex[0].y(),
                    triList[triIndx]->vertex[0].z());

        glColor3f(vert1XPercent,vert1YPercent,vert1ZPercent);
        glVertex3f( triList[triIndx]->vertex[1].x(),
                    triList[triIndx]->vertex[1].y(),
                    triList[triIndx]->vertex[1].z());

        glColor3f(vert2XPercent,vert2YPercent,vert2ZPercent);
        glVertex3f( triList[triIndx]->vertex[2].x(),
                    triList[triIndx]->vertex[2].y(),
                    triList[triIndx]->vertex[2].z());
    }
    glEnd();
}












//几何
void B9ModelInstance::BakeGeometry(bool withsupports)
{
    unsigned long int t;
    unsigned short int v;
    Triangle3D* pNewTri;

	UnBakeGeometry();
	
    //更新边界
    maxbound = QVector3D(-999999.0,-999999.0,-999999.0);
    minbound = QVector3D(999999.0,999999.0,999999.0);

    //该功能通常出现在用户在等待时调用，高亮等待光标。
    Enable_User_Waiting_Cursor();


    //从pData拷贝三角数据到应用变换列表
    for(t = 0; t < pData->triList.size(); t++)
	{
        //所有数据已拷贝
        pNewTri = new Triangle3D(pData->triList[t]);

        //应用变换到法线
        //反向缩放
        pNewTri->normal *= QVector3D(1/scale.x(),1/scale.y(),1/scale.z());
        //翻动
        if(isFlipped)
            pNewTri->normal.setX(-pNewTri->normal.x());

        //旋转
        RotateVector(pNewTri->normal,rot.y(),QVector3D(0,1,0));
        RotateVector(pNewTri->normal,rot.x(),QVector3D(1,0,0));
        RotateVector(pNewTri->normal,rot.z(),QVector3D(0,0,1));

        pNewTri->normal.normalize();//再次强制转换到单元长度


        for(v=0;v<3;v++)
		{
            //放缩三角数据第一个顶点
            pNewTri->vertex[v] *= scale;
            if(isFlipped)
                pNewTri->vertex[v].setX(-pNewTri->vertex[v].x());


            //旋转第二个顶点
            RotateVector(pNewTri->vertex[v], rot.y(), QVector3D(0,1,0));//y
            RotateVector(pNewTri->vertex[v], rot.x(), QVector3D(1,0,0));//x
            RotateVector(pNewTri->vertex[v], rot.z(), QVector3D(0,0,1));//z


            //平移第三个顶点
			pNewTri->vertex[v] += pos;
		}


        //更新三角数据边界
		pNewTri->UpdateBounds();

        if(withsupports == false)//如果我们连同支撑一起拷贝了三角数据，我们还需要做边界检查
        {
            //更新实例边界
            if(pNewTri->maxBound.x() > this->maxbound.x())
                this->maxbound.setX(pNewTri->maxBound.x());
            if(pNewTri->maxBound.y() > this->maxbound.y())
                this->maxbound.setY(pNewTri->maxBound.y());
            if(pNewTri->maxBound.z() > this->maxbound.z())
                this->maxbound.setZ(pNewTri->maxBound.z());

            if(pNewTri->minBound.x() < this->minbound.x())
                this->minbound.setX(pNewTri->minBound.x());
            if(pNewTri->minBound.y() < this->minbound.y())
                this->minbound.setY(pNewTri->minBound.y());
            if(pNewTri->minBound.z() < this->minbound.z())
                this->minbound.setZ(pNewTri->minBound.z());
        }

        triList.push_back(pNewTri);
	}

    //承重支撑根据设定的标志位.
    if(withsupports)
    AddSupportsToBake(true);

    //qDebug() << "Baked instance";
    Disable_User_Waiting_Cursor();
}

//返回所添加的三角数据数值
unsigned int B9ModelInstance::AddSupportsToBake(bool recompBounds)
{
    unsigned int s, t;
    unsigned int trisAdded = 0;
    B9SupportStructure* pSup;
    Triangle3D* pCurTri;

    for(s = 0; s < supportStructureList.size(); s++)//遍历所有支撑
    {
        pSup = supportStructureList[s];
        trisAdded += pSup->BakeToInstanceGeometry();
    }

    if(basePlateSupport)
        trisAdded += basePlateSupport->BakeToInstanceGeometry();

    if(recompBounds)
    {
        //既然我们添加了支撑，故而需要重新计算实例边界
        for(t = 0; t < triList.size(); t++)
        {
            pCurTri = triList[t];
            //更新实例边界
            if(pCurTri->maxBound.x() > this->maxbound.x())
                this->maxbound.setX(pCurTri->maxBound.x());
            if(pCurTri->maxBound.y() > this->maxbound.y())
                this->maxbound.setY(pCurTri->maxBound.y());
            if(pCurTri->maxBound.z() > this->maxbound.z())
                this->maxbound.setZ(pCurTri->maxBound.z());

            if(pCurTri->minBound.x() < this->minbound.x())
                this->minbound.setX(pCurTri->minBound.x());
            if(pCurTri->minBound.y() < this->minbound.y())
                this->minbound.setY(pCurTri->minBound.y());
            if(pCurTri->minBound.z() < this->minbound.z())
                this->minbound.setZ(pCurTri->minBound.z());
        }
    }

    return trisAdded;
}

void B9ModelInstance::UnBakeGeometry()
{
    if(!triList.size())
        return;

    for(unsigned long int i=0;i<triList.size();i++)
    {
        delete triList[i];
    }
    triList.clear();
    //qDebug() << "Unbaked instance";
}
void B9ModelInstance::UpdateBounds()
{
	BakeGeometry();
	UnBakeGeometry();

    //提示该工程需要更新整体边界
    pData->pMain->ProjectData()->UpdateZSpace();
    pData->pMain->UpdateInterface();
}


//常规切片调用
void B9ModelInstance::PrepareForSlicing(double containerThickness)
{
    BakeGeometry(true);
    SortBakedTriangles();
    AllocateTriContainers(containerThickness);
    FillTriContainers();
}
void B9ModelInstance::FreeFromSlicing()
{
    UnBakeGeometry();
    FreeTriContainers();
}


//将triList按相对高度升序排列，产生并填充容器
//切片厚度需要确定容器的大小和数量
void B9ModelInstance::SortBakedTriangles()
{
    std::sort(triList.begin(),triList.end(), Triangle3D::GreaterBottomAltitude);
}

void B9ModelInstance::AllocateTriContainers(double containerThickness)
{
    unsigned int i;
    unsigned int numContainers;
    double modelZExtent;
    B9VerticalTriContainer* newContainer = NULL;
    B9VerticalTriContainer* prevContainer = NULL;

    modelZExtent = maxbound.z() - minbound.z();
    numContainers = modelZExtent/containerThickness;
    if(numContainers <= 0)
        numContainers = 1;//以防万一，我们有一个非常薄的模型。

    containerExtents = modelZExtent/numContainers;

    //开始生成容器并将它们关联起来
    for( i = 0; i < numContainers; i++ )//从下到上
    {
        newContainer = new B9VerticalTriContainer();

        newContainer->minZ = minbound.z() + containerExtents*i;
        newContainer->maxZ = newContainer->minZ + containerExtents;
        newContainer->downContainer = prevContainer;

        if(prevContainer != NULL)
            prevContainer->upContainer = newContainer;

        prevContainer = newContainer;

        triContainers.push_back(newContainer);
    }

    qDebug() << "num containers created" << triContainers.size();
}

//遍历排序后的三角数据列表
//再有效地填充三角容器
void B9ModelInstance::FillTriContainers()
{
    unsigned int tIndx;
    B9VerticalTriContainer* currContainer;
    B9VerticalTriContainer* iterContainer;
    Triangle3D* currTri;

    qDebug() << "B9ModelInstance: filling tri containers...";

    currContainer = triContainers[0];

    for(tIndx = 0; tIndx < triList.size(); tIndx++)//bottom -> top
    {
        currTri = triList[tIndx];
        //如果需要，先移动的参考底座（base）
        while(!currContainer->TriangleFits(currTri))
        {
            if(currContainer->upContainer == NULL)
            {
                qDebug() << "ERROR: last container exists below model top";

            }
            currContainer = currContainer->upContainer;
        }
        iterContainer = currContainer;

        while((iterContainer != NULL) && (iterContainer->TriangleFits(currTri)))
        {
            iterContainer->tris.push_back(currTri);
            iterContainer = iterContainer->upContainer;
        }
    }
}

std::vector<Triangle3D*>* B9ModelInstance::GetTrianglesAroundZ(double zAltitude)
{
    B9VerticalTriContainer* targetContainer;

    unsigned int indx = int(minbound.z() + zAltitude*containerExtents);
    if(indx <= 0) indx = 0;
    if(indx > triContainers.size()-1)
        indx = triContainers.size()-1;

    targetContainer = triContainers[ indx ];

    //以防万一 - 请仔细检查，这个容器应包含此Z轴-高度
    while((targetContainer->upContainer != NULL) && (targetContainer->maxZ < zAltitude))//向上
    {
        targetContainer = targetContainer->upContainer;
    }
    while((targetContainer->downContainer != NULL) && (targetContainer->minZ > zAltitude))//向下
    {
        targetContainer = targetContainer->downContainer;
    }

    return &targetContainer->tris;
}



void B9ModelInstance::FreeTriContainers()
{
    unsigned int i;
    for(i = 0; i < triContainers.size(); i++)
    {
        delete triContainers[i];
    }
    triContainers.clear();
}


//////////////////////////////////////////////////////////
//Private
///////////////////////////////////////////////////////////

void B9ModelInstance::CorrectRot(QVector3D &rot)
{
	while (rot.x() < 0)
        rot += QVector3D(360,0,0);
    while (rot.x() >= 360 )
        rot -= QVector3D(360,0,0);

	while (rot.y() < 0)
        rot += QVector3D(0,360,0);
    while (rot.y() >= 360 )
        rot -= QVector3D(0,360,0);

	while (rot.z() < 0)
        rot += QVector3D(0,0,360);
    while (rot.z() >= 360 )
        rot -= QVector3D(0,0,360);
}
void B9ModelInstance::CorrectScale(QVector3D &scl)
{
    if(scl.x() <= INSTANCE_MIN_SCALE)
        scl.setX(INSTANCE_MIN_SCALE);

    if(scl.y() <= INSTANCE_MIN_SCALE)
        scl.setY(INSTANCE_MIN_SCALE);

    if(scl.z() <= INSTANCE_MIN_SCALE)
        scl.setZ(INSTANCE_MIN_SCALE);
}
