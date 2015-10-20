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

#ifndef SLICEEDITVIEW_H
#define SLICEEDITVIEW_H
#include <QMainWindow>
#include "ui_sliceeditview.h"
#include "DrawingContext.h"
#include "b9edit.h"
#include "crushbitmap.h"
#include <QDialog>
#include <QImage>
#include <QColor>
#include <QTimer>
class B9Edit;

class SliceEditView : public QMainWindow
{
	Q_OBJECT
public:
    SliceEditView(QWidget *parent = 0, Qt::WindowFlags flags = Qt::Window);
	~SliceEditView();

	Ui::SliceEditViewClass ui;
    CrushedPrintJob* pCPJ;//一个指向打印工程文件的指针.
    B9Edit* pBuilder;
	bool modified;
	bool supportMode;	
	int currSlice;
	QString GetEditMode();

    DrawingContext* pDrawingContext; //创建指向绘制控件的指针
public slots:
	void TogSupportMode();
    void GoToSlice(int slicenumber); //开始展示下一个切片的进程..
	void DeCompressIntoContext();
	void ReCompress();
    void RefreshContext(bool alreadywhite);			//刷新
    void UpdateWidgets(); //更新窗口标题，滑块等...
	void RefreshWithGreen();
    //基础类
    void PromptBaseOptions(); //显示基本/填充对话框...
	void PrepareBase(int baselayers, int filledlayers);


    //支撑相关
	void AddSupport(QPoint pos, int size, SupportType type, int fastmode); //creates a new support by startin
	void DeleteAllSupports();

    //撤销
	void ClearUndoBuffer();
	void SaveToUndoBuffer();
	void Undo();
	void Redo();

    //剪贴板（只有在图像模式下可用！）
    void CopyToClipboard();//将当前的图像拷贝到剪贴板。
    void PasteFromClipboard();//从剪贴板取下粘贴，用新的来更换当前切片，也需要做些压缩处理



    //工具
	void SelectPenColor();
    void SelectPenWidth();
	void SetDrawTool(QString tool);
	void SetSupportTool(QString tool);

    //导航栏
	void NextSlice();
	void PrevSlice();
	void PgUpSlice(int increment);
	void PgDownSlice(int increment);
	void BaseSlice();
	void TopSlice();

    //工具栏
    void PopDrawButtons();//取消选中所有绘制按钮
    void SetDrawButtonsEnabled(bool enabled);//设置所有绘制按钮为可用
    void ShowDrawButtons(bool show);//隐藏/显示所有绘制按钮
	
    void PopSupportButtons();//取消选中所有支撑按钮
    void SetSupportButtonsEnabled(bool enabled);//设置所有支撑按钮为可用
    void ShowSupportButtons(bool show);//隐藏/显示所有支撑按钮
	
    //小工具回调
	void on_actionFlood_Fill_activated();
	void on_actionFlood_Void_activated();
	void on_actionWhite_Pen_activated();
	void on_actionBlack_Pen_activated();
	
	void on_actionCircle_activated();
	void on_actionSquare_activated();
	void on_actionTriangle_2_activated();
	void on_actionDiamond_activated();


signals:
    void sliceDirtied(QImage* pNewImg, int slicenumber); //发送信号到父类图像已编辑，并发送指针指向新图像和需要被替换的CrushedBitmap！
protected:
    void keyPressEvent(QKeyEvent * pEvent);		//处理按键事件
    void keyReleaseEvent(QKeyEvent * pEvent);		// 处理按键释放事件
    void mouseReleaseEvent(QMouseEvent * pEvent);	// 处理鼠标按钮释放事件

	

	
    bool bGrid;				// 如果为真，则绘制网格布局
	int m_xOffset, m_yOffset;
	
private:
	
	QImage topImg;
	QImage botImg;

	QTimer greenTimer;

    QList<QImage> imgBackup;//图像列表，撤消/重做
	int backupIndx;
};

#endif // SliceEditView_H
