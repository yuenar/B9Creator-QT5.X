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

#include <QApplication>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QMessageBox>
#include "b9printercomm.h"
#include "b9updatemanager.h"
#include "qextserialport.h"
#include "qextserialenumerator.h"
#include "OS_Wrapper_Functions.h"

void B9PrinterStatus::reset(){
    //复位所有的状态变量为UNKNOWN未知
    iV1 = iV2 = iV3 = -1;
    m_eHomeStatus = HS_UNKNOWN;
    m_eProjStatus = PS_UNKNOWN;
    m_bProjCmdOn  = false;
    m_bCanControlProjector = false;
    m_bHasShutter = false;
    m_iPU = 635;
    m_iUpperZLimPU = -1;
    m_iCurZPosInPU = -1;
    m_iCurVatPercentOpen = -1;
    m_iLastHomeDiff = 0;

    m_iNativeX = 0;
    m_iNativeY = 0;
    m_iXYPizelSize = 0;
    m_iHalfLife = 2000;

    lastMsgTime.start();
    bResetInProgress = false;
    setLampHrs(-1); // unknown

	
}

void B9PrinterStatus::setVersion(QString s)
{
    // s必须是格式为“N1 N2 N3”，其中NX是整数。
    int i=0;
    QChar c;
    int p=0;
    QString v;
    while (i<s.length()){
        c = s.at(i);
        v+=c;
        if(s.at(i)==' ' || i==s.length()-1){
            p++;
            if(p==1){
                iV1 = v.toInt();
            }
            else if(p==2){
                iV2 = v.toInt();
            }
            if(p==3){
                iV3 = v.toInt();
            }
            v="";
        }
        i++;
    }
}

bool B9PrinterStatus::isCurrentVersion(){
    int iFileVersion = B9UpdateManager::GetLocalFileVersion(FIRMWAREHEXFILE);
    qDebug()<< "Local Firmware version: " << iFileVersion;
    qDebug()<< "Printer Firmware version" << getIntVersion();
    return getIntVersion() >= iFileVersion;
}

bool B9PrinterStatus::isValidVersion(){
    if(iV1<0||iV2<0||iV3<0)
        return false;
    return true;
}

QString B9PrinterStatus::getVersion(){
    if(iV1<0||iV2<0||iV3<0)
        return "?";
    return "v" + QString::number(iV1)+ "."+ QString::number(iV2)+ "."+ QString::number(iV3);
}

int B9PrinterStatus::getIntVersion(){
    if(iV1<0||iV2<0||iV3<0)
        return -1;
    return iV1*100 + iV2*10 +iV3;
}

//上载十六进制文件固件从当前端口 - 这将冻结程序。
bool B9FirmwareUpdate::UploadHex(QString sCurPort)
{

    QDir avrdir = QDir(CROSS_OS_GetDirectoryFromLocationTag("APPLICATION_DIR"));



    QString launchcommand = "avrdude ";
    #ifndef Q_OS_WIN
        launchcommand = "./avrdude ";
    #endif


    QString args = "-Cavrdude.conf -v -v -v -v -patmega328p -carduino -P" + sCurPort + " -b" + QString::number(FIRMWAREUPDATESPEED) + " -D -Uflash:w:\"" + sHexFilePath + "\":i";


    qDebug() << "B9FirwareUpdate: Calling AVRDude...";

    QCoreApplication::processEvents(); //处理所有等待事件。
    QProcess* myProcess = new QProcess(this);
    qDebug() << "Looking for avrdude, avrdude.conf, and .hex file in working dir:" <<avrdir.path();
    QDir::setCurrent(avrdir.path());
    myProcess->setProcessChannelMode(QProcess::MergedChannels);
    myProcess->start(launchcommand + args);



    if (!myProcess->waitForFinished(120000)) //允许120秒AVRDUDE超时之前将固件编程
    {
        qDebug() << "B9FirmwareUpdate: AVRDude Firmware Update Timed Out.";
        return false;
    }
    else
    {
        qDebug() << "B9FirmwareUpdate: Begin Firmware Update on Port: " + sCurPort;
        qDebug() << myProcess->readAll();
    }


    if(myProcess->exitCode() != 0)
    {
        qDebug() << "B9FirmwareUpdate: Firmware Update FAILED, exit code:" << QString::number(myProcess->exitCode());
        return false;
    }
    qDebug() << "B9FirmwareUpdate: Firmware Update Complete";

    return true;
}

B9PrinterComm::B9PrinterComm()
{
    m_bIsPrinting = false;
    pPorts = new QList<QextPortInfo>;
    pEnumerator = new QextSerialEnumerator();
    m_serialDevice = NULL;
    m_bCloneBlanks = false;
    m_Status.reset();
    m_iWarmUpDelayMS = 15000;
	bAttachVirtual= true;
    qDebug() << "B9Creator COMM Start";
    QTimer::singleShot(500, this, SLOT(RefreshCommPortItems())); // 每0.5秒一检查
}
B9PrinterComm::~B9PrinterComm()
{
    if(pPorts)delete pPorts;
    if(pEnumerator) pEnumerator->deleteLater();
    if(m_serialDevice) delete m_serialDevice;
    qDebug() << "B9Creator COMM End";
}

void B9PrinterComm::SendCmd(QString sCmd)
{
    if(m_serialDevice)m_serialDevice->write(sCmd.toLatin1()+'\n');
    if(sCmd == "r" || sCmd == "R") m_Status.setHomeStatus(B9PrinterStatus::HS_SEEKING);
    qDebug() << "SendCmd->" << sCmd;
}

void B9PrinterComm::watchDog()
{
    //我们期待一个有效的操作m_SerialDevice以及最后10秒内从打印机发出readReady信号
    //除非打印机正忙于寻找归属位置
    int iTimeLimit = 10000;
    if(m_Status.getHomeStatus() == B9PrinterStatus::HS_SEEKING) iTimeLimit = 90000;

    if( m_serialDevice != NULL && (m_Status.getLastMsgElapsedTime() <= iTimeLimit || m_Status.getLastMsgElapsedTime()>120000)){
        //仍与B9Creator联系
        startWatchDogTimer();
        emit updateConnectionStatus(MSG_CONNECTED);
        emit BC_ConnectionStatusDetailed("Connected to port: "+m_serialDevice->portName());
        return;
    }

    // 失去通讯
    if(m_Status.getHomeStatus()==B9PrinterStatus::HS_SEEKING) m_Status.setHomeStatus(B9PrinterStatus::HS_UNKNOWN);
    if(m_serialDevice!=NULL){
        //端口设备仍然存在，但没有得到任何消息，超时请重新启动端口
        if (m_serialDevice->isOpen())
        m_serialDevice->close();
        delete m_serialDevice;
        m_serialDevice = NULL;
    }
    //如果我们发现该端口很短的时间内再次失联，再次尝试连接
    qDebug() << "WATCHDOG:  LOST CONTACT WITH B9CREATOR!";

    emit updateConnectionStatus(MSG_SEARCHING);
    emit BC_ConnectionStatusDetailed("Lost Contact with B9Creator.  Searching...");
    m_Status.reset();
    handleLostComm();
    RefreshCommPortItems();
}

void B9PrinterComm::handleLostComm(){
    emit BC_LostCOMM();
}

void B9PrinterComm::startWatchDogTimer()
{
    //看门狗每10秒一唤醒
    QTimer::singleShot(10000, this, SLOT(watchDog())); //每10秒做次检查
}

void B9PrinterComm::RefreshCommPortItems()
{
    if(m_bIsPrinting)return; //假设在打印过程中始终保持连接状态。如果我们断开，看门狗定时器就没用了
    QString sCommPortStatus = MSG_SEARCHING;
    QString sCommPortDetailedStatus = MSG_SEARCHING;
    QString sPortName;
	int eTime = 1000;  
	if(bAttachVirtual&&m_serialDevice==NULL)
	{
		 if(OpenB9CreatorCommPort("virtual")){
                //已连接
                sCommPortStatus = MSG_CONNECTED;
                sCommPortDetailedStatus = "Connected on Port: "+m_serialDevice->portName();
                eTime = 5000;  // 在5秒内再次检查连接
                if(m_serialDevice && m_Status.isCurrentVersion())
                    startWatchDogTimer();  //启动B9Creator“撞车”看门狗
                return;
            }
		
	}
    //加载当前已枚举的可用端口
    *pPorts = pEnumerator->getPorts();

    if(m_serialDevice){
        //打印机是否还在连接吗？
        for (int i = 0; i < pPorts->size(); i++) {
        //检查每个现有的端口，看看我们的依然存在
        #ifdef Q_OS_LINUX
            if(pPorts->at(i).physName == m_serialDevice->portName()){
        #else
            if(pPorts->at(i).portName == m_serialDevice->portName()){
        #endif
                //我们仍处于连接状态，设置一个计时器在5秒钟，然后退出再次检查
                QTimer::singleShot(5000, this, SLOT(RefreshCommPortItems()));
                return;
            }
        }
        //我们失去了以往的连接，应该删除此端口连接
        if (m_serialDevice->isOpen())
            m_serialDevice->close();
        delete m_serialDevice;
        m_serialDevice = NULL;
        m_Status.reset();
        qDebug() << sCommPortStatus;
        emit updateConnectionStatus(sCommPortStatus);
        sCommPortDetailedStatus = "Lost Comm on previous port. Searching...";
        emit BC_ConnectionStatusDetailed("Lost Comm on previous port. Searching...");
        handleLostComm();
    }

    //现在我们搜索B9Creator
   //未连接状态下每秒中搜索一次

    sNoFirmwareAurdinoPort = "";  // 扫描端口重置字符串为空
    if(pPorts->size()>0){
        // 一些端口可用，他们是B9Creator吗？
        qDebug() << "Scanning For Serial Port Devices (" << pPorts->size() << "found )";
        for (int i = 0; i < pPorts->size(); i++) {
            //COMMENTED BECAUSE ITS ANNOYYING...
            //qDebug() << "  port name   " << pPorts->at(i).portName;
            //qDebug() << "  locationInfo" << pPorts->at(i).physName;
         #ifndef Q_OS_LINUX
            //注意：我们只相信friendName，VENDORID和使用的productID Windows和OS_X
            //COMMENTED BECAUSE ITS ANNOYYING...
            qDebug() << "  description " << pPorts->at(i).friendName;
            qDebug() << "  vendorID    " << pPorts->at(i).vendorID;
            qDebug() << "  productID   " << pPorts->at(i).productID;
         #endif
         #ifdef Q_OS_LINUX
            // linux的ID端口名即为物理设备名
            sPortName = pPorts->at(i).physName;
            // 我们通过是否包含ttyA来过滤端口名
            if(pPorts->at(i).portName.left(4) == "ttyA" && OpenB9CreatorCommPort(sPortName)){
         #else
            // Windows和OSX使用端口名即为ID端口名
            sPortName = pPorts->at(i).portName;


            // 我们通过是否与提供的厂商ID值 9025 (Arduino)匹配来过滤端口值
            if(pPorts->at(i).vendorID==9025 && OpenB9CreatorCommPort(sPortName)){
         #endif
                //已连接!
                sCommPortStatus = MSG_CONNECTED;
                sCommPortDetailedStatus = "Connected on Port: "+m_serialDevice->portName();
                eTime = 5000;  // Re-check connection again in 5 seconds
                if(m_serialDevice && m_Status.isCurrentVersion())startWatchDogTimer();  //启动B9Creator“撞车”看门狗
                break;
            }
        }
        bool bUpdateFirmware = false;
        if( m_serialDevice==NULL && sNoFirmwareAurdinoPort!=""){
            //我们没有找到有效的固件B9Creator，但我们却找到一个Arduino
            //我们假设这是一个新B9Creator，需要固件！
            //但是，我们不会上传固件，除非m_bCloneBlanks是真的！
            if(m_bCloneBlanks){
                // Ask before we clone...
                QMessageBox vWarn(QMessageBox::Warning,"Upload Firmware?", "A new B9Creator appears to be connected.  If this is a newly assembled B9Creator and no other Arduino devices are in use then you should upload the firmware.\n\nProcced with Firmware upload?",QMessageBox::Yes|QMessageBox::No);
                vWarn.setDefaultButton(QMessageBox::No);
                if(vWarn.exec()==QMessageBox::No){
                    bUpdateFirmware = false;
                }
                else
                {
                    //克隆它
                    qDebug() << "\"Clone Firmware\" Option is enabled.  Attempting Firmware Upload to possible B9Creator found on port: "<< sNoFirmwareAurdinoPort;
                    bUpdateFirmware = true;
                    sPortName = sNoFirmwareAurdinoPort;
                }
                m_bCloneBlanks = false;
            }
            else {
                qDebug() << "\"Clone Firmware\" Option is disabled.  No Firmware upload attempted on possible B9Creator found on port: " << sNoFirmwareAurdinoPort;
                bUpdateFirmware = false;
            }
        }
        else if (m_serialDevice!=NULL && !m_Status.isCurrentVersion()){
            // 我们发现有错误的固件版本B9Creator，更新！
            qDebug() << "Incorrect Firmware version found on connected B9Creator"<< sPortName << "  Attempting B9Creator Firmware Update";
            bUpdateFirmware = true;
            if(m_serialDevice!=NULL) {
                m_serialDevice->flush();
                m_serialDevice->close();

                delete m_serialDevice;

            }
            m_bCloneBlanks = false;
            m_serialDevice = NULL;
            m_Status.reset();
        }
        if(bUpdateFirmware){
            //更新sPortName设备上固件
            emit updateConnectionStatus(MSG_FIRMUPDATE);
            emit BC_ConnectionStatusDetailed("Updating Firmware on port: "+sPortName);
            B9FirmwareUpdate Firmware;
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            Firmware.UploadHex(sPortName);
            QApplication::restoreOverrideCursor();
            emit updateConnectionStatus(MSG_SEARCHING);
            emit BC_ConnectionStatusDetailed("Firmware Update Complete.  Searching...");
        }
        emit updateConnectionStatus(sCommPortStatus);
        emit BC_ConnectionStatusDetailed(sCommPortDetailedStatus);
    }
    QTimer::singleShot(eTime, this, SLOT(RefreshCommPortItems())); //如果已连接在5秒内再次检查，如果未连接每1秒一检查
}

bool B9PrinterComm::OpenB9CreatorCommPort(QString sPortName)
{
    if(m_serialDevice!=NULL) qFatal("Error:  We found an open port handle that should have been deleted!");




    // 尝试建立与B9Creator串口连接
	if(sPortName=="virtual")
	{
		m_serialDevice = new QVirtualSerialPort(QextSerialPort::EventDriven, this);
	}else{
		m_serialDevice = new QextSerialPort(sPortName, QextSerialPort::EventDriven, this);
	}
    if (m_serialDevice->open(QIODevice::ReadWrite) == true) {
        m_serialDevice->setBaudRate(BAUD115200);
        m_serialDevice->setDataBits(DATA_8);
        m_serialDevice->setParity(PAR_NONE);
        m_serialDevice->setStopBits(STOP_1);
        m_serialDevice->setFlowControl(FLOW_OFF);
        m_serialDevice->setDtr(true);   // 重置Arduino
        m_serialDevice->setDtr(false);

        connect(m_serialDevice, SIGNAL(readyRead()), this, SLOT(ReadAvailable()));
        qDebug() << "Opened Comm port:" << sPortName;
    }
    else {
        //设备打开失败
        if(m_serialDevice!=NULL) delete m_serialDevice;
        m_serialDevice = NULL;
        m_Status.reset();
        qDebug() << "Failed to open Comm port:" << sPortName;
        return false;
    }

    //延迟可达5秒，而我们等待来自打印机的响应
    QTime delayTime = QTime::currentTime().addSecs(5);
    while( QTime::currentTime() < delayTime && !m_Status.isValidVersion() )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    if(!m_Status.isValidVersion()){
        if(m_serialDevice!=NULL) {
            m_serialDevice->flush();
            m_serialDevice->close();
            delete m_serialDevice;
        }
        m_serialDevice = NULL;
        m_Status.reset();
        qDebug() << "Found a possible Arduino Device, perhaps a B9Creator without firmware loaded, on Port: " << sPortName;
        sNoFirmwareAurdinoPort = sPortName;
        return false;
    }
    return true;
}

QString B9PrinterComm::errorString(){
    return m_serialDevice->errorString();
}

void B9PrinterComm::ReadAvailable() {
    if(m_serialDevice==NULL) qFatal("Error:  slot 'ReadAvailable()' but NULL Port Handle");

    m_Status.resetLastMsgTime();  //看门狗更新

    if(m_Status.getHomeStatus() == B9PrinterStatus::HS_SEEKING) {
        m_Status.setHomeStatus(B9PrinterStatus::HS_UNKNOWN);
        // 如果我们接收数据，我们必须不再寻求主页
        //我们将状态设置为HS_FOUND一旦我们收到“X”异步广播
    }

    QByteArray ba = m_serialDevice->readAll();  //可用原始数据读取块

    //我们处理的原始数据的一行时，牢记它们可跨越多个块传播。
    int iCurPos = 0;
    int iLampHrs = -1;
    int iInput = -1;
    char c;
    while(iCurPos<ba.size()){
        c = ba.at(iCurPos);
        iCurPos++;
        if(c!='\r') m_sSerialString+=QString(c);
        if(c=='\n'){
            //行读取完成，开始操作数据

           if(m_sSerialString.left(1) != "P" && m_sSerialString.left(1) != "L" && m_sSerialString.length()>0){
                //我们只放出这一数据用来显示和log记录
                if(m_sSerialString.left(1) == "C"){
                    qDebug() << m_sSerialString.right(m_sSerialString.length()-1) << "\n";
                }
                else{
                    emit BC_RawData(m_sSerialString);
                    qDebug() << m_sSerialString << "\n";
                }
            }

            int iCmdID = m_sSerialString.left(1).toUpper().toLatin1().at(0);
            QMessageBox vWarn(QMessageBox::Critical,"Runaway X Motor!","X Motor Failure Detected, Code:  " + m_sSerialString +"\nClose program and troubleshoot hardware.");
            switch (iCmdID){
            case 'U':  // 编码器的机械故障？
                SendCmd("S"); //自停装置
                SendCmd("P0");//停止投影仪
                qDebug() << "WARNING:  Printer has sent 'U' report, runaway X Motor indication: " + m_sSerialString << "\n";
                vWarn.exec();
                break;
            case 'Q':
                //打印机厌倦了等待命令和关闭投影机
                //我们可能永远都不会认为这是一件坏事
                //（就像我们已经有crashed或在印刷过程中被关闭）
                //因此，如果我们得到它，我们就发布到日志中，并等待超时纠正。
                qDebug() << "WARNING:  Printer has sent 'Q' report, lost comm with host." << "\n";
                break;
            case 'P':  //考虑投影机状态
                iInput = m_sSerialString.right(m_sSerialString.length()-1).toInt();
                if(iInput!=1)iInput = 0;
                if(handleProjectorBC(iInput)){
                    //投影机的状态改变
                    emit BC_RawData(m_sSerialString);
                    qDebug() << m_sSerialString << "\n";
                }
                break;
            case 'L':  // 考虑投影机灯泡使用时长更新
                iLampHrs = m_sSerialString.right(m_sSerialString.length()-1).toInt();
                if(m_Status.getLampHrs()!= iLampHrs){
                    m_Status.setLampHrs(iLampHrs);
                    emit BC_ProjectorStatusChanged();
                    emit BC_RawData(m_sSerialString);
                    qDebug() << m_sSerialString << "\n";
                }
                break;

            case 'X':  //利用不同偏移量找到主页
                m_Status.setHomeStatus(B9PrinterStatus::HS_FOUND);
                m_Status.setLastHomeDiff(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_HomeFound();
                break;

            case 'R':  //需要重置吗
                iInput = m_sSerialString.right(m_sSerialString.length()-1).toInt();
                if(iInput==0) m_Status.setHomeStatus(B9PrinterStatus::HS_FOUND);
                else m_Status.setHomeStatus(B9PrinterStatus::HS_UNKNOWN);
                break;

            case 'K':  // 当前灯泡1/2寿命已使用
                m_Status.setHalfLife(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_HalfLife(m_Status.getHalfLife());
                break;

            case 'D':  // 目前本地的X投影机的分辨率
                m_Status.setNativeX(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_NativeX(m_Status.getNativeX());
                break;

            case 'E':  // 目前本地Y投影机的分辨率
                m_Status.setNativeY(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_NativeY(m_Status.getNativeY());
                break;

            case 'H':  // 当前XY像素大小
                m_Status.setXYPixelSize(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_XYPixelSize(m_Status.getXYPixelSize());
                break;

            case 'I':  // 当前的PU
                m_Status.setPU(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_PU(m_Status.getPU());
                break;

            case 'A':  // 投影机控制功能
                m_Status.setProjectorRemoteCapable(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_ProjectorRemoteCapable(m_Status.isProjectorRemoteCapable());
                break;

            case 'J':  // 投影机快门功能
                m_Status.setHasShutter(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_HasShutter(m_Status.hasShutter());
                break;

            case 'M':  // 在PUs中当前的Z轴上限
                m_Status.setUpperZLimPU(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_UpperZLimPU(m_Status.getUpperZLimPU());
                break;

            case 'Z':  //当前Z位置更新
                m_Status.setCurZPosInPU(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_CurrentZPosInPU(m_Status.getCurZPosInPU());
                break;

            case 'S':  // 当前的Vat(Shutter)百分比位置更新
                m_Status.setCurVatPercentOpen(m_sSerialString.right(m_sSerialString.length()-1).toInt());
                emit BC_CurrentVatPercentOpen(m_Status.getCurVatPercentOpen());
                break;

            case 'C':  // 注解
                emit BC_Comment(m_sSerialString.right(m_sSerialString.length()-1));
                break;

            case 'F':  // 打印发布周期结束
                emit BC_PrintReleaseCycleFinished();
                break;

            case 'V':  //版本
                m_Status.setVersion(m_sSerialString.right(m_sSerialString.length()-1).trimmed());
                emit BC_FirmVersion(m_Status.getVersion());
                break;

            case 'W':  // 模型
                m_Status.setModel(m_sSerialString.right(m_sSerialString.length()-1).trimmed());
                emit BC_ModelInfo(m_Status.getModel());
                break;

            case 'Y':  // 在PU中当前的Z轴原点
                // ignored
                break;

            default:
                qDebug() <<"WARNING:  IGNORED UNKNOWN CMD:  " << m_sSerialString << "\n";
                break;
            }
            m_sSerialString=""; //处理，清除它为下一行开辟空间
        }
    }
}

void B9PrinterComm::setProjectorPowerCmd(bool bPwrFlag){
    if(bPwrFlag){
        SendCmd("P1"); // 打开命令
        m_Status.setProjectorStatus(B9PrinterStatus::PS_TURNINGON);
        if(m_bIsMirrored) SendCmd("P4"); else SendCmd("P3");
    }
    else {
        SendCmd("P0"); // 打开命令
        m_Status.setProjectorStatus(B9PrinterStatus::PS_COOLING);
    }
    m_Status.resetLastProjCmdTime();
    emit BC_ProjectorStatusChanged();
}

bool B9PrinterComm::handleProjectorBC(int iBC){
    bool bStatusChanged = false;
    if(m_Status.isProjectorPowerCmdOn() && iBC == 0){
        // 投影机已接受命令应开启，但目前报告仍是关闭
        switch(m_Status.getProjectorStatus()){
        case B9PrinterStatus::PS_OFF:
        case B9PrinterStatus::PS_UNKNOWN:
        case B9PrinterStatus::PS_TIMEOUT:
        case B9PrinterStatus::PS_FAIL:
            setProjectorPowerCmd(true); //现在打开它
            bStatusChanged = true;
            break;
        case B9PrinterStatus::PS_TURNINGON:
            if(m_Status.getLastProjCmdElapsedTime()>45000){
                // 打开时间太长，有地方出错了！
                m_Status.setProjectorStatus(B9PrinterStatus::PS_TIMEOUT);
                emit BC_ProjectorFAIL();
                m_Status.cmdProjectorPowerOn(false);
                bStatusChanged = true;
            }
            break;
        case B9PrinterStatus::PS_WARMING:
        case B9PrinterStatus::PS_ON:
            // 强制（指令性）关机！停电或灯泡烧了吗？
            m_Status.setProjectorStatus(B9PrinterStatus::PS_FAIL);
            emit BC_ProjectorFAIL();
            m_Status.cmdProjectorPowerOn(false);
            bStatusChanged = true;
            break;
        case B9PrinterStatus::PS_COOLING:
            // 冷却完成
            m_Status.setProjectorStatus(B9PrinterStatus::PS_OFF);
            emit BC_ProjectorStatusChanged();
            bStatusChanged = true;
            break;
        default:
            // 无事可做
            break;
        }
    }
    else if(m_Status.isProjectorPowerCmdOn() && iBC == 1){
        // 投影仪接受命令为打开确实报告为打开
        switch(m_Status.getProjectorStatus()){
        case B9PrinterStatus::PS_COOLING:
        case B9PrinterStatus::PS_OFF:
        case B9PrinterStatus::PS_UNKNOWN:
        case B9PrinterStatus::PS_TIMEOUT:
        case B9PrinterStatus::PS_FAIL:
            //测试了那么多情况，实际上打开了吗？
            //我们能做的最多的就是设置状态，并报告
            m_Status.setProjectorStatus(B9PrinterStatus::PS_ON);
            emit BC_ProjectorStatusChanged();
            bStatusChanged = true;
            break;
        case B9PrinterStatus::PS_TURNINGON:
            // 打印机打开需要预热一下
            m_Status.setProjectorStatus(B9PrinterStatus::PS_WARMING);
            startWarmingTime.start();
            emit BC_ProjectorStatusChanged();
            bStatusChanged = true;
            break;
        case B9PrinterStatus::PS_WARMING:
            if(startWarmingTime.elapsed()>m_iWarmUpDelayMS){
                //预热完成，可以使用
                m_Status.setProjectorStatus(B9PrinterStatus::PS_ON);
                emit BC_ProjectorStatusChanged();
                bStatusChanged = true;
            }
            break;
        case B9PrinterStatus::PS_ON:
        default:
            // 没有改变，这很好
            break;
        }
    }
    else if(!m_Status.isProjectorPowerCmdOn() && iBC == 0){
        //投影仪接受命令为关闭确实报告为关闭
        switch(m_Status.getProjectorStatus()){
        case B9PrinterStatus::PS_UNKNOWN:
        case B9PrinterStatus::PS_TURNINGON:
        case B9PrinterStatus::PS_WARMING:
        case B9PrinterStatus::PS_ON:
        case B9PrinterStatus::PS_COOLING:
        case B9PrinterStatus::PS_TIMEOUT:
        case B9PrinterStatus::PS_FAIL:
            // 测试了那么多情况，现在确实关闭了
            m_Status.setProjectorStatus(B9PrinterStatus::PS_OFF);
            emit BC_ProjectorStatusChanged();
            bStatusChanged = true;
            break;
        case B9PrinterStatus::PS_OFF:
        default:
            // 没有改变，这很好
            break;
        }
    }
    else if(!m_Status.isProjectorPowerCmdOn() && iBC == 1){
        // 投影仪接受命令为关闭却是报告为打开
        switch(m_Status.getProjectorStatus()){
        case B9PrinterStatus::PS_COOLING:
            if(m_Status.getLastProjCmdElapsedTime()>30000){
                //关闭超时，重新发送关闭指令
                m_Status.setProjectorStatus(B9PrinterStatus::PS_ON);
                emit BC_ProjectorStatusChanged();
                bStatusChanged = true;
            }
            break;
        case B9PrinterStatus::PS_OFF:
            //用户必须手动切换它，然后打印机设置状态命令，并打开
            cmdProjectorPowerOn(true);
            setProjectorPowerCmd(true);
            bStatusChanged = true;
            break;

        case B9PrinterStatus::PS_TURNINGON:
        case B9PrinterStatus::PS_WARMING:
        case B9PrinterStatus::PS_ON:
        case B9PrinterStatus::PS_UNKNOWN:
        case B9PrinterStatus::PS_TIMEOUT:
        case B9PrinterStatus::PS_FAIL:
        default:
            setProjectorPowerCmd(false); //现在关闭它
            bStatusChanged = true;
            break;
        }
    }
    return bStatusChanged;
}
