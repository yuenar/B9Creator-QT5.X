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
#ifndef LOOP_H
#define LOOP_H

#include <vector>
#include <QtOpenGL>
#include "OS_GL_Wrapper.h"

class Segment;
class Slice;

class Loop
{
public:
	Loop(Segment* startingSeg, Slice* parentSlice);
	~Loop();
	Slice* pSlice;

	std::vector<Segment*> segListp;
    Segment* pOriginSeg;//该循环开始于这个段（指针）
	int numSegs;
	
	
    std::vector<QVector2D> polygonStrip;
    std::vector<QVector2D> triangleStrip;//由triangulator创建的有序列表中顶点将被渲染

	bool isfill;

    //调试标志位
    bool isComplete;//指示循环是否闭合完全。
	bool isPatched;
    double totalAngle;//环路（循环）周围的总角变化
	

	void ResetOrigin();
	
	int Grow();
	
	void AttachSegment(Segment* seg);

	bool SealHole(Segment* pLastSeg);
	
    void Simplify();//移除介于循环的最小段和密封起来的间隙。
	
	bool ThrowoutSmallestSegment();

    int NudgeSharedPoints();//寻找"self touching"分段点然后用这个方法微移(Nudges)
	
    int DetermineTypeBySides();

	void formPolygon();
	
    bool formTriStrip();

    void Destroy();//销毁循环中的所有分段。

    void RenderTriangles();//opengl渲染代码

	int CorrectDoubleBacks();

	bool AttemptSplitUp(Slice* pslice);

private:

};













#endif
