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

#ifndef B9EDIT_H
#define B9EDIT_H

#include <QMainWindow>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include "ui_b9edit.h"
#include "crushbitmap.h"
#include "SliceEditView.h"
#include "aboutbox.h"
#include <QCloseEvent>


class B9Edit : public QMainWindow
{
	Q_OBJECT

public:
    B9Edit(QWidget *parent = 0, Qt::WindowFlags flags = 0, QString inputfile = "");
    ~B9Edit();

public slots:
    //文件
	void newJob();
    void openJob(QString infile = "");
	void saveJob();
	void saveJobAs();
    //导入
    void importSlices();//按钮接入 , 细分为 firstfile() or loadsvg()
	void importSlicesFromFirstFile(QString firstfile);
    void importSlicesFromSvg(QString file,double pixelsizemicrons = -1.0);
    void importSlicesFromSlc(QString file,double pixelsizemicrons = -1.0);
    void CancelLoading(); //当进度条过早取消时调用.
	 
    //导出
	void ExportToFolder();

    //相对路径
	void SetDir(QString dir);
	QString GetDir();
	
	void updateVersion(QString s){sVersion=s; cPJ.setVersion(s); dirtied = true; updateWindowTitle();}
	void updateName(QString s){sName=s; cPJ.setName(s); dirtied = true; updateWindowTitle();}
	void updateDescription(QString s){sDescription=s; cPJ.setDescription(s); dirtied = true; updateWindowTitle();}
	void updateXY(QString s){XYPixel=s; cPJ.setXYPixel(s); dirtied = true; updateWindowTitle();}
    void updateZ(QString s){ZLayer=s; cPJ.setZLayer(QString::number(s.toDouble()/1000.0,'f',6)); dirtied = true; updateWindowTitle();}
	void updateSliceIndicator();
	void updateWindowTitle();
	
     //切片编辑视图
	void ShowSliceWindow();
	void HideSliceWindow();

    void PatchJobData(QImage* pNewImg, int slicenumber);//被编辑视图调用，表示一个图像已被修改并需要再压缩到数据结构
	void SetDirty();

signals:
    void eventHiding();
    void selectedDirChanged(QString s);
	void setVersion(QString s);	
	void setName(QString s);
	void setDescription(QString s);
	void setXYPixel(QString s);
	void setZLayer(QString s);
	void setSliceIndicator(QString s);

protected:
	void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    void hideEvent(QHideEvent *event);
    Ui::B9EditClass ui;

    //创建SliceEditView指针
	SliceEditView* pEditView;

    //创建aboutbox指针
    aboutbox* pAboutBox;

	QString sVersion;
	QString sName;
	QString sDescription;
	QString XYPixel;
	QString ZLayer;
	QString sDirPath;

	bool dirtied;
	bool continueLoading;
	QString currJobFile;

	CrushedPrintJob cPJ;

	int PromptSaveOnQuit();

};

#endif // B9EDIT_H







