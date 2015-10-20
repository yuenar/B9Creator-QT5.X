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

#ifndef B9PRINTERCOMM_H
#define B9PRINTERCOMM_H
#include <QObject>
#include <QTime>
#include <QtDebug>
#include "QVirtualSerialPort.h"

#define FIRMWAREUPDATESPEED 115200
#define FIRMWAREHEXFILE "B9Firmware.hex"
#define MSG_SEARCHING "Searching..."
#define MSG_CONNECTED "Connected"
#define MSG_FIRMUPDATE "Updating Firmware..."


class  QextSerialPort;
class  QextSerialEnumerator;
struct QextPortInfo;

/////////////////////////////////////////////////////////////////////////////
class B9PrinterStatus
{
public:
    enum ProjectorStatus{PS_OFF, PS_TURNINGON, PS_WARMING, PS_ON, PS_COOLING, PS_UNKNOWN, PS_TIMEOUT, PS_FAIL};
    enum HomeStatus{HS_SEEKING, HS_FOUND, HS_UNKNOWN};


    B9PrinterStatus(){reset();}
    void reset();

    QString getVersion();
    int getIntVersion();
    void setVersion(QString s);
    bool isCurrentVersion();
    bool isValidVersion();

    void cmdProjectorPowerOn(bool bOn){m_bProjCmdOn = bOn;}
    bool isProjectorPowerCmdOn(){return m_bProjCmdOn;}

    QString getModel(){return m_sModel;}
    void setModel(QString sModel){m_sModel = sModel;}

    bool isProjectorRemoteCapable(){ return m_bCanControlProjector;}
    void setProjectorRemoteCapable(bool bIsCapable){m_bCanControlProjector=bIsCapable;}

    bool hasShutter(){ return m_bHasShutter;}
    void setHasShutter(bool bHS){m_bHasShutter=bHS;}

    HomeStatus getHomeStatus() {return m_eHomeStatus;}
    void setHomeStatus(HomeStatus eHS) {m_eHomeStatus = eHS;}

    ProjectorStatus getProjectorStatus() {return m_eProjStatus;}
    void setProjectorStatus(ProjectorStatus ePS) {m_eProjStatus = ePS;}

    int getLampHrs(){return m_iLampHours;}
    void setLampHrs(int iLH){m_iLampHours = iLH;}

    void setPU(int iPU){m_iPU = iPU;}
    int getPU(){return m_iPU;}

    void setHalfLife(int iHL){m_iHalfLife = iHL;}
    int getHalfLife(){return m_iHalfLife;}

    void setNativeX(int iX){m_iNativeX = iX;}
    int getNativeX(){return m_iNativeX;}

    void setNativeY(int iY){m_iNativeY = iY;}
    int getNativeY(){return m_iNativeY;}

    void setXYPixelSize(int iPS){m_iXYPizelSize = iPS;}
    int getXYPixelSize(){return m_iXYPizelSize;}

    void setUpperZLimPU(int iZLimPU){m_iUpperZLimPU = iZLimPU;}
    int getUpperZLimPU(){return m_iUpperZLimPU;}

    void setCurZPosInPU(int iCZ){m_iCurZPosInPU = iCZ;}
    int getCurZPosInPU(){return m_iCurZPosInPU;}

    void setCurVatPercentOpen(int iPO){m_iCurVatPercentOpen = iPO;}
    int getCurVatPercentOpen(){return m_iCurVatPercentOpen;}

    int getLastHomeDiff() {return m_iLastHomeDiff;}
    void setLastHomeDiff(int iDiff) {m_iLastHomeDiff = iDiff;}

    void resetLastMsgTime() {lastMsgTime.start();}
    int getLastMsgElapsedTime() {return lastMsgTime.elapsed();}

    void resetLastProjCmdTime() {lastProjCmdTime.start();}
    int getLastProjCmdElapsedTime() {return lastProjCmdTime.elapsed();}

private:
    QTime lastMsgTime; //更新于每一个串行读就绪信号
    bool bResetInProgress; // 在归属位置复位设置为真

    int iV1,iV2,iV3; // 版本值
    QString m_sModel; //产品型号说明
    bool m_bCanControlProjector; //如果打印机传入命令控制投影机，则设置为真
    bool m_bHasShutter; // 如果打印机传入命令关闭快门，则设置为真
    int m_iLastHomeDiff; // 当我们重置到主页，这个为Z的期望值
    HomeStatus m_eHomeStatus; // 主页是否已被固定
    ProjectorStatus m_eProjStatus; //那投影仪在干什么？
    bool m_bProjCmdOn;  //如果我们想要打开投影机时为真，关闭为假
    QTime lastProjCmdTime; //更新每一个串行cmd命令到投影机;
    int m_iLampHours;  // 报告的投影机灯泡的总时间

    int m_iPU;
    int m_iUpperZLimPU;
    int m_iCurZPosInPU;
    int m_iCurVatPercentOpen;
    int m_iNativeX;
    int m_iNativeY;
    int m_iXYPizelSize;
    int m_iHalfLife;
	
};

/////////////////////////////////////////////////////////////////////////////
class B9FirmwareUpdate : public QObject
{
    Q_OBJECT

    //使用AVRDUDE更新B9Creator的Arduino的固件
        // averdude和averdue.config必须存在
        // B9Firmware.hex文件必须存在
public:
    B9FirmwareUpdate()
    {
        sHexFilePath = FIRMWAREHEXFILE;
    }
    ~B9FirmwareUpdate(){}
    bool UploadHex(QString sCurPort);
private:
    QString sHexFilePath;
};
//该B9PrinterComm应当由主窗口一旦创建
//并通过与需要的信号/槽访问类共享
class B9PrinterComm : public QObject
{
    Q_OBJECT

public:
    B9PrinterComm();
    ~B9PrinterComm();

    bool isConnected(){return m_Status.isValidVersion();}
    void enableBlankCloning(bool bEnable){m_bCloneBlanks = bEnable;}

    void cmdProjectorPowerOn(bool bOn){m_Status.cmdProjectorPowerOn(bOn);}
    bool isProjectorPowerCmdOn(){return m_Status.isProjectorPowerCmdOn();}
    B9PrinterStatus::ProjectorStatus getProjectorStatus(){return m_Status.getProjectorStatus();}
    int getLampHrs(){return m_Status.getLampHrs();}
    int getPU(){return m_Status.getPU();}
    int getUpperZLimPU(){return m_Status.getUpperZLimPU();}
    int getHalfLife(){return m_Status.getHalfLife();}
    int getNativeX(){return m_Status.getNativeX();}
    int getNativeY(){return m_Status.getNativeY();}
    int getXYPixelSize() {return m_Status.getXYPixelSize();}
    int getLastHomeDiff() {return m_Status.getLastHomeDiff();}
    B9PrinterStatus::HomeStatus getHomeStatus() {return m_Status.getHomeStatus();}
    void setHomeStatus(B9PrinterStatus::HomeStatus eHS) {m_Status.setHomeStatus(eHS);}
    QString errorString();

signals:
    void updateConnectionStatus(QString sText); //Connected或者Searching两种状态
    void BC_ConnectionStatusDetailed(QString sText); //Connected或者Searching两种状态
    void BC_LostCOMM(); // 失去当前连接!
    void BC_RawData(QString sText);    // 原始数据
    void BC_Comment(QString sComment); //注释字符串
    void BC_HomeFound(); // 完成与复位，重新启用的控制器，等等。
    void BC_ProjectorStatusChanged(); //投影机的状态已经改变
    void BC_ProjectorFAIL(); //投影机经常性和指令性断电

    void BC_ModelInfo(QString sModel); // 打印机型号描述信息
    void BC_FirmVersion(QString sVersion); //打印机固件版本号
    void BC_ProjectorRemoteCapable(bool isCapable); // 投影机是否有能力进行远程控制？
    void BC_HasShutter(bool hasShutter); // 投影机具有机械快门
    void BC_PU(int); //更新打印机单元（微米*100）
    void BC_UpperZLimPU(int); //更新PU上Z轴上限值
    void BC_HalfLife(int); //更新投影机灯泡所剩使用寿命
    void BC_NativeX(int); //更新本地的X分辨率
    void BC_NativeY(int); //更新本地的Y分辨率
    void BC_XYPixelSize(int); //更新XY像素大小

    void BC_CurrentZPosInPU(int); //每当我们得到的Z位置的更新发送广播
    void BC_CurrentVatPercentOpen(int); //每当我们得到的还原位置的更新发送广播
    void BC_PrintReleaseCycleFinished();  //当我们收到的“F”通知打印机广播
public slots:
    void SendCmd(QString sCmd);
    void setProjectorPowerCmd(bool bPwrFlag); //调用发送投影机的电源开/关命令
    void setWarmUpDelay(int iDelayMS){m_iWarmUpDelayMS = iDelayMS;}
    void setMirrored(bool bIsMirrored){m_bIsMirrored = bIsMirrored;}

private slots:
    void ReadAvailable();
    void RefreshCommPortItems();
    void watchDog();  // 检查，看看我们收到来自Arduino的定期更新

public:
    bool m_bIsPrinting;
	bool bAttachVirtual;
private:
    QextSerialPort *m_serialDevice;
    QextSerialEnumerator *pEnumerator;		// 枚举寻找可用的通讯端口
    QList<QextPortInfo> *pPorts;			// 可用通讯端口列表
    QString sNoFirmwareAurdinoPort;         // 如果我们找到一个Arduino没有固件，将其设置为端口名作为标志
    bool m_bCloneBlanks;                    // 如果为假，我们不会烧固件到可能空白的Arduino板子

    QString m_sSerialString; // 用于读取可用来存储当前广播线
    B9PrinterStatus m_Status;

    bool OpenB9CreatorCommPort(QString sPortName);
    void startWatchDogTimer();
    void handleLostComm();
    bool handleProjectorBC(int iBC);
    QTime startWarmingTime;
    int m_iWarmUpDelayMS;
    bool m_bIsMirrored;
	
};
#endif // B9PRINTERCOMM_H
