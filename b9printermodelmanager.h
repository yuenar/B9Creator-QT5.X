#ifndef B9PRINTERMODELMANAGER_H
#define B9PRINTERMODELMANAGER_H

#include <QObject>
#include <QVector>
#include "b9printermodeldata.h"

class QString;


class b9PrinterModelManager : public QObject
{
    Q_OBJECT
public:
    explicit b9PrinterModelManager(QObject *parent = 0);
    ~b9PrinterModelManager();

    void ImportDefinitions(QString defFilePath);//从给定的定义类型文件导入

    b9PrinterModelData* GetCurrentOperatingPrinter();
    std::vector<b9PrinterModelData*> GetPrinterModels();
    bool SetCurrentOperatingPrinter(b9PrinterModelData* modelDataPtr);
    bool SetCurrentOperatingPrinter(QString modelName);
    b9PrinterModelData* FindPrinterDataByName(QString modelName);





signals:
    void FilesUpdated();
    
public slots:


private:
    //成员
    std::vector<b9PrinterModelData*> m_PrinterDataList;
    b9PrinterModelData* m_CurrentOperatingPrinter;

    //函数
    b9PrinterModelData* AddPrinterData(QString modelName);





};

#endif // B9PRINTERMODELMANAGER_H
