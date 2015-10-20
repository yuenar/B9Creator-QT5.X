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

#include "loop.h"
#include "segment.h"
#include "slice.h"
#include "geometricfunctions.h"
#include "math.h"


#include <QtDebug>
#include <QtOpenGL>
#include "OS_GL_Wrapper.h"
#include "b9tesselator.h"
#include "segment.h"


//Public:
Loop::Loop(Segment* startingSeg,Slice* parentSlice)
{
	pSlice = parentSlice;
	numSegs = 0;

    isfill = 0;//by default4

    //调试
	isComplete = 0;
	isPatched = 0;
	totalAngle = 0.0;
	
	pOriginSeg = startingSeg;
    ResetOrigin();//寻找更优化的源始分段
	AttachSegment(pOriginSeg);
}
Loop::~Loop()
{
}

void Loop::ResetOrigin()
{
	bool exit = 0;
	Segment* trailSeg = pOriginSeg;
    //第一次回溯以求找到一个很好的源始分段
	//qDebug() << "Reseting Origin";
	do{
		if(!trailSeg->trailingSeg)
		{
			pOriginSeg = trailSeg;
			//qDebug() << "Origin Follower: " << pOriginSeg->trailingSeg;
			return;
		}
		trailSeg = trailSeg->trailingSeg;

		if(trailSeg == pOriginSeg)
		{
            return;//无限循环 - 没有改变任何值
		}
		
	}while(!exit);
}

int Loop::Grow()//使用初始化始分段，后面的环节一路走去，返回所有分段数（包括初始化分段）
//返回0，如未能完成一个循环！
{
	bool grow = 1;
	Segment* thisSeg = pOriginSeg;
	Segment* nextSeg = pOriginSeg->leadingSeg;

	do
	{
        thisSeg = segListp[numSegs - 1];//最后被加入的分段
		nextSeg = thisSeg->leadingSeg;
		
		if(nextSeg)
		{
				

            if(nextSeg->pLoop)//下一分段是否已经进入某一循环
			{
                if(nextSeg->pLoop == this)//是不是这个循环
				{
                    if(nextSeg == pOriginSeg)//是否我们已经遇见了循环的开端
					{
                            //这是正常的完成选项
							isComplete = true;
							grow = 0;
					}
                    else//循环过早复归! 分段丢失，这不应发生
					{
						qDebug() << "Loop Crossover at " << numSegs << " segments, Check Neighbor code!";
						grow = 0;
					}
				}
                else//或是另一种循环？（分支）
				{
                    //也不应该发生
					qDebug() << "De-Rail Detected at " << numSegs << " segments, Check Neighbor code!";
					grow = 0;
				}
			}
            else//下一分段未被调用。
			{
				AttachSegment(nextSeg);
                grow = 1;//为了清楚起见，
			}
			
		}
        else//断路!
		{
			SealHole(thisSeg);
			isPatched = true;
			isComplete = true;
			grow = false;
		}
	}while(grow);
	
	return numSegs;
}

void Loop::AttachSegment(Segment* seg)
{
	segListp.push_back(seg);
	numSegs++;
	seg->pLoop = this;
}

bool Loop::SealHole(Segment* pLastSeg)//增加了铰链分段从pLastSeg到原点
{
	Segment* pGapSeg =  new Segment(pLastSeg->p2,pOriginSeg->p1);
	

	pSlice->AddSegment(pGapSeg);
	AttachSegment(pGapSeg);

	
	pGapSeg->leadingSeg = pOriginSeg;
	pGapSeg->trailingSeg = pLastSeg;

	pLastSeg->leadingSeg = pGapSeg;
	pOriginSeg->trailingSeg = pGapSeg;

    //也许一些相交测试？
	return 1;
}

void Loop::Simplify()
{
    unsigned int s;
	double length;
    unsigned int chuckouts = 0;
	double dotwithlead;
	double dotwithtrail;
	std::vector<Segment*> templist;
	if(segListp.size() <= 3)
	{
		return;
	}
	for(s=0; s < segListp.size(); s++)
	{
		length = Distance2D(segListp[s]->p1, segListp[s]->p2);
		if(IsZero(length, SIMPLIFY_THRESH) && (chuckouts <= segListp.size() - 3))
		{
			dotwithlead = (segListp[s]->normal.x() * segListp[s]->leadingSeg->normal.x()) + (segListp[s]->normal.y() * segListp[s]->leadingSeg->normal.y());
			dotwithtrail = (segListp[s]->normal.x() * segListp[s]->trailingSeg->normal.x()) + (segListp[s]->normal.y() * segListp[s]->trailingSeg->normal.y());
			
			segListp[s]->trailingSeg->leadingSeg = segListp[s]->leadingSeg;
			segListp[s]->leadingSeg->trailingSeg = segListp[s]->trailingSeg;

            if(dotwithlead < dotwithtrail)//从头部开始
			{
				segListp[s]->trailingSeg->p2 = segListp[s]->leadingSeg->p1;
				segListp[s]->trailingSeg->FormNormal();

			}
            else//从尾部开始
			{
				segListp[s]->leadingSeg->p1 = segListp[s]->trailingSeg->p2;
				segListp[s]->leadingSeg->FormNormal();
			}
			
			segListp[s]->trailingSeg = NULL;
			segListp[s]->leadingSeg = NULL;
			chuckouts++;
		}
		else
		{
			templist.push_back(segListp[s]);
		}
	}
	segListp.clear();
	segListp = templist;

	numSegs = segListp.size();
}

bool Loop::ThrowoutSmallestSegment()
{
	std::vector<Segment*> keepers;
    unsigned int s;
	double minsize = 1000000.0;
	double size;
	double dotwithlead;
	double dotwithtrail;
	Segment* throwout = NULL;
	if(segListp.size() <= 3)
		return false;
	for(s = 0; s < segListp.size(); s++)
	{
		size = Distance2D(segListp[s]->p1, segListp[s]->p1);
		if(size < minsize)
		{
			minsize = size;
			throwout = segListp[s];
		}
	}
	for(s = 0; s < segListp.size(); s++)
	{
		if(segListp[s] == throwout)
		{
			dotwithlead = (segListp[s]->normal.x() * segListp[s]->leadingSeg->normal.x()) + (segListp[s]->normal.y() * segListp[s]->leadingSeg->normal.y());
			dotwithtrail = (segListp[s]->normal.x() * segListp[s]->trailingSeg->normal.x()) + (segListp[s]->normal.y() * segListp[s]->trailingSeg->normal.y());
			
			segListp[s]->trailingSeg->leadingSeg = segListp[s]->leadingSeg;
			segListp[s]->leadingSeg->trailingSeg = segListp[s]->trailingSeg;

            if(dotwithlead < dotwithtrail)//从头部开始
			{
				segListp[s]->trailingSeg->p2 = segListp[s]->leadingSeg->p1;
				segListp[s]->trailingSeg->FormNormal();

			}
            else//从尾部开始
			{
				segListp[s]->leadingSeg->p1 = segListp[s]->trailingSeg->p2;
				segListp[s]->leadingSeg->FormNormal();
			}
			segListp[s]->trailingSeg = NULL;
			segListp[s]->leadingSeg = NULL;
		}
		else
		{
			keepers.push_back(segListp[s]);
		}
	}
	segListp = keepers;
	numSegs = segListp.size();
	return true;
}

int Loop::NudgeSharedPoints()
{
	int nudges = 0;
	double ThisCross;
    unsigned int s1;
    unsigned int s2;
	for(s1=0;s1<segListp.size(); s1++)
	{
		for(s2=0;s2<segListp.size(); s2++)
		{
			if(s1 == s2)
				continue;
			if((segListp[s1]->leadingSeg == segListp[s2]) || (segListp[s1]->trailingSeg == segListp[s2]))
			{
				continue;
			}

            if(IsZero(Distance2D(segListp[s1]->p2, segListp[s2]->p2),0.001))//我们需要微调
			{
				segListp[s1]->p2 = segListp[s2]->p2;
				segListp[s1]->leadingSeg->p1 = segListp[s1]->p2;
				segListp[s1]->FormNormal();
				segListp[s1]->leadingSeg->FormNormal();

			
				QVector2D aveVec((segListp[s1]->normal.x() + segListp[s1]->leadingSeg->normal.x())/2.0, (segListp[s1]->normal.y() + segListp[s1]->leadingSeg->normal.y())/2.0);
				aveVec.normalize();
				
				ThisCross = (segListp[s1]->normal.x() * segListp[s1]->leadingSeg->normal.y()) - (segListp[s1]->normal.y() * segListp[s1]->leadingSeg->normal.x());
				
				if(fabs(ThisCross) < 0.001)
				{
					aveVec = (segListp[s2]->normal + segListp[s2]->leadingSeg->normal)/2.0;
					segListp[s1]->p2 += (aveVec*0.002);
				}
				else
				{
					if(ThisCross > 0)
					{
						segListp[s1]->p2 += (aveVec*0.002);
					}
					else
					{
						segListp[s1]->p2 -= (aveVec*0.002);
					}
				}

				segListp[s1]->leadingSeg->p1 = segListp[s1]->p2;
				
				segListp[s1]->FormNormal();
				segListp[s1]->leadingSeg->FormNormal();
				nudges++;
			}
		}
	}
	return nudges;
}

void Loop::formPolygon()
{
	Segment* startSeg = segListp[0];
	Segment* currSeg = startSeg;

    do
    {
        polygonStrip.push_back( currSeg->p1 );
        currSeg = currSeg->leadingSeg;
    }while(currSeg != startSeg);

}

bool Loop::formTriStrip()
{
	triangleStrip.clear();

    //As of 1.5.2 use the glu tesselator to form triangles

    B9Tesselator tess;
    return tess.Triangulate(&polygonStrip,&triangleStrip);
}




//返回结果是fill或者void基于边界分段
//返回fill为+,void是-
int Loop::DetermineTypeBySides()
{
    Segment* negXSeg = NULL;
    Segment* posXSeg = NULL;
    Segment* negYSeg = NULL;
    Segment* posYSeg = NULL;
    Segment* currSeg = NULL;
    double minX = 99999.9;
    double maxX = -999999.9;
    double minY = 999999.9;
    double maxY = -999999.9;
    QVector3D cross;
    signed int TypeVotes = 0;
    bool concaveCheck = false;
    unsigned long int i;

    //查找边界分段
    for(i = 0; i < segListp.size(); i++)
    {
        currSeg = segListp[i];

        if(currSeg->p2.x() < minX)
        {
            negXSeg = currSeg;
            minX = currSeg->p2.x();
        }
        if(currSeg->p2.x() > maxX)
        {
            posXSeg = currSeg;
            maxX = currSeg->p2.x();
        }
        if(currSeg->p2.y() < minY)
        {
            negYSeg = currSeg;
            minY = currSeg->p2.y();
        }
        if(currSeg->p2.y() > maxY)
        {
            posYSeg = currSeg;
            maxY = currSeg->p2.y();
        }
    }


    //计算X负值 Vote
    if((negXSeg->normal.x() < 0) && (negXSeg->leadingSeg->normal.x() < 0))
    {
        TypeVotes++;//fill
    }
    else if((negXSeg->normal.x() > 0) && (negXSeg->leadingSeg->normal.x() > 0))
    {
        TypeVotes--;//void
    }
    else//disagreement, check concavity
    {
        concaveCheck = true;
        cross = QVector3D::crossProduct(QVector3D(negXSeg->normal),QVector3D(negXSeg->leadingSeg->normal));
        if(cross.z() > 0)
            TypeVotes--;//fill
        else
            TypeVotes++;//void
    }


    //计算X正值 Vote
    if((posXSeg->normal.x() > 0) && (posXSeg->leadingSeg->normal.x() > 0))
    {
        TypeVotes++;//fill
    }
    else if((posXSeg->normal.x() < 0) && (posXSeg->leadingSeg->normal.x() < 0))
    {
        TypeVotes--;//void
    }
    else//分歧，检查凹凸感
    {
        concaveCheck = true;
        cross = QVector3D::crossProduct(QVector3D(posXSeg->normal),QVector3D(posXSeg->leadingSeg->normal));
        if(cross.z() > 0)
            TypeVotes--;//fill
        else
            TypeVotes++;//void
    }

    //计算Y负值 Vote
    if((negYSeg->normal.y() < 0) && (negYSeg->leadingSeg->normal.y() < 0))
    {
        TypeVotes++;//fill
    }
    else if((negYSeg->normal.y() > 0) && (negYSeg->leadingSeg->normal.y() > 0))
    {
        TypeVotes--;//void
    }
    else//分歧，检查凹凸感
    {
        concaveCheck = true;
        cross = QVector3D::crossProduct(QVector3D(negYSeg->normal),QVector3D(negYSeg->leadingSeg->normal));
        if(cross.z() > 0)
            TypeVotes--;//fill
        else
            TypeVotes++;//void
    }

    //计算Y正值 Vote
    if((posYSeg->normal.y() > 0) && (posYSeg->leadingSeg->normal.y() > 0))
    {
        TypeVotes++;//fill
    }
    else if((posYSeg->normal.y() < 0) && (posYSeg->leadingSeg->normal.y() < 0))
    {
        TypeVotes--;//void
    }
    else//分歧，检查凹凸感
    {
        concaveCheck = true;
        cross = QVector3D::crossProduct(QVector3D(posYSeg->normal),QVector3D(posYSeg->leadingSeg->normal));
        if(cross.z() > 0)
            TypeVotes--;//fill
        else
            TypeVotes++;//void
    }

    if(TypeVotes > 0)
        isfill = 1;
    else
        isfill = 0;

    return TypeVotes;

}




void Loop::Destroy()//可自由返回被声明的分段
{
    unsigned int s;
	for(s=0;s<segListp.size(); s++)
	{
		segListp[s]->pLoop = NULL;
	}
	segListp.clear();
}

void Loop::RenderTriangles()
{
    unsigned int tris = triangleStrip.size()/3;

    glBegin(GL_TRIANGLES);                      //使用三角数据绘制
    for (unsigned int i=0; i<tris; i++)
	{

        glVertex3f( triangleStrip[i*3+0].x(), triangleStrip[i*3+0].y(), 0);
        glVertex3f( triangleStrip[i*3+1].x(), triangleStrip[i*3+1].y(), 0);
        glVertex3f( triangleStrip[i*3+2].x(), triangleStrip[i*3+2].y(), 0);
    }
    glEnd();
}

int Loop::CorrectDoubleBacks()
{
	int numDoubleBacks = 0;
    unsigned int s;

	double normalsDot;

	std::vector<Segment*> keepList;
	Segment* thisSeg;
	Segment* thatSeg;
	if(segListp.size() <= 3)
	{
		return 0;
	}
	
    //首先我们需要查找double backs....
	for(s = 0; s < segListp.size(); s++)
	{
		if(segListp[s]->chucked)
		{
            continue;//如果分段被抛出
		}
		
		thisSeg = segListp[s];
		thatSeg = thisSeg->leadingSeg;
		
        //获取点阵
		normalsDot = (thisSeg->normal.x() * thatSeg->normal.x()) + (thisSeg->normal.y() * thatSeg->normal.y()); 

		if(normalsDot < -0.999)//doubleback (1-Dot)*2*180 = Degrees freedom,  current(-0.999) = 0.36 degrees
		{
			numDoubleBacks++;
            //计算thisSeg和thatSeg的长度
			//thislength = Distance2D(thisSeg->p1, thisSeg->p2);
			//thatlength = Distance2D(thatSeg->p1, thatSeg->p2);

			
			thisSeg->p2 = thatSeg->p2;
			thisSeg->FormNormal();
			thisSeg->leadingSeg = thatSeg->leadingSeg;
			thatSeg->leadingSeg->trailingSeg = thisSeg;
			thatSeg->leadingSeg = NULL;
			thatSeg->trailingSeg = NULL;
			thatSeg->chucked = true;
			
            s = 0;//从列表中重新开始
		}
	}

	if(numDoubleBacks > 0)
	{
		for(s = 0; s < segListp.size(); s++)
		{
			if(segListp[s]->chucked)
			{
				segListp[s]->chucked = false;
			}
			else
			{
				keepList.push_back(segListp[s]);
			}
		}
		segListp.clear();
		segListp = keepList;
		numSegs = segListp.size();
	}

	return numDoubleBacks;
}

bool Loop::AttemptSplitUp(Slice* pslice)
{
    unsigned int s1;
    unsigned int s2;
    unsigned int s;
	QVector2D intersectpoint;
	Segment* currSeg;
	std::vector<Segment*> keeplist;
	for(s1 = 0; s1 < segListp.size(); s1++)
	{
		for(s2 = 0; s2 < segListp.size(); s2++)
		{
			if(s1 == s2)
				continue;
			if(segListp[s2] == segListp[s1]->leadingSeg || segListp[s2] == segListp[s1]->trailingSeg)
				continue;
			if(SegmentsAffiliated(segListp[s1], segListp[s2], 0.00001))
				continue;

			if(SegmentIntersection(intersectpoint, segListp[s1]->p1, segListp[s1]->p2, segListp[s2]->p1, segListp[s2]->p2))
			{
				Segment* seg1 = new Segment(intersectpoint, segListp[s1]->p2);
				Segment* seg2 = new Segment(segListp[s2]->p1, intersectpoint);

				seg1->trailingSeg = seg2;
				seg2->leadingSeg = seg1;

				seg1->leadingSeg = segListp[s1]->leadingSeg;
				seg2->trailingSeg = segListp[s2]->trailingSeg;

				segListp[s1]->leadingSeg->trailingSeg = seg1;
				segListp[s2]->trailingSeg->leadingSeg = seg2;

				segListp[s1]->p2 = intersectpoint;
				segListp[s2]->p1 = intersectpoint;
				segListp[s1]->FormNormal();
				segListp[s2]->FormNormal();

				segListp[s1]->leadingSeg = segListp[s2];
				segListp[s2]->trailingSeg = segListp[s1];

				pslice->segmentList.push_back(seg1);
				pslice->segmentList.push_back(seg2);
				segListp.push_back(seg1);
				segListp.push_back(seg2);

				currSeg = seg1;
				
                //为新的循环填充分段列表
				do
				{
					currSeg->pLoop = NULL;
					currSeg->chucked = true;
					currSeg = currSeg->leadingSeg;
				}while(currSeg != seg1);


				keeplist.clear();
				
				for(s = 0; s < segListp.size(); s++)
				{
					if(segListp[s]->chucked)
					{
						segListp[s]->chucked = false;
					}
					else
					{
						keeplist.push_back(segListp[s]);
					}
				}
				segListp.clear();
				segListp = keeplist;
				numSegs = segListp.size();

                //做一个新的循环，不断增加其关闭的SEG1;
				Loop newLoop(seg1->leadingSeg, pslice);
				if(newLoop.Grow() >= 3)
				{
					pSlice->loopList.push_back(newLoop);
				}
				else
				{
					newLoop.Destroy();
				}
				
				return true;
			}
		}
	}

	return false;
}
