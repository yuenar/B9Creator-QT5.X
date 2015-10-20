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

#include "b9updatemanager.h"
#include <QMessageBox>
#include <QNetworkConfiguration>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextStream>
#include <QFileDialog>
#include "OS_Wrapper_Functions.h"
#include <QFileInfo>
#include <stdio.h>
#include "loadingbar.h"

//B9Update Manger should be maintained such that an instance
//can be created without disrupting anything.
//only when update functions are called on the object should it
//alter files and such.
B9UpdateManager::B9UpdateManager(QObject *parent) :
    QObject(parent)
{

    //设置网络管理器连接
    netManager = new QNetworkAccessManager(this);
    connect(netManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(OnRecievedReply(QNetworkReply*)));

    //创建一个waitbar
    waitingbar = NULL;

    //当前没有下载
    currentReply = NULL;

    //设置定时器连接
    connect(&timeoutTimer,SIGNAL(timeout()),this,SLOT(OnDownloadTimeout()));

    downloadState = "Idle";//开关idle
    ignoreReplies = false;

    currentUpdateIndx = -1;

    silentUpdateChecking = false;

    RemoteManifestFileName = REMOTE_MANIFEST_FILENAME;
    RemoteManifestPath = REMOTE_MANIFEST_PATH;
}

B9UpdateManager::~B9UpdateManager()
{
    if(waitingbar) delete waitingbar;
    if(currentReply) delete currentReply;
}


void B9UpdateManager::ResetEverything()
{
    qDebug() << "UpdateManager: Reset Everything";
    if(currentReply != NULL)
    {
        currentReply->deleteLater();
        currentReply = NULL;
    }

    RemoteManifestFileName = REMOTE_MANIFEST_FILENAME;
    RemoteManifestPath = REMOTE_MANIFEST_PATH;

    waitingbar->hide();
    waitingbar->setMax(0);
    waitingbar->setMin(0);
    waitingbar->setDescription("Finished");


    timeoutTimer.stop();

    downloadState = "Idle";
    ignoreReplies = false;
    currentUpdateIndx = -1;

    updateEntries.clear();
    remoteEntries.clear();
    localEntries.clear();
}


void B9UpdateManager::StartNewDownload(QNetworkRequest request)
{
    //指派当前下载
    if(currentReply != NULL)
    {
        qDebug() << "UpdateManager: Danger!, Attempting to start new download with unfinished download.";
        return;
    }

    currentReply = netManager->get(request);
    connect(currentReply,SIGNAL(downloadProgress(qint64,qint64)),waitingbar,SLOT(setProgress(qint64,qint64)));
    connect(currentReply,SIGNAL(downloadProgress(qint64,qint64)),&timeoutTimer,SLOT(start())); //reset timer if we have progress.

    //现在我们启动超时定时器。
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(UPDATE_TIMOUT);
    waitingbar->setMax(0);
    waitingbar->setMin(0);

    if(downloadState == "DownloadingFileVersions")
    {
        if(!silentUpdateChecking) waitingbar->show();
        waitingbar->setDescription("Checking For Updates...");
    }
    if(downloadState == "DownloadingFiles")
    {
        waitingbar->show();
        waitingbar->setDescription("Downloading: " + QFileInfo(updateEntries[currentUpdateIndx].fileName).fileName() + "...");
    }
}

void B9UpdateManager::OnRecievedReply(QNetworkReply* reply)
{

    timeoutTimer.stop();
    if(ignoreReplies)
    {
        currentReply->deleteLater();
        currentReply = NULL;
        return;
    }


    if(reply->bytesAvailable() == 0)//没有下载。
    {
        qDebug() << "B9Update Manager, zero bytes downloaded.";
        if(!silentUpdateChecking)
        {
            QMessageBox msgBox;
            msgBox.setText("Online Update Attempt Failed.");
            if(downloadState == "DownloadingFileVersions")
            {
                waitingbar->hide();
                msgBox.setInformativeText("You may use a B9Creator update pack if available. \nBrowse to a local update pack now?");
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Cancel);
                int ret = msgBox.exec();

                if(ret == QMessageBox::Ok)
                {
                    ResetEverything();
                    PromptDoUpdates(true,true);
                    return;
                }//如果没有 - 继续下面的复位。
            }
            else
            {
                msgBox.setInformativeText("please check your internet connection.");
                msgBox.exec();
            }

        }
        ResetEverything();
        return;
    }

    if(downloadState == "DownloadingFileVersions")
    {
        //remoteEntries - localEntries = updateEntries
        CopyInRemoteEntries();
        CopyInLocalEntries();
        CalculateUpdateEntries();

        //所以现在我们有个需要更新本地的东西的列表...
        if(updateEntries.size() > 0)
        {
            waitingbar->hide();
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Question);
            if(updateEntries.size() == 1)
            {
                msgBox.setText("There is " + QString::number(updateEntries.size()) + " file update available.");
                msgBox.setInformativeText("Do you want to update it? <a href=\"http://b9creator.com/softwaredelta\">View change log</a>");
            }
            else
            {
                msgBox.setText("There are " + QString::number(updateEntries.size()) + " file updates available.");
                msgBox.setInformativeText("Do you want to update them? <a href=\"http://b9creator.com/softwaredelta\">View change log</a>");
            }
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);

            int doUpdates = msgBox.exec();

            currentUpdateIndx = 0;//在这一点上，我们知道我们可以遍历所需更新

            if(doUpdates == QMessageBox::Yes)
            {
                waitingbar->show();
                downloadState = "DownloadingFiles";//下一状态位
                //发送一个新的请求
                currentReply->deleteLater();
                currentReply = NULL;

                //请求服务器上正确路径中的文件
                QNetworkRequest request(QUrl(RemoteManifestPath + updateEntries[currentUpdateIndx].OSDir + updateEntries[currentUpdateIndx].fileName));


                StartNewDownload(request);

            }
            else//no
            {
                ResetEverything();
            }
        }
        else
        {
            ResetEverything();
            emit NotifyUpdateFinished();//发出更新完成信号
            if(!silentUpdateChecking)
            {
                QMessageBox msgBox;
                msgBox.setText("All files are up to date");
                msgBox.setIcon(QMessageBox::Information);
                msgBox.exec();
            }
        }

    }
    else if(downloadState == "DownloadingFiles")
    {
        //让我们把文件放到临时目录
        if(!OnDownloadDone())
        {
            //bail
            ResetEverything();
            return;
        }
        if(currentUpdateIndx < updateEntries.size() - 1)
        {
            currentUpdateIndx++;
            currentReply->deleteLater();
            currentReply = NULL;


            QNetworkRequest request(QUrl(RemoteManifestPath + updateEntries[currentUpdateIndx].OSDir + updateEntries[currentUpdateIndx].fileName));

            StartNewDownload(request);
        }
        else
        {
            if(!CopyFromTemp())
            {
                //bail
                qDebug() << "UpdateManager: Unable To Copy Files From Temp Directory";
                QMessageBox msgBox;
                msgBox.setText("Unable to update,\nUnable To Copy Files From Temp Directory.");
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.exec();
                ResetEverything();
                return;
            }

            if(!UpdateLocalFileVersionList())
            {
                qDebug() << "UpdateManager: Unable To overwrite local FileVersions.txt";
                QMessageBox msgBox;
                msgBox.setText("Unable to update,\nUnable To overwrite local FileVersions.txt.");
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.exec();
                ResetEverything();
                return;
            }

            waitingbar->hide();
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText("All files up to date");
            msgBox.setInformativeText("Please Re-Launch B9Creator.");
            msgBox.exec();

            ResetEverything();

            //安全地退出程序
            QCoreApplication::exit();
        }
    }
}

bool B9UpdateManager::OnDownloadDone()
{
    QString downloadFileName = QString(CROSS_OS_GetDirectoryFromLocationTag("TEMP_DIR") + "/" + updateEntries[currentUpdateIndx].fileName);
    QByteArray data = currentReply->readAll();

    //为所需临时文件创建一些文件夹.
    QDir().mkpath(QFileInfo(downloadFileName).absolutePath());


    QFile downloadCopy(downloadFileName);
    downloadCopy.open(QIODevice::WriteOnly | QIODevice::Truncate);
    int success = downloadCopy.write(data);
    downloadCopy.close();

    if(!success)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Unable to to copy download to temp directory");
        msgBox.exec();
        return false;
    }
    qDebug() << "UpdateManager: Downloaded " << updateEntries[currentUpdateIndx].fileName << " to temp folder";
    return true;
}

void B9UpdateManager::OnDownloadTimeout()
{
    ignoreReplies = true;
    ResetEverything();
    qDebug() << "UpdateManager: Timed Out";
    if(!silentUpdateChecking){
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Unable to update,\nplease check your internet connection.");
    msgBox.exec();
    }
}

void B9UpdateManager::OnCancelUpdate()
{
    ignoreReplies = true;
    timeoutTimer.stop();
    ResetEverything();
    QMessageBox msgBox;
    msgBox.setText("Update Cancelled");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
    qDebug() << "UpdateManager: User Aborted, Update Cancelled";
}


//拷贝已下载的版本文件导入到内部数据结构;
void B9UpdateManager::CopyInRemoteEntries()
{
    QString entryPlatform;

    remoteEntries.clear();


    currentReply->setTextModeEnabled(true);
    QTextStream inStream(currentReply);



    B9UpdateEntry entry;

    while(!inStream.atEnd())
    {
        inStream >> entry.localLocationTag;
        if(entry.localLocationTag.startsWith("//"))
        {   inStream.readLine();
            inStream.skipWhiteSpace();
            continue;
        }
        entry.fileName = StreamInTextQuotes(inStream);
        inStream >> entry.version;
        inStream >> entryPlatform;

        if(entryPlatform == "ANY")
        {
            entry.OSDir = "COMMON/";
            remoteEntries.push_back(entry);
        }
        else
        {
            #ifdef Q_OS_WIN
            if((entryPlatform == "WIN"))
            {
                entry.OSDir = "WINDOWS/";
            #endif
            #ifdef Q_OS_MAC
            if((entryPlatform == "MAC"))
            {
                entry.OSDir = "MAC/";
            #endif
            #ifdef Q_OS_LINUX
            if((entryPlatform == "LINUX"))
            {
                entry.OSDir = "LINUX/";
            #endif

                remoteEntries.push_back(entry);
            }
        }

        inStream.skipWhiteSpace();
    }
    qDebug() << "UpdateManager: " << remoteEntries.size() << " Remote Entrees Loaded";
}


void B9UpdateManager::CopyInLocalEntries()
{
    localEntries.clear();

    QFile localvfile(CROSS_OS_GetDirectoryFromLocationTag("APPLICATION_DIR") + "/FileVersions.txt");
    localvfile.open(QIODevice::ReadOnly);
    QTextStream stream(&localvfile);

    while(!stream.atEnd())
    {
        B9UpdateEntry newEntry;
        stream >> newEntry.localLocationTag;
        newEntry.fileName = StreamInTextQuotes(stream);

        stream >> newEntry.version;
//我们从本地fileversions.txt读取，有可能是本地文件丢失即使它被列在文本文件中，要解决这个问题，我们需要在实际工作检查文件是否存在。
//并设置版本一个非常低的数字，因此必须在稍后更新。
        if(!QFile::exists(CROSS_OS_GetDirectoryFromLocationTag(newEntry.localLocationTag) + "/" + newEntry.fileName))
            newEntry.version = -1;


        localEntries.push_back(newEntry);
        stream.skipWhiteSpace();//eat up new line
    }
    localvfile.close();

    qDebug() << "UpdateManager: " << localEntries.size() << " Local Entrees Loaded";
}


//比较远程条目和本地条目，并填充updateEntries列表，然后我们知道整个列表的文件已更新
void B9UpdateManager::CalculateUpdateEntries()
{
    int r;
    int l;
    bool dne;
    updateEntries.clear();
    for(r = 0; r < remoteEntries.size(); r++)
    {
        dne = true;
        for(l = 0; l < localEntries.size(); l++)
        {
            if(localEntries[l].fileName == remoteEntries[r].fileName)
            {
                dne = false;
                if(NeedsUpdated(localEntries[l],remoteEntries[r]))
                {
                    updateEntries.push_back(remoteEntries[r]);
                }
            }
        }
        if(dne == true)//如果没有一个文件匹配的地方，我们还是要更新
        {
                updateEntries.push_back(remoteEntries[r]);
        }
    }
}


//此函数以确定是否一个条目需要更新。
bool B9UpdateManager::NeedsUpdated(B9UpdateEntry &candidate, B9UpdateEntry &remote)
{


    if(candidate.version < remote.version)
        return true;

    return false;
}



void B9UpdateManager::PromptDoUpdates(bool showCheckingBar, bool promptLocalLocation)
{

    silentUpdateChecking = !showCheckingBar;
    //如果状态机什么都不做，就有必要考虑更新了
    if(downloadState != "Idle" && !silentUpdateChecking)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("B9Creator is already checking for updates");
        msgBox.exec();
        return;
    }


    if(netManager->networkAccessible() == QNetworkAccessManager::NotAccessible && !silentUpdateChecking)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("B9Creator does not have accesss to the network");
        msgBox.exec();
        return;

    }
    //最好做其他一些早期的检查，如果可能的话。



    //创建一个waitingbar
    waitingbar = new LoadingBar(0,0,false);
    if(silentUpdateChecking) waitingbar->hide();
    connect(waitingbar,SIGNAL(rejected()),this,SLOT(OnCancelUpdate()));


    //获得网络访问管理器找到清单文件versions.txt;
    QNetworkRequest request;

    if(promptLocalLocation)//提示用户输入新的清单位置
    {
        QString path = QFileDialog::getExistingDirectory(NULL,
                       "Locate the B9Creator Update Pack Folder",
                       CROSS_OS_GetDirectoryFromLocationTag("DOCUMENTS_DIR"));
        qDebug() << "Opening Update Pack: " << path;
        if(path.isEmpty())
            return;

        QString manifestURL = QUrl().fromLocalFile(path + "/" + RemoteManifestFileName).toString();
        RemoteManifestPath = QUrl().fromLocalFile(path).toString();
        request.setUrl(QUrl(manifestURL));
    }
    else//用户上网清单位置。
    {
        request.setUrl(QUrl(RemoteManifestPath + RemoteManifestFileName));
    }


    //设置状态
    downloadState = "DownloadingFileVersions";

    qDebug() << "UpdateManager: User Started Update Check and Download";

    StartNewDownload(request);
}


bool B9UpdateManager::CopyFromTemp()
{
    qDebug() << "UpdateManager: Copying downloaded files from temp into actuall locations...";
    waitingbar->setDescription("Copying...");
    waitingbar->setMax(updateEntries.size());

    for(int i = 0; i < updateEntries.size(); i++)
    {

        QString src = CROSS_OS_GetDirectoryFromLocationTag("TEMP_DIR") + "/" + updateEntries[i].fileName;
        QString dest = CROSS_OS_GetDirectoryFromLocationTag(updateEntries[i].localLocationTag) + "/"
                + updateEntries[i].fileName;


        qDebug() << "from: " << src;
        qDebug() << "to: " << dest;

        if(QFile::exists(dest))
        {   //如果是更新可执行文件 - 我们必须重命名而不是删除它（旧的那个）......

            #ifdef Q_OS_WIN
            if(updateEntries[i].fileName == "B9Creator.exe")
            {
            #endif
            #ifdef Q_OS_MAC
            if(updateEntries[i].fileName == "B9Creator")
            {
            #endif
            #ifdef Q_OS_LINUX
            if(updateEntries[i].fileName == "B9Creator")
            {
            #endif

                //删除任何旧的b9creator.exe.old文件
                if(QFile::exists(QString(dest).append(".old")))
                    QFile::remove(QString(dest).append(".old"));

                //重命名我们正在运行的可执行文件以.old为后缀
                if(rename(dest.toLatin1(),
                           QString(dest).append(".old").toLatin1()))
                    return false;
            }
            else
                QFile::remove(dest);
        }

        //我们需要作出必要的子目录用来复制。
         QDir().mkpath(QFileInfo(dest).absolutePath());

        if(!QFile::copy(src,dest))
            return false;
        else
        {
            //这时我们已经复制好文件。
            //在UNIX上 - 我们必须将文件标记为可执行文件。
            if(updateEntries[i].fileName == "B9Creator")
            {
                #ifdef Q_OS_MAC
                system(QString("chmod +x " + dest).toLatin1());
                #endif
                #ifdef Q_OS_LINUX
                    system(QString("chmod +x \"" + dest + "\"").toLatin1());
                #endif
            }
            if(updateEntries[i].fileName == "avrdude")
            {
                qDebug() << "APPLYING EXECUTABLNESS TO AVRDUDE!";
                #ifdef Q_OS_MAC
                    system(QString("chmod +x \"" + dest + "\"").toLatin1());
                #endif
                #ifdef Q_OS_LINUX
                    system(QString("chmod +x \"" +  dest + "\"").toLatin1());
                #endif
            }
            if(updateEntries[i].fileName == "avrdude.conf")
            {
                #ifdef Q_OS_MAC
                system(QString("chmod +x \"" + dest + "\"").toLatin1());
                #endif
                #ifdef Q_OS_LINUX
                system(QString("chmod +x \"" + dest + "\"").toLatin1());
                #endif
            }
        }

        waitingbar->setValue(i);

    }

    return true;
}


bool B9UpdateManager::UpdateLocalFileVersionList()
{
    int i;
    qDebug() << "UpdateManager: Updating Local File Versions List...";

    QFile vfile(CROSS_OS_GetDirectoryFromLocationTag("APPLICATION_DIR") + "/FileVersions.txt");
    if(!vfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    QTextStream outStream(&vfile);

    for(i = 0; i < remoteEntries.size(); i++)
    {
        outStream << remoteEntries[i].localLocationTag;
        outStream << " ";
        outStream << "\"" << remoteEntries[i].fileName << "\"";
        outStream << " ";
        outStream << remoteEntries[i].version;
        if(i != remoteEntries.size() - 1)
            outStream << "\n";

    }

    vfile.close();
    return true;
}


//将文件移动到正确的位置或删除无用的东西。
void B9UpdateManager::TransitionFromPreviousVersions()
{
#ifndef Q_OS_LINUX
    //old 1.4 files in the wrong location
    QFile::remove(CROSS_OS_GetDirectoryFromLocationTag("EXECUTABLE_DIR") + "/B9Creator_LOG.txt");
    QFile::remove(CROSS_OS_GetDirectoryFromLocationTag("EXECUTABLE_DIR") + "/B9Firmware_1_0_9.hex");
    QFile::remove(CROSS_OS_GetDirectoryFromLocationTag("EXECUTABLE_DIR") + "/avrdude.exe");
    QFile::remove(CROSS_OS_GetDirectoryFromLocationTag("EXECUTABLE_DIR") + "/avrdude");
    QFile::remove(CROSS_OS_GetDirectoryFromLocationTag("EXECUTABLE_DIR") + "/avrdude.conf");
#endif
    //旧料文件应该由b9matcat寻找和清除。

    //检查旧的可执行文件并将其删除
    QFile(CROSS_OS_GetDirectoryFromLocationTag("EXECUTABLE_DIR") + "/B9Creator.exe.old").remove();
}



//查找到本地清单和返回的本地版本，
//这是静态的，因为它可以在程序的任何地方访问。
//返回-1，如果文件不存在，但在清单中。
//返回-2，如果该文件不存在，并且是不在清单！
int B9UpdateManager::GetLocalFileVersion(QString filename)
{
    unsigned int i;

    B9UpdateManager tempManager;

    tempManager.CopyInLocalEntries();

    for(i = 0; i < tempManager.localEntries.size(); i++)
    {
        if(tempManager.localEntries[i].fileName == filename)
        {
            return tempManager.localEntries[i].version;
        }
    }
    return -2;
}



