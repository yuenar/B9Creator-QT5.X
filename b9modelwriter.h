#ifndef B9MODELWRITER_H
#define B9MODELWRITER_H

#include <QObject>
#include <QFile>
#include <QDataStream>



//用法: 创建一个B9ModelWriter对象要有文件名同时可读写,
//在使用WriteNextTri()方法时，开始填充三角数据;
//当完成填充模型三角数据操作后调用Finalize()方法;


class Triangle3D;
class B9ModelWriter : public QObject
{
    Q_OBJECT
public:

    explicit B9ModelWriter(QString filename, bool &readyWrite, QObject *parent = 0);
    ~B9ModelWriter();


signals:

    //void PercentCompletedUpdate(qint64 frac, qint64 total);

public:
    void WriteNextTri(Triangle3D* pTri);
    void Finalize();//追溯到文件的开头，写三个计数。

private:
    QFile binOUT;
    QDataStream binStream;
    unsigned long int triCount;

    void WriteHeader(quint32 triCount);


};

#endif // B9MODELWRITER_H
