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
#ifndef MODELDATA_H
#define MODELDATA_H

#include <QString>
#include "b9layout.h"
#include "triangle3d.h"

#include "b9modelinstance.h"


#include <vector>
class B9Layout;
class aiScene;
class ModelData {

    friend class B9ModelInstance;

public:
    ModelData(B9Layout* main, bool bypassDisplayLists = false);
	~ModelData();
	
	QString GetFilePath();
	QString GetFileName();
	
    //数据导入
    bool LoadIn(QString filepath); //返回成功
	
    //实例
	B9ModelInstance* AddInstance();
	int loadedcount;


//渲染的模型可以潜在地拥有多个显示列表，让显卡驱动程序，以便更好地分配需要非常大的模型记忆。在需要的时候生成这些翻转列表。
//递归实现
    std::vector<unsigned int> normDispLists;
    std::vector<unsigned int> flippedDispLists;

    bool FormFlippedDisplayLists();
    bool FormNormalDisplayLists();

    //geometry
    std::vector<Triangle3D> triList;
	QVector3D maxbound;
	QVector3D minbound;

	std::vector<B9ModelInstance*> instList;
    B9Layout* pMain;
private:
	
	
	
    QString filepath;//物理文件路径
    QString filename;//文件名(larry.stl)

    //效用
    void CenterModel();//此方法实现导入模型后调整其位置在0,0,0元心
	
	const aiScene* pScene;

    //渲染

    int displaySkipping; // 渲染巨大的对象时跳过多少个三角数据。。
    bool bypassDispLists;
	
    int GetGLError();

};
#endif
