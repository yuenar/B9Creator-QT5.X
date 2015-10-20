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

#include <QMessageBox>
#include <QTimer>
#include "b9print.h"
#include "ui_b9print.h"

B9Print::B9Print(B9Terminal *pTerm, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::B9Print)
{
    m_pTerminal = pTerm;
    if(m_pTerminal == NULL) qFatal("FATAL Call to B9Creator with null B9Terminal Pointer");
    ui->setupUi(this);

    //清除旧信息
    ui->lineEditSerialStatus->setText("");
    ui->lineEditProjectorOutput->setText("");

    m_iTbase = m_iTover = 0;
    m_iTattach = 0;
    m_iNumAttach = 1;
    m_iXOff = m_iYOff =0;
    m_iPrintState = PRINT_NO;
    m_iPaused = PAUSE_NO;
    m_bAbort = false;
    m_sAbortMessage = "Unknown Abort";
    m_iCurLayerNumber = 0;
    m_dLayerThickness = 0.0;
    m_iLastLayer = 0;

    connect(m_pTerminal, SIGNAL(updateConnectionStatus(QString)), this, SLOT(on_updateConnectionStatus(QString)));
    connect(m_pTerminal, SIGNAL(updateProjectorOutput(QString)), this, SLOT(on_updateProjectorOutput(QString)));
    connect(m_pTerminal, SIGNAL(updateProjectorStatus(QString)), this, SLOT(on_updateProjectorStatus(QString)));
    connect(m_pTerminal, SIGNAL(updateProjector(B9PrinterStatus::ProjectorStatus)), this, SLOT(on_updateProjector(B9PrinterStatus::ProjectorStatus)));
    connect(m_pTerminal, SIGNAL(signalAbortPrint(QString)), this, SLOT(on_pushButtonAbort_clicked(QString)));
    connect(m_pTerminal, SIGNAL(PrintReleaseCycleFinished()), this, SLOT(exposeTBaseLayer()));
    connect(m_pTerminal, SIGNAL(pausePrint()), this, SLOT(on_pushButtonPauseResume_clicked()));
    connect(m_pTerminal, SIGNAL(sendStatusMsg(QString)),this, SLOT(setProjMessage(QString)));

    QString sTime = QDateTime::currentDateTime().toString("hh:mm");
    ui->lcdNumberTime->setDigitCount(9);
    ui->lcdNumberTime->display(sTime);
}

B9Print::~B9Print()
{
    delete ui;
}

void B9Print::keyPressEvent(QKeyEvent * pEvent)
{
    //有了这个功能，吸收了ESC键！
    QDialog::keyReleaseEvent(pEvent);
}

void B9Print::hideEvent(QHideEvent *event)
{
    emit eventHiding();
    event->accept();
}

void B9Print::closeEvent ( QCloseEvent * event )
{
    event->ignore();
    on_pushButtonAbort_clicked();
}

void B9Print::showHelp()
{
    m_HelpSystem.showHelpFile("openfile.html");
}

void B9Print::on_updateConnectionStatus(QString sText)
{
    ui->lineEditSerialStatus->setText(sText);
}

void B9Print::on_updateProjectorOutput(QString sText)
{
    ui->lineEditProjectorOutput->setText(sText);
}

void B9Print::on_updateProjectorStatus(QString sText)
{
    ui->lineEditProjectorStatus->setText(sText);
}

void B9Print::setProjMessage(QString sText)
{
    m_pTerminal->rcSetProjMessage(sText);
}

QString B9Print::updateTimes()
{
    QTime vTimeFinished, vTimeRemains, t;
    int iTime = m_pTerminal->getEstCompleteTimeMS(m_iCurLayerNumber,m_iLastLayer,m_pCPJ->getZLayermm(),m_iTbase+m_iTover);
    int iM = iTime/60000;
    int iH = iM/60;
    iM = (int)((double)iM+0.5) - iH*60;
    QString sLZ = ":0"; if(iM>9)sLZ = ":";
    QString sTimeRemaining = QString::number(iH)+sLZ+QString::number(iM);
    t.setHMS(0,0,0); vTimeRemains = t.addMSecs(iTime);
    vTimeFinished = QTime::currentTime().addMSecs(iTime);
    ui->lcdNumberTime->display(vTimeFinished.toString("hh:mm AP"));
    ui->lcdNumberTimeRemaining->display(sTimeRemaining);
    return "Estimated time remaining: "+sTimeRemaining+"  Estimated Completion Time: "+vTimeFinished.toString("hh:mm AP");
}

double B9Print::curLayerIndexMM()
{
    //“0”层的厚度为0.00001
    return (double)m_iCurLayerNumber * m_dLayerThickness + 0.00001;
}

void B9Print::on_signalAbortPrint()
{
    if(m_iPrintState!=PRINT_ABORT) return;
    m_iPrintState=PRINT_NO;

    //在这里处理中止信号
    if(m_sAbortMessage.contains("Jammed Mechanism"))
        m_pTerminal->rcProjectorPwr(false); // 不要试图释放，如果可能卡住！
    else
        m_pTerminal->rcFinishPrint(5); //完成在当前的Z轴位置+ 5毫米，然后投影机关闭

    m_pTerminal->onScreenCountChanged(); // 如果需要将主显示器设置切换到屏幕
    hide();
    m_pTerminal->setUsePrimaryMonitor(false);
    m_pTerminal->setPrintPreview(false);
    m_pTerminal->onScreenCountChanged();
    m_pTerminal->setEnabled(true);



    QMessageBox::information(0,"Printing Aborted!","PRINT ABORTED\n\n"+m_sAbortMessage);
}

//////////////////////////////////////////////////////////////////////////////////////////
void B9Print::print3D(CrushedPrintJob* pCPJ, int iXOff, int iYOff, int iTbase, int iTover, int iTattach, int iNumAttach, int iLastLayer, bool bPrintPreview, bool bUsePrimaryMonitor, bool bIsMirrored)
{
    //注意：如果，iLastLayer<1，打印所有层。
   //如果bPrintPreview，运行，则无需打开投影机

    m_iMinimumTintMS = m_vSettings.value("m_iMinimumTintMS",50).toInt(); // 抓取旧值
    if(m_iMinimumTintMS>500)
        m_iMinimumTintMS=555; //不应该得到这个大的，解决它。
    else if (m_iMinimumTintMS<50)
        m_iMinimumTintMS=56;  //或者这个小值
    m_vSettings.setValue("m_iMinimumTintMS",(int)((double)m_iMinimumTintMS*.9)); //再回到90％，它的最后一个值，只是为了保持接近边缘
    m_iPrintState = PRINT_NO;
    m_pTerminal->setEnabled(false);
    m_pCPJ = pCPJ;
    m_pTerminal->createNormalizedMask(m_pCPJ->getXYPixelmm());
    m_iTbase = iTbase; m_iTover = iTover; m_iTattach = iTattach; m_iNumAttach = iNumAttach;
    m_iXOff = iXOff; m_iYOff = iYOff;
    m_iCurLayerNumber = 0;
    m_iPaused = PAUSE_NO;
    m_bAbort = false;
    m_iLastLayer = iLastLayer;
    if(m_iLastLayer<1)m_iLastLayer = m_pCPJ->getTotalLayers();

    m_pTerminal->setUsePrimaryMonitor(bUsePrimaryMonitor);
    m_pTerminal->setPrintPreview(bPrintPreview);
    m_pTerminal->onScreenCountChanged();

    ui->lineEditJobName->setText(m_pCPJ->getName());
    ui->lineEditJobDescription->setText(m_pCPJ->getDescription());
    ui->progressBarPrintProgress->setMinimum(0);
    ui->progressBarPrintProgress->setMaximum(m_iLastLayer);
    ui->progressBarPrintProgress->setValue(0);
    ui->lineEditLayerCount->setText("Total Layers To Print: "+QString::number(m_iLastLayer)+"  Powering up the projector.");

    ui->lcdNumberTime->display(m_pTerminal->getEstCompleteTime(m_iCurLayerNumber,m_iLastLayer,m_pCPJ->getZLayermm(),m_iTbase+m_iTover).toString("hh:mm"));
    QString sTimeUpdate = updateTimes();
    setProjMessage("Total Layers to print: "+QString::number(m_iLastLayer)+"  "+sTimeUpdate);
	if(m_pTerminal->getIsVirtualDevice())
	{
		m_iPrintState = PRINT_SETUP1;
	}
    if(!bPrintPreview){
        //打开投影机，并设置以毫秒为预热时间
        ui->pushButtonPauseResume->setEnabled(false);
        ui->pushButtonAbort->setEnabled(false);
        m_pTerminal->rcIsMirrored(bIsMirrored);
        m_pTerminal->rcSetWarmUpDelay(20000);
        m_pTerminal->rcProjectorPwr(true);
    }
    else {
        ui->lineEditProjectorStatus->setText("OFF:  'Print Preview' Mode");
        ui->pushButtonPauseResume->setEnabled(true);
        ui->pushButtonAbort->setEnabled(true);
        m_iPrintState = PRINT_SETUP1;
        m_dLayerThickness = m_pCPJ->getZLayer().toDouble();
        m_pTerminal->rcBasePrint(-m_pTerminal->getHardZDownMM()); //动态Z零，过度直到我们按下硬件和电机“跳过”
    }
}
void B9Print::on_updateProjector(B9PrinterStatus::ProjectorStatus eStatus)
{
    if(m_iPrintState==PRINT_NO && eStatus==B9PrinterStatus::PS_ON){
        //投影机预热并打开！
        ui->pushButtonPauseResume->setEnabled(true); //现在启用暂停/恢复和中止
        ui->pushButtonAbort->setEnabled(true);
        m_iPrintState = PRINT_SETUP1;
        m_dLayerThickness = m_pCPJ->getZLayer().toDouble();
        m_pTerminal->rcBasePrint(-m_pTerminal->getHardZDownMM()); //动态Z零，过度直到我们按下硬件和电机“跳过”
    }
}

void B9Print::on_pushButtonPauseResume_clicked()
{
    if(m_iPrintState == PRINT_NO) return; // 未开始打印

    if(m_iPaused==PAUSE_YES){
        // 时间恢复...
        m_iPaused = PAUSE_NO;
        ui->pushButtonPauseResume->setText("Pause");
        ui->pushButtonAbort->setEnabled(true);
        exposureOfTOverLayersFinished();
    }
    else if(m_iPaused==PAUSE_NO){
        // 时间暂停....
        m_iPaused = PAUSE_WAIT;
        ui->pushButtonPauseResume->setText("Pausing...");
        ui->pushButtonPauseResume->setEnabled(false);
        ui->pushButtonAbort->setEnabled(false);
        setProjMessage("Pausing...");
    }
}

//从b9terminal各种微调按钮那调用（由sAbortText给出）
void B9Print::on_pushButtonAbort_clicked(QString sAbortText)
{
    //提示用户，以确保他们希望中止。
    if(sAbortText == "User Directed Abort.")
    {
        QMessageBox msgBox;
        msgBox.setText("Are you sure you want to abort?");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();

        if(ret == QMessageBox::No)
            return;
    }



    m_sAbortMessage = sAbortText;
    if(m_sAbortMessage.contains("Jammed Mechanism")||m_sAbortMessage.contains("Lost Printer Connection")||
       (m_sAbortMessage.contains("Projector"))){
        //特殊情况下，总是尽快处理。
        m_pTerminal->rcSetCPJ(NULL); //blank
        ui->pushButtonAbort->setText("Abort");
        m_iPrintState = PRINT_ABORT;
        on_signalAbortPrint();
        return;
    }

    if(m_iPrintState == PRINT_NO||m_iPrintState == PRINT_ABORT||m_iPaused==PAUSE_WAIT) return; // no abort if pausing, not printing or already aborting
    ui->pushButtonAbort->setText("Aborting...");
    ui->lineEditLayerCount->setText("Aborting...");
    ui->pushButtonPauseResume->setEnabled(false);
    ui->pushButtonAbort->setEnabled(false);
    setProjMessage("Aborting...");
    m_bAbort = true;
    if(m_iPaused==PAUSE_YES) on_pushButtonPauseResume_clicked();
}

void B9Print::setSlice(int iSlice)
{
    if(m_iLastLayer<1)
        m_pTerminal->rcSetCPJ(NULL);
    else {
        m_pCPJ->setCurrentSlice(iSlice);
        m_pTerminal->rcSetCPJ(m_pCPJ);
    }
}

void B9Print::exposeTBaseLayer(){
    //发布暨复位周期内完成，时间露出新的图层
    if(m_iPrintState==PRINT_NO || m_iPrintState == PRINT_ABORT)return;

    if(m_iPrintState==PRINT_SETUP1){
        //我们使用 - getHardZDownMM（）过渡，并获得在这里，
        //现在重置当前位置为0，并拉升至+ getZFlushMM
        m_pTerminal->rcResetCurrentPositionPU(0);
        m_pTerminal->rcBasePrint(m_pTerminal->getZFlushMM());
        m_iPrintState = PRINT_SETUP2;
        return;
    }

    if(m_iPrintState==PRINT_SETUP2){
        //现在，我们应复位当前位置为0，并继续
        m_pTerminal->rcResetCurrentPositionPU(0);
        m_iPrintState = PRINT_RELEASING;
    }

    if(m_iPrintState == PRINT_DONE){
        m_iPrintState=PRINT_NO;
        m_pTerminal->setEnabled(true);
        if(m_pTerminal->getPrintPreview()){
            m_pTerminal->setPrintPreview(false);
            m_pTerminal->setUsePrimaryMonitor(false);
        }
        m_pTerminal->onScreenCountChanged();
        hide();
        return;
    }

    if(m_bAbort){
        //完成后释放和提高
        m_pTerminal->rcSetCPJ(NULL); //空白
        ui->pushButtonAbort->setText("Abort");
         m_iPrintState = PRINT_ABORT;
        on_signalAbortPrint();
        return;
    }

    //启动TBASE打印曝光
    ui->progressBarPrintProgress->setValue(m_iCurLayerNumber+1);
    ui->lineEditLayerCount->setText("Creating Layer "+QString::number(m_iCurLayerNumber+1)+" of "+QString::number(m_iLastLayer)+",  "+QString::number(100.0*(double)(m_iCurLayerNumber+1)/(double)m_iLastLayer,'f',1)+"% Complete");
    setSlice(m_iCurLayerNumber);
    m_vClock.start(); //图像就在那里，启动时钟运行！
    QString sTimeUpdate = updateTimes();
    if(m_iPaused==PAUSE_WAIT){
        ui->lineEditLayerCount->setText("Pausing...");
        setProjMessage("Pausing...");
    }
    else{
        setProjMessage("(Press'p' to pause, 'A' to ABORT)  " + sTimeUpdate+"  Creating Layer "+QString::number(m_iCurLayerNumber+1)+" of "+QString::number(m_iLastLayer));
    }
    m_iPrintState = PRINT_EXPOSING;
    //设置计时器
    int iAdjExposure = m_pTerminal->getLampAdjustedExposureTime(m_iTbase);
    if(m_iCurLayerNumber<m_iNumAttach) iAdjExposure = m_pTerminal->getLampAdjustedExposureTime(m_iTattach);  //第一层可以具有不同的曝光时延
    if(iAdjExposure>0){
        QTimer::singleShot(iAdjExposure-m_vClock.elapsed(), this, SLOT(startExposeTOverLayers()));
        return;
    }
    else
    {
        startExposeTOverLayers(); // 如果这是获取调用，我们正在获取太长时间了！
        qDebug() << "EXPOSURE TIMING ERROR:  Tbase exposed for too long!, Tbase is set too small or computer too slow?" << iAdjExposure;
        return;
    }
}

void B9Print::startExposeTOverLayers(){
    if(m_iCurLayerNumber==0){exposureOfTOverLayersFinished(); return;} //第一层（0）跳过

    m_vClock.start();  //重新启动TOVER区间步伐

    m_iMinimumTintMS = m_vSettings.value("m_iMinimumTintMS",50).toInt(); //我们默认为50毫秒，在它被击中向上调整它。
    m_iMinimumTintMSWorstCase=m_iMinimumTintMS;

    int iAdjTover = m_pTerminal->getLampAdjustedExposureTime(m_iTover);
    m_iTintNum = (int)((double)iAdjTover/(double)m_iMinimumTintMS);
    if(m_iTintNum > 255) m_iTintNum = 255; // TOVER时间间隔最大数值为255

    m_dTintMS = (double)iAdjTover/(double)m_iTintNum; //在分数毫秒每个间隔的时间，将始终>= m_iMinimumTintMS
    m_iCurTintIndex = 0;
    exposureOfCurTintLayerFinished();
}

void B9Print::exposureOfCurTintLayerFinished(){
    // 关闭的象素的当前点
    if(m_pTerminal->rcClearTimedPixels((double)m_iCurTintIndex*255.0/(double)m_iTintNum) || m_iCurTintIndex>=m_iTintNum)
    {
        exposureOfTOverLayersFinished();  //我们正在与TOVER完成
        m_vSettings.setValue("m_iMinimumTintMS",m_iMinimumTintMSWorstCase);
        return;
    }

    m_iCurTintIndex ++;
    int iAdjustedInt = (int)(m_dTintMS * (double)m_iCurTintIndex)-m_vClock.elapsed();
    if(iAdjustedInt>0){
        QTimer::singleShot(iAdjustedInt, this, SLOT(exposureOfCurTintLayerFinished()));
        return;
    }
    else
    {
        if(m_iCurTintIndex==1)m_iMinimumTintMSWorstCase=m_iMinimumTintMS-iAdjustedInt;
        exposureOfCurTintLayerFinished(); //如果这是获取调用，我们正在获取太长时间了！
        return;
    }
}

void B9Print::exposureOfTOverLayersFinished(){
    if(m_iPrintState==PRINT_NO)return;

    m_pTerminal->rcSetCPJ(NULL); //空白
    //循环到下一层或完成
    if(m_iPaused==PAUSE_WAIT){
        m_iPaused=PAUSE_YES;
        m_pTerminal->rcSTOP();
        m_pTerminal->rcCloseVat();
        ui->pushButtonPauseResume->setText("Resume");
        ui->pushButtonPauseResume->setEnabled(true);
        ui->pushButtonAbort->setEnabled(true);
        ui->lineEditLayerCount->setText("Paused.  Manual positioning toggle switches are enabled.");
        m_pTerminal->rcSetProjMessage(" Paused.  Manual toggle switches are enabled.  Press 'p' when to resume printing, 'A' to abort.");
        return;
    }

    if(m_bAbort){
        //已完成
        m_pTerminal->rcSetCPJ(NULL); //blank
        ui->pushButtonAbort->setText("Abort");
        m_iPrintState = PRINT_ABORT;
        on_signalAbortPrint();
        return;
    }
    else if(m_iCurLayerNumber==m_iLastLayer-1){
        // 已完成，释放并升高
        ui->pushButtonAbort->setEnabled(false);
        ui->pushButtonPauseResume->setEnabled(false);
        m_iPrintState=PRINT_DONE;
        m_pTerminal->rcFinishPrint(50.8); //完成在当前的Z轴位置+50.8毫米，将投影机关闭
        ui->lineEditLayerCount->setText("Finished!");
        setProjMessage("Finished!");
        return;
    }
    else
    {
        //操作下一图层
        m_iCurLayerNumber++;  //设置下一图层数值
        m_pTerminal->rcNextPrint(curLayerIndexMM());
        m_iPrintState = PRINT_RELEASING;
        ui->lineEditLayerCount->setText("Releasing Layer "+QString::number(m_iCurLayerNumber)+", repositioning to layer "+QString::number(m_iCurLayerNumber+1));
        QString sTimeUpdate = updateTimes();
        setProjMessage("(Press'p' to pause, 'A' to ABORT)  " + sTimeUpdate+"  Release and cycle to Layer "+QString::number(m_iCurLayerNumber+1)+" of "+QString::number(m_iLastLayer));
     }
}
