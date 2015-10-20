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

#ifndef SLICE_H
#define SLICE_H

#include "b9modelinstance.h"
#include "segment.h"
#include "loop.h"
#include "SlcExporter.h"
#include <vector>

class Triangle3D;
class Loop;

class Slice
{

public:
	

    Slice(double alt, int layerIndx);
	~Slice();

	void AddSegment(Segment* pSeg);
	
    int GenerateSegments(B9ModelInstance* inputInstance);//返回初始段的数值

    void SortSegmentsByX();

    void ConnectSegmentNeighbors(); //返回微移的数量
	
	int GenerateLoops();

    void Render();//OpenGL渲染代码 - 渲染全部切片.
    void RenderOutlines();
    void DebugRender(bool normals = true, bool connections = true, bool fills = true, bool outlines = true);//渲染可见调试信息


    //导出助手
	void WriteToSlc(SlcExporter* pslc);


    std::vector<Segment*> segmentList;//分段列表
	
	std::vector<Loop> loopList;
	int numLoops;

	double realAltitude;//in mm;
    int layerIndx;//在Job工程中指数层的进展（或类似结构）
    bool inProccessing;//多线程帮助...
    QImage* pImg;//对于rasturing帮助..
private:
	bool TestIntersection(QVector2D &vec,Segment* seg1, Segment* seg2);
    void GetTrianglesAroundZ(std::vector<Triangle3D*> &outList, double z);
    void GetSegmentsAroundX(std::vector<Segment*> &outList, double x);

};


#endif
