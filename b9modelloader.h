#ifndef B9MODELLOADER_H
#define B9MODELLOADER_H

#include <QObject>
#include <QFile>
#include <QTextStream>


//简单结构三角数据读入
//TODO假设浮点数在编译平台上是32位
struct STLTri
{
    float nx,ny,nz;
//根据STL定义，该模型被认为是唯一积极的坐标 使用unsigned值...
    float x0,y0,z0;
    float x1,y1,z1;
    float x2,y2,z2;
};


class B9ModelLoader : public QObject
{
    Q_OBJECT
public:
    explicit B9ModelLoader( QString filename, bool &readyRead, QObject *parent = 0);
    ~B9ModelLoader();

    bool LoadNextTri(STLTri *&tri, bool &errorFlag);
    float GetPercentDone();
    QString GetError(){return lastError;}//在加载过程中发生任何错误都将返回。


signals:
    void PercentCompletedUpdate(qint64 frac, qint64 total);
    
public slots:

private:

    QString fileType;// "BIN_STL", "ASCII_STL", "UNCOMPRESSED_AMF"......
    QFile asciifile;
    QTextStream asciiStream;
    QFile binfile;

    quint32 triCount;
    unsigned long int byteCount;

    QString lastError;

    float lastpercent;
    QString DetermineFileType(QString filename, bool &readyRead);
    void PrepareSTLForReading(QString filename, bool &readyRead);
    void PrepareAMFForReading(){}
    bool ReadBinHeader();
    bool ReadAsciiHeader();
    bool CheckBinFileValidity();
};

#endif // B9MODELLOADER_H
