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

#include "sliceset.h"
#include "slice.h"
#include "math.h"
#include "slicecontext.h"
#include "b9layoutprojectdata.h"
#include "crushbitmap.h"
#include <QColor>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <QFutureSynchronizer>


SliceSet::SliceSet(B9ModelInstance* pParentInstance)
{
	if(!pParentInstance)
	{
		qDebug() << "SliceSet Constructor - No Parent Clone Specified!";
	}
	pInstance = pParentInstance;
    raster = NULL;
    SetSingleModelCompressHint(false);
}

SliceSet::~SliceSet()
{
    delete raster;
}
void SliceSet::SetupRasturizer()
{
    if(raster == NULL)
    {
        raster = new SliceContext(NULL,pInstance->pData->pMain->ProjectData());
        raster->makeCurrent();
    }
}

void SliceSet::SetupFutureWorkers()
{
    int threads = 0;
    if(workerThreads.size() == 0)
    {
        threads = QThread::idealThreadCount();
        qDebug() << "SliceSet: Ideal Thread Count: " << threads;
        if(threads == -1)
        {
            threads = 2;
            qDebug() << "SliceSet: Ideal Thread Count Reset to 2";
        }

        for(int i = 0; i < QThread::idealThreadCount(); i++)
        {
            workerThreads.push_back(QFuture<void>());
        }
    }
}

void SliceSet::SetSingleModelCompressHint(bool hint)
{
    singleModelCompression = hint;
}


//告诉当前切片设置为就绪状态，在排队的“阵地”，以创建新的切片。
void SliceSet::QueNewSlice(double realAltitude, int layerIndx)
{
    SliceRequest newReq;
    newReq.altitude = realAltitude;
    newReq.layerIndx = layerIndx;

    SliceRequests.push(newReq);
}

//告诉切片设置进行更新的具有异步程序，使用非阻塞信号
//指定的工程文件会告诉算法如何渲染和压缩到另一个工程文件。
//这个功能不应该阻塞
Slice* SliceSet::ParallelCreateSlices(bool &slicesInTransit, CrushedPrintJob* toJob)
{
    unsigned int i;
    Slice* pSlice = NULL;
    SliceRequest req;
    int buzythreads = 0;
    SetupFutureWorkers(); // 确保我们的线程数符合我们的核心（按照建议更换核数）
    SetupRasturizer();

    //部署线程，智能分配工作
    for(i = 0; i < workerThreads.size(); i++)
    {
        if(workerThreads[i].isRunning()){
            buzythreads++;
            continue;
        }

        //压缩
        if(RasturizedSlices.size() && toJob)
        {

           pSlice = RasturizedSlices.front();
           if(!pSlice->inProccessing)
           {
               RasturizedSlices.pop();

               pSlice->inProccessing = true;
               CompressedSlices.push(pSlice);
               workerThreads[i] = QtConcurrent::run(this, &SliceSet::AddSliceToJob, pSlice, toJob);

               continue;
           }
        }

        //切片
        if((SliceRequests.size()) && (SlicedSlices.size() < 20))//缓冲区最多存放19个切片
        {
            req = SliceRequests.front();
            SliceRequests.pop();

            pSlice = new Slice(req.altitude,req.layerIndx);
            pSlice->inProccessing = true;
            SlicedSlices.push(pSlice);
            workerThreads[i] = QtConcurrent::run(this, &SliceSet::ComputeSlice, pSlice);
            continue;
        }


    }
    //光栅化
    if(SlicedSlices.size() && toJob)
    {
        pSlice = SlicedSlices.front();
        if(!pSlice->inProccessing)
        {
            SlicedSlices.pop();
            pSlice->inProccessing = true;
            RasterizeSlice(pSlice);
            RasturizedSlices.push(pSlice);
        }
    }


    if(toJob)
    {
        if(SliceRequests.size() || SlicedSlices.size() || RasturizedSlices.size() || CompressedSlices.size())
            slicesInTransit = true;
        else
            slicesInTransit = false;
        if(CompressedSlices.size() && !CompressedSlices.front()->inProccessing){
            pSlice = CompressedSlices.front();
            CompressedSlices.pop();
            return pSlice;}
        else
            return NULL;
    }
    else
    {
        if(SliceRequests.size() || SlicedSlices.size())
            slicesInTransit = true;
        else
            slicesInTransit = false;

        if(SlicedSlices.size() && !SlicedSlices.front()->inProccessing){
            pSlice = SlicedSlices.front();
            SlicedSlices.pop();
            return pSlice;}
        else
            return NULL;
    }


}


//为给定切片生成所有几何图样。
void SliceSet::ComputeSlice(Slice* slice)
{
    slice->GenerateSegments(pInstance);//实际生成切片内的分段

    slice->SortSegmentsByX();//在X方向排序分段

    slice->ConnectSegmentNeighbors();//连接相邻分段

    slice->GenerateLoops();//产生循环结构

    slice->inProccessing = false;
}

//光栅化给定切片的空隙，并填写图像。
void SliceSet::RasterizeSlice(Slice* slice)
{
    B9LayoutProjectData* projectData = pInstance->pData->pMain->ProjectData();
    unsigned int xres = projectData->GetResolution().x();
    unsigned int yres = projectData->GetResolution().y();

    //渲染到2个图层，填充和虚化。
    raster->makeCurrent();
    raster->SetSlice(slice);
    slice->pImg = new QImage(raster->renderPixmap(xres,yres).toImage());
    slice->inProccessing = false;

}

//采用层叠从主工程，或者是与位图和重新压缩到主工程。
void SliceSet::AddSliceToJob(Slice* rasSlice, CrushedPrintJob* job)
{

    B9LayoutProjectData* projectData = pInstance->pData->pMain->ProjectData();
    unsigned int xres = projectData->GetResolution().x();
    unsigned int yres = projectData->GetResolution().y();
    QPainter painter;
    QImage img(xres,yres,QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::black);

    SubtractVoidFromFill(rasSlice->pImg);

    //我们只需要层叠覆盖切片，如果有多个模型
    if(!singleModelCompression)
    {
        job->inflateSlice(rasSlice->layerIndx, &img);
        if(img.size() == QSize(0,0))
        {
            img = QImage(xres,yres,QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::black);
        }

        //或图像组合在一起
        painter.begin(rasSlice->pImg);
        painter.setRenderHint(QPainter::Antialiasing,false);
        painter.setCompositionMode(QPainter::CompositionMode_Plus);
        painter.drawImage(0,0,img);
        painter.end();
    }

    job->crushSlice(rasSlice->layerIndx, rasSlice->pImg);

    rasSlice->inProccessing = false;   
}



//减去无效图像从填充的图像。
//填充的图像更正后返回。
void SliceSet::SubtractVoidFromFill(QImage *img)
{
    B9LayoutProjectData* projectData = pInstance->pData->pMain->ProjectData();
    unsigned int xres = projectData->GetResolution().x();
    unsigned int yres = projectData->GetResolution().y();
    QRgb pickedcolor;
    int x,y;

    for(x = 0; x < xres; x++)
    {
        for(y = 0; y < yres; y++)
        {
            pickedcolor = img->pixel(x,y);
            if(qRed(pickedcolor) || qGreen(pickedcolor))
            {
                int result = qRed(pickedcolor) - qGreen(pickedcolor);
                if(result > 0)
                {
                    result = 255;
                }else result = 0;
                img->setPixel(x,y,QColor(result,0,0).rgb());
            }
        }
    }
}




