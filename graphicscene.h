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
using namespace arma;
using namespace GEOM_FADE2D;
class GraphicScene : public QGraphicsScene
{
    Q_OBJECT
public:
    int getMapData(int key);
    explicit GraphicScene(QObject *parent = 0);
    void setQualPoints(int n);
    void get_tri2d_E(mat v, double nr_nodes, double nr_trs, mat nds, mat trs);
    void show_mesh(mat v, double nr_trs, mat nds, mat trs);
    void setActiveDrawer(int type);
    bool find_nearest_point(QPointF p);
    QColor interpolate(double value, double max);
    QColor interpolate1(double ratio, double max);
    mat nodes, trs;
    QVector<std::pair<double, double>> nodes_vec;
    QVector<double>  trs_vec;
    std::vector<std::pair<double, double>> bcs_vec;
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
signals:
    void move_scene_sig(bool mode);
    void send_triangles(int tr);
    void mouse_positionChanged(QPointF pos);


public slots:
    void drawFinalRect();
    void drawFinalCircle();
    void connectPoints();
    void eraseAll();
    void newObj();
    void doTriangles();
    void recalcPoints(QList <QPointF> *current_p);
    void fullRecalc();
    void EnterPointsZone();
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
    QVector<QList <QPointF> *> inside_points;
    QList <QGraphicsItem *> m_items;
};


#endif // GraphicScene_H