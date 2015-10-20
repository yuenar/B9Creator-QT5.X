#include <QtDebug>
#include <QMessageBox>
#include <QSettings>
#include "b9terminal.h"
#include "ui_b9terminal.h"
#include "dlgcyclesettings.h"
#include "dlgmaterialsmanager.h"

#define RAISE1 80
#define LOWER1 80
#define CLOSE1 100
#define OPEN1 50
#define BREATHE1 2.0
#define SETTLE1 3.0
#define OVERLIFT1 6.35

#define RAISE2 80
#define LOWER2 80
#define CLOSE2 50
#define OPEN2 50
#define BREATHE2 1.5
#define SETTLE2 3.5
#define OVERLIFT2 0.0

#define CUTOFF 0.5
#define HARDDOWN 0.0
#define UP2FLUSH 0.0

void PCycleSettings::updateValues()
{
    DlgCycleSettings dlg(this);
    dlg.exec();
}

void PCycleSettings::loadSettings()
{
    QSettings settings;
    m_iRSpd1 = settings.value("RSpd1",RAISE1).toInt();
    m_iLSpd1 = settings.value("LSpd1",LOWER1).toInt();
    m_iCloseSpd1 = settings.value("CloseSpd1",CLOSE1).toInt();
    m_iOpenSpd1 = settings.value("OpenSpd1",OPEN1).toInt();
    m_dBreatheClosed1 = settings.value("BreatheClosed1",BREATHE1).toDouble();
    m_dSettleOpen1 = settings.value("SettleOpen1",SETTLE1).toDouble();
    m_dOverLift1 = settings.value("OverLift1",OVERLIFT1).toDouble();

    m_iRSpd2 = settings.value("RSpd2",RAISE2).toInt();
    m_iLSpd2 = settings.value("LSpd2",LOWER2).toInt();
    m_iCloseSpd2 = settings.value("CloseSpd2",CLOSE2).toInt();
    m_iOpenSpd2 = settings.value("OpenSpd2",OPEN2).toInt();
    m_dBreatheClosed2 = settings.value("BreatheClosed2",BREATHE2).toDouble();
    m_dSettleOpen2 = settings.value("SettleOpen2",SETTLE2).toDouble();
    m_dOverLift2 = settings.value("OverLift2",OVERLIFT2).toDouble();

    m_dBTClearInMM = settings.value("BTClearInMM",CUTOFF).toDouble();

    m_dHardZDownMM = settings.value("HardDownZMM",HARDDOWN).toDouble();
    m_dZFlushMM    = settings.value("ZFlushMM",   UP2FLUSH).toDouble();
}

void PCycleSettings::saveSettings()
{
    QSettings settings;
    settings.setValue("RSpd1",m_iRSpd1);
    settings.setValue("LSpd1",m_iLSpd1);
    settings.setValue("CloseSpd1",m_iCloseSpd1);
    settings.setValue("OpenSpd1",m_iOpenSpd1);
    settings.setValue("BreatheClosed1",m_dBreatheClosed1);
    settings.setValue("SettleOpen1",m_dSettleOpen1);
    settings.setValue("OverLift1",m_dOverLift1);

    settings.setValue("RSpd2",m_iRSpd2);
    settings.setValue("LSpd2",m_iLSpd2);
    settings.setValue("CloseSpd2",m_iCloseSpd2);
    settings.setValue("OpenSpd2",m_iOpenSpd2);
    settings.setValue("BreatheClosed2",m_dBreatheClosed2);
    settings.setValue("SettleOpen2",m_dSettleOpen2);
    settings.setValue("OverLift2",m_dOverLift2);

    settings.setValue("BTClearInMM",m_dBTClearInMM);

    settings.setValue("HardDownZMM",m_dHardZDownMM);
    settings.setValue("ZFlushMM",   m_dZFlushMM   );
}

void PCycleSettings::setFactorySettings()
{
    m_iRSpd1 = RAISE1;
    m_iLSpd1 = LOWER1;
    m_iRSpd2 = RAISE2;
    m_iLSpd2 = LOWER2;
    m_iOpenSpd1 = OPEN1;
    m_iCloseSpd1 = CLOSE1;
    m_iOpenSpd2 = OPEN2;
    m_iCloseSpd2 = CLOSE2;
    m_dBreatheClosed1 = BREATHE1;
    m_dSettleOpen1 = SETTLE1;
    m_dBreatheClosed2 = BREATHE2;
    m_dSettleOpen2 = SETTLE2;
    m_dOverLift1 = OVERLIFT1;
    m_dOverLift2 = OVERLIFT2;
    m_dBTClearInMM = CUTOFF;
    m_dHardZDownMM = HARDDOWN;
    m_dZFlushMM = UP2FLUSH;
}

B9Terminal::B9Terminal(QWidget *parent, Qt::WindowFlags flags) :
    QWidget(parent, flags),
    ui(new Ui::B9Terminal)
{
    setWindowFlags(Qt::WindowContextHelpButtonHint);
    m_bWaiverPresented = false;
    m_bWaiverAccepted = false;
    m_bWavierActive = false;
    m_bNeedsWarned = true;
    m_iFillLevel = -1;

    ui->setupUi(this);
    ui->commStatus->setText("Searching for B9Creator...");

    qDebug() << "Terminal Start";

    m_pCatalog = new B9MatCat;
    onBC_ModelInfo("B9C1");

    pSettings = new PCycleSettings;
    resetLastSentCycleSettings();

    // 始终设置B9PrinterComm在此终端构造器
    pPrinterComm = new B9PrinterComm;
    pPrinterComm->enableBlankCloning(true); // 允许对固件更新疑似“空白”B9Creator的Arduino的主板

    // 始终设置B9PrinterComm在此终端构造器
    m_pDesktop = QApplication::desktop();
    pProjector = NULL;
    m_bPrimaryScreen = false;
    m_bPrintPreview = false;
    m_bUsePrimaryMonitor = false;

    connect(m_pDesktop, SIGNAL(screenCountChanged(int)),this, SLOT(onScreenCountChanged(int)));

    connect(pPrinterComm,SIGNAL(updateConnectionStatus(QString)), this, SLOT(onUpdateConnectionStatus(QString)));
    connect(pPrinterComm,SIGNAL(BC_ConnectionStatusDetailed(QString)), this, SLOT(onBC_ConnectionStatusDetailed(QString)));
    connect(pPrinterComm,SIGNAL(BC_LostCOMM()),this,SLOT(onBC_LostCOMM()));

    connect(pPrinterComm,SIGNAL(BC_RawData(QString)), this, SLOT(onUpdateRAWPrinterComm(QString)));
    connect(pPrinterComm,SIGNAL(BC_Comment(QString)), this, SLOT(onUpdatePrinterComm(QString)));

    connect(pPrinterComm,SIGNAL(BC_ModelInfo(QString)),this,SLOT(onBC_ModelInfo(QString)));
    connect(pPrinterComm,SIGNAL(BC_FirmVersion(QString)),this,SLOT(onBC_FirmVersion(QString)));
    connect(pPrinterComm,SIGNAL(BC_ProjectorRemoteCapable(bool)), this, SLOT(onBC_ProjectorRemoteCapable(bool)));
    connect(pPrinterComm,SIGNAL(BC_HasShutter(bool)), this, SLOT(onBC_HasShutter(bool)));
    connect(pPrinterComm,SIGNAL(BC_ProjectorStatusChanged()), this, SLOT(onBC_ProjStatusChanged()));
    connect(pPrinterComm,SIGNAL(BC_ProjectorFAIL()), this, SLOT(onBC_ProjStatusFAIL()));

    // Z轴位置控制
    connect(pPrinterComm, SIGNAL(BC_PU(int)), this, SLOT(onBC_PU(int)));
    connect(pPrinterComm, SIGNAL(BC_UpperZLimPU(int)), this, SLOT(onBC_UpperZLimPU(int)));
    m_pResetTimer = new QTimer(this);
    connect(m_pResetTimer, SIGNAL(timeout()), this, SLOT(onMotionResetTimeout()));
    connect(pPrinterComm, SIGNAL(BC_HomeFound()), this, SLOT(onMotionResetComplete()));
    connect(pPrinterComm, SIGNAL(BC_CurrentZPosInPU(int)), this, SLOT(onBC_CurrentZPosInPU(int)));
    connect(pPrinterComm, SIGNAL(BC_HalfLife(int)), this, SLOT(onBC_HalfLife(int)));
    connect(pPrinterComm, SIGNAL(BC_NativeX(int)), this, SLOT(onBC_NativeX(int)));
    connect(pPrinterComm, SIGNAL(BC_NativeY(int)), this, SLOT(onBC_NativeY(int)));
    connect(pPrinterComm, SIGNAL(BC_XYPixelSize(int)), this, SLOT(onBC_XYPixelSize(int)));


    m_pVatTimer = new QTimer(this);
    connect(m_pVatTimer, SIGNAL(timeout()), this, SLOT(onMotionVatTimeout()));
    connect(pPrinterComm, SIGNAL(BC_CurrentVatPercentOpen(int)), this, SLOT(onBC_CurrentVatPercentOpen(int)));

    m_pPReleaseCycleTimer = new QTimer(this);
    connect(m_pPReleaseCycleTimer, SIGNAL(timeout()), this, SLOT(onReleaseCycleTimeout()));
    connect(pPrinterComm, SIGNAL(BC_PrintReleaseCycleFinished()), this, SLOT(onBC_PrintReleaseCycleFinished()));

    onScreenCountChanged(0);
}

B9Terminal::~B9Terminal()
{
    delete ui;
    delete pProjector;
    delete pPrinterComm;
    qDebug() << "Terminal End";
}

void B9Terminal::resetLastSentCycleSettings(){
    m_iD=m_iE=m_iJ=m_iK=m_iL=m_iW=m_iX = -1;
}

void B9Terminal::dlgEditPrinterCycleSettings()
{
    DlgCycleSettings dlgPCycles(pSettings,0);
    dlgPCycles.exec();
}

void B9Terminal::dlgEditMatCat()
{
    DlgMaterialsManager dlgMatMan(m_pCatalog,0);

    QSettings settings;
    int indexMat=0;
    for(int i=0; i<m_pCatalog->getMaterialCount(); i++) {
        if(settings.value("CurrentMaterialLabel","B9R-1-Red").toString()==m_pCatalog->getMaterialLabel(i)) {
            indexMat = i;
            break;
        }
    }
    m_pCatalog->setCurMatIndex(indexMat);

    int indexXY = 0;
    if(settings.value("CurrentXYLabel","100").toString()=="75 (祄)")indexXY=1;
    else if(settings.value("CurrentXYLabel","100").toString()=="100 (祄)")indexXY = 2;
    dlgMatMan.setXY(indexXY);
    m_pCatalog->setCurXYIndex(indexXY);
    dlgMatMan.exec();
}

void B9Terminal::on_pushButtonModMatCat_clicked()
{
    dlgEditMatCat();
}


void B9Terminal::makeProjectorConnections()
{
    // 创建任何一个新的投影机对象时调用
    if(pProjector==NULL)return;
    connect(pProjector, SIGNAL(keyReleased(int)),this, SLOT(getKey(int)));
    connect(this, SIGNAL(sendStatusMsg(QString)),pProjector, SLOT(setStatusMsg(QString)));
    connect(this, SIGNAL(sendGrid(bool)),pProjector, SLOT(setShowGrid(bool)));
    connect(this, SIGNAL(sendCPJ(CrushedPrintJob*)),pProjector, SLOT(setCPJ(CrushedPrintJob*)));
    connect(this, SIGNAL(sendXoff(int)),pProjector, SLOT(setXoff(int)));
    connect(this, SIGNAL(sendYoff(int)),pProjector, SLOT(setYoff(int)));
}


void B9Terminal::warnSingleMonitor(){
    if(m_bPrimaryScreen && m_bNeedsWarned){
        m_bNeedsWarned = false;
        QMessageBox msg;
        msg.setWindowTitle("Projector Connection?");
        msg.setText("WARNING:  The printer's projector is not connected to a secondary video output.  Please check that all connections (VGA or HDMI) and system display settings are correct.  Disregard this message if your system has only one video output and will utilize a splitter to provide video output to both monitor and Projector.");
        if(isEnabled())msg.exec();
    }
}

void B9Terminal::setEnabledWarned(){
    if(isHidden())return;
    if(!m_bWaiverPresented||m_bWaiverAccepted==false){
        // 当前的Waiver
        m_bWaiverPresented = true;
        m_bWaiverAccepted = false;
        if(!m_bWavierActive){
            m_bWavierActive = true;
            int ret = QMessageBox::information(this, tr("Enable Terminal Control?"),
                                           tr("Warning: Manual operation can damage the VAT coating.\n"
                                              "If your VAT is installed and empty of resin care must be\n"
                                              "taken to ensure it is not damaged.  Operation is only safe\n"
                                              "with either the VAT removed, or the Sweeper and Build Table removed.\n"
                                              "The purpose of this utility is to assist in troubleshooting.\n"
                                              "Its use is not required during normal printing operations.\n"
                                              "Do you want to enable manual control?"),
                                           QMessageBox::Yes | QMessageBox::No
                                           | QMessageBox::Cancel);

            if(ret==QMessageBox::Cancel){m_bWavierActive = false;m_bWaiverPresented=false;hide();return;}
            else if(ret==QMessageBox::Yes)m_bWaiverAccepted=true;
            warnSingleMonitor();
            m_bWavierActive = false;
        }
    }
    ui->groupBoxMain->setEnabled(m_bWaiverAccepted&&pPrinterComm->isConnected()&&ui->lineEditNeedsInit->text()!="Seeking");
}

void B9Terminal::hideEvent(QHideEvent *event)
{
    emit eventHiding();
    event->accept();
}

void B9Terminal::sendCommand()
{
    pPrinterComm->SendCmd(ui->lineEditCommand->text());
    ui->lineEditCommand->clear();
}

void B9Terminal::onBC_LostCOMM(){
    //广播警报
    if(!isEnabled())emit signalAbortPrint("ERROR: Lost Printer Connection.  Possible reasons: Power Loss, USB cord unplugged.");
    qDebug() << "BC_LostCOMM signal received.";
}

void B9Terminal::onBC_ConnectionStatusDetailed(QString sText)
{
    setEnabledWarned();

    ui->commStatus->setText(sText);
}

void B9Terminal::onUpdateConnectionStatus(QString sText)
{
    emit (updateConnectionStatus(sText));
}

void B9Terminal::onUpdatePrinterComm(QString sText)
{
    QString html = "<font color=\"Black\">" + sText + "</font><br>";
    ui->textEditCommOut->insertHtml(html);
    html = ui->textEditCommOut->toHtml();
    ui->textEditCommOut->clear();
    ui->textEditCommOut->insertHtml(html.right(2000));
    ui->textEditCommOut->setAlignment(Qt::AlignBottom);
}
void B9Terminal::onUpdateRAWPrinterComm(QString sText)
{
    QString html = "<font color=\"Blue\">" + sText + "</font><br>";
    ui->textEditCommOut->insertHtml(html);
    html = ui->textEditCommOut->toHtml();
    ui->textEditCommOut->clear();
    ui->textEditCommOut->insertHtml(html.right(2000));
    ui->textEditCommOut->setAlignment(Qt::AlignBottom);
}

void B9Terminal::on_pushButtonProjPower_toggled(bool checked)
{
    //用户改变了投影机电源设置
    ui->pushButtonProjPower->setChecked(checked);
    if(checked)
        ui->pushButtonProjPower->setText("ON");
    else
        ui->pushButtonProjPower->setText("OFF");
    pPrinterComm->cmdProjectorPowerOn(checked);
    emit(setProjectorPowerCmd(checked));

    // 如果m_bPrimaryScreen是真的，我们需要在打开投影机之前显示它！
    if(m_bPrimaryScreen) onScreenCountChanged();
    emit sendStatusMsg("B9Creator - Projector status: TURN ON");

    // 我们始终通电时关闭Vat
    if(checked){
        emit onBC_CurrentVatPercentOpen(0);
        emit on_spinBoxVatPercentOpen_editingFinished();
    }
}

void B9Terminal::setProjectorPowerCmd(bool bPwrFlag){
    pPrinterComm->setProjectorPowerCmd(bPwrFlag);
}


void B9Terminal::onBC_ProjStatusFAIL()
{
    onBC_ProjStatusChanged();
    on_pushButtonProjPower_toggled(pPrinterComm->isProjectorPowerCmdOn());
}

void B9Terminal::onBC_ProjStatusChanged()
{
    QString sText = "UNKNOWN";
    switch (pPrinterComm->getProjectorStatus()){
    case B9PrinterStatus::PS_OFF:
        ui->pushButtonProjPower->setEnabled(true);
        sText = "OFF";
        break;
    case B9PrinterStatus::PS_TURNINGON:
        ui->pushButtonProjPower->setEnabled(false);
        sText = "TURN ON";
        break;
    case B9PrinterStatus::PS_WARMING:
        ui->pushButtonProjPower->setEnabled(true);
        sText = "WARM UP";
        break;
    case B9PrinterStatus::PS_ON:
        ui->pushButtonProjPower->setEnabled(true);
        emit(ProjectorIsOn());
        sText = "ON";
        break;
    case B9PrinterStatus::PS_COOLING:
        ui->pushButtonProjPower->setEnabled(false);
        sText = "COOL DN";
        break;
    case B9PrinterStatus::PS_TIMEOUT:
        ui->pushButtonProjPower->setEnabled(false);
        sText = "TIMEOUT";
        if(!isEnabled())emit signalAbortPrint("Timed out while attempting to turn on projector.  Check Projector's Power Cord and RS-232 cable.");
        break;
    case B9PrinterStatus::PS_FAIL:
        ui->pushButtonProjPower->setEnabled(false);
        sText = "FAIL";
        if(!isEnabled())emit signalAbortPrint("Lost Communications with Projector.  Possible Causes:  Manually powered off, Power Failure, Cord Disconnected or Projector Lamp Failure");
        break;
    case B9PrinterStatus::PS_UNKNOWN:
        ui->pushButtonProjPower->setEnabled(true);
    default:
        sText = "UNKNOWN";
        break;
    }

    // 更新电源按钮状态
    if(pPrinterComm->isProjectorPowerCmdOn()){
        pProjector->show();
        activateWindow();
        ui->pushButtonProjPower->setChecked(true);
        ui->pushButtonProjPower->setText("ON");
    }
    else{
        pProjector->hide();
        activateWindow();
        ui->pushButtonProjPower->setChecked(false);
        ui->pushButtonProjPower->setText("OFF");
    }

    if(!isEnabled())emit sendStatusMsg("B9Creator - Projector status: "+sText);
    if(!isEnabled())emit updateProjectorStatus(sText);
    if(!isEnabled())emit updateProjector(pPrinterComm->getProjectorStatus());

    ui->lineEditProjState->setText(sText);
    sText = "UNKNOWN";
    int iLH = pPrinterComm->getLampHrs();
    if(iLH >= 0 && (pPrinterComm->getProjectorStatus()==B9PrinterStatus::PS_ON||
                    pPrinterComm->getProjectorStatus()==B9PrinterStatus::PS_WARMING||
                    pPrinterComm->getProjectorStatus()==B9PrinterStatus::PS_COOLING))sText = QString::number(iLH);
    ui->lineEditLampHrs->setText(sText);
}


void B9Terminal::on_pushButtonFirmwareReset_clicked()
{
    QMessageBox vWarn(QMessageBox::Warning,"Reset Firmware Default Values?","This will change the firmware's persistent values back to defaults for version 1.1 hardware.\n\n Reset firmware variables?",QMessageBox::Ok|QMessageBox::Cancel);
    vWarn.setDefaultButton(QMessageBox::Cancel);
    if(vWarn.exec()==QMessageBox::Cancel) return;
    pPrinterComm->SendCmd("H1024");
    pPrinterComm->SendCmd("I768");
    pPrinterComm->SendCmd("Q5000");
    pPrinterComm->SendCmd("T0");
    pPrinterComm->SendCmd("U100");
    pPrinterComm->SendCmd("Y8135");
    pPrinterComm->SendCmd("$2000");
    pPrinterComm->SendCmd("A");
}

void B9Terminal::on_pushButtonCmdReset_clicked()
{
    int iTimeoutEstimate = 80000; // 80秒（不应该采取的上限超过75秒）
    ui->groupBoxMain->setEnabled(false);
    ui->lineEditNeedsInit->setText("Seeking");
    //复位远程激活（查看他的主页）
    m_pResetTimer->start(iTimeoutEstimate);
    pPrinterComm->SendCmd("R");
    resetLastSentCycleSettings();
}

void B9Terminal::onMotionResetComplete()
{
    ui->groupBoxMain->setEnabled(true);
    if(pPrinterComm->getHomeStatus()==B9PrinterStatus::HS_FOUND) ui->lineEditNeedsInit->setText("No");
    else if(pPrinterComm->getHomeStatus()==B9PrinterStatus::HS_UNKNOWN) ui->lineEditNeedsInit->setText("Yes");
    else ui->lineEditNeedsInit->setText("Seeking");
    ui->lineEditZDiff->setText(QString::number(pPrinterComm->getLastHomeDiff()).toLatin1());
    m_pResetTimer->stop();

    //检查发出重置并填充的命令
    if(m_iFillLevel>=0){
        pPrinterComm->SendCmd("G"+QString::number(m_iFillLevel));
        m_iFillLevel=-1;
    }
    emit HomeFound();
}

void B9Terminal::onMotionResetTimeout(){
    ui->groupBoxMain->setEnabled(true);
    m_pResetTimer->stop();
    QMessageBox msg;
    msg.setText("ERROR: TIMEOUT attempting to locate home position.  Check connections.");
    if(isEnabled())msg.exec();
}

void B9Terminal::onBC_ModelInfo(QString sModel){
    m_sModelName = sModel;
    m_pCatalog->load(m_sModelName);
    ui->lineEditModelInfo->setText(m_sModelName);
    resetLastSentCycleSettings();
}

void B9Terminal::onBC_FirmVersion(QString sVersion){
    ui->lineEditFirmVersion->setText(sVersion);
}

void B9Terminal::onBC_ProjectorRemoteCapable(bool bCapable){
    ui->groupBoxProjector->setEnabled(bCapable);
}
void B9Terminal::onBC_HasShutter(bool bHS){
    ui->groupBoxVAT->setEnabled(bHS);
}

void B9Terminal::onBC_PU(int iPU){
    double dPU = (double)iPU/100000.0;
    ui->lineEditPUinMicrons->setText(QString::number(dPU,'g',8));
}

void B9Terminal::onBC_HalfLife(int iHL){
    ui->lineEditHalfLife->setText(QString::number(iHL));
}

void B9Terminal::onBC_NativeX(int iNX){
    ui->lineEditNativeX->setText(QString::number(iNX));
}

void B9Terminal::onBC_NativeY(int iNY){
    ui->lineEditNativeY->setText(QString::number(iNY));
    if(pProjector == NULL)emit onScreenCountChanged();
}

void B9Terminal::onBC_XYPixelSize(int iPS){
    int i=2;
    if(iPS==50)i=0;
    else if(iPS==75)i=1;
    ui->comboBoxXPPixelSize->setCurrentIndex(i);
}

void B9Terminal::onBC_UpperZLimPU(int iUpZLimPU){
    double dZUpLimMM = (iUpZLimPU * pPrinterComm->getPU())/100000.0;
    ui->lineEditUpperZLimMM->setText(QString::number(dZUpLimMM,'g',8));
    ui->lineEditUpperZLimInches->setText(QString::number(dZUpLimMM/25.4,'g',8));
    ui->lineEditUpperZLimPU->setText(QString::number(iUpZLimPU,'g',8));
}

void B9Terminal::onBC_CurrentZPosInPU(int iCurZPU){
    double dZPosMM = (iCurZPU * pPrinterComm->getPU())/100000.0;
    ui->lineEditCurZPosInMM->setText(QString::number(dZPosMM,'g',8));
    ui->lineEditCurZPosInInches->setText(QString::number(dZPosMM/25.4,'g',8));
    ui->lineEditCurZPosInPU->setText(QString::number(iCurZPU,'g',8));
    emit ZMotionComplete();
}

void B9Terminal::on_lineEditTgtZPU_editingFinished()
{
    int iValue=ui->lineEditTgtZPU->text().toInt();
    if(QString::number(iValue)!=ui->lineEditTgtZPU->text()||
            iValue<0 || iValue >31497){
        QMessageBox::information(this, tr("Target Level (Z steps) Out of Range"),
                                       tr("Please enter an integer value between 0-31497.\n"
                                          "This will be the altitude for the next layer.\n"),
                                       QMessageBox::Ok);
        iValue = 0;
        ui->lineEditTgtZPU->setText(QString::number(iValue));
        ui->lineEditTgtZPU->setFocus();
        ui->lineEditTgtZPU->selectAll();
    }
    setTgtAltitudePU(iValue);
}

void B9Terminal::on_lineEditTgtZMM_editingFinished()
{
    double dValue=ui->lineEditTgtZMM->text().toDouble();
    double dTest = QString::number(dValue).toDouble();
    if((dTest==0 && ui->lineEditTgtZMM->text().length()>2)|| dTest!=ui->lineEditTgtZMM->text().toDouble()||
            dValue<0 || dValue >200.00595){
        QMessageBox::information(this, tr("Target Level (Inches) Out of Range"),
                     tr("Please enter an integer value between 0-7.87425.\n"
                     "This will be the altitude for the next layer.\n"),QMessageBox::Ok);
        dValue = 0;
        ui->lineEditTgtZMM->setText(QString::number(dValue));
        ui->lineEditTgtZMM->setFocus();
        ui->lineEditTgtZMM->selectAll();
        return;
    }
    setTgtAltitudeMM(dValue);
}


void B9Terminal::on_lineEditTgtZInches_editingFinished()
{
    double dValue=ui->lineEditTgtZInches->text().toDouble();
    double dTest = QString::number(dValue).toDouble();
    if((dTest==0 && ui->lineEditTgtZInches->text().length()>2)|| dTest!=ui->lineEditTgtZInches->text().toDouble()||
            dValue<0 || dValue >7.87425){
        QMessageBox::information(this, tr("Target Level (Inches) Out of Range"),
                     tr("Please enter an integer value between 0-7.87425.\n"
                        "This will be the altitude for the next layer.\n"),QMessageBox::Ok);
        dValue = 0;
        ui->lineEditTgtZInches->setText(QString::number(dValue));
        ui->lineEditTgtZInches->setFocus();
        ui->lineEditTgtZInches->selectAll();
        return;
    }
    setTgtAltitudeIN(dValue);
}

void B9Terminal::setTgtAltitudePU(int iTgtPU)
{
    double dTgtMM = (iTgtPU * pPrinterComm->getPU())/100000.0;
    ui->lineEditTgtZPU->setText(QString::number(iTgtPU));
    ui->lineEditTgtZMM->setText(QString::number(dTgtMM,'g',8));
    ui->lineEditTgtZInches->setText(QString::number(dTgtMM/25.4,'g',8));
}

void B9Terminal::setTgtAltitudeMM(double dTgtMM){
    double dPU = (double)pPrinterComm->getPU()/100000.0;
    setTgtAltitudePU((int)(dTgtMM/dPU));
}

void B9Terminal::setTgtAltitudeIN(double dTgtIN){
    setTgtAltitudeMM(dTgtIN*25.4);
}

void B9Terminal::on_lineEditCurZPosInPU_returnPressed()
{
    int iValue=ui->lineEditCurZPosInPU->text().toInt();
    if(QString::number(iValue)!=ui->lineEditCurZPosInPU->text()|| iValue<0 || iValue >32000){
        // 错值，返回即可
        ui->lineEditCurZPosInPU->setText("Bad Value");
        return;
    }
    pPrinterComm->SendCmd("G"+QString::number(iValue));
    ui->lineEditCurZPosInPU->setText("In Motion...");
    ui->lineEditCurZPosInMM->setText("In Motion...");
    ui->lineEditCurZPosInInches->setText("In Motion...");
}

void B9Terminal::on_lineEditCurZPosInMM_returnPressed()
{
    double dPU = (double)pPrinterComm->getPU()/100000.0;
    double dValue=ui->lineEditCurZPosInMM->text().toDouble();
    if((dValue==0 && ui->lineEditCurZPosInMM->text().length()>1 )||dValue<0 || dValue >203.2){
        // 错值，返回即可
        ui->lineEditCurZPosInMM->setText("Bad Value");
        return;
    }

    pPrinterComm->SendCmd("G"+QString::number((int)(dValue/dPU)));
    ui->lineEditCurZPosInPU->setText("In Motion...");
    ui->lineEditCurZPosInMM->setText("In Motion...");
    ui->lineEditCurZPosInInches->setText("In Motion...");
}

void B9Terminal::on_lineEditCurZPosInInches_returnPressed()
{
    double dPU = (double)pPrinterComm->getPU()/100000.0;
    double dValue=ui->lineEditCurZPosInInches->text().toDouble();
    if((dValue==0 && ui->lineEditCurZPosInMM->text().length()>1 )||dValue<0 || dValue >8.0){
        // 错值，返回即可
        ui->lineEditCurZPosInInches->setText("Bad Value");
        return;
    }

    pPrinterComm->SendCmd("G"+QString::number((int)(dValue*25.4/dPU)));
    ui->lineEditCurZPosInPU->setText("In Motion...");
    ui->lineEditCurZPosInMM->setText("In Motion...");
    ui->lineEditCurZPosInInches->setText("In Motion...");

}

void B9Terminal::on_pushButtonStop_clicked()
{
    m_pPReleaseCycleTimer->stop();
    m_pVatTimer->stop();
    pPrinterComm->SendCmd("S");
    ui->lineEditCycleStatus->setText("Cycle Stopped.");
    ui->pushButtonPrintBase->setEnabled(true);
    ui->pushButtonPrintNext->setEnabled(true);
    ui->pushButtonPrintFinal->setEnabled(true);
    ui->groupBoxVAT->setEnabled(true);
    resetLastSentCycleSettings();
}

void B9Terminal::on_checkBoxVerbose_clicked(bool checked)
{
    if(checked)pPrinterComm->SendCmd("T1"); else pPrinterComm->SendCmd("T0");
}

void B9Terminal::on_spinBoxVatPercentOpen_editingFinished()
{
    if(m_pVatTimer->isActive()) return;
    m_pVatTimer->start(3000); //即使速度很慢，也不应花费这么长时间
    int iValue = ui->spinBoxVatPercentOpen->value();
    ui->groupBoxVAT->setEnabled(false);
    pPrinterComm->SendCmd("V"+QString::number(iValue));
}

void B9Terminal::on_pushButtonVOpen_clicked()
{
    ui->groupBoxVAT->setEnabled(false);
    m_pVatTimer->start(3000); //即使速度很慢，也不应花费这么长时间
    pPrinterComm->SendCmd("V100");
}

void B9Terminal::on_pushButtonVClose_clicked()
{
    ui->groupBoxVAT->setEnabled(false);
    m_pVatTimer->start(3000); //即使速度很慢，也不应花费这么长时间
    pPrinterComm->SendCmd("V0");
}

void B9Terminal::onMotionVatTimeout(){
    m_pVatTimer->stop();
    on_pushButtonStop_clicked(); // 停止
    QMessageBox msg;
    msg.setText("Vat Timed out");
    if(isEnabled())msg.exec();
    ui->groupBoxVAT->setEnabled(true);
}
void B9Terminal::onBC_CurrentVatPercentOpen(int iPO){
    m_pVatTimer->stop();
    int iVPO = iPO;
    if (iVPO>-3 && iVPO<4)iVPO=0;
    if (iVPO>97 && iVPO<104)iVPO=100;
    ui->spinBoxVatPercentOpen->setValue(iVPO);
    ui->groupBoxVAT->setEnabled(true);
}

void B9Terminal::onBC_PrintReleaseCycleFinished()
{
    m_pPReleaseCycleTimer->stop();
    ui->lineEditCycleStatus->setText("Cycle Complete.");
    ui->pushButtonPrintBase->setEnabled(true);
    ui->pushButtonPrintNext->setEnabled(true);
    ui->pushButtonPrintFinal->setEnabled(true);
    emit PrintReleaseCycleFinished();
}

void B9Terminal::onReleaseCycleTimeout()
{
    m_pPReleaseCycleTimer->stop();
    if(true){  // 如果由于超时我们希望中止，则设置为真。
        qDebug()<<"Release Cycle Timeout.  Possible reasons: Power Loss, Jammed Mechanism.";
        on_pushButtonStop_clicked(); //停止
        ui->lineEditCycleStatus->setText("ERROR: TimeOut");
        ui->pushButtonPrintBase->setEnabled(true);
        ui->pushButtonPrintNext->setEnabled(true);
        ui->pushButtonPrintFinal->setEnabled(true);
        if(!isEnabled())emit signalAbortPrint("ERROR: Cycle Timed Out.  Possible reasons: Power Loss, Jammed Mechanism.");
        return;
    }
    else {
        qDebug()<<"Release Cycle Timeout.  Possible reasons: Power Loss, Jammed Mechanism. IGNORED";
        qDebug()<<"Serial Port Last Error:  "<<pPrinterComm->errorString();
    }
}

void B9Terminal::on_pushButtonPrintBase_clicked()
{
    ui->lineEditCycleStatus->setText("Moving to Base...");
    ui->pushButtonPrintBase->setEnabled(false);
    ui->pushButtonPrintNext->setEnabled(false);
    ui->pushButtonPrintFinal->setEnabled(false);
    resetLastSentCycleSettings();
    SetCycleParameters();
    int iTimeout = getEstBaseCycleTime(ui->lineEditCurZPosInPU->text().toInt(), ui->lineEditTgtZPU->text().toInt());
    pPrinterComm->SendCmd("B"+ui->lineEditTgtZPU->text());
    m_pPReleaseCycleTimer->start(iTimeout * 2.0); // 请求后超过预计时间的200％视为超时
}

void B9Terminal::on_pushButtonPrintNext_clicked()
{
    ui->lineEditCycleStatus->setText("Cycling to Next...");
    ui->pushButtonPrintBase->setEnabled(false);
    ui->pushButtonPrintNext->setEnabled(false);
    ui->pushButtonPrintFinal->setEnabled(false);

    SetCycleParameters();
    int iTimeout = getEstNextCycleTime(ui->lineEditCurZPosInPU->text().toInt(), ui->lineEditTgtZPU->text().toInt());
    pPrinterComm->SendCmd("N"+ui->lineEditTgtZPU->text());
    m_pPReleaseCycleTimer->start(iTimeout * 2.0); // 请求后超过预计时间的200％视为超时
}

void B9Terminal::on_pushButtonPrintFinal_clicked()
{
    rcProjectorPwr(false);  // 命令投影机关闭
    ui->lineEditCycleStatus->setText("Final Release...");
    ui->pushButtonPrintBase->setEnabled(false);
    ui->pushButtonPrintNext->setEnabled(false);
    ui->pushButtonPrintFinal->setEnabled(false);
    SetCycleParameters();
    int iTimeout = getEstFinalCycleTime(ui->lineEditCurZPosInPU->text().toInt(), ui->lineEditTgtZPU->text().toInt());
    pPrinterComm->SendCmd("F"+ui->lineEditTgtZPU->text());
    m_pPReleaseCycleTimer->start(iTimeout * 2.0); // 请求后超过预计时间的200％视为超时
}

void B9Terminal::SetCycleParameters(){
    int iD, iE, iJ, iK, iL, iW, iX;
    if(pSettings->m_dBTClearInMM*100000/(double)pPrinterComm->getPU()>(double)ui->lineEditTgtZPU->text().toInt()){
        iD = (int)(pSettings->m_dBreatheClosed1*1000.0); // 评估延迟时间
        iE = (int)(pSettings->m_dSettleOpen1*1000.0); // 结算延迟时间
        iJ = (int)(pSettings->m_dOverLift1*100000.0/(double)pPrinterComm->getPU()); // Overlift Raise Gap coverted to PU
        iK = pSettings->m_iRSpd1;  // 加速
        iL = pSettings->m_iLSpd1;  // 减速
        iW = pSettings->m_iOpenSpd1;  // Vat打开速度
        iX = pSettings->m_iCloseSpd1; // Vat关闭速度
    }
    else{
        iD = (int)(pSettings->m_dBreatheClosed2*1000.0); // 评估延迟时间
        iE = (int)(pSettings->m_dSettleOpen2*1000.0); // 结算延迟时间
        iJ = (int)(pSettings->m_dOverLift2*100000.0/(double)pPrinterComm->getPU()); // Overlift Raise Gap coverted to PU
        iK = pSettings->m_iRSpd2;  // 加速
        iL = pSettings->m_iLSpd2;  // 减速
        iW = pSettings->m_iOpenSpd2;  // Vat打开速度
        iX = pSettings->m_iCloseSpd2; // Vat关闭速度
    }
    if(iD!=m_iD){pPrinterComm->SendCmd("D"+QString::number(iD)); m_iD = iD;}
    if(iE!=m_iE){pPrinterComm->SendCmd("E"+QString::number(iE)); m_iE = iE;}
    if(iJ!=m_iJ){pPrinterComm->SendCmd("J"+QString::number(iJ)); m_iJ = iJ;}
    if(iK!=m_iK){pPrinterComm->SendCmd("K"+QString::number(iK)); m_iK = iK;}
    if(iL!=m_iL){pPrinterComm->SendCmd("L"+QString::number(iL)); m_iL = iL;}
    if(iW!=m_iW){pPrinterComm->SendCmd("W"+QString::number(iW)); m_iW = iW;}
    if(iX!=m_iX){pPrinterComm->SendCmd("X"+QString::number(iX)); m_iX = iX;}
}

void B9Terminal::rcProjectorPwr(bool bPwrOn){
    on_pushButtonProjPower_toggled(bPwrOn);
}

void B9Terminal::rcResetFirmwareDefaults(){
    on_pushButtonFirmwareReset_clicked();
}

void B9Terminal::rcResetHomePos(){
    on_pushButtonCmdReset_clicked();
}

void B9Terminal::rcSendCmd(QString sCmd)
{
    pPrinterComm->SendCmd(sCmd);
}

void B9Terminal::rcGotoFillAfterReset(int iFillLevel){
    m_iFillLevel = iFillLevel;
}

void B9Terminal::rcResetCurrentPositionPU(int iCurPos){
    pPrinterComm->SendCmd("O"+QString::number(iCurPos));
}

void B9Terminal::rcBasePrint(double dBaseMM)
{
    setTgtAltitudeMM(dBaseMM);
    on_pushButtonPrintBase_clicked();
}

void B9Terminal::rcNextPrint(double dNextMM)
{
    setTgtAltitudeMM(dNextMM);
    on_pushButtonPrintNext_clicked();
}

void B9Terminal::rcSTOP()
{
    on_pushButtonStop_clicked();
}

void B9Terminal::rcCloseVat()
{
    on_pushButtonVClose_clicked();
}

void B9Terminal::rcSetWarmUpDelay(int iDelayMS)
{
    pPrinterComm->setWarmUpDelay(iDelayMS);
}

void B9Terminal::rcIsMirrored(bool bIsMirrored)
{
    pPrinterComm->setMirrored(bIsMirrored);
}

void B9Terminal::rcFinishPrint(double dDeltaMM)
{
    // 计算基于当前+ dDeltaMM最终位置
    int newPos = dDeltaMM*100000.0/(double)pPrinterComm->getPU();
    newPos += ui->lineEditCurZPosInPU->text().toInt();
    int curPos = ui->lineEditCurZPosInPU->text().toInt();
    int upperLim = ui->lineEditUpperZLimPU->text().toInt();

    if(curPos >= upperLim)
        newPos = curPos;
    else if(newPos > upperLim)
        newPos = upperLim;

    setTgtAltitudePU(newPos);
    on_pushButtonPrintFinal_clicked();
}

void B9Terminal::rcSetCPJ(CrushedPrintJob *pCPJ)
{
    //设置一个指向要显示CMB的指针, 如果CMB为空则（指针）置为NULL
    emit sendCPJ(pCPJ);
}

void B9Terminal::rcSetProjMessage(QString sMsg)
{
    if(pProjector==NULL)return;
    // 随着投影机屏幕传递消息
    pProjector->setStatusMsg("B9Creator  -  "+sMsg);
}

int B9Terminal::getLampAdjustedExposureTime(int iBaseTimeMS)
{
    if(pPrinterComm==NULL||pPrinterComm->getLampHrs()<0||pPrinterComm->getHalfLife()<0)return iBaseTimeMS;

    //  dLife = 0.0 灯泡未使用时是0 and 使用寿命过半时为1
    //我们乘以dLife的基准时间，返回原来的量+产品。
    //在半衰期,我们已经增加了一倍的标准曝光时间。
    double dLife = (double)pPrinterComm->getLampHrs()/(double)pPrinterComm->getHalfLife();
    if(dLife > 1.0)dLife = 1.0; //灯泡达到并超过一半使用时间后,将dlife始终置为1
    return iBaseTimeMS + (double)iBaseTimeMS*dLife;
}

QTime B9Terminal::getEstCompleteTime(int iCurLayer, int iTotLayers, double dLayerThicknessMM, int iExposeMS)
{
    return QTime::currentTime().addMSecs(getEstCompleteTimeMS(iCurLayer, iTotLayers, dLayerThicknessMM, iExposeMS));
}

int B9Terminal::getEstCompleteTimeMS(int iCurLayer, int iTotLayers, double dLayerThicknessMM, int iExposeMS)
{
    //返回预计完成时间
    int iTransitionPointLayer = (int)(pSettings->m_dBTClearInMM/dLayerThicknessMM);

    int iLowerCount = (int)(pSettings->m_dBTClearInMM/dLayerThicknessMM);
    int iUpperCount = iTotLayers - iLowerCount;

    if(iLowerCount>iTotLayers)iLowerCount=iTotLayers;
    if(iUpperCount<0)iUpperCount=0;

    if(iCurLayer<iTransitionPointLayer)iLowerCount = iLowerCount-iCurLayer; else iLowerCount = 0;
    if(iCurLayer>=iTransitionPointLayer) iUpperCount = iTotLayers - iCurLayer;

    int iTotalTimeMS = iExposeMS*iLowerCount + iExposeMS*iUpperCount;

    iTotalTimeMS = getLampAdjustedExposureTime(iTotalTimeMS);

    // 加上评估和结算
    iTotalTimeMS += iLowerCount*(pSettings->m_dBreatheClosed1 + pSettings->m_dSettleOpen1)*1000;
    iTotalTimeMS += iUpperCount*(pSettings->m_dBreatheClosed2 + pSettings->m_dSettleOpen2)*1000;

    // Z轴行程时间
    int iGap1 = iLowerCount*(int)(pSettings->m_dOverLift1*100000.0/(double)pPrinterComm->getPU());
    int iGap2 = iUpperCount*(int)(pSettings->m_dOverLift2*100000.0/(double)pPrinterComm->getPU());

    int iZRaiseDistance1 = iGap1 + iLowerCount*(int)(dLayerThicknessMM*100000.0/(double)pPrinterComm->getPU());
    int iZLowerDistance1 = iGap1;

    int iZRaiseDistance2 = iGap2 + iUpperCount*(int)(dLayerThicknessMM*100000.0/(double)pPrinterComm->getPU());
    int iZLowerDistance2 = iGap2;

    iTotalTimeMS += getZMoveTime(iZRaiseDistance1,pSettings->m_iRSpd1);
    iTotalTimeMS += getZMoveTime(iZRaiseDistance2,pSettings->m_iRSpd2);
    iTotalTimeMS += getZMoveTime(iZLowerDistance1,pSettings->m_iLSpd1);
    iTotalTimeMS += getZMoveTime(iZLowerDistance2,pSettings->m_iLSpd2);

    // Vat移动时间
    iTotalTimeMS += iLowerCount*getVatMoveTime(pSettings->m_iOpenSpd1) + iLowerCount*getVatMoveTime(pSettings->m_iCloseSpd1);
    iTotalTimeMS += iUpperCount*getVatMoveTime(pSettings->m_iOpenSpd2) + iUpperCount*getVatMoveTime(pSettings->m_iCloseSpd2);
    return iTotalTimeMS;
}

int B9Terminal::getZMoveTime(int iDelta, int iSpd){
    // 返回走完iDelta到PU距离的时间（以毫秒为单位）
    // 准确的，假定100％是130转和0％是50转
    // 同时假定200 PU（步骤）每转
    // 返回移动iDelta PU的所需的时间（以毫秒为单位）
    if(iDelta==0)return 0;
    double dPUms; // 每毫秒打印机单元
    dPUms = ((double)iSpd/100.0)*80.0 + 50.0;
    dPUms *= 200.0; // PU每分钟
    dPUms /= 60; // PU每秒钟
    dPUms /= 1000; // PU每毫秒
    return (int)(double(iDelta)/dPUms);
}

int B9Terminal::getVatMoveTime(int iSpeed){
    double dPercent = (double)iSpeed/100.0;
//    return 999 - dPercent*229.0; // 基于速度测试 of B9C1 on 11/14/2012
    return 1500 - dPercent*229.0; // 超时测试 11/18/2012
}

int B9Terminal::getEstBaseCycleTime(int iCur, int iTgt){
    int iDelta = abs(iTgt - iCur);
    int iLowerSpd,iOpnSpd,iSettle;
    double cutOffPU = pSettings->m_dBTClearInMM*100000.0/(double)pPrinterComm->getPU();
    if((double)iTgt<cutOffPU){
        iLowerSpd = pSettings->m_iLSpd1;
        iOpnSpd = pSettings->m_iOpenSpd1;
        iSettle = pSettings->m_dSettleOpen1*1000.0;
    }
    else{
        iLowerSpd = pSettings->m_iLSpd2;
        iOpnSpd = pSettings->m_iOpenSpd2;
        iSettle = pSettings->m_dSettleOpen2*1000.0;
    }
    // 移动iDelta时间
    int iTimeReq = getZMoveTime(iDelta, iLowerSpd);
    // 增加打开vat的时间
    iTimeReq += getVatMoveTime(iOpnSpd);
    // 增加结算时间;
    iTimeReq += iSettle;
    return iTimeReq;
}

int B9Terminal::getEstNextCycleTime(int iCur, int iTgt){
    int iDelta = abs(iTgt - iCur);
    int iRaiseSpd,iLowerSpd,iOpnSpd,iClsSpd,iGap,iBreathe,iSettle;
    double cutOffPU = pSettings->m_dBTClearInMM*100000.0/(double)pPrinterComm->getPU();
    if((double)iTgt<cutOffPU){
        iRaiseSpd = pSettings->m_iRSpd1;
        iLowerSpd = pSettings->m_iLSpd1;
        iOpnSpd = pSettings->m_iOpenSpd1;
        iClsSpd = pSettings->m_iCloseSpd1;
        iGap = (int)(pSettings->m_dOverLift1*100000.0/(double)pPrinterComm->getPU());
        iBreathe = pSettings->m_dBreatheClosed1*1000.0;
        iSettle = pSettings->m_dSettleOpen1*1000.0;
    }
    else{
        iRaiseSpd = pSettings->m_iRSpd2;
        iLowerSpd = pSettings->m_iLSpd2;
        iOpnSpd = pSettings->m_iOpenSpd2;
        iClsSpd = pSettings->m_iCloseSpd2;
        iGap = (int)(pSettings->m_dOverLift2*100000.0/(double)pPrinterComm->getPU());
        iBreathe = pSettings->m_dBreatheClosed2*1000.0;
        iSettle = pSettings->m_dSettleOpen2*1000.0;
    }
    //移动+ iDelta和+ IGAP的时间，向上或向下
    int iTimeReq = getZMoveTime(iDelta+iGap, iRaiseSpd);
    iTimeReq += getZMoveTime(iDelta+iGap, iLowerSpd);
    // 增加关闭及打开vat的时间
    iTimeReq += getVatMoveTime(iClsSpd)+getVatMoveTime(iOpnSpd);
    // 增加评估和结算时间;
    iTimeReq += iBreathe + iSettle;
    return iTimeReq;
}

int B9Terminal::getEstFinalCycleTime(int iCur, int iTgt){
    int iDelta = abs(iTgt - iCur);
    int iRaiseSpd,iClsSpd;
    double cutOffPU = pSettings->m_dBTClearInMM*100000.0/(double)pPrinterComm->getPU();
    if((double)iTgt<cutOffPU){
        iRaiseSpd = pSettings->m_iRSpd1;
        iClsSpd = pSettings->m_iCloseSpd1;
    }
    else{
        iRaiseSpd = pSettings->m_iRSpd2;
        iClsSpd = pSettings->m_iCloseSpd2;
    }
    // 移动+iDelta向上的时间
    int iTimeReq = getZMoveTime(iDelta, iRaiseSpd);
    // 关闭vat的时间
    iTimeReq += getVatMoveTime(iClsSpd);
    return iTimeReq;
}

void B9Terminal::onScreenCountChanged(int iCount){
    QString sVideo = "Disconnected or Primary Monitor";
    if(pProjector) {
        delete pProjector;
        pProjector = NULL;
        if(pPrinterComm->getProjectorStatus()==B9PrinterStatus::PS_ON)
            if(!isEnabled())emit signalAbortPrint("Print Aborted:  Connection to Projector Lost or Changed During Print Cycle");
    }
    pProjector = new B9Projector(true, 0,Qt::WindowStaysOnTopHint);
    makeProjectorConnections();
    int i=iCount;
    int screenCount = m_pDesktop->screenCount();
    QRect screenGeometry;

    if(m_bUsePrimaryMonitor)
    {
        screenGeometry = m_pDesktop->screenGeometry(0);
    }
    else{
        for(i=screenCount-1;i>= 0;i--) {
            screenGeometry = m_pDesktop->screenGeometry(i);
            if(screenGeometry.width() == pPrinterComm->getNativeX() && screenGeometry.height() == pPrinterComm->getNativeY()) {
                //发现投影机!
                sVideo = "Connected to Monitor: " + QString::number(i+1);
                m_bNeedsWarned = true;
                break;
            }
        }
    }
    if(i<=0||m_bUsePrimaryMonitor)m_bPrimaryScreen = true; else m_bPrimaryScreen = false;

    emit updateProjectorOutput(sVideo);

    pProjector->setShowGrid(true);
    pProjector->setCPJ(NULL);

    emit sendStatusMsg("B9Creator - Idle");
    pProjector->setGeometry(screenGeometry);
    if(!m_bPrimaryScreen){
        pProjector->showFullScreen(); // 只显示它，如果它是一个辅助监视器
        pProjector->hide();
        activateWindow(); // 如果不使用主显示器，采取焦点回到这里。
    }
    else if(m_bPrintPreview||(pPrinterComm->getProjectorStatus() != B9PrinterStatus::PS_OFF &&
            pPrinterComm->getProjectorStatus() != B9PrinterStatus::PS_COOLING &&
            pPrinterComm->getProjectorStatus() != B9PrinterStatus::PS_UNKNOWN)) {
        // 如果投影机不会关闭，我们最好设置黑屏
    if(getIsVirtualDevice())
    {
                screenGeometry.setY(20);
                screenGeometry.setWidth(1024);
                screenGeometry.setHeight(768);
                pProjector->setGeometry(screenGeometry);
                pProjector->show();
    }else{

            pProjector->showFullScreen();
        }
    }
    //else warnSingleMonitor();
}

void B9Terminal::createNormalizedMask(double XYPS, double dZ, double dOhMM)
{
    //当我们显示或调整时调用
    pProjector->createNormalizedMask(XYPS, dZ, dOhMM);
}


void B9Terminal::on_comboBoxXPPixelSize_currentIndexChanged(int index)
{
    QString sCmd;
    bool bUnChanged = false;
    switch (index){
        case 0: // 50 微米
            sCmd = "U50";
            if(getXYPixelSize()==50)bUnChanged=true;
            break;
        case 1: // 75 微米
            sCmd = "U75";
            if(getXYPixelSize()==75)bUnChanged=true;
            break;
        case 2: // 100 微米
            sCmd = "U100";
            if(getXYPixelSize()==100)bUnChanged=true;
        default:
            break;
    }
    if(!bUnChanged){
        pPrinterComm->SendCmd(sCmd);
        pPrinterComm->SendCmd("A"); // 强制刷新打印机状态
    }
}

void B9Terminal::on_pushButtonCycleSettings_clicked()
{
    pSettings->updateValues();
}

void B9Terminal::getKey(int iKey)
{
    if(!m_bPrimaryScreen)return; // 忽略从打印窗口击键除非我们正在使用主监视器
    if(isVisible()&&isEnabled())
    {
        // 我们必须“校准”。如果我们得到任何按键，我们应该关闭投影机窗口
        if(pProjector!=NULL) pProjector->hide();
    }
    switch(iKey){
    case 112:		// 按下'P' 暂停/恢复
        emit pausePrint();
        break;
    case 65:        // 按下 'A'中止
        if(!isEnabled()){
            m_pPReleaseCycleTimer->stop();
            emit signalAbortPrint("User Directed Abort.");
        }
        break;
    default:
        break;
    }
}


