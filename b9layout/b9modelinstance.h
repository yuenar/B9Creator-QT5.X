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

#ifndef B9MODELINSTANCE_H
#define B9MODELINSTANCE_H
#include <QVector3D>
#include <QColor>
#include <QListWidget>
#include "triangle3d.h"
#include "modeldata.h"
#include "sliceset.h"
#include "b9verticaltricontainer.h"


#define INSTANCE_MIN_SCALE 0.01

class ModelData;
class Triangle3D;
class SliceSet;
class B9SupportStructure;

class B9ModelInstance
{

//友元类ModelData;
public:
    B9ModelInstance(ModelData* parent);
    ~B9ModelInstance();
	
    //翻译
    void RestOnBuildSpace(); //在打印区域尽可能低的搁置模型。
	void SetPos(QVector3D pos);
	void SetX(double x);
	void SetY(double y);
	void SetZ(double z);

    void SetScale(QVector3D scale);
	void SetScaleX(double x);
	void SetScaleY(double y);
	void SetScaleZ(double z);
	void SetRot(QVector3D r);
    void SetFlipped(int flipped);//X翻转

    //人工边界设定
    void SetBounds(QVector3D newmax, QVector3D newmin);

    //预移动检查
    bool OnPosChangeRequest(QVector3D deltaPos);
    bool OnScaleChangeRequest(QVector3D deltaScale);
    bool OnRotationChangeRequest(QVector3D deltaRot);

    //移动后回调
    void OnPosChanged(QVector3D deltaPos);
    void OnScaleChanged(QVector3D deltaScale);
    void OnRotChanged(QVector3D deltaRot);
    void OnRotXOrYChanged(QVector3D deltaRot);
    void OnRotZChangedOnly(QVector3D deltaRot);
    //增量
	void Scale(QVector3D scalar);
	void Move(QVector3D translation);
	void Rotate(QVector3D rotation);

    //获取
	QVector3D GetPos();
	QVector3D GetRot();
	QVector3D GetScale();
	QVector3D GetMaxBound();
	QVector3D GetMinBound();
    bool GetFlipped();
    QVector3D GetTriangleLocalPosition(unsigned int tri); // 返回给定的三索引的位置。

    //选择
	void SetTag(QString tag);
	void SetSelected(bool sel);

    //支撑
    bool IsInSupportMode();
    void SetInSupportMode(bool s);
    std::vector<B9SupportStructure*> GetSupports();
    B9SupportStructure* GetBasePlate();
    void EnableBasePlate();
    void DisableBasePlate();
    B9SupportStructure* AddSupport(QVector3D localTopPosition = QVector3D(), QVector3D localBottomPosition = QVector3D());//以局部的坐标，将支撑加在模型上。
    void RemoveSupport(B9SupportStructure* sup);
    void RemoveAllSupports();
    void UpdateSupports();
    void RotateSupports(QVector3D deltaRot);
    void ScaleSupportPositionsAroundCenter(QVector3D newScale, QVector3D oldScale);
    void FlipSupports();
    void ShowSupports();
    void HideSupports();

    //绘制
    void RenderGL(bool disableColor = false);//渲染那些使用实例变换且modeldata列表中的实例
    void RenderSupportsGL(bool solidColor, float topAlpha, float bottomAlpha);//渲染支撑模型.
    void renderSupportGL(B9SupportStructure* pSup, bool solidColor, float topAlpha, float bottomAlpha);
    void RenderSupportsTipsGL();
    void RenderPickGL();//简单的平面渲染的色彩标识
    void RenderTrianglePickGL();//渲染所有三角数据以轻微不同的颜色,辅以三角数据各平面阴影效果.
    void RenderSingleTrianglePickGL(unsigned int triIndx);//以精确位置渲染一个彩色编码的三角形。
    bool FormTriPickDispLists();
    void FreeTriPickDispLists();

    //几何
    void BakeGeometry(bool withsupports = false);//从进行转换模型数据拷贝三角形数据，还可以计算范围。还复制支持几何了。
    unsigned int AddSupportsToBake(bool recompBounds);//添加支持的几何形状来支撑，也更新边界..
    void UnBakeGeometry();//释放了该模型的三角形数据。
    void UpdateBounds();//更新边界数据，这是有点费时！
    //当实例处于“撑重”的状态，这个三角形列表会满
    std::vector<Triangle3D*> triList;

    //切片 - 这些成员只在被调用/使用【切片】功能下使用！
    void PrepareForSlicing(double containerThickness);//完成所有支撑，分选和灌装程序
        void SortBakedTriangles();//按绝对高度升序排列三角列表
        void AllocateTriContainers(double containerThickness); //建立垂直三角容器
        void FillTriContainers();

    std::vector<Triangle3D*>* GetTrianglesAroundZ(double zAltitude);

    void FreeFromSlicing();//unbakes -清空列表
        void FreeTriContainers();
    std::vector<B9VerticalTriContainer*> triContainers;
    double containerExtents;


	QListWidgetItem* listItem;
	ModelData* pData;
    SliceSet* pSliceSet;//切片设置
    unsigned char pickcolor[3];//采集颜色的实例

    static QColor selectedcolor;
    QColor normalcolor;
    QColor currcolor;
    QColor visualcolor;//实际的渲染色彩 - 始终平稳运行，以配合当前颜色
private:

    //平移/旋转/缩放
	QVector3D pos;
	QVector3D rot;
	QVector3D scale;
	QVector3D maxbound;
	QVector3D minbound;

    bool isFlipped;


    void CorrectRot(QVector3D &rot);//控制旋转范围在0-360
    void CorrectScale(QVector3D& scale);//不允许0或负值

    //选择
	static unsigned char gColorID[3];
	bool isselected;


    //支撑
    bool isInSupportMode;
    unsigned int supportCounter;

    //在支撑编辑模态下，我们需要一个显示列表采集三角数据。
    std::vector<unsigned int> triPickDispLists;
    //支撑结构的列表 - 支撑也属于个别情况
   //所以没有共享
    std::vector<B9SupportStructure*> supportStructureList;

    //底板支撑结构;
    B9SupportStructure* basePlateSupport;
};
#endif
