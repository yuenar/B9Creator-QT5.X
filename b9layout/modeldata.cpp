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

#include "modeldata.h"
#include "b9modelloader.h"
#include "loadingbar.h"

#include <QtOpenGL>
#include <QFileInfo>




//////////////////////////////////////
//Public 
//////////////////////////////////////
ModelData::ModelData(B9Layout* main, bool bypassDisplayLists)
{
	pMain = main;
	filepath = "";
    loadedcount = 0;
    displaySkipping = 0;
    bypassDispLists = bypassDisplayLists;

	maxbound = QVector3D(-999999.0,-999999.0,-999999.0);
	minbound = QVector3D(999999.0,999999.0,999999.0);

}

ModelData::~ModelData()
{
    unsigned int i;


    triList.clear();


	for(i=0; i < instList.size();i++)
	{
		delete instList[i];
	}


    for( i = 0; i < normDispLists.size(); i++)
    {
        glDeleteLists(normDispLists[i],1);
    }
    for( i = 0; i < flippedDispLists.size(); i++)
    {
        glDeleteLists(flippedDispLists[i],1);
    }



}

QString ModelData::GetFilePath()
{	
	return filepath;
}
QString ModelData::GetFileName()
{
	return filename;
}

//数据载入
bool ModelData::LoadIn(QString filepath)
{	
    bool loaderReady;
    bool abort;

    STLTri* pLoadedTri = NULL;
    Triangle3D newtri;

	this->filepath = filepath;
	
	if(filepath.isEmpty())
		return false;
	
    //从路径中提取文件名！
    filename = QFileInfo(filepath).fileName();

    B9ModelLoader mLoader(filepath,loaderReady,NULL);

    if(loaderReady == false)//打开模型数据出错
	{
        //显示加载程序出错
		QMessageBox msgBox;
        msgBox.setText(mLoader.GetError());
        msgBox.exec();
		return false;
	}

    //做一个进度条，并把它连接到
    LoadingBar loadbar(0,100);
    loadbar.useCancelButton(false);
    loadbar.setDescription("Importing: " + filename);
    QObject::connect(&mLoader,SIGNAL(PercentCompletedUpdate(qint64,qint64)),
                     &loadbar,SLOT(setProgress(qint64,qint64)));



    //现已经准备好启动装载程序（mLoader）读入每个三角数据
    //并复制它到这个模型数据。
    while(mLoader.LoadNextTri(pLoadedTri,abort))
    {
        if(abort)
        {
            //显示加载程序中止时出错
            QMessageBox msgBox;
            msgBox.setText(mLoader.GetError());
            msgBox.exec();
            return false;
        }
        else
        {
            //newtri.normal.setX(pLoadedTri->nx);
            //newtri.normal.setY(pLoadedTri->ny);
            //newtri.normal.setZ(pLoadedTri->nz);

            newtri.vertex[0].setX(pLoadedTri->x0);
            newtri.vertex[0].setY(pLoadedTri->y0);
            newtri.vertex[0].setZ(pLoadedTri->z0);
            newtri.vertex[1].setX(pLoadedTri->x1);
            newtri.vertex[1].setY(pLoadedTri->y1);
            newtri.vertex[1].setZ(pLoadedTri->z1);
            newtri.vertex[2].setX(pLoadedTri->x2);
            newtri.vertex[2].setY(pLoadedTri->y2);
            newtri.vertex[2].setZ(pLoadedTri->z2);

            //use right hand rule for importing normals - ignore file normals..
            newtri.UpdateNormalFromGeom();

            delete pLoadedTri;
            newtri.UpdateBounds();

            triList.push_back(newtri);
        }
    }



    qDebug() << "Loaded triangles: " << triList.size();
    //将模型居中!
	CenterModel();

    //产生一个法线的显示列表。

    int displaySuccess = FormNormalDisplayLists();

    if(displaySuccess)
        return true;
    else
        return false;
}
//例子
B9ModelInstance* ModelData::AddInstance()
{
	loadedcount++;
	B9ModelInstance* pNewInst = new B9ModelInstance(this);
	instList.push_back(pNewInst);
	pNewInst->SetTag(filename + " " + QString().number(loadedcount));
	return pNewInst;
}


//////////////////////////////////////
//Private
//////////////////////////////////////
void ModelData::CenterModel()
{
    //找出该模型边界到目前中心..
    unsigned int f;
    unsigned int v;
	QVector3D center;


	for(f=0;f<triList.size();f++)
	{
		for(v=0;v<3;v++)
		{
            //更新模型边界
            //最大值
            if(triList[f].maxBound.x() > maxbound.x())
			{
                maxbound.setX(triList[f].vertex[v].x());
			}
            if(triList[f].maxBound.y() > maxbound.y())
			{
                maxbound.setY(triList[f].vertex[v].y());
			}
            if(triList[f].maxBound.z() > maxbound.z())
			{
                maxbound.setZ(triList[f].vertex[v].z());
			}
			
            //最小值
            if(triList[f].minBound.x() < minbound.x())
			{
                minbound.setX(triList[f].vertex[v].x());
			}
            if(triList[f].minBound.y() < minbound.y())
            {
                minbound.setY(triList[f].vertex[v].y());
			}
            if(triList[f].minBound.z() < minbound.z())
			{
                minbound.setZ(triList[f].vertex[v].z());
			}
		}
	}

	center = (maxbound + minbound)*0.5;

    for(f=0;f<triList.size();f++)
	{
		for(v=0;v<3;v++)
		{
            triList[f].vertex[v] -= center;
		}
        triList[f].UpdateBounds(); // 因为我们正在移动每个三角数据，故需要更新它们的边界。
	}
	maxbound -= center;
	minbound -= center;
}

//渲染
//生成的OpenGL显示列表，翻转x轴顺带颠倒x法线和顶点
bool ModelData::FormNormalDisplayLists() //返回OpenGL的枚举出错。
{
    unsigned int l;
    unsigned int t;
    const unsigned int listSize = 10000;//每个列表可以存放10000大小
    unsigned int tSeamCount = 0;

    //若显示列表非空那么先置为空
    for( l = 0; l < normDispLists.size(); l++)
    {
        glDeleteLists(normDispLists[l],1);
    }

    if(bypassDispLists)
        return true;

    normDispLists.push_back(glGenLists(1));
    if(normDispLists.at(normDispLists.size()-1) == 0)
        return false;//未能分配一个列表索引???

    glNewList(normDispLists.at(normDispLists.size()-1),GL_COMPILE);
    glBegin(GL_TRIANGLES);// 绘制三角数据
    for(t = 0; t < triList.size(); t = t + 1 + displaySkipping)//遍历所有三角数据
    {

        glNormal3f(triList[t].normal.x(),triList[t].normal.y(),triList[t].normal.z());//法线

        glVertex3f( triList[t].vertex[0].x(), triList[t].vertex[0].y(), triList[t].vertex[0].z());
        glVertex3f( triList[t].vertex[1].x(), triList[t].vertex[1].y(), triList[t].vertex[1].z());
        glVertex3f( triList[t].vertex[2].x(), triList[t].vertex[2].y(), triList[t].vertex[2].z());

        //做一个新的接缝（seam）.
        if(tSeamCount >= listSize)
        {
            glEnd();
            glEndList();
            normDispLists.push_back(glGenLists(1));
            if(normDispLists.at(normDispLists.size()-1) == 0)
                return false;//未能分配一个列表索引???

            //做一个新的接缝（seam）用来检查图形错误
            int err = GetGLError();
            if(err)
            {
                if(err == GL_OUT_OF_MEMORY)
                {
                    displaySkipping += 10;
                    return FormNormalDisplayLists();
                }
                else
                    return false;
            }

            glNewList(normDispLists.at(normDispLists.size()-1),GL_COMPILE);
            glBegin(GL_TRIANGLES);// 绘制三角数据
            tSeamCount = 0;
        }

        tSeamCount++;
    }
    glEnd();
    glEndList();

    int err = GetGLError();
    if(err)
    {
        if(err == GL_OUT_OF_MEMORY)
        {
            displaySkipping += 10;
            return FormNormalDisplayLists();
        }
        else
            return false;
    }



    qDebug() << normDispLists.size() << "Normal Display Lists created for model " << filename;


    return true;
}


bool ModelData::FormFlippedDisplayLists() //返回OpenGL的错误枚举。
{
    unsigned int l;
    unsigned int t;
    const unsigned int listSize = 10000;//每个列表可以存放10000大小
    unsigned int tSeamCount = 0;

    //若显示列表非空那么先置为空
    for( l = 0; l < flippedDispLists.size(); l++)
    {
        glDeleteLists(flippedDispLists[l],1);
    }

    if(bypassDispLists)
        return true;


    flippedDispLists.push_back(glGenLists(1));
    if(flippedDispLists.at(flippedDispLists.size()-1) == 0)
        return false;//未能分配一个列表索引???

    glNewList(flippedDispLists.at(flippedDispLists.size()-1),GL_COMPILE);
    glBegin(GL_TRIANGLES);// 绘制三角数据
    for(t = 0; t < triList.size(); t = t + 1 + displaySkipping)//遍历所有三角数据
    {

        glNormal3f(-triList[t].normal.x(),triList[t].normal.y(),triList[t].normal.z());//法线

        glVertex3f( -triList[t].vertex[2].x(), triList[t].vertex[2].y(), triList[t].vertex[2].z());
        glVertex3f( -triList[t].vertex[1].x(), triList[t].vertex[1].y(), triList[t].vertex[1].z());
        glVertex3f( -triList[t].vertex[0].x(), triList[t].vertex[0].y(), triList[t].vertex[0].z());
        //创建一个seam.
        if(tSeamCount >= listSize)
        {
            glEnd();
            glEndList();
            flippedDispLists.push_back(glGenLists(1));
            if(flippedDispLists.at(flippedDispLists.size()-1) == 0)
                return false;//未能分配一个列表索引???
            //做一个新的接缝（seam）用来检查图形错误
            int err = GetGLError();
            if(err)
            {
                if(err == GL_OUT_OF_MEMORY)
                {
                    displaySkipping += 10;
                    return FormFlippedDisplayLists();
                }
                else
                    return false;
            }

            glNewList(flippedDispLists.at(flippedDispLists.size()-1),GL_COMPILE);
            glBegin(GL_TRIANGLES);// 绘制三角数据
            tSeamCount = 0;
        }

        tSeamCount++;
    }
    glEnd();
    glEndList();

    int err = GetGLError();
    if(err)
    {
        if(err == GL_OUT_OF_MEMORY)
        {
            displaySkipping += 10;
            return FormFlippedDisplayLists();
        }
        else
            return false;
    }

    qDebug() << flippedDispLists.size() << "Flipped Display Lists created for model " << filename;


    return true;

}

int ModelData::GetGLError()
{
    int displayerror = glGetError();

    if(displayerror)
    {
        //显示Assimp出错
        qDebug() << "Display List Error: " << displayerror; //写入LOG日志
        QMessageBox msgBox;

        switch(displayerror)
        {
        case GL_OUT_OF_MEMORY:
            msgBox.setText("OpenGL Error:  GL_OUT_OF_MEMORY\nModel is too large to render on your graphics card.\nAttemping To Draw Sparse Triangles.");
            break;
        case GL_INVALID_ENUM:
            msgBox.setText("OpenGL Error:  GL_INVALID_ENUM");
            break;
        case GL_INVALID_VALUE:
            msgBox.setText("OpenGL Error:  GL_INVALID_VALUE");
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            msgBox.setText("OpenGL Error:  GL_INVALID_FRAMEBUFFER_OPERATION");
            break;
        case GL_STACK_UNDERFLOW:
            msgBox.setText("OpenGL Error:  GL_STACK_UNDERFLOW");
            break;
        case GL_STACK_OVERFLOW:
            msgBox.setText("OpenGL Error:  GL_STACK_OVERFLOW");
            break;
        default:
            break;
        }

        msgBox.exec();
        return displayerror;
   }
   else
        return 0;

}
