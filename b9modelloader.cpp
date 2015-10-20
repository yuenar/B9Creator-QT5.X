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

#include "b9modelloader.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>




//用法：创建一个b9modelloader对象，如果readyread返回true，开始使用loadnexttri()
B9ModelLoader::B9ModelLoader( QString filename, bool &readyRead, QObject *parent) :
    QObject(parent)
{   
    qDebug() << "B9ModelLoader: Initializing..";
    this->byteCount = 0;
    this->triCount = 0;
    this->lastError = "";
    this->lastpercent = 0.0;
    QFileInfo info(filename);

    fileType = DetermineFileType(filename, readyRead);

    if(readyRead == false)//它可以是文件可能不能被打开.
        return;

    if(fileType == "BIN_STL")
    {
        PrepareSTLForReading(filename,readyRead);
    }
    else if(fileType == "ASCII_STL")
    {
        PrepareSTLForReading(filename,readyRead);
    }
    else if(fileType == "UNCOMPRESSED_AMF")
    {

    }




}

B9ModelLoader::~B9ModelLoader()
{
    binfile.close();
    asciifile.close();
}

QString B9ModelLoader::DetermineFileType(QString filename, bool &readyRead)
{
    QFileInfo info(filename);
    QString suffix = info.suffix();
    QFile parsefile(filename);
    QTextStream parseStream(&parsefile);
    QString asciiTest;
    unsigned int i = 0;


    if(suffix.toLower() == "stl")
    {
        //看看我们是否能够打开文件
        if(!parsefile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            lastError = "Unable to open file:" + filename;
            qDebug() << "B9UpdateLoader: Unable to open file: " << filename;
            readyRead = false;
            return "";
        }

        //我们可以随即得到字节数
        byteCount = parsefile.size();

        //现在确定STL是否是二进制或ASCII
        while(i < 50)
        {
            parseStream >> asciiTest;
            if(asciiTest.toLower().trimmed() == "facet")
            {
                parsefile.close();
                qDebug() << "B9ModelLoader: the file is an ascii stl";
                readyRead = true;
                return "ASCII_STL";
            }
            i++;
        }
        //如果我们没有发现在50个字字符串“层面“，那么假设它是一个bin文件.
        qDebug() << "B9ModelLoader: the file is a binary stl";
        readyRead = true;
        parsefile.close();
        return "BIN_STL";
    }
    else if(suffix.toLower() == "amf")
    {

        readyRead = false;//TODO AMF NOT IMPLEMENTED...
        return "UNCOMPRESSED_AMF";
    }

    return "";
}






void B9ModelLoader::PrepareSTLForReading(QString filename, bool &readyRead)
{
    QString buff;
    unsigned int i = 0;
    unsigned int pos;


    if(fileType == "ASCII_STL")//ascii
    {
        //准备ASCII文件进行读取。
        asciifile.setFileName(filename);
        asciifile.open(QIODevice::ReadOnly | QIODevice::Text);
        asciiStream.setDevice(&asciifile);
        //我们需要得到asciistream中的位置
       //它是关于阅读的第一个“层面”字段
        while(i < 50)
        {
            pos = asciiStream.pos();
            asciiStream >> buff;
            if(buff.toLower().trimmed() == "facet")
            {
                asciiStream.seek(pos);//相反之前，我们阅读层面。
                readyRead = true;
                return;
            }
            i++;
        }
        lastError = "No facet string found in stl file.";
        qDebug() << "B9ModelLoader: No facet string found in stl file.";
        readyRead = false;
    }
    else if(fileType == "BIN_STL")//二进制文件
    {
        //准备二进制文件进行读取
        binfile.setFileName(filename);
        binfile.open(QIODevice::ReadOnly);
        //从头部获得信息。
        if(ReadBinHeader())
        {
            //确保三角数据数量与文件的大小相匹配
            if(CheckBinFileValidity())
            {
                readyRead = true;
            }
            else
            {
                lastError = "Incomplete or non-standard file";
                qDebug() << "B9ModelLoader: Incomplete or non-standard file.";
                readyRead = false;
            }
        }
        else
        {
            lastError = "Error reading binary file header.";
            qDebug() << "B9ModelLoader: Error reading binary file header.";
            readyRead = false;
        }
    }
    else
    {
        qDebug() << "B9ModelLoader: File Type Confusion";
        readyRead = false;
    }
}



bool B9ModelLoader::ReadBinHeader()
{
    int triCountSuccess;
    binfile.seek(80);//跳过头部
    triCountSuccess = binfile.read((char *)&triCount,4);

    if(triCountSuccess == 4){
        qDebug() << "B9ModelLoader: binary header triangle count: " << triCount;
        return true;
    }
    else
    {
        qDebug() << "B9ModelLoader: error reading binary header triangle count";
        return false;
    }
}


bool B9ModelLoader::CheckBinFileValidity()
{
    //每个层面应该50bytes，如果有头部则84字节。
    if(byteCount >= (triCount*50 + 84))
        return true;
    else
        return false;
}


//返回一个指向加载下一个三角数据的指针，
//完成后，用户必须删除指针！
bool B9ModelLoader::LoadNextTri(STLTri* &tri, bool &errorFlag)
{

    STLTri* pNewTri;
    QString buff;
    unsigned int bytesRead;


    //如果是ascii
    if(fileType == "ASCII_STL")
    {
        asciiStream >> buff;//（缓存）吃入层面
        if(buff == "endsolid" || asciiStream.atEnd())
        {
            tri = NULL;
            errorFlag = false;
            return false;
        }
        pNewTri = new(std::nothrow) STLTri;
        if(pNewTri == NULL)
        {
            qDebug() << "B9ModelLoader: Not Enough Memory to allocate new triange";
            errorFlag = true;
            lastError = "Not Enough Memory to allocate new triange";
            return false;
        }

        //跳过"法线"
        asciiStream >> buff;
        asciiStream >> pNewTri->nx;
        asciiStream >> pNewTri->ny;
        asciiStream >> pNewTri->nz;
        asciiStream.skipWhiteSpace();

        asciiStream >> buff; //跳过"outer"
        asciiStream >> buff;//跳过"loop"
        asciiStream.skipWhiteSpace();
        asciiStream >> buff;//吃入“顶点”
        asciiStream >> pNewTri->x0;
        asciiStream >> pNewTri->y0;
        asciiStream >> pNewTri->z0;
        asciiStream.skipWhiteSpace();
        asciiStream >> buff;//吃入“顶点”
        asciiStream >> pNewTri->x1;
        asciiStream >> pNewTri->y1;
        asciiStream >> pNewTri->z1;
        asciiStream.skipWhiteSpace();
        asciiStream >> buff;//吃入“顶点”
        asciiStream >> pNewTri->x2;
        asciiStream >> pNewTri->y2;
        asciiStream >> pNewTri->z2;
        asciiStream.skipWhiteSpace();
        asciiStream >> buff;//吃入末端循环
        asciiStream.skipWhiteSpace();
        asciiStream >> buff;//吃入末端层面
        asciiStream.skipWhiteSpace();


        tri = pNewTri;



    }
    else if(fileType == "BIN_STL")//从二进制文件读入三角数据.
    {
        bytesRead = 0;

        if(binfile.atEnd())
        {
            tri = NULL;
            errorFlag = false;
            return false;
        }
        pNewTri = new(std::nothrow) STLTri;
        if(pNewTri == NULL)
        {
            qDebug() << "B9ModelLoader: Not Enough Memory to allocate new triange";
            lastError = "Not Enough Memory to allocate new triange";
            errorFlag = true;
            return false;
        }

        bytesRead += binfile.read((char *)&pNewTri->nx,4);
        bytesRead += binfile.read((char *)&pNewTri->ny,4);
        bytesRead += binfile.read((char *)&pNewTri->nz,4);

        bytesRead += binfile.read((char *)&pNewTri->x0,4);
        bytesRead += binfile.read((char *)&pNewTri->y0,4);
        bytesRead += binfile.read((char *)&pNewTri->z0,4);
        bytesRead += binfile.read((char *)&pNewTri->x1,4);
        bytesRead += binfile.read((char *)&pNewTri->y1,4);
        bytesRead += binfile.read((char *)&pNewTri->z1,4);
        bytesRead += binfile.read((char *)&pNewTri->x2,4);
        bytesRead += binfile.read((char *)&pNewTri->y2,4);
        bytesRead += binfile.read((char *)&pNewTri->z2,4);



        //跳过属性字节计数器;
        binfile.seek(binfile.pos()+ 2);

        if(bytesRead != 48)
        {
            qDebug() << "B9ModelLoader: end of file before triangle done reading";
            tri = NULL;
            errorFlag = true;
            return false;
        }
        tri = pNewTri;
    }
    else
    {
        //amf....
    }


    //只发送信号上的进度条按1％的增长，2％，3％，等
    float t = GetPercentDone();
    if(t > (lastpercent + 0.01))
    {
        lastpercent = t;
        emit PercentCompletedUpdate(100.0*lastpercent,100);
        QCoreApplication::processEvents();

    }




    errorFlag = false;
    return true;
}

float B9ModelLoader::GetPercentDone()
{

    if(fileType == "ASCII_STL")
    {
        return float(asciifile.pos())/ byteCount;
    }
    else if(fileType == "BIN_STL")
    {
        return float(binfile.pos())/ byteCount;
    }
    else
    {
        //amf 百分比
    }

    return 0.0;
}








