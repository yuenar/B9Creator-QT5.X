#ifndef B9UPDATEENTRY_H
#define B9UPDATEENTRY_H
#include <QString>




struct B9UpdateEntry
{
    QString localLocationTag;
    QString fileName;
    int version;
    QString OSDir;//使用标志位提醒，从子目录下载。
};


#endif // B9UPDATEENTRY_H
