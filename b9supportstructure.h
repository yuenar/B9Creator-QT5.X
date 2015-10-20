#ifndef B9SUPPORTSTRUCTURE_H
#define B9SUPPORTSTRUCTURE_H


#include <vector>
#include <QVector3D>
#include <QListWidgetItem>
#include "b9layout/triangle3d.h"


#define SUPPORT_MID_PRINTABLE_ANGLE_RATIO 0.4 //(vertical_difference/mid_length)





class B9ModelInstance;

//该B9SupportAttachmentData所保存的信息pretaining
//为了支持附件的几何形状，名称等。
//类似于B9ModelData，数据将有助于避免不必要的几何
//复刻。
//此数据存储在静态列表中B9SupportStructure类。
//////////////////////////////////////////////////////////////////////////////////////
class B9SupportAttachmentData
{


private:
    std::vector<Triangle3D> triList;
    QString name;//支撑名字 例如 "Simple Cone"
    unsigned int dispListIndx;

public:
    B9SupportAttachmentData();

    ~B9SupportAttachmentData();


    void CenterGeometry();
    void FormDisplayList();
    bool HasDisplayList(){if(dispListIndx) return true; return false;}
    void Render();//在0,0绘制支撑几何形状，没有旋转。
    //设置
    void SetName(QString newName){name = newName;}

    //获取
    std::vector<Triangle3D>* GetTriangles(){return &triList;}
    QString GetName(){return name;}

};





//B9SupportStructure//
///////////////////////////////////////////////////////////////////////////
//  |
//  |   TOP
//  |
//  +
//  |
//  |
//  |   MID
//  |
//  |
//  +
//  |
//  |   BOTTOM
//  |
//
class B9SupportStructure
{

public: //枚举和静态变量

    static std::vector<B9SupportAttachmentData> AttachmentDataList;
    static void ImportAttachmentDataFromStls();
    static void FillRegistryDefaults( bool reset = false, QString supportWeight = "LIGHT");
    static void FreeAttachmentData(){}
    static unsigned char sColorID[3];

public:
    B9SupportStructure(B9ModelInstance* parent = NULL);
    ~B9SupportStructure();

    void AssignPickId();

    void CopyFrom(B9SupportStructure* referenceSupport);

    //获取主要特点
    QVector3D GetTopPoint();
    QVector3D GetTopPivot();
    QVector3D GetBottomPoint();
    QVector3D GetBottomPivot();
    double GetTopRadius();
    double GetMidRadius();
    double GetBottomRadius();
    double GetTopLength();
    double GetMidLength();
    double GetBottomLength();
    QVector3D GetTopNormal();
    QVector3D GetBottomNormal();
    double GetTopAngleFactor();
    double GetBottomAngleFactor();
    double GetTopPenetration();
    double GetBottomPenetration();
    QString GetTopAttachShape();
    QString GetMidAttachShape();
    QString GetBottomAttachShape();
    bool GetIsGrounded();



    bool IsUpsideDown();
    bool IsVertical();
    bool IsUnderground(double &depth);
    bool IsConstricted(double &depthPastContricted);

    //有效性检查
    bool IsPrintable();
        bool IsTopPrintable();
            bool IsTopAngleUp();
        bool IsMidPrintable();
        bool IsBottomPrintable();
            bool IsBottomAngleDown();

    bool IsVisible();



    //组特性
    void SetInstanceParent(B9ModelInstance* parentInst);
    void SetTopAttachShape(QString shapeName);
    void SetMidAttachShape(QString shapeName);
    void SetBottomAttachShape(QString shapeName);
    void SetTopPoint(QVector3D cord);
    void SetMidRadius(double rad);
    void SetTopRadius(double rad);
    void SetBottomRadius(double rad);
    void SetBottomPoint(QVector3D cord);
    void SetTopNormal(QVector3D normalVector);
    void SetBottomNormal(QVector3D normalVector);
    void SetTopAngleFactor(double factor);
    void SetBottomAngleFactor(double factor);
    void SetTopPenetration(double pen);
    void SetBottomPenetration(double pen);
    void SetTopLength(double len);
    void SetBottomLength(double len);
    void SetIsGrounded(bool grnd);

    void ForceUpdate();//从源值更新所有的值..

    //旋转
    void Rotate(QVector3D deltaRot);


    //选择
    void SetSelected(bool sel);//彩色渲染可用


    //绘制
    void RenderUpper(bool disableColor = false, float alpha = 1.0);//必须调用实例转换
    void RenderLower(bool disableColor = false, float alpha = 1.0);//必须调用实例转换

        void RenderTopGL();
        void RenderMidGL();
        void RenderBottomGL();
    void RenderPickGL(bool renderBottom = true, bool renderTop = true);
    void RenderPartPickGL(bool renderBottom = true, bool renderTop = true);//独立的可调用的渲染函数
    void EnableErrorGlow();
    void DisableErrorGlow();
    void DebugRender();
    void SetVisible(bool vis);




    //支撑
    unsigned int BakeToInstanceGeometry();
    std::vector<Triangle3D>* GetTriList();

    int SupportNumber;
    unsigned char pickcolor[3];//挑选颜色不基于静态递增变量。


private:

    B9ModelInstance* instanceParent;

    //所有向量和值被定义为以毫米为单位。
        //它们也定义为本地的模型数据！


    //已保存和源始的变量值
    QVector3D topPoint;//即“顶部”的支撑
    QVector3D bottomPoint;
    double topRadius, midRadius, bottomRadius;
    double length;//支撑总长
    double topLength;
    double bottomLength;
    double topPenetration;
    double bottomPenetration;
    QVector3D topNormal; //从顶部/底部的铰链一般会向外
    QVector3D bottomNormal;
    double topAngleFactor; //“百分比”使用真正正常。
    double bottomAngleFactor;
    B9SupportAttachmentData* midAttachShape;
    B9SupportAttachmentData* bottomAttachShape;
    B9SupportAttachmentData* topAttachShape;
    bool isGrounded;


    //计算助手变量（可以从源始变量在ForceUpdate导出）
    double midLength;
    double topMidExtension;
    QVector3D topMidExtensionPoint;
    double bottomMidExtension;
    QVector3D bottomMidExtensionPoint;
    QVector3D topPivot;
    QVector3D bottomPivot;
    QVector3D topPenetrationPoint;
    QVector3D bottomPenetrationPoint;
    double topThetaX, topThetaZ;
    double midThetaX, midThetaZ;
    double bottomThetaX, bottomThetaZ;


    //选择
    bool isSelected;

    //绘制
    bool isErrorGlowing;


private: //函数
    void ReBuildGeometry();
    bool isVisible;
};

#endif // B9SUPPORTSTRUCTURE_H
