#ifndef B9TESSELATOR_H
#define B9TESSELATOR_H

#ifndef CALLBACK
#define CALLBACK
#endif

#include <QVector2D>
#include "loop.h"
#include <vector>
#include <QtOpenGL>
#include <OS_GL_Wrapper.h>


class B9Tesselator;
// CALLBACK functions for GLU_TESS ////////////////////////////////////////////
// NOTE: must be declared with CALLBACK
void CALLBACK tessBeginCB(GLenum which, void *user_data);
void CALLBACK tessEndCB();
void CALLBACK tessErrorCB(GLenum errorCode, void* user_data);
void CALLBACK tessVertexCB(const GLvoid *data, void *user_data);
void CALLBACK tessCombineCB(const GLdouble newVertex[3], const GLdouble *neighborVertex[4],
                            const GLfloat neighborWeight[4], GLdouble **outData, void* user_data);





class B9Tesselator
{
public:
    B9Tesselator();
    ~B9Tesselator();

    //输入plygonList必须按顺序 - 描绘填充值或void值
    int Triangulate( const std::vector<QVector2D> *polygonList, std::vector<QVector2D> *triangleStrip);
    std::vector<QVector2D>* GetTrangleStrip();


    GLenum currentEnumType;
    bool fanFirstTri;
    bool fanSecondTri;
    bool stripFirstTri;
    bool stripSecondTri;
    unsigned long int stripCount;
    QVector2D fanOriginVertex;
    QVector2D prevVertex;
    QVector2D prevPrevVertex;

    //为triangulator建立48兆字节的缓冲区
    unsigned int CombineSize;
    GLdouble Combinevertices[2048][6];               //二维数组来存储新创建的顶点（X，Y，Z，R，G，B）通过联合回调
    unsigned int CombineVertexIndex;
    bool memoryFull;

    int errorAcumulations;
private:
    unsigned long int numPolyVerts;
    GLdouble ** polyverts;//指向所有的顶点数据的指针
    std::vector<QVector2D>* triStrip;


};

#endif // B9TESSELATOR_H
