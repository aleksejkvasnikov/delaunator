#ifndef GraphicScene_H
#define GraphicScene_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QList>
#include <QGraphicsItem>
#include <tr_object.h>
#include <include_fade2d/Fade_2D.h>
#include "armadillo"
#include "QColor"
#include "QVector"
#include "rectdetails.h"
#include "circledetails.h"
#include "pointszone.h"
#include "visualization.h"
using namespace arma;
using namespace GEOM_FADE2D;
class GraphicScene : public QGraphicsScene
{
    Q_OBJECT
public:
    int getMapData(int key);
    QVector<QList <QPointF> *> getInside(){return  inside_points;}
    int getInsideSize () {return inside_points.size();}
    QVector<QList <QPointF> *> getOutside(){return outside_points;}
    QVector<QList <QPointF> *> getDielectric(){return dielectric_points;}
    void SetInside(QVector<QList <QPointF> *> inside){inside_points=inside;}
    void SetOutside(QVector<QList <QPointF> *> outside){outside_points=outside;}
    void SetDielectric(QVector<QList <QPointF> *> dielectric){dielectric_points=dielectric;}
    explicit GraphicScene(QObject *parent = 0);
    void setQualPoints(int n);
    void get_tri2d_E(mat v, double nr_nodes, double nr_trs, mat nds, mat trs);
    void show_mesh(mat v, double nr_trs, mat nds, mat trs,double K, mat domains);
    void setActiveDrawer(int type);
    bool find_nearest_point(QPointF p);
    QColor interpolate(double value, double max);
    QColor interpolate1(double ratio, double max);
    QColor interpolate2(double ratio);
    mat nodesD, trsD;
    QVector<std::pair<double, double>> nodes_vec;
    QVector<double>  trs_vec;
    std::vector<std::pair<double, double>> bcs_vec;
    std::vector<std::pair<double, double>> domains;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * mouseEvent);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent);
    void setParams(double mAngle, double minELen, double maxELen){
        minAngle = mAngle;
        minEdgeLen = minELen;
        maxEdgeLen = maxELen;
    }
    bool drawingMode = false;
    std::vector<Triangle2*> vZoneT;
    PointsZone *pointszone;
    visualization *visual;
signals:
    void move_scene_sig(bool mode);
    void send_triangles(int tr);
    void mouse_positionChanged(QPointF pos);
    void RefinementDone (double nr_trs, double nr_nds);


public slots:
    void drawLoad();
    void drawFinalRect();
    void drawFinalCircle();
    void connectPoints();
    void eraseAll();
    void newObj();
    void doTriangles();
    void recalcPoints(QList <QPointF> *current_p);
    void fullRecalc();
    void EnterPointsZone();
    QPointF calcMidGradPoint(QPointF Pmax,QPointF Pmin,double vmax,double vmid);
    void RefinementMesh(mat trs,mat v, mat nds, double K);
    void potential_line_plot(QList<QPolygonF> poly_list, QVector<double> Vecv);
    void potential_line_calc();
    void Sort_Vpoints (QVector< QPointF > temppoints, QPolygonF &poly);
    void Sort_Vpoints_new (QVector< QPointF > &temppoints,QPolygonF &poly, QMap<std::pair<float,float>,std::pair<int,int>> p_trs);
    static bool sortonx_up(const QPointF &p1, const QPointF &p2);
    static bool sort_right_rotate(const QPointF &p1, const QPointF &p2, const QPointF &p3);
    static bool sort_on_map(const QPointF &p1, const QPointF &p2, QMap<std::pair<float,float>,std::pair<int,int>> p_trs);
    float distance(const QPointF& pt1, const QPointF& pt2);
    QPointF getLineStart(const QPointF& pt1, const QPointF& pt2);
    QPointF getLineEnd(const QPointF& pt1, const QPointF& pt2);
    void RectMesh (mat trs, mat nds, mat v);
    void RectMesh2 (mat trs, mat nds, mat v);
    bool IsPIn_Vector (double aAx,double aAy, double aBx,double aBy,double aCx, double aCy, double aPx,double aPy);
    bool IsPointIn_Geron (double aAx,double aAy, double aBx,double aBy,double aCx, double aCy, double aPx,double aPy);
    double potentialIn (double aAx,double aAy, double aBx,double aBy,double aCx, double aCy, double aPx,double aPy,
                        double v1,double v2,double v3);

private:
    int quapoints = 36;//кол-во точек по умолчанию
    int activeDrawer = 0;
    QPointF firstPoint;    
    rectDetails *rectDet;
    circledetails *circleDet;


    QMap<int,int> map;

    bool selectitem_b=false;
    QPointF lastPoint;
    QPointF origin;
    QGraphicsRectItem * selectionRect;
    QGraphicsLineItem * preview_ellipse;
    double minAngle = 30, minEdgeLen = 0.01, maxEdgeLen = 50;
    QList <tr_object> objects;
    QList <QPointF> m_points;
    QVector<QList <QPointF> *> outside_points;
    QVector<QList <QPointF> *> inside_zero_points;
    QVector<QList <QPointF> *> dielectric_points;
    QVector<QList <QPointF> *> inside_points;
    QVector<QList <QPointF> *> gr_cond_points;
    QList <QGraphicsItem *> m_items;
};


#endif // GraphicScene_H
