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

#include "b9edit.h"
#include "loadingbar.h"
#include <QFileInfo>
#include <QDesktopWidget>
#include <QFile>
#include <QSvgRenderer> // see copyright distribution notice on qt's website!
#include <OS_Wrapper_Functions.h>
#include <QInputDialog>
//Public
B9Edit::B9Edit(QWidget *parent, Qt::WindowFlags flags, QString infile)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
    setAcceptDrops(true);
	setStatusBar(0);
	ui.mainToolBar->setMovable(false);
	ui.mainToolBar->setMaximumHeight(24);
	ui.mainToolBar->addAction(ui.actionNew_Job);
	ui.mainToolBar->addAction(ui.actionOpen_Exsisting_Job_File);
	ui.mainToolBar->addAction(ui.actionSave_To_Job);
	ui.mainToolBar->addSeparator();
	ui.mainToolBar->addAction(ui.actionShow_Slice_Window);

    pAboutBox = new aboutbox(this);


	pEditView = new SliceEditView(this);
	pEditView->pCPJ = &cPJ;
	pEditView->pBuilder = this;
	dirtied = false;
	continueLoading = true;
    if(infile == "")
    {
        newJob();
    }
    else
    {
        openJob(infile);
    }
}
B9Edit::~B9Edit()
{
}
void B9Edit::hideEvent(QHideEvent *event)
{
    emit eventHiding();
    event->accept();
}




////////////////////////////////////////
//SLOTS
////////////////////////////////////////
//File
void B9Edit::newJob()
{	
	if(dirtied)
	{
		QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("B9Edit"),
				tr("The document has been modified.\n"
					"Do you want to save your changes?"),
					QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		
		if (ret == QMessageBox::Yes)
		{
			saveJob();
		}
		else if (ret == QMessageBox::Cancel)
		{
			return;
		}
	}

	sVersion = "1";
	emit setVersion(sVersion);
	cPJ.setVersion(sVersion);

	sName = "Untitled";
	emit setName(sName);
	cPJ.setName(sName);

	sDescription = "Untitled";
	emit setDescription(sDescription);
	cPJ.setDescription(sDescription);

	XYPixel = "100";
	emit setXYPixel(XYPixel);
	cPJ.setXYPixel(XYPixel);

	ZLayer = "100";
	emit setZLayer(ZLayer);
	cPJ.setZLayer(ZLayer);

	sDirPath = "";
	emit selectedDirChanged(sDirPath);
	
	currJobFile = "";

	dirtied = false;
	cPJ.clearAll();
    cPJ.DeleteAllSupports();
	updateSliceIndicator();
	updateWindowTitle();
	pEditView->UpdateWidgets();
}
void B9Edit::openJob(QString infile)
{
    QSettings settings;
    if(infile.isEmpty())
    {
        QFileDialog dialog(this);
        infile = dialog.getOpenFileName(this,"Open B9 Job", settings.value("WorkingDir").toString(),tr("B9 Job Files (*.b9j);;All files (*.*)"));
        if(infile.isEmpty())//用户点击取消....
            return;
    }

    if(dirtied)
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("B9Edit"),
                tr("The document has been modified.\n"
                "Do you want to save your changes?"),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (ret == QMessageBox::Yes)
        {
            saveJob();
        }
        else if (ret == QMessageBox::Cancel)
        {
            return;
        }
    }


    QFile file(infile);
	SetDir(QFileInfo(file).absolutePath());
	if(!cPJ.loadCPJ(&file))
	{
        QMessageBox::warning(this, tr("B9Edit"), QString("Unable To Load Job:\n" +QFileInfo(infile).baseName()),QMessageBox::Ok);
        return;
	}
	
    currJobFile = infile;

	emit setVersion(cPJ.getVersion());
	emit setName(cPJ.getName());
    emit setDescription(cPJ.getDescription());
    emit setXYPixel(QString().number(cPJ.getXYPixel().toDouble()*1000));
    emit setZLayer(QString().number(cPJ.getZLayer().toDouble()*1000));

	dirtied = false;

	updateSliceIndicator();
	updateWindowTitle();
	pEditView->ClearUndoBuffer();
	pEditView->GoToSlice(0);
    pEditView->UpdateWidgets();

	
}
void B9Edit::saveJob()
{
	if(currJobFile.isEmpty())
	{
		saveJobAs();
	}
	else
	{
		QFile file(currJobFile);
		if(!cPJ.saveCPJ(&file))
		{
            QMessageBox::warning(this, tr("B9Edit"), tr("Unable To Save Job"),QMessageBox::Ok);
			return;
		}
	}
	dirtied = false;
	updateWindowTitle();
	
}
void B9Edit::saveJobAs()
{
    QSettings settings;
    QString saveFile;

    saveFile = CROSS_OS_GetSaveFileName(this,"Save B9 Job",settings.value("WorkingDir").toString() + "//" + sName,
                                        tr("B9 Job Files (*.b9j);;All files(*.*)"),QStringList() << "b9j");



	QFile file(saveFile);
	SetDir(QFileInfo(file).absolutePath());
	if(!cPJ.saveCPJ(&file))
		return;

	currJobFile = saveFile;
	dirtied = false;
}
//导入
void B9Edit::importSlices()//按钮访问 ,导入firstfile()或者loadsvg()
{
    QSettings settings;
    QFileDialog dialog(this);
    QString openFile = dialog.getOpenFileName(this,"Select First Of Multiple Images", settings.value("WorkingDir").toString(),tr("Image Files (*.bmp *.png *.jpg *.jpeg *.tiff *.tif *.svg *.slc);;All files (*.*)"));
	if(openFile.isEmpty()) return;

	SetDir(QFileInfo(openFile).absolutePath());

	if(QFileInfo(openFile).completeSuffix() == "svg")
	{
		importSlicesFromSvg(openFile);
	}
    else if(QFileInfo(openFile).completeSuffix() == "slc")
    {
        importSlicesFromSlc(openFile);
    }
    else //标准图像
	{
		importSlicesFromFirstFile(openFile);
	}
}
void B9Edit::importSlicesFromFirstFile(QString firstfile)
{
	int i;
	int n;
	bool hasExt = false;
    QString extension; //包括 '.'
	QString shortenedStr;
	QString number;
	int startingnumber;
	int endingnumber = 0;
	QString openFile = firstfile;
	
	

	for(i=openFile.size()-1;i>=0;i--)
	{
		extension.prepend(openFile.at(i));
		if(openFile.at(i) == '.')
		{	
			hasExt = true;
			break;
		}
	}
	if(!hasExt)
		return;
	
    //查看最后一个字符是否一个数
		
	shortenedStr = openFile;
    shortenedStr.chop(extension.size());//缩短字符串（一切不带扩展名）。

	i = shortenedStr.size() - 1;
	while(shortenedStr.at(i).isDigit())
	{
		number.prepend(openFile.at(i));//number will hold any number left of the '.'
		i--;
	}
    shortenedStr.chop(number.size());//从缩短的字符串中削减数值. (C:/blablbabl10blblb/layer)
	startingnumber = number.toInt();
	qDebug() << "Starting Number: " << startingnumber;
	
    //探头看看有多少文件存在。
	endingnumber = startingnumber;
	while(QFileInfo(shortenedStr + QString::number(endingnumber) + extension).isFile())
	{
		endingnumber++;
	}

    //创建一个loadingbar的新实例。
    LoadingBar load(0,endingnumber);
	load.setDescription("Importing Images...");
	QObject::connect(&load,SIGNAL(rejected()),this,SLOT(CancelLoading()));
	
	QImage img;
	cPJ.clearAll();
	n = startingnumber;
	while(img.load(shortenedStr + QString::number(n) + extension))
	{
		cPJ.addImage(&img);
		n++;
		load.setValue(n - startingnumber);

        if(!continueLoading)//中断装载循环
		{
			continueLoading = true;
			cPJ.clearAll();
			break;
		}
        //让应用程序来处理进程事件
		QApplication::processEvents();
	}

	updateSliceIndicator();
	pEditView->ClearUndoBuffer();
	pEditView->GoToSlice(0);
	pEditView->UpdateWidgets();
	dirtied = true;
}
void B9Edit::importSlicesFromSvg(QString file, double pixelsizemicrons)
{
	int layers = 0;
	double xsizemm = 0.0;
	double ysizemm = 0.0;
	double x = 0;
	double y = 0;
    bool inverted = false;

	QSvgRenderer SvgRender;
	
	
	if(!SvgRender.load(file))
	{
		QMessageBox msgBox;
		msgBox.setText("Unable to import SVG file.");
		msgBox.exec();
		return;
	}
	if(!SvgRender.elementExists("layer0"))
	{
		QMessageBox msgBox;
		msgBox.setText("SVG file does not contain compatible layer information\nUnable to import.");
		msgBox.exec();
		return;
	}
    //做一个快速搜索单词：“slic3r:type="contour"”，然后在下面的"style="fill:"
        //找出图像是否被反转与否。
    QString buff = "";
    QFile searchfile(file);
    searchfile.open(QIODevice::ReadOnly);
    QTextStream searchstream(&searchfile);


    while(buff != "slic3r:type=\"contour\"" && !searchstream.atEnd())
    {
        searchstream >> buff;
    }
    if(!searchstream.atEnd())//we must have found it.
    {
        while(buff != "style=\"fill:" && !searchstream.atEnd())
        {
            searchstream >> buff;
        }
        if(!searchstream.atEnd())
        {
            searchstream >> buff;
            if(buff == "white\"")
            {
                inverted = false;
            }
            else
            {
                inverted = true;
            }
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setText("Could not determine color scheme\nImage may be inverted.");
            msgBox.exec();
            qDebug() << "Can't find style=\"fill";
            return;
        }
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Could not determine color scheme\nImage may be inverted.");
        msgBox.exec();
        qDebug() << "Can't find slic3r:type";
        return;
    }
    searchfile.close();
    //结束搜索
	
	
    //开始循环查找在SVG文件中有多少层。
	while(SvgRender.elementExists("layer" + QString().number(layers)))
	{
		x = SvgRender.boundsOnElement("layer" + QString().number(layers)).right();
		y = SvgRender.boundsOnElement("layer" + QString().number(layers)).bottom();

		if(x > xsizemm)
			xsizemm = x;

		if(y > ysizemm)
			ysizemm = y;

		layers++;
		QApplication::processEvents();
	}



	if(xsizemm <= 0 || ysizemm <= 0)
	{
		QMessageBox msgBox;
		msgBox.setText("Bad bounds in SVG file.");
        msgBox.exec();
		return;
	}
	
//我们要搞清楚像素尺寸，单位精确到微米

    double pixelsizemm;
    double layersizemm;
    if(pixelsizemicrons < 0)
    {
        bool ok;
        pixelsizemm = QInputDialog::getDouble(this, tr("XY Resolution"),
                                        tr("XY Resolution in Microns:"), 100.0, 1, 10000, 2, &ok)*0.001;
        if(!ok)
            return;

        layersizemm = QInputDialog::getDouble(this, tr("Z Resolution"),
                                        tr("Layer Thickness in Microns:"), 100.0, 1, 10000, 2, &ok)*0.001;
        if(!ok)
            return;
    }
    else
    {
        pixelsizemm = pixelsizemicrons*0.001;
        layersizemm = pixelsizemicrons*0.001;
    }


    cPJ.setXYPixel(QString().number(pixelsizemm));
    cPJ.setZLayer(QString().number(layersizemm));//在导入时按3D长宽比1:1建模
    emit setXYPixel(QString::number(cPJ.getXYPixelmm()*1000));
    emit setZLayer(QString::number(cPJ.getZLayermm()*1000));

    cPJ.setName(QFileInfo(file).baseName());
    emit setName(cPJ.getName());

    //创建一个loadingbar的新实例。
    LoadingBar load(0,layers);
	load.setDescription("Importing SVG...");
	QObject::connect(&load,SIGNAL(rejected()),this,SLOT(CancelLoading()));

	
    QImage img(double(xsizemm)/pixelsizemm,double(ysizemm)/pixelsizemm,QImage::Format_ARGB32);

	QPainter painter(&img);

	cPJ.clearAll();

    for( int i = 0; i <= layers; i++)
	{
        if(!inverted) img.fill(Qt::black); else img.fill(Qt::white);
		QRectF realelementbounds = SvgRender.boundsOnElement("layer" + QString().number(i));

        QRectF pixelbounds(realelementbounds.left()/pixelsizemm, realelementbounds.top()/pixelsizemm, realelementbounds.width()/pixelsizemm, realelementbounds.height()/pixelsizemm);

        painter.begin(&img);

        SvgRender.render(&painter, "layer" + QString().number(i),pixelbounds);

        painter.end();

        if(inverted) img.invertPixels();

        img = img.mirrored (false, true);
        cPJ.addImage(&img);
		load.setValue(load.GetValue()+1);
		QApplication::processEvents();
        if(!continueLoading)
		{
			continueLoading = true;
			cPJ.clearAll();
			break;
		}
	}

	updateSliceIndicator();
	pEditView->ClearUndoBuffer();
	pEditView->GoToSlice(0);
	pEditView->UpdateWidgets();
	dirtied = true;
}
void B9Edit::importSlicesFromSlc(QString file, double pixelsizemicrons)
{
    quint8 readbyte;
    quint32 readfloat;
    float x;
    float y;
    float* pfloat;

    const int maxheadersize = 2048;
    int headersize = 0;

    QFile slcfile(file);
    if(!slcfile.open(QIODevice::ReadOnly))
    {
        QMessageBox msgBox;
        msgBox.setText("Unable to open SLC file..");
        msgBox.exec();
        return;
    }

    double pixelsizemm;
    double layersizemm;
    if(pixelsizemicrons < 0)
    {
        bool ok;
        pixelsizemm = QInputDialog::getDouble(this, tr("XY Resolution"),
                                        tr("XY Resolution in Microns:"), 100.0, 1, 10000, 2, &ok)*0.001;
        if(!ok)
            return;

        layersizemm = QInputDialog::getDouble(this, tr("Z Resolution"),
                                        tr("Layer Thickness in Microns:"), 100.0, 1, 10000, 2, &ok)*0.001;
        if(!ok)
            return;

    }
    else
    {
        pixelsizemm = pixelsizemicrons*0.001;
        layersizemm = pixelsizemicrons*0.001; //命令行缺省值
    }
    //svg导入同理
        cPJ.setXYPixel(QString().number(pixelsizemm));
        cPJ.setZLayer(QString().number(layersizemm));//在导入时按3D长宽比1:1建模
        emit setXYPixel(QString::number(cPJ.getXYPixelmm()*1000));
        emit setZLayer(QString::number(cPJ.getZLayermm()*1000));

        cPJ.setName(QFileInfo(file).baseName());
        emit setName(cPJ.getName());
    //////////
    QDataStream slcstream(&slcfile);
    QTextStream slctextstream(&slcfile);
    slcstream.setByteOrder(QDataStream::LittleEndian);
    QString headertext;
    QString fileunits;
    cPJ.clearAll();



    //头部分：
    int unitit = 0;
    while(headersize <= maxheadersize)//readbyte != 0x1a && readbyte != 0x0d && readbyte != 0x0a)
    {
       slctextstream >> headertext;
       unitit++;
       if(headertext == "-UNIT")
       {
           slctextstream >> fileunits;
           qDebug() << fileunits;
           break;
       }
       if(unitit >= 10)
       {
           break;
       }
    }

    slctextstream.device()->reset();

    while(headersize <= maxheadersize)//readbyte != 0x1a && readbyte != 0x0d && readbyte != 0x0a)
    {
       slcstream >> readbyte;
       headersize++;
       if(readbyte == 0x0d)
       {
           slcstream >> readbyte;
           headersize++;
           if(readbyte == 0x0a)
           {
               slcstream >>readbyte;
               headersize++;
               if(readbyte == 0x1a)
               {
                   break;
               }
           }
       }
    }

    //3D保留部分：
   //跳过256个字节
    slcstream.skipRawData((256));

    //示例：
    slcstream >> readbyte;
    int tableentries = (int)readbyte;
    qDebug() << "Table Entries: " << tableentries;

    for(int t = 0; t < tableentries; t++)
    {
        slcstream >> readfloat;
        slcstream >> readfloat;
        slcstream >> readfloat;
        slcstream >> readfloat;

    }

    //轮廓部分
    quint32 numslices = 0;
    quint32 numboundries = 0;
    quint32 numvertices = 0;
    quint32 numgaps = 0;
    quint32 minzlevel = 0;

    QVector2D maxb(-99999,-99999);
    QVector2D minb(99999,99999);

    double scalefactor = 1.0;

    if(fileunits == "INCH")
        scalefactor = 25.4;

    qint64 returnpos = slcstream.device()->pos();
    //FIRST PASS
    //我们需要计算边界第一遍
    bool boundsdetermined = false;
    while(numboundries != 0xffffffff)
    {

        slcstream >> minzlevel;
        slcstream >> numboundries;
        if(numboundries == 0xffffffff)
        {
            break;
        }
        numslices++;
        //用顶点数据填充新的循环
        for(unsigned b = 0; b < numboundries; b++)
        {
            slcstream >> numvertices;
            slcstream >> numgaps;
            for(uint v = 0; v < numvertices; v++)
            {
                slcstream >> readfloat;//x线
                pfloat = (float*)&readfloat;
                x = *pfloat;

                slcstream >> readfloat;//y线
                pfloat = (float*)&readfloat;
                y = *pfloat;

                if(x > maxb.x())
                    maxb.setX(x);
                if(y > maxb.y())
                    maxb.setY(y);
                if(x < minb.x())
                    minb.setX(x);
                if(y < minb.y())
                    minb.setY(y);

                boundsdetermined = true;
            }
        }
    }


    if(!boundsdetermined)
    {
        QMessageBox msgBox;
        msgBox.setText("Invalid File");
        msgBox.exec();
        return;
    }

    maxb*=scalefactor;
    minb*=scalefactor;

    slcstream.device()->seek(returnpos);
    numboundries = 0;

    //创建一个loadingbar的新实例。
    LoadingBar load(0,numslices);
    load.setDescription("Importing SLC..");
    QObject::connect(&load,SIGNAL(rejected()),this,SLOT(CancelLoading()));


    QImage img((maxb - minb).x()/pixelsizemm,(maxb - minb).y()/pixelsizemm,QImage::Format_ARGB32);

    qDebug() << maxb << " " << minb;

    double x1;
    double y1;
    double x2;
    double y2;
    double Dot;
    double Cross;
    double diff;
    double totalAngle;
    //SECOND PASS, 实际形成。
    while(numboundries != 0xffffffff)
    {
        slcstream >> minzlevel;
        slcstream >> numboundries;
        if(numboundries == 0xffffffff)
        {
            break;
        }
        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_Difference);
        painter.setRenderHint(QPainter::Antialiasing,false);

        QBrush fillbrush(QColor(1,0,0));
        QBrush voidbrush(QColor(0,1,0));
        //用顶点数据填充新的循环
        img.fill(Qt::black);


        for(unsigned b = 0; b < numboundries; b++)
        {
            slcstream >> numvertices;
            slcstream >> numgaps;
            QPointF* points = new QPointF[numvertices+1];

            totalAngle = 0;
            QPointF firstpoint;
            for(uint v = 0; v <= numvertices; v++)
            {

                if(v == numvertices)
                {
                   x = firstpoint.x();
                   y = firstpoint.y();

                }
                else
                {

                    slcstream >> readfloat;//x线
                    pfloat = (float*)&readfloat;
                    x = *pfloat*scalefactor;

                    slcstream >> readfloat;//y线
                    pfloat = (float*)&readfloat;
                    y = *pfloat*scalefactor;


                 }

                if(v == 1)
                {
                    firstpoint = QPointF(x,y);
                }
                QVector2D cord(x,y);
                cord -= minb;
                cord *= (1/pixelsizemm);


                if(v >= 2)//在角度计算之后 我们可以用点表示
                {
                    x1 = points[v-1].x() - points[v-2].x();
                    y1 = points[v-1].y() - points[v-2].y();
                    x2 = cord.x() - points[v-1].x();
                    y2 = cord.y() - points[v-1].y();
                    QVector2D vec1(x1,y1); vec1.normalize();
                    QVector2D vec2(x2,y2); vec2.normalize();
                    x1 = vec1.x();
                    y1 = vec1.y();
                    x2 = vec2.x();
                    y2 = vec2.y();

                    Dot = (x1 * x2) + (y1 * y2);
                    Cross = (x1 * y2) - (y1 * x2);

                    //过滤Dot和Cross
                    if(Dot > 1)
                        Dot = 1;
                    else if(Dot < -1)
                        Dot = -1;
                    if(Cross > 1)
                        Cross = 1;
                    else if(Cross < -1)
                        Cross = -1;

                    if(Cross >= 0)
                        diff = -acos(Dot);
                    else
                        diff = acos(Dot);

                    totalAngle = totalAngle + diff;
                }

                points[v] = QPointF(cord.x(),cord.y());
            }


            if(totalAngle > 0)
            {
                painter.setBrush(voidbrush);

            }
            else
            {
                painter.setBrush(fillbrush);

            }

            //绘制代码实现
            painter.drawPolygon(points,numvertices,Qt::WindingFill);

            delete[] points;
        }


        for(int imgx = 0; imgx < img.size().width(); imgx++)
        {
            for(int imgy = 0; imgy < img.size().height(); imgy++)
            {
                QRgb pickedcolor = img.pixel(imgx,imgy);
                if(qRed(pickedcolor) || qGreen(pickedcolor))
                {
                    int result = qRed(pickedcolor) - qGreen(pickedcolor);

                    if(result > 0)
                    {
                        result = 255;
                    }
                    else if(result <= 0)
                    {
                        result = 1;
                    }
                    img.setPixel(imgx,imgy,QColor(result,0,0).rgb());
                }



            }
        }

        painter.end();
        img = img.mirrored(false,true);
        cPJ.addImage(&img);

        load.setValue(load.GetValue()+1);
        QApplication::processEvents();
        if(!continueLoading)
        {
            continueLoading = true;
            cPJ.clearAll();
            break;
        }
    }

    slcfile.close();

    updateSliceIndicator();
    pEditView->ClearUndoBuffer();
    pEditView->GoToSlice(0);
    pEditView->UpdateWidgets();
    dirtied = true;
}


void B9Edit::CancelLoading()
{
	continueLoading = false;
}

//导出
void B9Edit::ExportToFolder()
{
	int s = 0;
	bool cont;
	int quality;
	bool previousStatus;
	QImage buff;

    //文件夹对话框
    QSettings settings;
	QFileDialog dialog(this);
    QString folder = dialog.getExistingDirectory(this,"Select Folder", settings.value("WorkingDir").toString());

	if(folder.isEmpty())
		return;
	SetDir(folder);
    //获取用户所选文件格式：
	QStringList choices;
    choices << tr("BMP") << tr("TIFF") << tr("JPEG");
	QString format = QInputDialog::getItem(this, tr("Image Export"),
                                          tr("Format:"), choices , 0, false, &cont);
	if(!cont)
		return;

	if(format == "JPEG")
	{
        quality = QInputDialog::getInt(this, tr("QInputDialog::getInteger()"),
                                  tr("Quality:"), 100, 0, 100, 1, &cont);
		if (!cont)
			return;
	}




    //进度条
    LoadingBar bar(0,cPJ.getTotalLayers() - 1);
	QObject::connect(&bar,SIGNAL(rejected()),this,SLOT(CancelLoading()));


	previousStatus = cPJ.renderingSupports();
	cPJ.showSupports(true);
	for(s=0;s<cPJ.getTotalLayers();s++)
	{
		if(!continueLoading)
		{
			continueLoading = true;
			break;
		}

		cPJ.setCurrentSlice(s);
		cPJ.inflateCurrentSlice(&buff,0,0,true);
        //save the image with 4 leading zeros so that the images sort well in the OS explorer.
        buff.save(folder + "\\" + cPJ.getName() + "_" +
                  QString("%1").arg(s+1,4,10,QChar('0')) + "." +
                    format.toLower(),format.toLatin1(),100);
		bar.setValue(s);
		//give the app the ability to proccess events
		QApplication::processEvents();
	}
	cPJ.showSupports(previousStatus);
}


//持续的目录
void B9Edit::SetDir(QString dir)
{
    QSettings settings;
    settings.setValue("WorkingDir", dir);
}
QString B9Edit::GetDir()
{
    QSettings settings;
    if(settings.value("WorkingDir").toString() == "")
    {
        QDir currdir = QCoreApplication::applicationDirPath();
        #ifdef Q_OS_OSX
             if(currdir.dirName() == "MacOS")
             {
                 currdir.cdUp();
                 currdir.cdUp();
                 currdir.cdUp();
             }
        #endif
        #ifdef Q_OS_WIN
             currdir = QDir::home();
        #endif
        settings.setValue("WorkingDir", currdir.absolutePath());
    }

    return settings.value("WorkingDir").toString();
}
//视图编辑
void B9Edit::ShowSliceWindow()
{
	pEditView->show();

    if(!pEditView->isMaximized() && pEditView->pDrawingContext->pActiveImage)
        pEditView->resize(pEditView->pDrawingContext->pActiveImage->width() + pEditView->ui.horizontalSlider->width()-30, pEditView->pDrawingContext->pActiveImage->height() + 100);


    if(QApplication::desktop()->width() < pEditView->size().width()+100 || QApplication::desktop()->height() < pEditView->size().height()+100)
    {
            pEditView->showMaximized();
    }





}
void B9Edit::HideSliceWindow()
{
	pEditView->hide();
}

//convinience
void B9Edit::updateSliceIndicator()
{
	emit setSliceIndicator(QString().number(cPJ.getTotalLayers()));
    //也只启用了切片管理器 然后导出
	if(cPJ.getTotalLayers() <= 0)
	{
		ui.actionShow_Slice_Window->setEnabled(false);
		ui.actionExport_Slices->setEnabled(false);
	}
	else
	{
		ui.actionShow_Slice_Window->setEnabled(true);
		ui.actionExport_Slices->setEnabled(true);
	}
}
void B9Edit::updateWindowTitle()
{
	int i;
	if(!currJobFile.isEmpty())
	{
		QString onlyName(currJobFile);
		i = onlyName.size() - 1;
		while(onlyName.at(i) != '/' && i >= 0)
		{i--;}
		onlyName.remove(0,i+1);
		if(dirtied)
			onlyName.append("*");

        setWindowTitle(onlyName + " - B9Edit");
	}
	else
	{
		if(dirtied){
            setWindowTitle(sName + "* - B9Edit");}
		else{
            setWindowTitle(sName + " - B9Edit");
		}
	}
}

void B9Edit::PatchJobData(QImage* pNewImg, int slicenumber)
 {
	 dirtied = true;
	 updateWindowTitle();
	 if(cPJ.getTotalLayers() && pNewImg->width()*pNewImg->height() > 0)
	 {
        cPJ.setCurrentSlice(slicenumber);
        cPJ.crushCurrentSlice(pNewImg);
	 }
 }
void B9Edit::SetDirty()
{
	 dirtied = true;
	 updateWindowTitle();
}

//Protected
//事件
void B9Edit::closeEvent(QCloseEvent *event)
{
    //close SliceManager as well关闭
    pEditView->close();

    //当我们关闭窗口 - 我们要建立一个新的项目。
    //因为我们可能会再次打开窗口，我们希望有一个新的开始。
    //这将提示用户保存为好。
    newJob();
    event->accept();
}
void B9Edit::dragEnterEvent(QDragEnterEvent *event)
{
     if (event->mimeData()->hasUrls())
         event->acceptProposedAction();
}
void B9Edit::dropEvent(QDropEvent *event)
{
    openJob(event->mimeData()->urls()[0].toLocalFile());
    event->acceptProposedAction();
}

//Private
int B9Edit::PromptSaveOnQuit()
{
	QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("B9Edit"),
				tr("The document has been modified.\n"
					"Do you want to save your changes?"),
				QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
       
	if (ret == QMessageBox::Save)
	{
		return 1;
	}
	if (ret == QMessageBox::Discard)
	{
		return 2;
	}
	else if (ret == QMessageBox::Cancel)
	{
		return 0;
	}
	return 2;
}

