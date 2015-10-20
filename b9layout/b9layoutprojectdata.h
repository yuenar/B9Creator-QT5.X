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

#ifndef B9LAYOUTPROJECTDATA_H
#define B9LAYOUTPROJECTDATA_H
#include <QStringList>
#include <QVector3D>
#include <QVector2D>
#include <vector>
#include <QMessageBox>
#include "b9layout.h"

#define LAYOUT_FILE_VERSION 15

class B9Layout;
class B9LayoutProjectData : public QObject
{
	Q_OBJECT
public:
    B9LayoutProjectData();
    ~B9LayoutProjectData();

    //文件访问:
    void New();//清除了内部项目的数据，创建一个新使用默认值
    bool Open(QString filepath, bool withoutVisuals = false); //返回成功
    bool Save(QString filepath); //返回成功
    //结构访问:
    //获取
	bool IsDirtied();
    QString GetFileName();//无标题如果之前没有保存过
	QStringList GetModelFileList();
	QVector3D GetBuildSpace();
	double GetPixelSize();
    double GetPixelThickness();
	QVector2D GetResolution();
    double GetAttachmentSurfaceThickness(){return attachmentSurfaceThickness;}
    QString GetJobName();
    QString GetJobDescription();

    //设置
	void SetDirtied(bool dirt);

	void SetBuildSpaceSize(QVector3D size);
	void SetPixelSize(double size);
    void SetPixelThickness(double thick);
	void SetResolution(QVector2D dim);
    void SetAttachmentSurfaceThickness(double thick){attachmentSurfaceThickness = thick;}
    void SetJobName(QString);
    void SetJobDescription(QString);

	void CalculateBuildArea();
    void UpdateZSpace();//计算出的Z框的大小基于实例的边界
	
    B9Layout* pMain;
signals:
	void DirtChanged(bool dirt);
	void ProjectLoaded();

private:
	

	bool dirtied;
    QString mfilename;
	QStringList modelfilelist;
	QVector3D dimentions;
	QVector2D resolution;
	double xypixel;
	double zthick;
    double attachmentSurfaceThickness;
    QString jobExportName;//切片保存到工作文件时使用。
    QString jobExportDesc;
    QMessageBox *msgBox;

    void LoadDefaults();

    void PromptFindLostModel(B9ModelInstance* &pinst, QString modelPath);//提示用户寻找丢失模型，如果找到了返回为真。

    void StreamOutSupportInfo(B9SupportStructure* sup, QTextStream &out);
    void StreamInSupportInfo(B9ModelInstance* pinst, QTextStream &in, bool asFoundation = false);

};
#endif
