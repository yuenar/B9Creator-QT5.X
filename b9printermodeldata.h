#ifndef B9PRINTERMODELDATA_H
#define B9PRINTERMODELDATA_H

#include <QObject>
#include <QVector>
#include <QList>
#include "b9material.h"

class QString;



class b9PrinterModelData
{
public:
    b9PrinterModelData();
    b9PrinterModelData(QString modelName);
    ~b9PrinterModelData();

    QString GetName() const;
    void AddXYPixelSize(double size){m_dXYPixelSizes.append(size);}
    void SetZStepSizeMicrons(double stepSize){m_dStepSizeMicrons = stepSize;}
    void SetMaxSteps(int maxSteps){m_iMaxSteps = maxSteps;}


    void ClearMaterials();
    void AddMaterial(B9Material mat);
    QVector<B9Material>* GetMaterials();// 只是返回指向实际材料的指针。
    B9Material* FindMaterialByLabel(QString label);
    double GetXYSizeByIndex(int index);

private:
    QList<double> m_dXYPixelSizes;
    double m_dStepSizeMicrons;
    int m_iMaxSteps;
    QString m_sModelName;//命名 例如B9C1
    QVector<B9Material> m_Materials;//与此打印机关联的材料。
};





#endif // B9PRINTERMODELDATA_H
