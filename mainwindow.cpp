#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "OS_Wrapper_Functions.h"
#include "b9printermodelmanager.h"
#include "b9updatemanager.h"
#include "b9supportstructure.h"
#include "b9layout/b9layoutprojectdata.h"
#include <QDebug>

#define B9CVERSION "Version 1.6.0     Copyright 2013 B9Creations, LLC     www.b9creator.com\n "


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Set up Identity
    QCoreApplication::setOrganizationName("B9Creations, LLC");
    QCoreApplication::setOrganizationDomain("b9creator.com");
    QCoreApplication::setApplicationName("B9Creator");

    ui->setupUi(this);
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);

    // 总是设置了日志管理器在主窗口构造时
    pLogManager = new LogFileManager(CROSS_OS_GetDirectoryFromLocationTag("DOCUMENTS_DIR") + "/B9Creator_LOG.txt", "B9Creator Log Entries");
        m_bOpenLogOnExit = false;
        qDebug() << "Program Start";
        qDebug() << "Relevent Used Application Directories";
        qDebug() << "   EXECUTABLE_DIR: " << CROSS_OS_GetDirectoryFromLocationTag("EXECUTABLE_DIR");
        qDebug() << "   APPLICATION_DIR: " << CROSS_OS_GetDirectoryFromLocationTag("APPLICATION_DIR");
        qDebug() << "   TEMP_DIR: " << CROSS_OS_GetDirectoryFromLocationTag("TEMP_DIR");
        qDebug() << "   DOCUMENTS_DIR: " << CROSS_OS_GetDirectoryFromLocationTag("DOCUMENTS_DIR");


    //创建更新管理器
    m_pUpdateManager = new B9UpdateManager(this);

        //do things like move, delete old files from previous
        //installations
        //m_pUpdateManager->TransitionFromPreviousVersions();

        //Schedule an auto update check
        //QTimer::singleShot(1000,m_pUpdateManager,SLOT(AutoCheckForUpdates()));


    //create Printer Model Manager, withough importing definitions - it will start with a default printer
    pPrinterModelManager = new b9PrinterModelManager(this);
        //pPrinterModelManager->ImportDefinitions(CROSS_OS_SPOT + "/B9Printer.DEF")
        //pPrinterModelManager->ImportMaterials();//looks at mat file and user registry.

    //import premade stls for support structures
    B9SupportStructure::ImportAttachmentDataFromStls();
        B9SupportStructure::FillRegistryDefaults();//if needed

    //创建终端
    pTerminal = new B9Terminal(QApplication::desktop());
    pTerminal->setEnabled(true);

    connect(pTerminal, SIGNAL(updateConnectionStatus(QString)), ui->statusBar, SLOT(showMessage(QString)));

    ui->statusBar->showMessage(MSG_SEARCHING);

    pMW1 = new B9Layout(0);
    pMW2 = new B9Slice(0,pMW1);
    pMW3 = new B9Edit(0);
    pMW4 = new B9Print(pTerminal, 0);

    m_pCPJ = new CrushedPrintJob;

    connect(pMW1, SIGNAL(eventHiding()),this, SLOT(handleW1Hide()));
    connect(pMW2, SIGNAL(eventHiding()),this, SLOT(handleW2Hide()));
    connect(pMW3, SIGNAL(eventHiding()),this, SLOT(handleW3Hide()));
    connect(pMW4, SIGNAL(eventHiding()),this, SLOT(handleW4Hide()));
}

MainWindow::~MainWindow()
{
    delete m_pCPJ;
    delete pTerminal;

    if(m_bOpenLogOnExit)
        pLogManager->openLogFileInFolder(); // 查看日志文件的位置
    delete pLogManager; //删除已记录抓取日志后所有的消息
    delete ui;
    delete pPrinterModelManager;//清空打印机模型管理器
    B9SupportStructure::FreeAttachmentData();
    delete pMW1;
    delete pMW2;
    delete pMW3;
    delete pMW4;


    qDebug() << "Program End";
}

void MainWindow::showSplash()
{
    if(m_pSplash!=NULL){
        m_pSplash->showMessage(B9CVERSION,Qt::AlignBottom|Qt::AlignCenter,QColor(255,130,36));
        m_pSplash->show();
        QTimer::singleShot(1000,this,SLOT(hideSplash()));
    }
}

void MainWindow::showAbout()
{
    if(m_pSplash!=NULL){
        m_pSplash->showMessage(B9CVERSION,Qt::AlignBottom|Qt::AlignCenter,QColor(255,130,36));
        m_pSplash->show();
    }
}

void MainWindow::showLayout()
{
    emit on_commandLayout_clicked(true);
}

void MainWindow::showSlice()
{
    emit on_commandSlice_clicked(true);
}

void MainWindow::showEdit()
{
    emit on_commandEdit_clicked(true);
}

void MainWindow::showPrint()
{
    emit on_commandPrint_clicked();
}

void MainWindow::showLogAndExit()
{
    m_bOpenLogOnExit = true;
    emit(this->close());
}

void MainWindow::showTerminal()
{
    pTerminal->showIt();
}


void MainWindow::showCalibrateBuildTable()
{
    if(!pTerminal->isConnected()){
        QMessageBox::information(this,"Printer Not Found", "You must be connected to the printer to Calibrate",QMessageBox::Ok);
        return;
    }

    dlgCalBuildTable dlgCalBT(pTerminal);
    dlgCalBT.exec();
}

void MainWindow::showCalibrateProjector()
{
    if(!pTerminal->isConnected()){
        QMessageBox::information(this,"Printer Not Found", "You must be connected to the printer to Calibrate",QMessageBox::Ok);
        return;
    }

    dlgCalProjector dlgCalProj(pTerminal);
    dlgCalProj.exec();
}

void MainWindow::showCatalog()
{
    pTerminal->dlgEditMatCat();
}

void MainWindow::showPrinterCycles()
{
    pTerminal->dlgEditPrinterCycleSettings();
}

void MainWindow::showHelp()
{
    m_HelpSystem.showHelpFile("index.html");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    pMW1->hide();
    pMW2->hide();
    pMW3->hide();
    pMW4->hide();
    pTerminal->hide();
    event->accept();
}

void MainWindow::handleW1Hide()
{
    this->show();
    ui->commandLayout->setChecked(false);
}
void MainWindow::handleW2Hide()
{
    this->show();
    ui->commandSlice->setChecked(false);
}
void MainWindow::handleW3Hide()
{
    this->show();
    //ui->commandEdit->setChecked(false);
}
void MainWindow::handleW4Hide()
{
    this->show();  //注释说明，如果没有隐藏主窗口同时显示该窗口
    ui->commandPrint->setChecked(false);
    pLogManager->setPrinting(false);
    pTerminal->setIsPrinting(false);
    CROSS_OS_DisableSleeps(false);// 返回系统屏保恢复正常。

}
void MainWindow::CheckForUpdates()
{
    m_pUpdateManager->PromptDoUpdates();
}
void MainWindow::OpenLayoutFile(QString file)
{
    showLayout();
    pMW1->ProjectData()->Open(file);
}
void MainWindow::OpenJobFile(QString file)
{
    AttemptPrintDialogWithFile(file);
}


void MainWindow::on_commandLayout_clicked(bool checked)
{
    if(checked) {
        pMW1->show();
        this->hide(); // 注释说明，如果没有隐藏主窗口同时显示该窗口
    }
    else pMW1->hide();
}
void MainWindow::on_commandSlice_clicked(bool checked)
{
    if(checked) {
        pMW2->show();
        this->hide(); // 注释说明，如果没有隐藏主窗口同时显示该窗口
    }
    else pMW2->hide();
}
void MainWindow::on_commandEdit_clicked(bool checked)
{
    if(checked) {
        pMW3->show();
        this->hide(); //注释说明，如果没有隐藏主窗口同时显示该窗口
    }
    else pMW3->hide();
}

void MainWindow::on_commandPrint_clicked()
{
    QFileDialog dialog(0);
    QSettings settings;
    QString openFile = dialog.getOpenFileName(this,"Select a B9Creator Job File to print", settings.value("WorkingDir").toString(), tr("B9Creator Job Files (*.b9j)"));
    if(openFile.isEmpty()) return;
    settings.setValue("WorkingDir", QFileInfo(openFile).absolutePath());

    AttemptPrintDialogWithFile(openFile);

}

void MainWindow::AttemptPrintDialogWithFile(QString openFile)
{
    /////////////////////////////////////////////////
    // 打开 .b9j文件
    m_pCPJ->clearAll();

    QFile file(openFile);
    if(!m_pCPJ->loadCPJ(&file)) {
        QMessageBox msgBox;
        msgBox.setText("Error Loading File.  Unknown Version?");
        msgBox.exec();
        return;
    }
    m_pCPJ->showSupports(true);
    int iXYPixelMicrons = m_pCPJ->getXYPixelmm()*1000;
    if( pTerminal->isConnected()&& iXYPixelMicrons != (int)pTerminal->getXYPixelSize()){
        QMessageBox msgBox;
        msgBox.setText("WARNING");
        msgBox.setInformativeText("The XY pixel size of the selected job file ("+QString::number(iXYPixelMicrons)+" 祄) does not agree with the Printer's calibrated XY pixel size ("+QString::number(pTerminal->getXYPixelSize())+" 祄)!\n\n"
                                  "Printing will likely result in an object with incorrect scale and/or apsect ratio.\n\n"
                                  "Do you wish to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        if(ret==QMessageBox::No)return;
    }


    m_pPrintPrep = new DlgPrintPrep(m_pCPJ, pTerminal, this);
    connect (m_pPrintPrep, SIGNAL(accepted()),this,SLOT(doPrint()));
    m_pPrintPrep->exec();
}




void MainWindow::doPrint()
{

    //打印使用由向导设置的变量...
    this->hide(); //注释说明，如果没有隐藏主窗口同时显示该窗口
    pMW4->show();
    pLogManager->setPrinting(false); // 设置为true以停止打印时抓取日志
    pTerminal->setIsPrinting(true);
    CROSS_OS_DisableSleeps(true);//禁用像屏幕保护程序 - 和电源选项。
    pMW4->print3D(m_pCPJ, 0, 0, m_pPrintPrep->m_iTbaseMS, m_pPrintPrep->m_iToverMS, m_pPrintPrep->m_iTattachMS, m_pPrintPrep->m_iNumAttach, m_pPrintPrep->m_iLastLayer, m_pPrintPrep->m_bDryRun, m_pPrintPrep->m_bDryRun, m_pPrintPrep->m_bMirrored);

    return;
}
