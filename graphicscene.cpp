#include "graphicscene.h"
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <include_fade2d/Fade_2D.h>
#include "qmath.h"
#include <utility>
#include "QVector"
#include "armadillo"
#include "rectdetails.h"
#include "circledetails.h"
#include "pointszone.h"
#include "visualization.h"
#include <QPainter>
#include <QtAlgorithms>
#include <QMap>
using namespace arma;
// нажимаем new,
// галочкой ставим режим
// рисуем что-то
// отображается в списке объектов - добавляется в класс сцены в QList
// нажимаем триангуляцию - для начала просто объединение -> подумать как и где хранить информацию о полигонах, возможно в отдельном классе


using namespace GEOM_FADE2D;
class CustomParameters:public MeshGenParams
{
public:
        CustomParameters(Zone2* pZone):MeshGenParams(pZone)
        {
        }
        double getMaxTriangleArea(Triangle2* pT)
        {
                Point2 barycenter(pT->getBarycenter());
                if(barycenter.x()<20 && barycenter.y()<40)
                {
                        // Dense meshing in the lower left corner
                        return 0.5;
                }
                else
                {
                        // No density restriction otherwise
                        return DBL_MAX;
                }
        }
};
int GraphicScene::getMapData(int key)
{
    return map[key];
}

GraphicScene::GraphicScene(QObject *parent) :
    QGraphicsScene(parent)
{
    //outside_points = new QVector<QList <QPointF> *>;
    //inside_points = new QVector<QList <QPointF> *>;
    this->setBackgroundBrush(Qt::white);

    QPen penZero(Qt::darkGray, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen penLine(Qt::lightGray, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    this->addLine(-3600, 0, 3600, 0, penZero);
    this->addLine(0, -1800, 0, 1800, penZero);
    for(int j=-100; j<100; j++){
        this->addLine(-3600, (j*10), 3600, (j*10), penLine);
        this->addLine((j*10), -1800, (j*10), 1800, penLine);
       // this->addLine(-160+(j*5), -2000, -160+(j*5), 1000);
    }

    for(int j=-3600; j<3600; j=j+50){
        QGraphicsTextItem *x_p = this->addText(QString::number(j));
        x_p->setDefaultTextColor(Qt::black);
        x_p->setPos(j, 0);


        QGraphicsTextItem *y_p = this->addText(QString::number(j*-1));
        y_p->setDefaultTextColor(Qt::black);
        y_p->setPos(0, j);
    }

    rectDet = new rectDetails;
    circleDet = new circledetails;
    pointszone = new PointsZone;
    QObject::connect(rectDet, SIGNAL(detail_entered()), this, SLOT(drawFinalRect()));
    QObject::connect(circleDet, SIGNAL(detail_entered()), this, SLOT(drawFinalCircle()));

}

void GraphicScene::setQualPoints(int n)
{
        this->quapoints = n;
}
void GraphicScene::setActiveDrawer(int type)
{
    this->activeDrawer = type;
}

bool GraphicScene::find_nearest_point(QPointF p)
{
    bool result = false;
    for(int i=0; i<nodes_vec.size(); i++){
        //qDebug() << i;
        if(abs(nodes_vec.at(i).first-p.rx())<=5 && abs(nodes_vec.at(i).second-p.ry())<=5)
        {
            //qDebug() << "here = " << i+1;
            bcs_vec.push_back(std::make_pair(i, 1));
            //qDebug () << i;
            result = true;
            break;
        }
    }
    return result;
}

void GraphicScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
    qDebug() << Q_FUNC_INFO << mouseEvent->scenePos();
    QGraphicsScene::mouseDoubleClickEvent(mouseEvent);
}

void GraphicScene::mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
    emit mouse_positionChanged(mouseEvent->scenePos());
    if (selectitem_b && this->activeDrawer == 2){
       // qDebug() << "here";
        //qDebug() << origin << mouseEvent->pos();
        selectionRect->setRect(origin.x()<mouseEvent->scenePos().x() ? origin.x() : mouseEvent->scenePos().x(), origin.y()<mouseEvent->scenePos().y() ?
                    origin.y() : mouseEvent->scenePos().y(), abs(mouseEvent->scenePos().x()-origin.x()), abs(mouseEvent->scenePos().y()-origin.y()));
    } else if(selectitem_b && this->activeDrawer == 3){
        preview_ellipse->setLine(origin.x(), origin.y(), mouseEvent->scenePos().x(), mouseEvent->scenePos().y());
    }
    //qDebug() << Q_FUNC_INFO << mouseEvent->scenePos();
   /* if(mouseEvent->buttons() == Qt::LeftButton){
        movePoint = mouseEvent->scenePos();
        //this->removeItem(selectionRect);
        selectionRect = this->addRect(origin.x()<movePoint.x() ? origin.x() : movePoint.x(), origin.y()<movePoint.y() ?
            origin.y() : movePoint.y(), abs(movePoint.x()-origin.x()), abs(movePoint.y()-origin.y()));
    } */
    QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void GraphicScene::mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
    origin = mouseEvent->scenePos();
    if(mouseEvent->button() == Qt::RightButton)
    {
        emit move_scene_sig(true);
    } else {
        if (!selectitem_b && this->activeDrawer == 2){
            selectionRect = this->addRect(0,0,0,0);
            selectitem_b = true;
            QPen pen(Qt::cyan, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            selectionRect->setPen(pen);
        } else if(!selectitem_b && this->activeDrawer == 3){
            preview_ellipse = this->addLine(0,0,0,0);
            selectitem_b = true;
            QPen pen(Qt::cyan, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            preview_ellipse->setPen(pen);
        }
        else{
            QGraphicsScene::mousePressEvent(mouseEvent);
        }
    }
}

void GraphicScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * me)
{
    if(me->button() == Qt::RightButton)
    {
        emit move_scene_sig(false);
    } else {
        if (selectitem_b && this->activeDrawer == 2){
            selectionRect->setRect(0,0,0,0);
            selectitem_b = false;
        } else if(selectitem_b && this->activeDrawer == 3){
            preview_ellipse->setLine(0,0,0,0);
            selectitem_b = false;
        }
        //qDebug() << Q_FUNC_INFO << me->scenePos();
        if(this->activeDrawer == 1){
            // а вот тут надо крепко подумать
            int radius = 2;
            QGraphicsEllipseItem * ellipse = this->addEllipse(me->scenePos().x() - radius, me->scenePos().y() - radius, radius*2, radius*2);


            //if (pointszone->getStatus())
            //{
            //    ellipse->setBrush(Qt::blue);
            //    inside_points.push_back(me->scenePos());
            //} else {
            //    ellipse->setBrush(Qt::red);
            //    outside_points.push_back(ne->scenePos());
            //}
            //m_points.append(me->scenePos());
        }
        if(this->activeDrawer == 4) // DELETE OR REDO
        {
            if(find_nearest_point(me->scenePos())){
                int radius = 1;
                QGraphicsEllipseItem * ellipse = this->addEllipse(me->scenePos().x() - radius, me->scenePos().y() - radius, radius*2, radius*2);

                ellipse->setBrush(Qt::red);
            }
        }
        if(this->activeDrawer == 3){ // отрисовка квадратом
            //создать объект класса
            lastPoint = me->scenePos();
            circleDet->setMinMax(round(origin.x()/10)*10, round(origin.y()/10)*10,
                               round(lastPoint.x()/10)*10, round(lastPoint.y()/10)*10);
            circleDet->show();
          /*  QGraphicsRectItem * rect = this->addRect(origin.x()<lastPoint.x() ? round(origin.x()/10)*10 : round(lastPoint.x()/10)*10,
                                                     origin.y()<lastPoint.y() ? round(origin.y()/10)*10 : round(lastPoint.y()/10)*10,
                                                     round(abs(lastPoint.x()-origin.x())/10)*10, round(abs(lastPoint.y()-origin.y())/10)*10);
            rect->setBrush(Qt::transparent);
            QRectF qrf = rect->rect();
            // LOOK HERE 16.10
            QList <QPointF> *temp_points = new QList <QPointF>;
            temp_points->append(qrf.topLeft());
            temp_points->append(qrf.topRight());
            temp_points->append(qrf.bottomRight());
            temp_points->append(qrf.bottomLeft());
            if(in_out_Mode){
                inside_points.push_back(temp_points);
            } else {
                outside_points.push_back(temp_points);
            }
    */

            //m_points.append(qrf.topLeft());
            //m_points.append(qrf.topRight());
            //m_points.append(qrf.bottomRight());
            //m_points.append(qrf.bottomLeft());

            // добавленике объекта в виджет-таблицу объектов
            //

        }
        if (this->activeDrawer == 2){ //алгоритм помтроения в графсцене
            //определение радиуса по положению точек начала и конца нажатия курсора      
            lastPoint = me->scenePos();
            rectDet->setMinMax(origin.x()<lastPoint.x() ? round(origin.x()/10)*10 : round(lastPoint.x()/10)*10,
                               origin.y()<lastPoint.y() ? round(origin.y()/10)*10 : round(lastPoint.y()/10)*10,
                               round(abs(lastPoint.x()-origin.x())/10)*10, round(abs(lastPoint.y()-origin.y())/10)*10);
            rectDet->show();
        }
        QGraphicsScene::mouseReleaseEvent(me);
    }
}

void GraphicScene::drawLoad()
{
    QPen red(Qt::red,Qt::DashLine);
    QPen blue(Qt::blue,Qt::DashLine);
    QPen green(Qt::green);
    for( int i=0; i<inside_points.size(); ++i )
    {
        for(int j=1; j<inside_points.at(i)->size();j++)
        this->addLine(inside_points.at(i)->at(j-1).x(),inside_points.at(i)->at(j-1).y(),
                      inside_points.at(i)->at(j).x(),
                      inside_points.at(i)->at(j).y(),blue);

        this->addLine(inside_points.at(i)->at(inside_points.at(i)->size()-1).x(),inside_points.at(i)->at(inside_points.at(i)->size()-1).y(),
                      inside_points.at(i)->at(0).x(),inside_points.at(i)->at(0).y(),blue);
    }
    for( int i=0; i<outside_points.size(); ++i )
    {
        for(int j=1; j<outside_points.at(i)->size();j++)
        this->addLine(outside_points.at(i)->at(j-1).x(),outside_points.at(i)->at(j-1).y(),
                      outside_points.at(i)->at(j).x(),
                      outside_points.at(i)->at(j).y(),red);

        this->addLine(outside_points.at(i)->at(outside_points.at(i)->size()-1).x(),outside_points.at(i)->at(outside_points.at(i)->size()-1).y(),
                      outside_points.at(i)->at(0).x(),outside_points.at(i)->at(0).y(),red);
    }
    for( int i=0; i<dielectric_points.size(); ++i )
    {
        for(int j=1; j<dielectric_points.at(i)->size();j++)
        this->addLine(dielectric_points.at(i)->at(j-1).x(),dielectric_points.at(i)->at(j-1).y(),
                      dielectric_points.at(i)->at(j).x(),
                      dielectric_points.at(i)->at(j).y(),green);

        this->addLine(dielectric_points.at(i)->at(dielectric_points.at(i)->size()-1).x(),dielectric_points.at(i)->at(dielectric_points.at(i)->size()-1).y(),
                      dielectric_points.at(i)->at(0).x(),dielectric_points.at(i)->at(0).y(),green);
    }
}

void GraphicScene::drawFinalRect()
{

    //qDebug () << "here";
    QGraphicsRectItem * rect = this->addRect(rectDet->getMinX(), rectDet->getMinY(), rectDet->getMaxX(), rectDet->getMaxY());
    rect->setBrush(Qt::transparent);
    QPen red_pen(Qt::red,Qt::DashLine);
    QPen blue_pen(Qt::blue,Qt::DashLine);
    QPen green_pen(Qt::green,Qt::DashLine);
    QPen dark_green(Qt::darkGreen,Qt::DashLine);
    QPen dark_blue (Qt::darkBlue,Qt::DashLine);
    QRectF qrf = rect->rect();
    // LOOK HERE 16.10
    QList <QPointF> *temp_points = new QList <QPointF>;
    temp_points->append(qrf.topLeft());
    temp_points->append(qrf.topRight());
    temp_points->append(qrf.bottomRight());
    temp_points->append(qrf.bottomLeft());
    if(rectDet->getStatus()==1){
        qDebug () << "inside";
        rect->setPen(blue_pen);
        inside_points.push_back(temp_points);
    } else if(rectDet->getStatus()==0){
        qDebug () << "outside";
        rect->setPen(dark_blue);
        outside_points.push_back(temp_points);
    }
    else if(rectDet->getStatus()==2) {
        rect->setPen(red_pen);
        dielectric_points.push_back(temp_points);
    }
    else {
        rect->setPen(dark_green);
        gr_cond_points.push_back(temp_points);
    }
    rectDet->hide();
}

void GraphicScene::drawFinalCircle()
{
    double R;
    QList <QPointF> *temp_points = new QList <QPointF>;
   // lastPoint = me->scenePos();
    R = sqrt(pow((circleDet->getX1() - circleDet->getX0()), 2) + pow((circleDet->getY1() - circleDet->getY0()), 2));
//определение координат 1 точки окружности
    double x;
    double y;
    x=circleDet->getX0() + R;
    y=circleDet->getY0();
    QPointF t;//точка для ввода ее координат в массив точек(не получилось корректно вводить сразу в массив)
    double anglepoint = 360/(circleDet->getPointNumber());//шаг построения точек
    for (int angle=0; angle<360;)//определение точек окружности
    {
    //определение координат из уравнения окружности
        x=R*cos((double)angle*M_PI/180.0)+circleDet->getX0();
        y=R*sin((double)angle*M_PI/180.0)+circleDet->getY0();
        qDebug() << x << y;
    //задание координат
        t.setX(x);
        t.setY(y);
        temp_points->append(t); //добавление координат в массив для триангуляции

    //построение точки
    int radius = 1;
       this->addEllipse(x - radius, y - radius, radius*2, radius*2);
       angle+=anglepoint;
    }
    if(circleDet->getStatus()==1){
        qDebug () << "inside";
        inside_points.push_back(temp_points);
    } else if(circleDet->getStatus()==0){
        qDebug () << "outside";
        outside_points.push_back(temp_points);
    }
    else{
        dielectric_points.push_back(temp_points);
    }
}

void GraphicScene::connectPoints()
{
    QList <QPointF> *temp_points = new QList <QPointF>;
    QPen red(Qt::red,Qt::DashLine);
    QPen blue(Qt::blue,Qt::DashLine);

    if (pointszone->getStatus()==1)
    {
        temp_points->append(m_points[0]);
        for( int i=1; i<m_points.count(); ++i )
        {
           this->addLine(m_points.at(i-1).x(),m_points.at(i-1).y(), m_points.at(i).x(),m_points.at(i).y(), red);
            temp_points->append(m_points[i]);
        }
        this->addLine(m_points.at(m_points.count()-1).x(),m_points.at(m_points.count()-1).y(), m_points.at(0).x(),m_points.at(0).y(), red);
        inside_points.push_back(temp_points);
    }
    else if(pointszone->getStatus()==0)
    {
        temp_points->append(m_points[0]);

        for( int i=1; i<m_points.count(); ++i )
        {
           this->addLine(m_points.at(i-1).x(),m_points.at(i-1).y(), m_points.at(i).x(),m_points.at(i).y(), blue);
            temp_points->append(m_points[i]);
        }
        this->addLine(m_points.at(m_points.count()-1).x(),m_points.at(m_points.count()-1).y(), m_points.at(0).x(),m_points.at(0).y(), blue);
        outside_points.push_back(temp_points);
    }
    else
    {
        temp_points->append(m_points[0]);

        for( int i=1; i<m_points.count(); ++i )
        {
           this->addLine(m_points.at(i-1).x(),m_points.at(i-1).y(), m_points.at(i).x(),m_points.at(i).y(), blue);
            temp_points->append(m_points[i]);
        }
        this->addLine(m_points.at(m_points.count()-1).x(),m_points.at(m_points.count()-1).y(), m_points.at(0).x(),m_points.at(0).y(), blue);
        dielectric_points.push_back(temp_points);
    }
    m_points.clear();
    setActiveDrawer(0);
}

void GraphicScene::eraseAll()
{
    m_points.clear();
    outside_points.clear();
    inside_points.clear();
    dielectric_points.clear();
    map.clear();
    this->clear();
    QGraphicsTextItem *zero_p = this->addText("0");
    zero_p->setDefaultTextColor(Qt::red);
    zero_p->setPos(0, 0);
    QPen penZero(Qt::darkGray, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen penLine(Qt::lightGray, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    this->addLine(-3600, 0, 3600, 0, penZero);
    this->addLine(0, -1800, 0, 1800, penZero);
    for(int j=-100; j<100; j++){
        this->addLine(-3600, (j*10), 3600, (j*10), penLine);
        this->addLine((j*10), -1800, (j*10), 1800, penLine);
       // this->addLine(-160+(j*5), -2000, -160+(j*5), 1000);
    }
    for(int j=-3600; j<3600; j=j+50){
        QGraphicsTextItem *x_p = this->addText(QString::number(j));
        x_p->setDefaultTextColor(Qt::black);
        x_p->setPos(j, 0);

        QGraphicsTextItem *y_p = this->addText(QString::number(j*-1));
        y_p->setDefaultTextColor(Qt::black);
        y_p->setPos(0, j);
    }

}

void GraphicScene::newObj()
{
    m_points.clear();
    outside_points.clear();
    inside_points.clear();
    if(drawingMode){ //по точкам - для прямоугольников автоматизировать
        //завершить редактирование предыдущего объекта
        //сохранить данный объект в виджет
    } else{
        drawingMode=true;
    }
}
void GraphicScene::get_tri2d_E(mat v, double nr_nodes, double nr_trs, mat nds, mat trs)
{
    double eps0 = (1.0/(36.0*M_PI))*pow(10.0,-9);
    mat d(3,3, fill::ones);
    mat E_mag(nr_trs,1, fill::zeros);
    mat Ex = E_mag;
    mat Ey = E_mag;
    //cout << trs;
    mat E_x(nr_trs,1, fill::zeros);
    mat E_y(nr_trs,1, fill::zeros);
    //mat E_mag(nr_trs,1, fill::zeros);
    double b1,b2,b3,c1,c2,c3;
    for(int i=0; i<nr_trs; i++){
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        d(0,1) = nds(n1,0); d(0,2) = nds(n1,1);
        d(1,1) = nds(n2,0); d(1,2) = nds(n2,1);
        d(2,1) = nds(n3,0); d(2,2) = nds(n3,1);
        mat tempest, temp;
        tempest << 1.0 << endr << 0.0 << endr << 0.0;
        temp = solve(d, tempest);
        b1 = temp(1,0);
        c1 = temp(2,0);
        tempest << 0.0 << endr << 1.0 << endr << 0.0;
        temp = solve(d, tempest);
        b2 = temp(1,0);
        c2 = temp(2,0);
        tempest << 0.0 << endr << 0.0 << endr << 1.0;
        temp = solve(d, tempest);
        b3 = temp(1,0);
        c3 = temp(2,0);
        E_x(i,0) = v(n1, 0)*b1 + v(n2,0)*b2 + v(n3, 0)*b3;
        E_y(i,0) = v(n1, 0)*c1 + v(n2,0)*c2 + v(n3, 0)*c3;
        E_mag(i,0) = pow(E_x(i, 0), 2) + pow(E_y(i,0), 2);

       // qDebug () << E_mag(i,0);
    }
    double E_peak = sqrt(E_mag.max());
    //qDebug () << E_peak;
    E_mag = sqrt(E_mag/E_mag.max());
   // E_x = (E_x)/E_x.max();
  //  E_y = (E_y)/E_y.max();

    //this->clear();

   /* for (int i=0; i<nr_trs; i++){
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        //qDebug() << n1 << n2 << n3 << E_mag(i,0)/E_mag.max();
        QPolygonF poly;
        poly << QPointF(nds(n1,0), nds(n1,1)) << QPointF(nds(n2,0), nds(n2,1)) << QPointF(nds(n3,0), nds(n3,1))<< QPointF(nds(n1,0), nds(n1,1));
        QPen pen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        this->addPolygon(poly, Qt::NoPen, interpolate2(E_mag(i,0)));
        QBrush brush(Qt::red);

        QPolygonF out;
        QList<QPolygonF> inside;
        out.append(outside_points.at(0)->toVector());
        for (int i=0; i<inside_points.size(); i++)
        {
            inside.push_back(inside_points.at(i)->toVector());
        }
        QPen bord_pen (Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        this->addPolygon(out,bord_pen,Qt::NoBrush);
        for (int i=0; i<inside.size();i++)
            this->addPolygon(inside.at(i),bord_pen,Qt::NoBrush);
    }*/

        for (int i=0; i<nr_trs; i++){ //постоение линий
            double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        double x0,y0,xn,yn;
        QPen pen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        x0=(nds(n1,0)+nds(n2,0)+nds(n3,0))/3;
        y0=(nds(n1,1)+nds(n2,1)+nds(n3,1))/3;
        qDebug()<<E_x(i,0)<<"   "<<E_y(i,0);
        xn=x0-E_x(i,0)/E_x.max()*10;
        yn=y0-E_y(i,0)/E_x.max()*10;
        double ostr = 0.25;        // острота стрелки
        this->addLine(x0, y0, xn, yn, pen);
        double x,y,lons,ugol;
        double f1x2 , f1y2;
        x = xn - x0;
        y = yn - y0;

        lons = sqrt(x*x + y*y) / 7;     // длина лепестков % от длины стрелки
        ugol = atan2(y, x);             // угол наклона линии

        //lons = 12;

        f1x2 = xn - lons * cos(ugol - ostr);
        f1y2 = yn - lons * sin(ugol - ostr);

        this->addLine(xn, yn, f1x2, f1y2,pen);

        f1x2 = xn - lons * cos(ugol + ostr);
        f1y2 = yn - lons * sin(ugol + ostr);

        this->addLine(xn, yn, f1x2, f1y2,pen);
    }

}

void GraphicScene::show_mesh(mat v, double nr_trs, mat nds, mat trs, double K, mat domains)
{
    //cout << v;
    double max = 0;

    for (int i=0; i<nr_trs; i++){
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
      //  if((v(n1)+v(n2)+v(n3)/3) > max) max = (v(n1)+v(n2)+v(n3)/3);
        if (v(n1)>max)
            max = v(n1);
        else if (v(n2)>max)
            max = v(n2);
        else if (v(n3)>max)
            max = v(n3);
    }

    for (int i=0; i<nr_trs; i++){
        double vmax;
        double vmin;
        double vmid;
        QPointF Pmax, Pmin,Pmid;
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        //qDebug() << n1 << n2 << n3 << E_mag(i,0)/E_mag.max();
        QPolygonF poly;
        poly << QPointF(nds(n1,0), nds(n1,1)) << QPointF(nds(n2,0), nds(n2,1)) << QPointF(nds(n3,0), nds(n3,1))<< QPointF(nds(n1,0), nds(n1,1));
        QPen pen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        // /////////////////////////////////////////
        if (v(n1)>=v(n2) && v(n1)>=v(n3))
            vmax = n1;
        else if (v(n2)>=v(n1) && v(n2)>=v(n3))
            vmax = n2;
        else if (v(n3)>=v(n1) && v(n3)>=v(n2))
            vmax = n3;

        if (v(n1)<=v(n2) && v(n1)<=v(n3))
            vmin = n1;
        else if (v(n2)<=v(n1) && v(n2)<=v(n3))
            vmin = n2;
        else if (v(n3)<=v(n1) && v(n3)<=v(n2))
            vmin = n3;

        if (n1!=vmax && n1!=vmin){
            vmid = n1;
        }
        else if (n2!=vmax && n2!=vmin){
            vmid = n2;
        }
        else if (n3!=vmax && n3!=vmin){
            vmid = n3;
        }



        //qDebug()<< v(n1) << v(n2) << v(n3);
        //qDebug()<< v(vmax) << v(vmin) << v(vmid);

        Pmax.setX(nds(vmax,0));
        Pmax.setY(nds(vmax,1));

        Pmin.setX(nds(vmin,0));
        Pmin.setY(nds(vmin,1));

        Pmid.setX(nds(vmid,0));
        Pmid.setY(nds(vmid,1));

        //cout << v(vmax) <<"\t" << v(vmid) << "\t" <<v(vmin);
        double maxV,midV,minV;
        maxV=v(vmax);
        midV=v(vmid);
        minV=v(vmin);
      //  qDebug()<<maxV<<midV<<minV;

        if (maxV==midV)
        {
            Pmax.setX(nds(vmax,0)+nds(vmid,0)/2);
            Pmax.setY(nds(vmax,1)+nds(vmid,1)/2);
            QLinearGradient grad(Pmin, Pmax);
            grad.setColorAt(0,interpolate1(v(vmin),max));
            grad.setColorAt(1,interpolate1(v(vmax),max));
            this->addPolygon(poly, Qt::NoPen, grad);
        }
        else if (minV==midV)
        {
            Pmin.setX(nds(vmin,0)+nds(vmid,0)/2);
            Pmin.setY(nds(vmin,1)+nds(vmid,1)/2);
            QLinearGradient grad(Pmin, Pmax);
            grad.setColorAt(0,interpolate1(v(vmin),max));
            grad.setColorAt(1,interpolate1(v(vmax),max));
            this->addPolygon(poly, Qt::NoPen, grad);
        }
        else {
            QPen pen1(Qt::white);
           QPointF Pmidgrad = calcMidGradPoint(Pmax,Pmin,v(vmax),v(vmid));
           double Amin,Amax,k;
           Amin = sqrt(pow((Pmidgrad.x()-Pmin.x()),2)+pow((Pmidgrad.y()-Pmin.y()),2));
           Amax = sqrt(pow((Pmax.x()-Pmin.x()),2)+pow((Pmax.y()-Pmin.y()),2));
           k=Amin/Amax;
           if (k>=1)
           {
               k=1;
           }
           QLinearGradient grad(Pmin,Pmax);
           grad.setColorAt(0,interpolate1(v(vmin),max));
           grad.setColorAt(k,interpolate1(v(vmid),max));
           grad.setColorAt(1,interpolate1(v(vmax),max));
           this->addPolygon(poly,Qt::NoPen,grad);
         //  this->addLine(Pmid.x(),Pmid.y(),Pmidgrad.x(),Pmidgrad.y(),pen1);

        }


        if (abs(v(n1)-v(n2))>K || abs(v(n2)-v(n3))>K || abs(v(n1)-v(n3))>K)
        {
            this->addPolygon(poly,pen,Qt::white);
        }

        //QLinearGradient grad(Pmin, Pmax);
        //grad.setColorAt(0,interpolate1(v(vmin),max));
        //grad.setColorAt(0.5,interpolate1(v(vmid),max));
        //grad.setColorAt(1,interpolate1(v(vmax),max));
        // ///////////////////////////////////////
       // this->addPolygon(poly, pen, grad);
        //qDebug() << abs((v(n1)+v(n2)+v(n3)/3))/v.max();
       // QBrush brush(Qt::red);
        if (!domains.empty())
        {
        if(domains(i,1)==2)
        {
        this->addPolygon(poly,Qt::NoPen,Qt::green);
        }
        else if(domains(i,1)==3)
        {
        this->addPolygon(poly,Qt::NoPen,Qt::blue);
        }
        else
            this->addPolygon(poly,Qt::NoPen,Qt::white);
        }



    }


}
QColor GraphicScene::interpolate(double value, double max){
    //if(value <0) value = 0;
    double ratio = value/max;
    QColor start = Qt::red;
    QColor end = Qt::blue;
    int r = (int)(ratio*start.red() + (1-ratio)*end.red());
    int g = (int)(ratio*start.green() + (1-ratio)*end.green());
    int b = (int)(ratio*start.blue() + (1-ratio)*end.blue());
   // qDebug()<<start.green()<<"  "<<end.green();
    return QColor::fromRgb(r,g,b);
}
QColor GraphicScene::interpolate1(double ratio, double max)
{
    int r , g ,b;
    if (ratio<0) ratio=0;
    double val = ratio/max;
    //qDebug() << ratio << max << ratio/max;
    if(val>=0 && val <0.2){
        // qDebug () << 1;
         QColor start = Qt::cyan;  QColor mid = Qt::blue;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.2 && val <0.4){
       // qDebug () << 2;
         QColor start = Qt::green;  QColor mid = Qt::cyan;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.4 && val <0.6){
      //  qDebug () << 3;
         QColor start = Qt::yellow;  QColor mid = Qt::green;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.6 && val <0.8){
     //   qDebug () << 4;
         QColor start = QColor( 0xFF, 0xA0, 0x00 );  QColor mid = Qt::yellow;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.8 && val <=1){
     //   qDebug () << 5;
         QColor start =  Qt::red;  QColor mid =QColor( 0xFF, 0xA0, 0x00 );
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    return QColor::fromRgb(r,g,b);
}

QColor GraphicScene::interpolate2(double val)
{
    if (val>=0 && val <0.1)
        return QColor::fromRgb(0, 0, 255);
    else if (val>=0.1 && val <0.2)
        return QColor::fromRgb(0, 206, 209);
    else if (val>=0.2 && val <0.3)
        return QColor::fromRgb(0, 255, 127);
    else if (val>=0.3 && val <0.4)
        return QColor::fromRgb(124, 252, 0);
    else if (val>=0.4 && val <0.5)
        return QColor::fromRgb(255, 255, 0);
    else if (val>=0.5 && val <0.6)
        return QColor::fromRgb(255, 215, 0);
    else if (val>=0.6 && val <0.7)
        return QColor::fromRgb(255, 165, 0);
    else if (val>=0.7 && val <0.8)
        return QColor::fromRgb(255, 69, 0);
    else if (val>=0.8 && val <0.9)
        return QColor::fromRgb(255, 20, 147);
    else if (val>=0.9 && val <=1)
        return QColor::fromRgb(255, 0, 255);
    else
        return QColor::fromRgb(0, 0, 0);
}
void GraphicScene::recalcPoints(QList <QPointF> *current_p){
    // каким-то образом переделать для конкретно выбранного объекта или всей сцены
    QList <QPointF> temp_points;
    float tempx, tempy;
    int radius = 2;
    for( int i=0; i<current_p->count(); ++i )
    {
        if(i==0){
            tempx = (current_p->at(current_p->count()-1).x()+10*current_p->at(i).x())/(1+10);
            tempy = (current_p->at(current_p->count()-1).y()+10*current_p->at(i).y())/(1+10);
            temp_points.append(QPointF(tempx,tempy));
            QGraphicsEllipseItem * ellipse1 = this->addEllipse(tempx - radius, tempy - radius, radius*2, radius*2);
            ellipse1->setBrush(Qt::white);
            temp_points.append(current_p->at(i));
            tempx = (current_p->at(i+1).x()+10*current_p->at(i).x())/(1+10);
            tempy = (current_p->at(i+1).y()+10*current_p->at(i).y())/(1+10);
            temp_points.append(QPointF(tempx,tempy));
            QGraphicsEllipseItem * ellipse2 = this->addEllipse(tempx - radius, tempy - radius, radius*2, radius*2);
            ellipse2->setBrush(Qt::white);
        }
        else if(i==current_p->count()-1)
        {
            tempx = (current_p->at(i-1).x()+10*current_p->at(i).x())/(1+10);
            tempy = (current_p->at(i-1).y()+10*current_p->at(i).y())/(1+10);
            temp_points.append(QPointF(tempx,tempy));
            QGraphicsEllipseItem * ellipse1 = this->addEllipse(tempx - radius, tempy - radius, radius*2, radius*2);
            ellipse1->setBrush(Qt::white);
            temp_points.append(current_p->at(i));
            tempx = (current_p->at(0).x()+10*current_p->at(i).x())/(1+10);
            tempy = (current_p->at(0).y()+10*current_p->at(i).y())/(1+10);
            temp_points.append(QPointF(tempx,tempy));
            QGraphicsEllipseItem * ellipse2 = this->addEllipse(tempx - radius, tempy - radius, radius*2, radius*2);
            ellipse2->setBrush(Qt::white);
        }
        else{
            tempx = (current_p->at(i-1).x()+10*current_p->at(i).x())/(1+10);
            tempy = (current_p->at(i-1).y()+10*current_p->at(i).y())/(1+10);
            temp_points.append(QPointF(tempx,tempy));
            QGraphicsEllipseItem * ellipse1 = this->addEllipse(tempx - radius, tempy - radius, radius*2, radius*2);
            ellipse1->setBrush(Qt::white);
            temp_points.append(current_p->at(i));
            tempx = (current_p->at(i+1).x()+10*current_p->at(i).x())/(1+10);
            tempy = (current_p->at(i+1).y()+10*current_p->at(i).y())/(1+10);
            temp_points.append(QPointF(tempx,tempy));
            QGraphicsEllipseItem * ellipse2 = this->addEllipse(tempx - radius, tempy - radius, radius*2, radius*2);
            ellipse2->setBrush(Qt::white);
        }
    }
    *current_p = temp_points;
}

void GraphicScene::fullRecalc()
{
    if(inside_points.size()>0){
        for(int j=0; j<inside_points.size(); j++)
        {
            recalcPoints(inside_points.at(j));
        }
    }
    if(outside_points.size()>0){
        for(int j=0; j<outside_points.size(); j++)
        {
            recalcPoints(outside_points.at(j));
        }
    }
}

void GraphicScene::EnterPointsZone()
{
    pointszone->show();
}

QPointF GraphicScene::calcMidGradPoint(QPointF Pmax, QPointF Pmin, double vmax,double vmid) // нахождение точки середины градиента
{
        double x1,y1,x2,y2,x0,y0,k,k1,L;
        x1 = Pmax.x(); y1 = Pmax.y();
        x2 = Pmin.x(); y2 = Pmin.y();
        k=vmid/vmax;
        k1=1-k;
        L=k1/k;
       // qDebug() << k1 << k;
        x0 = (x1 + L * x2)/(1 + L);
        y0 = (y1 + L * y2)/(1 + L);

        QPointF MidGradPoint(x0,y0);
        return MidGradPoint;
}

void GraphicScene::RefinementMesh(mat trs, mat v, mat nds, double K) //уточнение сетки
{
    bcs_vec.clear(); nodes_vec.clear(); trs_vec.clear();
    this->clear();
    QGraphicsTextItem *zero_p = this->addText("0");
    zero_p->setDefaultTextColor(Qt::red);
    zero_p->setPos(0, 0);
    QPen penZero(Qt::darkGray, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen penLine(Qt::lightGray, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    this->addLine(-3600, 0, 3600, 0, penZero);
    this->addLine(0, -1800, 0, 1800, penZero);
    for(int j=-100; j<100; j++){
        this->addLine(-3600, (j*10), 3600, (j*10), penLine);
        this->addLine((j*10), -1800, (j*10), 1800, penLine);
       // this->addLine(-160+(j*5), -2000, -160+(j*5), 1000);
    }
    for(int j=-3600; j<3600; j=j+50){
        QGraphicsTextItem *x_p = this->addText(QString::number(j));
        x_p->setDefaultTextColor(Qt::black);
        x_p->setPos(j, 0);

        QGraphicsTextItem *y_p = this->addText(QString::number(j*-1));
        y_p->setDefaultTextColor(Qt::black);
        y_p->setPos(0, j);
    }


    QPen pen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QVector<QList<QPointF>> temp_trs;
    double nr_trs = trs.n_rows;
    double nr_nds = nds.n_rows;
    for (int i = 0; i<nr_trs; i++)
    {
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        if (abs(v(n1)-v(n2))>K || abs(v(n2)-v(n3))>K || abs(v(n3)-v(n1))>K)
        {
           QPointF p1,p2,p3,p12,p23,p31;
            p1.setX(nds(n1,0));
            p1.setY(nds(n1,1));
            p2.setX(nds(n2,0));
            p2.setY(nds(n2,1));
            p3.setX(nds(n3,0));
            p3.setY(nds(n3,1));
            p12.setX((nds(n1,0)+nds(n2,0))/2);
            p12.setY((nds(n1,1)+nds(n2,1))/2);
            p23.setX((nds(n2,0)+nds(n3,0))/2);
            p23.setY((nds(n2,1)+nds(n3,1))/2);
            p31.setX((nds(n1,0)+nds(n3,0))/2);
            p31.setY((nds(n1,1)+nds(n3,1))/2);

            QList<QPointF> tempList;
            tempList.push_back(p1);
            tempList.push_back(p12);
            tempList.push_back(p31);
            temp_trs.push_back(tempList);
            tempList.clear();

            tempList.push_back(p12);
            tempList.push_back(p2);
            tempList.push_back(p23);
            temp_trs.push_back(tempList);
            tempList.clear();

            tempList.push_back(p23);
            tempList.push_back(p3);
            tempList.push_back(p31);
            temp_trs.push_back(tempList);
            tempList.clear();

            tempList.push_back(p31);
            tempList.push_back(p12);
            tempList.push_back(p23);
            temp_trs.push_back(tempList);
            tempList.clear();
        }
        else {
            QPointF p1,p2,p3;
            p1.setX(nds(n1,0));
            p1.setY(nds(n1,1));
            p2.setX(nds(n2,0));
            p2.setY(nds(n2,1));
            p3.setX(nds(n3,0));
            p3.setY(nds(n3,1));
            QList<QPointF> tempList;
            tempList.push_back(p1);
            tempList.push_back(p2);
            tempList.push_back(p3);
            temp_trs.push_back(tempList);
        }
     }
    int k=0;
    nodes_vec.clear();
    trs_vec.clear();
    bcs_vec.clear();
    nodesD.clear();
    trsD.clear();
    nodesD.set_size(temp_trs.size()*3, 2);
    trsD.set_size(temp_trs.size(),3);
    domains.clear();
    gr_cond_points.clear();



    for(int i=0;i<temp_trs.size();++i)
    {
        for(int j=0; j<3; j++){
            if(nodes_vec.indexOf(std::make_pair(temp_trs.at(i).at(j).x(),temp_trs.at(i).at(j).y()))==-1){
                nodes_vec.push_back(std::make_pair(temp_trs.at(i).at(j).x(), temp_trs.at(i).at(j).y()));
                trs_vec.push_back(k);

                for( int z=0; z<outside_points.size(); ++z )
                {
                    for( int q=0; q<outside_points.at(z)->count(); ++q )
                    {
                        double x1,x2,y1,y2;
                        if(q<outside_points.at(z)->count()-1){
                                x1 = outside_points.at(z)->at(q).x();
                                x2 = outside_points.at(z)->at(q+1).x();
                                y1 = outside_points.at(z)->at(q).y();
                                y2 = outside_points.at(z)->at(q+1).y();

                        } else {
                                x1 = outside_points.at(z)->at(q).x();
                                x2 = outside_points.at(z)->at(0).x();
                                y1 = outside_points.at(z)->at(q).y();
                                y2 = outside_points.at(z)->at(0).y();
                        }
                        double x = temp_trs.at(i).at(j).x();
                        double y = temp_trs.at(i).at(j).y();
                        double t = ((x-x1)*(x2-x1)+(y-y1)*(y2-y1))/
                                (pow((x2-x1),2)+pow((y2-y1),2));
                        if(t<0) t=0;
                        else if(t>1) t = 1;

                        double l = sqrt(pow(x1 - x + (x2-x1)*t,2) +
                                        pow(y1 - y + (y2-y1)*t,2));

                        if(l<1)
                        {
                            bcs_vec.push_back(std::make_pair(k,0));
                            break;
                        }

                    }
                }
                for( int z=0; z<inside_points.size(); ++z )
                {
                    for( int q=0; q<inside_points.at(z)->count(); ++q )
                    {
                        double x1,x2,y1,y2;
                        if(q<inside_points.at(z)->count()-1){
                                x1 = inside_points.at(z)->at(q).x();
                                x2 = inside_points.at(z)->at(q+1).x();
                                y1 = inside_points.at(z)->at(q).y();
                                y2 = inside_points.at(z)->at(q+1).y();
                        } else {
                                x1 = inside_points.at(z)->at(q).x();
                                x2 = inside_points.at(z)->at(0).x();
                                y1 = inside_points.at(z)->at(q).y();
                                y2 = inside_points.at(z)->at(0).y();

                        }
                        double x = temp_trs.at(i).at(j).x();
                        double y = temp_trs.at(i).at(j).y();

                        double t = ((x-x1)*(x2-x1)+(y-y1)*(y2-y1))/
                                (pow((x2-x1),2)+pow((y2-y1),2));
                        if(t<0) t=0;
                        else if(t>1) t = 1;

                        double l = sqrt(pow(x1 - x + (x2-x1)*t,2) +
                                        pow(y1 - y + (y2-y1)*t,2));
                        if(l<1){
                            bcs_vec.push_back(std::make_pair(k,1));
                            map[k]=z;
                            break;
                        }
                    }
                }
                k=k+1;
            } else {
                trs_vec.push_back(nodes_vec.indexOf(std::make_pair(temp_trs.at(i).at(j).x(), temp_trs.at(i).at(j).y())));
            }
            if(j==2){
                 this->addLine(temp_trs.at(i).at(j).x(), temp_trs.at(i).at(j).y(), temp_trs.at(i).at(0).x(), temp_trs.at(i).at(0).y(), pen);
            }
            else
                this->addLine(temp_trs.at(i).at(j).x(), temp_trs.at(i).at(j).y(), temp_trs.at(i).at(j+1).x(), temp_trs.at(i).at(j+1).y(), pen);
        }
        //trs << endr;
    }
    for(int k=0; k<trsD.size()/3;k++)
    {
        domains.push_back(std::make_pair(k,1));
        int rez=0;
        for (int i=0; i<dielectric_points.size();i++)
        {
            double Xmax,Xmin,Ymax,Ymin;
            Xmax=dielectric_points.at(i)->at(0).x();
            Xmin=dielectric_points.at(i)->at(0).x();
            Ymax=dielectric_points.at(i)->at(0).y();
            Ymin=dielectric_points.at(i)->at(0).y();
            //qDebug()<<dielectric_points.at(i)->size();
            for (int j=0; j<dielectric_points.at(i)->size();j++)
            {
                if (dielectric_points.at(i)->at(j).x()>Xmax)
                    Xmax=dielectric_points.at(i)->at(j).x();
                if (dielectric_points.at(i)->at(j).x()<Xmin)
                    Xmin=dielectric_points.at(i)->at(j).x();
                if (dielectric_points.at(i)->at(j).y()>Ymax)
                    Ymax=dielectric_points.at(i)->at(j).y();
                if (dielectric_points.at(i)->at(j).y()<Ymin)
                    Ymin=dielectric_points.at(i)->at(j).y();
                //qDebug()<<Xmax<<" "<<Xmin;
            }
            if (nodes_vec.at(trs_vec.at(k*3)).first>=Xmin && nodes_vec.at(trs_vec.at(k*3)).first<=Xmax)
            {
                if (nodes_vec.at(trs_vec.at(k*3)).second>=Ymin && nodes_vec.at(trs_vec.at(k*3)).second<=Ymax)
                    rez++;
            }
            if (nodes_vec.at(trs_vec.at(k*3+1)).first>=Xmin && nodes_vec.at(trs_vec.at(k*3+1)).first<=Xmax)
            {
                if (nodes_vec.at(trs_vec.at(k*3+1)).second>=Ymin && nodes_vec.at(trs_vec.at(k*3+1)).second<=Ymax)
                    rez++;
            }
            if (nodes_vec.at(trs_vec.at(k*3+2)).first>=Xmin && nodes_vec.at(trs_vec.at(k*3+2)).first<=Xmax)
            {
                if (nodes_vec.at(trs_vec.at(k*3+2)).second>=Ymin && nodes_vec.at(trs_vec.at(k*3+2)).second<=Ymax)
                    rez++;
            }
            if (rez==3)
            {
                domains.at(k).second=2;
            }
        }
        rez=0;
    }
    // //////////////////////////////////////////////////////////////////////////////////



}

void GraphicScene::potential_line_plot(QList<QPolygonF> poly_list, QVector<double> Vecv) // построение эквипотенциальных линий(реалзовано неправильно,переделываю)
{
    QBrush brushmesh (Qt::white);
    if (!visual->get_mesh())
    {

        this->clear();
    }


    QPolygonF out;
    QList<QPolygonF> inside;
    QList<QPolygonF> gr_c_p;
    out.append(outside_points.at(0)->toVector());
    for (int i=0; i<inside_points.size(); i++)
    {
        inside.push_back(inside_points.at(i)->toVector());
    }

    QPen line_pen;
    if (visual->get_line())
    {
     line_pen.setColor(Qt::darkGray);
     line_pen.setWidth(0);
     line_pen.setStyle(Qt::SolidLine);
     line_pen.setCapStyle(Qt::RoundCap);
     line_pen.setJoinStyle(Qt::RoundJoin);
    }
    else
        line_pen.setStyle(Qt::NoPen);

    QPen bord_pen (Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen di_pen (Qt::black, 0, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
    if (visual->get_color())
    this->addPolygon(out,bord_pen,interpolate2(0));
    else
        this->addPolygon(out,bord_pen,brushmesh);
    if (visual->get_color())
    {
    for (int i=0; i<poly_list.size();i++)
        this->addPolygon(poly_list.at(i),line_pen,interpolate2(Vecv.at(i)));
    }
    else
        for (int i=0; i<poly_list.size();i++)
            this->addPolygon(poly_list.at(i),line_pen,brushmesh);


    for (int i=0; i<inside.size();i++)
        this->addPolygon(inside.at(i),bord_pen,brushmesh);
    for (int i=0; i<gr_cond_points.size();i++)
        this->addPolygon(gr_cond_points.at(i)->toVector(),bord_pen,brushmesh);
    for (int i=0; i<dielectric_points.size();i++)
        this->addPolygon(dielectric_points.at(i)->toVector(),di_pen,Qt::NoBrush);


}

void GraphicScene::potential_line_calc()
{
    mat v;
    mat trs;
    mat nds;
    double start;
    double end;
    double step;
    visual->get_settings(v,trs,nds,start,end,step);
    QMap<std::pair<float,float>,std::pair<int,int>> p_trs;
    QVector<QPointF> temppoints;
    QPolygonF poly;
    QList<QPolygonF> poly_list;
    QVector<double> Ve_all;
    QPen pen1(Qt::red);
    QPen pen2(Qt::green);
    //int N = trs.n_rows;
    //double Ve=0.9;
    for (double Ve = start; Ve < end; Ve+=step)
    {
        temppoints.clear();
    for (int i=0; i < trs.n_rows; i++)
    {
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        double V1=v(n1); double V2=v(n2); double V3=v(n3);
        double x1,x2,x3,y1,y2,y3;
        x1 = nds(n1,0); y1 = nds(n1, 1);
        x2 = nds(n2,0); y2 = nds(n2, 1);
        x3 = nds(n3,0); y3 = nds(n3, 1);
        int sw = 0;
        if (V1>Ve)
            sw += 100;
        if (V2>Ve)
            sw += 10;
        if (V3>Ve)
            sw += 1;
        switch (sw) {
        case 0:
        case 111:
            break;
        case 1:
        case 110:
            double x13,y13,x23,y23;
            double l13,l23;
            l13 = (V1-Ve)/(Ve-V3);
            l23 = (V2-Ve)/(Ve-V3);
            x13 = (x1 + l13 * x3) / (1 + l13);
            y13 = (y1 + l13 * y3) / (1 + l13);
            x23 = (x2 + l23 * x3) / (1 + l23);
            y23 = (y2 + l23 * y3) / (1 + l23);
            temppoints.append(QPointF(x13,y13));
            temppoints.append(QPointF(x23,y23));

            if (p_trs[std::make_pair(x23,y23)].first == 0)
                p_trs[std::make_pair(x23,y23)].first = i+1;
            else
                p_trs[std::make_pair(x23,y23)].second = i+1;

            if (p_trs[std::make_pair(x13,y13)].first == 0)
                p_trs[std::make_pair(x13,y13)].first = i+1;
            else
                p_trs[std::make_pair(x13,y13)].second = i+1;

            this->addLine(x13,y13,x23,y23);
            break;
        case 10:
        case 101:
            double x12,y12,x32,y32;
            double l12,l32;
            l12 = (V1-Ve)/(Ve-V2);
            l32 = (V3-Ve)/(Ve-V2);
            x12 = (x1 + l12 * x2) / (1 + l12);
            y12 = (y1 + l12 * y2) / (1 + l12);
            x32 = (x3 + l32 * x2) / (1 + l32);
            y32 = (y3 + l32 * y2) / (1 + l32);
            temppoints.append(QPointF(x12,y12));
            temppoints.append(QPointF(x32,y32));

            if (p_trs[std::make_pair(x12,y12)].first == 0)
                p_trs[std::make_pair(x12,y12)].first = i+1;
            else
                p_trs[std::make_pair(x12,y12)].second = i+1;

            if (p_trs[std::make_pair(x32,y32)].first == 0)
                p_trs[std::make_pair(x32,y32)].first = i+1;
            else
                p_trs[std::make_pair(x32,y32)].second = i+1;

            this->addLine(x12,y12,x32,y32);
            break;
        case 100:
        case 11:
            double x21,y21,x31,y31;
            double l21,l31;
            l21 = (V2-Ve)/(Ve-V1);
            l31 = (V3-Ve)/(Ve-V1);
            x21 = (x2 + l21 * x1) / (1 + l21);
            y21 = (y2 + l21 * y1) / (1 + l21);
            x31 = (x3 + l31 * x1) / (1 + l31);
            y31 = (y3 + l31 * y1) / (1 + l31);
            temppoints.append(QPointF(x21,y21));
            temppoints.append(QPointF(x31,y31));

            if (p_trs[std::make_pair(x21,y21)].first == 0)
                p_trs[std::make_pair(x21,y21)].first = i+1;
            else
                p_trs[std::make_pair(x21,y21)].second = i+1;

            if (p_trs[std::make_pair(x31,y31)].first == 0)
                p_trs[std::make_pair(x31,y31)].first = i+1;
            else
                p_trs[std::make_pair(x31,y31)].second = i+1;

            this->addLine(x21,y21,x31,y31);
            break;
        }


    }

    Sort_Vpoints_new(temppoints,poly,p_trs);
    Ve_all.push_back(Ve);
    poly_list.push_back(poly);
    if (temppoints.size()>1)
    {
        poly.clear();
        Sort_Vpoints_new(temppoints,poly,p_trs);
        Ve_all.push_back(Ve);
        poly_list.push_back(poly);
    }
    poly.clear();

       /*   QPen pen3 (Qt::red,0,Qt::DashLine);
        * for (int i=0; i<temppoints.size(); i++)
        {
            QPen pen (Qt::black);
            this->addEllipse(temppoints.at(i).x() - 1, temppoints.at(i).y() - 1, 1*2, 1*2,pen);
            //qDebug()<<p_trs[i];
            //qDebug()<<p_trs[std::make_pair(temppoints.at(i).x(),temppoints.at(i).y())];
        }

    this->addPolygon(poly,pen3,interpolate2(Ve));
        QPen pen (Qt::blue);
        for (int i=0; i<poly.size(); i++)
        {
        QGraphicsTextItem *x_p = this->addText(QString::number(i));
        x_p->setDefaultTextColor(Qt::black);
        x_p->setPos(poly.at(i));
        QFont f ("times");
        f.setPixelSize(4);
        x_p->setFont(f);
        this->addEllipse(poly.at(i).x() - 1, poly.at(i).y() - 1, 1*2, 1*2,pen);
        }*/
    /*for (int i=0; i<poly_list.size();i++)
        this->addPolygon(poly_list.at(i),pen1,Qt::NoBrush);*/

  }

    potential_line_plot(poly_list, Ve_all);
}


void GraphicScene::Sort_Vpoints(QVector<QPointF> p, QPolygonF &poly)
{

    std::sort(p.begin(),p.end(),sortonx_up);
    auto last = std::unique (p.begin(),p.end());
    p.erase(last,p.end());

     QPen pen (Qt::blue);
     for (int i=0; i<p.size(); i++)
     {
     QGraphicsTextItem *x_p = this->addText(QString::number(i));
     x_p->setDefaultTextColor(Qt::black);
     QFont f ("times");
     f.setPixelSize(4);
     x_p->setFont(f);
     x_p->setPos(p.at(i));
     //this->addEllipse(p.at(i).x() - 1, p.at(i).y() - 1, 1*2, 1*2,pen);
     }

    QPointF Xmin;
    Xmin = p.at(0);
    int h=0;
     for (int i=0; i<p.size(); i++)
     {
         if (p.at(i).x() < Xmin.x())
         {
             Xmin = p.at(i);
             h = i;
         }
         else if (p.at(i).x() == Xmin.x())
             if (p.at(i).y() < Xmin.y())
             {
                 Xmin = p.at(i);
                 h = i;
             }
     }

     poly.append(Xmin);
     p.remove(h);
     p.append(Xmin);

     while (true)
     {
         int right = 0;
         for (int i=0; i<p.size(); i++)
         {
             if (sort_right_rotate(poly.at(poly.size()-1),p.at(right),p.at(i)))
                 right = i;
         }
         if (p.at(right) == poly.at(0))
             break;
         else
         {
             poly.append(p.at(right));
             p.remove(right);
         }
     }

}

void GraphicScene::Sort_Vpoints_new(QVector<QPointF> &p, QPolygonF &poly, QMap<std::pair<float,float>,std::pair<int,int>> map)
{
    //qDebug()<<p.size();
    std::sort(p.begin(),p.end(),sortonx_up);
    //qDebug()<<p.size();
    auto last = std::unique (p.begin(),p.end());
    p.erase(last,p.end());
    //qDebug()<<p.size();
    poly.append(p.at(0));
     //p.remove(0);

     while (true)
     {
         int true_trs = 0;
         for (int i=0; i<p.size(); i++)
         {
             if (sort_on_map(poly.at(poly.size()-1),p.at(i),map))
                 true_trs = i;
         }
         if (p.at(true_trs) == poly.at(0))
         {
             p.remove(true_trs);
             break;
         }
         else
         {
             poly.append(p.at(true_trs));
             p.remove(true_trs);
         }
     }


}
bool GraphicScene::sortonx_up(const QPointF &p1, const QPointF &p2)
{
    double tol = 1e-6;
    //int x1 = round(p1.x()); int x2 = round(p2.x());
    //int y1 = round(p1.y()); int y2 = round(p2.y());
    //if (x1 == x2)
      //  return (y1 < y2);
   // return (x1 < x2);
    if (abs(p1.x() - p2.x())<tol)
        return ((p2.y()-p1.y())>tol);
    return ((p2.x()-p1.x()) > tol);
}

bool GraphicScene::sort_right_rotate(const QPointF &a, const QPointF &b, const QPointF &c)
{
    QPointF p1,p2;
    p1 = b - a;
    p2 = c - a;
    double det;
    det = p1.x()*p2.y()-p2.x()*p1.y();
    if (det < 0)
        return true;
    return false;
}

bool GraphicScene::sort_on_map(const QPointF &p1, const QPointF &p2, QMap<std::pair<float, float>, std::pair<int, int>> map)
{
    int trs1, trs2, i_trs1, i_trs2;
    trs1 = map[std::make_pair(p1.x(),p1.y())].first;
    trs2 = map[std::make_pair(p1.x(),p1.y())].second;
    i_trs1 = map[std::make_pair(p2.x(),p2.y())].first;
    i_trs2 = map[std::make_pair(p2.x(),p2.y())].second;
    if (trs1 == i_trs1 || trs1 == i_trs2 ||trs2 == i_trs1 || trs2 == i_trs2)
        return true;
    return false;
}





float GraphicScene::distance(const QPointF &pt1, const QPointF &pt2)
{
    float hd = (pt1.x() - pt2.x()) * (pt1.x() - pt2.x());
            float vd = (pt1.y() - pt2.y()) * (pt1.y() - pt2.y());
            return std::sqrt(hd + vd);
}

QPointF GraphicScene::getLineStart(const QPointF &pt1, const QPointF &pt2)
{
            QPointF pt;
            float rat = 10.0 / distance(pt1, pt2);
            if (rat > 0.5) {
                rat = 0.5;
            }
            pt.setX((1.0 - rat) * pt1.x() + rat * pt2.x());
            pt.setY((1.0 - rat) * pt1.y() + rat * pt2.y());
            return pt;
}

QPointF GraphicScene::getLineEnd(const QPointF &pt1, const QPointF &pt2)
{
    QPointF pt;
            float rat = 10.0 / distance(pt1, pt2);
            if (rat > 0.5) {
                rat = 0.5;
            }
            pt.setX(rat * pt1.x() + (1.0 - rat)*pt2.x());
            pt.setY(rat * pt1.y() + (1.0 - rat)*pt2.y());
            return pt;
}


void GraphicScene::RectMesh(mat trs,mat nds, mat v) // новая сетка для построения силовых линий и экв потенциальных линий(переделываю)
{
    QList <QPointF> RectMesh_points;
    mat RectMes_V;
    QPen pen(Qt::black);
    QPen pen1(Qt::white);
    double dx,dy;
    int N=100;
    dx=(abs(outside_points.at(0)->at(0).x()-outside_points.at(0)->at(1).x()))/N;
    dy=(abs(outside_points.at(0)->at(2).y()-outside_points.at(0)->at(0).y()))/N;
   // qDebug()<<dx<<"  "<<dy;
    for (double yy=outside_points.at(0)->at(0).y();yy<=outside_points.at(0)->at(2).y();yy+=dy)
    {
    for (double xx=outside_points.at(0)->at(0).x(); xx<=outside_points.at(0)->at(1).x();xx+=dx)
    {
        RectMesh_points.push_back(QPointF(xx,yy));
    }
    }

    RectMes_V.set_size(RectMesh_points.size(),1);
    RectMes_V.zeros();



    for (int j=0;j<RectMesh_points.size();j++){  //вычисление потенциалов
    for (int i=0;i<trs.n_rows;i++)
    {
    double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
    double Ax,Ay,Bx,By,Cx,Cy,Px,Py;
    Ax=nds(n1,0); Ay=nds(n1,1);
    Bx=nds(n2,0); By=nds(n2,1);
    Cx=nds(n3,0); Cy=nds(n3,1);
    Px=RectMesh_points.at(j).x();
    Py=RectMesh_points.at(j).y();

    for (int k=0;k<inside_points.size();k++) //внутренние точки
    {
        double x1,x2,y1,y2;
        x1=inside_points.at(k)->at(0).x();
        x2=inside_points.at(k)->at(1).x();
        y1=inside_points.at(k)->at(0).y();
        y2=inside_points.at(k)->at(2).y();
        if (Px>x1 && Px<x2)
            if(Py>y1 && Py<y2)
            {
                RectMes_V(j,0)=1.0;
            }
    }
   if (RectMes_V(j,0)!=1.0)
    if (IsPointIn_Geron(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) // проверка принадлежности точки треугольнику формулами Герона
    {
        double v1,v2,v3;
        v1=v(n1,0); v2=v(n2,0); v3=v(n3,0);
        RectMes_V(j,0)=potentialIn(Ax,Ay,Bx,By,Cx,Cy,Px,Py,v1,v2,v3);

        double vv;
        vv=RectMes_V(j,0);
        qDebug()<<vv;
      /*  QPolygonF poly;
        poly<<QPointF(Ax,Ay)<<QPointF(Bx,By)<<QPointF(Cx,Cy)<<QPointF(Ax,Ay);
        this->addPolygon(poly,pen,interpolate1(RectMes_V(j,0),1));*/
        break;
    }
    }
    }

    mat Ex(RectMesh_points.size(),1,fill::zeros);
    mat Ey(RectMesh_points.size(),1,fill::zeros);
    for (int i=0;i<RectMesh_points.size();i++) // Ex Ey
    {
        double x,y;
        x=RectMesh_points.at(i).x();
        y=RectMesh_points.at(i).y();
  /*  int radius = 1;
       this->addEllipse(x - radius, y - radius, radius*2, radius*2,pen1);*/
    if (RectMes_V(i,0)!=0 && RectMes_V(i,0)!=1)
    {
        for (int NN=-N-1;NN<=N+1;NN+=N+1)
        {
            int j=i+NN;
            for (int k=-1;k<=1;k++)
            {
                int jj=j+k;
               // qDebug()<<i<<jj;
                if (RectMesh_points.at(jj).x()-x!=0.0)
                Ex(i,0)=Ex(i,0)-(RectMes_V(jj,0)-RectMes_V(i,0))/(RectMesh_points.at(jj).x()-x);
                if (RectMesh_points.at(jj).y()-y!=0.0)
                Ey(i,0)=Ey(i,0)-(RectMes_V(jj,0)-RectMes_V(i,0))/(RectMesh_points.at(jj).y()-y);
            }
        }
    }
    }
    Ex=Ex/Ex.max();
    Ey=Ey/Ey.max();
    Ex=Ex*20;
    Ey=Ey*20;
    for (int i=0;i<RectMesh_points.size();i++) //линии
    {
        double x,y;
        x=RectMesh_points.at(i).x();
        y=RectMesh_points.at(i).y();
         if (RectMes_V(i,0)!=0 && RectMes_V(i,0)!=1)
         {
             double xn,yn;
             xn=Ex(i,0)+x;
             yn=Ey(i,0)+y;
             this->addLine(x,y,x+Ex(i,0),y+Ey(i,0));
             double ostr = 0.25;        // острота стрелки
             double x0,y0,lons,ugol;
             double f1x2 , f1y2;
             x0 = xn - x;
             y0 = yn - y;
             lons = sqrt(x0*x0 + y0*y0) / 7;     // длина лепестков % от длины стрелки
             ugol = atan2(y0, x0);             // угол наклона линии
             f1x2 = xn - lons * cos(ugol - ostr);
             f1y2 = yn - lons * sin(ugol - ostr);
             this->addLine(xn, yn, f1x2, f1y2,pen);
             f1x2 = xn - lons * cos(ugol + ostr);
             f1y2 = yn - lons * sin(ugol + ostr);
             this->addLine(xn, yn, f1x2, f1y2,pen);
         }
    }

}

void GraphicScene::RectMesh2(mat trs, mat nds, mat v)
{

    QList <QPointF> RectMesh_points;
    mat RectMes_V;
    QPen pen(Qt::black);
    QPen pen1(Qt::white);
    double dx,dy;
    int N=101;
    dx=(abs(outside_points.at(0)->at(0).x()-outside_points.at(0)->at(1).x()))/(N-1);
    dy=(abs(outside_points.at(0)->at(2).y()-outside_points.at(0)->at(0).y()))/(N-1);
   // qDebug()<<dx<<"  "<<dy;
    for (double yy=outside_points.at(0)->at(0).y();yy<=outside_points.at(0)->at(2).y();yy+=dy)
    {
    for (double xx=outside_points.at(0)->at(0).x(); xx<=outside_points.at(0)->at(1).x();xx+=dx)
    {
        RectMesh_points.push_back(QPointF(xx,yy));

    }
    }

    RectMes_V.set_size(RectMesh_points.size(),2);
    RectMes_V.zeros();



    for (int j=0;j<RectMesh_points.size();j++){  //вычисление потенциалов

    for (int i=0;i<trs.n_rows;i++)
    {

    double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
    double Ax,Ay,Bx,By,Cx,Cy,Px,Py;
    Ax=nds(n1,0); Ay=nds(n1,1);
    Bx=nds(n2,0); By=nds(n2,1);
    Cx=nds(n3,0); Cy=nds(n3,1);
    Px=RectMesh_points.at(j).x();
    Py=RectMesh_points.at(j).y();

    for (int k=0;k<inside_points.size();k++) //внутренние точки
    {
        double x1,x2,y1,y2;
        x1=inside_points.at(k)->at(0).x();
        x2=inside_points.at(k)->at(1).x();
        y1=inside_points.at(k)->at(0).y();
        y2=inside_points.at(k)->at(2).y();
        if (Px>x1 && Px<x2)
            if(Py>y1 && Py<y2)
            {
                RectMes_V(j,0)=1.0;
                RectMes_V(j,1)=1.0;
            }
    }
   if (RectMes_V(j,0)!=1.0)
    if (IsPointIn_Geron(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) // проверка принадлежности точки треугольнику формулами Герона
    {
        double v1,v2,v3;
        v1=v(n1,0); v2=v(n2,0); v3=v(n3,0);
        RectMes_V(j,0)=potentialIn(Ax,Ay,Bx,By,Cx,Cy,Px,Py,v1,v2,v3);
        double vv;
        vv=RectMes_V(j,0);
       // qDebug()<<vv;
       // QPolygonF poly;
      //  poly<<QPointF(Ax,Ay)<<QPointF(Bx,By)<<QPointF(Cx,Cy)<<QPointF(Ax,Ay);
       // this->addPolygon(poly,pen,interpolate1(RectMes_V(j,0),1));

        break;
    }
    }
    }


   mat points_x(N,N,fill::zeros);
   mat points_y(N,N,fill::zeros);
   mat v_in_points(N,N,fill::zeros);

    qDebug()<<RectMesh_points.size();
    for (int i=0;i<N;i++)
        for (int j=0;j<N;j++)
        {
            points_x(i,j)=RectMesh_points.at(i*N+j).x();
            points_y(i,j)=RectMesh_points.at(i*N+j).y();
            v_in_points(i,j)=RectMes_V(i*N+j,0);
        }


   // double Ex,Ey;
    mat Ex(N,N,fill::zeros);
    mat Ey(N,N,fill::zeros);
    for (int i=1;i<N-1;i++)
        for (int j=1;j<N-1;j++)
        {
            double xx;
            xx=points_x(i,j);
            //qDebug()<<xx;
            for (int k=-1;k<=1;k++)
            {
                for (int q=-1;q<=1;q++)
                {
                   // qDebug()<<i+k<<j+q;
                    if(points_x(i+k,j+q)-points_x(i,j)!=0.0)
                    Ex(i,j)=Ex(i,j)+(v_in_points(i+k,j+q)-v_in_points(i,j))/(points_x(i+k,j+q)-points_x(i,j));
                    if(points_y(i+k,j+q)-points_y(i,j)!=0.0)
                    Ey(i,j)=Ey(i,j)+(v_in_points(i+k,j+q)-v_in_points(i,j))/(points_y(i+k,j+q)-points_y(i,j));
                }
            }
          //  qDebug()<<Ex<<"   "<<Ey;

        }

    Ex=Ex/Ex.max()*10;
    Ey=Ey/Ey.max()*10;

 /*   for (int i=1;i<N-1;i++) //силовые линии
        for (int j=1;j<N-1;j++)
        {
            if(RectMes_V(i*N+j,1)!=1.0)
            {

      /*  this->addLine(points_x(i,j),points_y(i,j),points_x(i,j)-Ex(i,j),points_y(i,j)-Ey(i,j),pen);
            int radius = 1;
                   this->addEllipse(points_x(i,j) - radius, points_y(i,j) - radius, radius*2, radius*2,pen1);*/

          /*      double x,y;
                x=points_x(i,j);
                y=points_y(i,j);
                     double xn,yn;
                     xn=x-Ex(i,j);
                     yn=y-Ey(i,j);
                     this->addLine(x,y,xn,yn);
                     double ostr = 0.25;        // острота стрелки
                     double x0,y0,lons,ugol;
                     double f1x2 , f1y2;
                     x0 = xn - x;
                     y0 = yn - y;
                     lons = sqrt(x0*x0 + y0*y0) / 7;     // длина лепестков % от длины стрелки
                     ugol = atan2(y0, x0);             // угол наклона линии
                     f1x2 = xn - lons * cos(ugol - ostr);
                     f1y2 = yn - lons * sin(ugol - ostr);
                     this->addLine(xn, yn, f1x2, f1y2,pen);
                     f1x2 = xn - lons * cos(ugol + ostr);
                     f1y2 = yn - lons * sin(ugol + ostr);
                     this->addLine(xn, yn, f1x2, f1y2,pen);
            }
        }*/


    QList<QPointF> temppoints;
    for (int i = 0; i<outside_points.at(0)->size(); i++)
        temppoints.append(outside_points.at(0)->at(i));
    //potential_line_plot(temppoints , 0.0);
    temppoints.clear();
    for (double Vecv=0.05;Vecv<1.0;Vecv+=0.05)
    {
    for (int i=1; i<N; i++) {
        for (int j=0; j<N; j++)
        {
            if (Vecv>=v_in_points(i,j) && Vecv<=v_in_points(i-1,j))
            {
                int radius = 1;
             //   this->addEllipse(points_x(i,j) - radius, points_y(i,j) - radius, radius*2, radius*2,pen);
            //    this->addEllipse(points_x(i-1,j) - radius, points_y(i-1,j) - radius, radius*2, radius*2,pen);

                double y1,y2,y0,k,k1,L,vmin,vmax;
                y1 = points_y(i-1,j);
                y2 = points_y(i,j);
                vmin=v_in_points(i,j);
                vmax = v_in_points(i-1,j);
                k=vmin/vmax;
                k1=1-k;
                L=k1/k;
                if (k==0.0)
                    L=0;
               // qDebug() << k1 << k;
                y0 = (y1 + L * y2)/(1 + L);
                temppoints.push_back(QPointF(points_x(i-1,j),y0));
                //this->addEllipse(points_x(i-1,j) - radius, y0 - radius, radius*2, radius*2,pen1);


            }
            else if (Vecv<=v_in_points(i,j) && Vecv>=v_in_points(i-1,j))
            {
                int radius = 1;
              //  this->addEllipse(points_x(i,j) - radius, points_y(i,j) - radius, radius*2, radius*2,pen);
              //  this->addEllipse(points_x(i-1,j) - radius, points_y(i-1,j) - radius, radius*2, radius*2,pen);
                double y1,y2,y0,k,k1,L,vmin,vmax;
                y1 = points_y(i,j);
                y2 = points_y(i-1,j);
                vmin=v_in_points(i-1,j);
                vmax = v_in_points(i,j);
                k=vmin/vmax;
                k1=1-k;
                L=k1/k;
                if (k==0.0)
                    L=0;
               // qDebug() << k1 << k;
                y0 = (y1 + L * y2)/(1 + L);
                temppoints.push_back(QPointF(points_x(i-1,j),y0));
                //this->addEllipse(points_x(i-1,j) - radius, y0 - radius, radius*2, radius*2,pen1);
            }
        }
    }
    for (int i=1; i<N; i++) {
        for (int j=0; j<N; j++)
        {
            if (Vecv>=v_in_points(j,i) && Vecv<=v_in_points(j,i-1))
            {
                int radius = 1;
              //  this->addEllipse(points_x(j,i) - radius, points_y(j,i) - radius, radius*2, radius*2,pen);
              //  this->addEllipse(points_x(j,i-1) - radius, points_y(j,i-1) - radius, radius*2, radius*2,pen);

                double x1,x2,x0,k,k1,L,vmin,vmax;
                x1 = points_x(j,i-1);
                x2 = points_x(j,i);
                vmin=v_in_points(j,i);
                vmax = v_in_points(j,i-1);
                k=vmin/vmax;
                k1=1-k;
                L=k1/k;
                if (k==0.0)
                    L=0;
               // qDebug() << k1 << k;
                x0 = (x1 + L * x2)/(1 + L);
                temppoints.push_back(QPointF(x0,points_y(j,i-1)));
               // this->addEllipse(x0 - radius, points_y(j,i-1) - radius, radius*2, radius*2,pen1);

            }
            else if (Vecv<=v_in_points(j,i) && Vecv>=v_in_points(j,i-1))
            {
                int radius = 1;
              //  this->addEllipse(points_x(j,i) - radius, points_y(j,i) - radius, radius*2, radius*2,pen);
              //  this->addEllipse(points_x(j,i-1) - radius, points_y(j,i-1) - radius, radius*2, radius*2,pen);
                double x1,x2,x0,k,k1,L,vmin,vmax;
                x1 = points_x(j,i);
                x2 = points_x(j,i-1);
                vmin=v_in_points(j,i-1);
                vmax = v_in_points(j,i);
                k=vmin/vmax;
                k1=1-k;
                L=k1/k;
                if (k==0.0)
                    L=0;
               // qDebug() << k1 << k;
                x0 = (x1 + L * x2)/(1 + L);
                temppoints.push_back(QPointF(x0,points_y(j,i-1)));
               // this->addEllipse(x0 - radius, points_y(j,i-1) - radius, radius*2, radius*2,pen1);
            }
        }
    }
    //potential_line_plot(temppoints,Vecv);
    temppoints.clear();
}


}
// принадлежность точки треугольнику,векторные метод
bool GraphicScene::IsPIn_Vector(double aAx, double aAy, double aBx, double aBy, double aCx, double aCy, double aPx, double aPy)
{
    double Bx,By,Cx,Cy,Px,Py;
    double m,l;
    Bx=aBx-aAx; By=aBy-aAy;
    Cx=aCx-aAx; Cy=aCy-aAy;
    Px=aPx-aAx; Py=aPy-aAy;
    m = (Px*By - Bx*Py) / (Cx*By - Bx*Cy);
    if (m>=0 && m<=1)
    {
        l=(Px - m*Cx) / Bx;
        if (l>=0 && (m+l)<=1){
            return true;
        }
        else return false;
    }
    else return false;
}
// принадлежность точки треугольнику, сумма площадей
bool GraphicScene::IsPointIn_Geron(double aAx, double aAy, double aBx, double aBy, double aCx, double aCy, double aPx, double aPy)
{

    double Area,Area1,Area2,Area3;
    Area=abs(aBx*aCy - aCx*aBy - aAx*aCy + aCx*aAy + aAx*aBy - aBx*aAy);
    Area1=abs(aBx*aCy - aCx*aBy - aPx*aCy + aCx*aPy + aPx*aBy - aBx*aPy);
    Area2=abs(aPx*aCy - aCx*aPy - aAx*aCy + aCx*aAy + aAx*aPy - aPx*aAy);
    Area3=abs(aBx*aPy - aPx*aBy - aAx*aPy + aPx*aAy + aAx*aBy - aBx*aAy);
    if ((Area1+Area2+Area3-Area)<=0.01)
        return true;
    else return false;

}
//потенциал в произвольной точке треугольника
double GraphicScene::potentialIn(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y, double v1, double v2, double v3)
{
    double a1,a2,a3,S2;
    S2=((x2*y3-x3*y2)+(x3*y1-x1*y3)+(x1*y2-x2*y1));
    a1=((x2*y3-x3*y2)+(y2-y3)*x+(x3-x2)*y)/S2;
    a2=((x3*y1-x1*y3)+(y3-y1)*x+(x1-x3)*y)/S2;
    a3=((x1*y2-x2*y1)+(y1-y2)*x+(x2-x1)*y)/S2;
    double v;
    v=a1*v1+a2*v2+a3*v3;
    if (abs(v)<1e-10)
        v=0;
    if (v>1)
        v=1;
    return v;

}


void GraphicScene::doTriangles()
{
    // реализовать проход по всем объектам и каким-то образом сделать выбор режима областей (булевы функции) (?)
    this->clear();
// REDO
    QGraphicsTextItem *zero_p = this->addText("0");
    zero_p->setDefaultTextColor(Qt::red);
    zero_p->setPos(0, 0);
    QPen penZero(Qt::darkGray, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen penLine(Qt::lightGray, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    this->addLine(-3600, 0, 3600, 0, penZero);
    this->addLine(0, -1800, 0, 1800, penZero);
    for(int j=-100; j<100; j++){
        this->addLine(-3600, (j*10), 3600, (j*10), penLine);
        this->addLine((j*10), -1800, (j*10), 1800, penLine);
       // this->addLine(-160+(j*5), -2000, -160+(j*5), 1000);
    }
    for(int j=-3600; j<3600; j=j+50){
        QGraphicsTextItem *x_p = this->addText(QString::number(j));
        x_p->setDefaultTextColor(Qt::black);
        x_p->setPos(j, 0);

        QGraphicsTextItem *y_p = this->addText(QString::number(j*-1));
        y_p->setDefaultTextColor(Qt::black);
        y_p->setPos(0, j);
    }
    Fade_2D dt;
    //qDebug() << dt.setNumCPU(1);


    QPen pen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen pen1(Qt::red, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
   // qDebug () << inside_points.size() << outside_points.size() ;
   // qDebug () << inside_points.at(0)->count() << outside_points.at(0)->count() ;
    if(inside_points.size()>0){ // redo
        for(int j =0; j<inside_points.size(); j++){
            for( int i=0; i<inside_points.at(j)->count(); ++i )
            {
                    if(i==inside_points.at(j)->count()-1){
                        this->addLine(inside_points.at(j)->at(i).x(),inside_points.at(j)->at(i).y(),
                                      inside_points.at(j)->at(0).x(),inside_points.at(j)->at(0).y(), pen);
                    }
                    else
                    this->addLine(inside_points.at(j)->at(i).x(),inside_points.at(j)->at(i).y(),
                                  inside_points.at(j)->at(i+1).x(),inside_points.at(j)->at(i+1).y(), pen);
            }
        }
    }
    if(outside_points.size()>0){ // redo
        for(int j =0; j<outside_points.size(); j++){
            for( int i=0; i<outside_points.at(j)->count(); ++i )
            {
                    if(i==outside_points.at(j)->count()-1){
                        this->addLine(outside_points.at(j)->at(i).x(),outside_points.at(j)->at(i).y(),
                                      outside_points.at(j)->at(0).x(),outside_points.at(j)->at(0).y(), pen);
                    }
                    else
                    this->addLine(outside_points.at(j)->at(i).x(),outside_points.at(j)->at(i).y(),
                                  outside_points.at(j)->at(i+1).x(),outside_points.at(j)->at(i+1).y(), pen);
            }
        }
    }
    if(gr_cond_points.size()>0){ // redo
        for(int j =0; j<gr_cond_points.size(); j++){
            for( int i=0; i<gr_cond_points.at(j)->count(); ++i )
            {
                    if(i==gr_cond_points.at(j)->count()-1){
                        this->addLine(gr_cond_points.at(j)->at(i).x(),gr_cond_points.at(j)->at(i).y(),
                                      gr_cond_points.at(j)->at(0).x(),gr_cond_points.at(j)->at(0).y(), pen);
                    }
                    else
                    this->addLine(gr_cond_points.at(j)->at(i).x(),gr_cond_points.at(j)->at(i).y(),
                                  gr_cond_points.at(j)->at(i+1).x(),gr_cond_points.at(j)->at(i+1).y(), pen);
            }
        }
    }

    /*for( int i=0; i<m_points.count(); ++i )
    {
            if(i==m_points.count()-1){
                this->addLine(m_points.at(i).x(),m_points.at(i).y(), m_points.at(0).x(),m_points.at(0).y(), pen);
            }
            else
            this->addLine(m_points.at(i).x(),m_points.at(i).y(), m_points.at(i+1).x(),m_points.at(i+1).y(), pen);
    }*/

    QVector<std::vector <Point2>> all_Outside_points;
    for (int i=0; i<outside_points.size();++i)
    {

        std::vector<Point2> vPoints1;
        vPoints1.clear();
        for (int j=0; j<outside_points.at(i)->count();++j)
        {
            vPoints1.push_back(Point2(outside_points.at(i)->at(j).toPoint().x(),outside_points.at(i)->at(j).toPoint().y()));
        }
        all_Outside_points.push_back(vPoints1);
    }
    for (int i=0;i<dielectric_points.size();i++)
    {
        std::vector<Point2> vPoints1;
        vPoints1.clear();
        for (int j=0; j<dielectric_points.at(i)->count();j++)
        {
            vPoints1.push_back(Point2(dielectric_points.at(i)->at(j).toPoint().x(),dielectric_points.at(i)->at(j).toPoint().y()));
        }
        all_Outside_points.push_back(vPoints1);
    }

    for (int i=0; i<all_Outside_points.size();i++)
    {
    dt.insert(all_Outside_points[i]);
    }

    QVector<std::vector <Point2>> all_Inside_points;
    for( int i=0; i<inside_points.size(); ++i )
        {
        std::vector<Point2> vPoints2;
        vPoints2.clear();
        for( int j=0; j<inside_points.at(i)->count(); ++j )
        {
            vPoints2.push_back(Point2(inside_points.at(i)->at(j).toPoint().x(),inside_points.at(i)->at(j).toPoint().y()));
            //std::cout << Point2(inside_points.at(i)->at(j).x(), inside_points.at(i)->at(j).y()) << std::endl;
        }
        all_Inside_points.push_back(vPoints2);
    }
    for( int i=0; i<gr_cond_points.size(); ++i )
        {
        std::vector<Point2> vPoints2;
        vPoints2.clear();
        for( int j=0; j<gr_cond_points.at(i)->count(); ++j )
        {
            vPoints2.push_back(Point2(gr_cond_points.at(i)->at(j).toPoint().x(),gr_cond_points.at(i)->at(j).toPoint().y()));
            //std::cout << Point2(gr_cond_points.at(i)->at(j).x(), gr_cond_points.at(i)->at(j).y()) << std::endl;
        }
        all_Inside_points.push_back(vPoints2);
    }

    QVector<std::vector<Segment2>> all_outside_vSegments1;
    for (int i=0; i<all_Outside_points.size();i++)
    {
        std::vector<Segment2> vSegments1;
        vSegments1.clear();
        for (size_t j=0; j<all_Outside_points[i].size();j++)
        {
            Point2 p0(all_Outside_points.at(i)[j]);
            Point2 p1(all_Outside_points.at(i)[(j+1)%all_Outside_points.at(i).size()]);
            vSegments1.push_back(Segment2(p0,p1));
        }
        all_outside_vSegments1.push_back(vSegments1);
    }

    QVector<std::vector<Segment2>> all_inside_vSegments2;
    for(int i = 0; i<all_Inside_points.size(); i++){
        std::vector<Segment2> vSegments2;
        vSegments2.clear();
        for(size_t j=0;j<all_Inside_points[i].size();++j)
        {
            Point2 p0(all_Inside_points.at(i)[j]);
            Point2 p1(all_Inside_points.at(i)[(j+1)%all_Inside_points.at(i).size()]);
            vSegments2.push_back(Segment2(p0,p1));
        }
        all_inside_vSegments2.push_back(vSegments2);
    }


    ConstraintGraph2* pCG1(NULL);

    QVector<Zone2*> all_outsideZone;
    for (int i = 0; i<all_outside_vSegments1.size();i++)
    {
        ConstraintGraph2* pCG1(NULL);
        pCG1=dt.createConstraint(all_outside_vSegments1[i],CIS_CONSTRAINED_DELAUNAY);
        all_outsideZone.push_back((dt.createZone(pCG1,ZL_INSIDE)));
    }

    Zone2* outsideSumZone = all_outsideZone[0];
    for (int i = 1; i<all_outsideZone.size();i++)
    {
        outsideSumZone = zoneUnion(outsideSumZone,all_outsideZone[i]);
    }

    QVector<Zone2*> all_insideZone;
    for(int i =0; i<all_inside_vSegments2.size(); i++){
        ConstraintGraph2* pCG2(NULL);
        pCG2=dt.createConstraint(all_inside_vSegments2[i],CIS_CONSTRAINED_DELAUNAY);
        all_insideZone.push_back((dt.createZone(pCG2,ZL_INSIDE)));
    }

    Zone2* insideSumZone = all_insideZone[0];
    for(int i = 1; i<all_insideZone.size(); i++){
        insideSumZone = zoneUnion(insideSumZone, all_insideZone[i]);
    }
    dt.applyConstraintsAndZones();
     qDebug()<<"13";
    Zone2* iZone(zoneDifference(outsideSumZone,insideSumZone));
    Zone2* pZone(iZone->convertToBoundedZone());
    MeshGenParams params(pZone);



    // МОЖЕТ ВЫЗЫВАТЬ ОШИБКУ
    dt.refine(pZone,minAngle,minEdgeLen,maxEdgeLen,true); // max should be max 1/3 of max
    //

    vZoneT.clear();
    pZone->getTriangles(vZoneT);
    nodesD.clear();
    trsD.clear();
    nodesD.set_size(vZoneT.size()*3, 2);
    trsD.set_size(vZoneT.size(),3);
    //qDebug() << vZoneT.size()*3;
    bcs_vec.clear(); nodes_vec.clear(); trs_vec.clear();
    int k=0;
    bool b = false;   
    //qDebug() << dt.numberOfTriangles();
    emit send_triangles(dt.numberOfTriangles());
    //qDebug() << inside_points.size() << outside_points.size();

        for(size_t i=0;i<vZoneT.size();++i)
    {
        for(int j=0; j<3; j++){
            if(nodes_vec.indexOf(std::make_pair(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y()))==-1)
            {
                nodes_vec.push_back(std::make_pair(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y()));
                trs_vec.push_back(k);

                //QGraphicsTextItem *zero_p = this->addText(QString::number(k));
               // zero_p->setDefaultTextColor(Qt::red);
               // zero_p->setPos(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y());

                for( int z=0; z<outside_points.size(); ++z )
                {
                    for( int q=0; q<outside_points.at(z)->count(); ++q )
                    {
                       // qDebug() << z << q << " outside";
                        // только для квадратов
                        double x1,x2,y1,y2;
                        if(q<outside_points.at(z)->count()-1){
                                x1 = outside_points.at(z)->at(q).x();
                                x2 = outside_points.at(z)->at(q+1).x();
                                y1 = outside_points.at(z)->at(q).y();
                                y2 = outside_points.at(z)->at(q+1).y();

                        } else {
                                x1 = outside_points.at(z)->at(q).x();
                                x2 = outside_points.at(z)->at(0).x();
                                y1 = outside_points.at(z)->at(q).y();
                                y2 = outside_points.at(z)->at(0).y();
                        }
                       // if(((vZoneT[i]->getCorner(j)->x() == x1) && (vZoneT[i]->getCorner(j)->y() >= y1)
                         //       && (vZoneT[i]->getCorner(j)->y() <= y2)) ||
                           //     ((vZoneT[i]->getCorner(j)->y() == y1) && (vZoneT[i]->getCorner(j)->x() >= x1)
                             //                                   && (vZoneT[i]->getCorner(j)->x() <= x2)))
                        //qDebug() << abs((vZoneT[i]->getCorner(j)->x()-x1)*(y2-y1)-(vZoneT[i]->getCorner(j)->y()-y1)*(x2-x1));
                        double x = vZoneT[i]->getCorner(j)->x();
                        double y = vZoneT[i]->getCorner(j)->y();
                        double t = ((x-x1)*(x2-x1)+(y-y1)*(y2-y1))/
                                (pow((x2-x1),2)+pow((y2-y1),2));
                        if(t<0) t=0;
                        else if(t>1) t = 1;

                        double l = sqrt(pow(x1 - x + (x2-x1)*t,2) +
                                        pow(y1 - y + (y2-y1)*t,2));
                        //qDebug() << x1 << y1 << x2 << y2 << x << y << pow(x1 - x + (x2-x1)*t,2) << pow(y1 - y + (y2-y1)*t,2) << t << " l="<< l << " outside " << k;
                        //qDebug () << l;
                        //if(abs((x-x1)*(y2-y1)-(y-y1)*(x2-x1)) <= 0.001
                        //        && ((x<=x2 && x>=x1) ||
                        //            (x>=x2 && x<=x1)))
                        if(l<1)
                        {
                           // QGraphicsTextItem *zero_p = this->addText(QString::number(k));
                           // zero_p->setDefaultTextColor(Qt::green);
                            //zero_p->setPos(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y());
                           // qDebug() << m_points.at(z).x()-vZoneT[i]->getCorner(j)->x() << m_points.at(z).y()-vZoneT[i]->getCorner(j)->y();
                            bcs_vec.push_back(std::make_pair(k,0));
                            //map[k]=0;
                            //qDebug() << k << " outside"; //WATCH HERE REDO!!!!!!!!!!!!!
                            break;
                        }

                    }
                }
                for( int z=0; z<inside_points.size(); ++z )
                {
                    for( int q=0; q<inside_points.at(z)->count(); ++q )
                    {
                      //  qDebug() << z << q << " inside";
                        double x1,x2,y1,y2;
                        if(q<inside_points.at(z)->count()-1){
                                x1 = inside_points.at(z)->at(q).x();
                                x2 = inside_points.at(z)->at(q+1).x();
                                y1 = inside_points.at(z)->at(q).y();
                                y2 = inside_points.at(z)->at(q+1).y();
                        } else {
                                x1 = inside_points.at(z)->at(q).x();
                                x2 = inside_points.at(z)->at(0).x();
                                y1 = inside_points.at(z)->at(q).y();
                                y2 = inside_points.at(z)->at(0).y();

                        }
                        //if(((vZoneT[i]->getCorner(j)->x() == x1) && (vZoneT[i]->getCorner(j)->y() >= y1)
                          //      && (vZoneT[i]->getCorner(j)->y() <= y2)) ||
                            //    ((vZoneT[i]->getCorner(j)->y() == y1) && (vZoneT[i]->getCorner(j)->x() >= x1)
                              //                                  && (vZoneT[i]->getCorner(j)->x() <= x2)))
                        //qDebug() << (vZoneT[i]->getCorner(j)->x()-x1)*(y2-y1) << (vZoneT[i]->getCorner(j)->y()-y1)*(x2-x1) << k;
                        double x = vZoneT[i]->getCorner(j)->x();
                        double y = vZoneT[i]->getCorner(j)->y();

                        double t = ((x-x1)*(x2-x1)+(y-y1)*(y2-y1))/
                                (pow((x2-x1),2)+pow((y2-y1),2));
                        if(t<0) t=0;
                        else if(t>1) t = 1;

                        double l = sqrt(pow(x1 - x + (x2-x1)*t,2) +
                                        pow(y1 - y + (y2-y1)*t,2));

                        //if(abs((vZoneT[i]->getCorner(j)->x()-x1)*(y2-y1)-(vZoneT[i]->getCorner(j)->y()-y1)*(x2-x1)) <= 0.001
                        //        && ((vZoneT[i]->getCorner(j)->x()<=x2 && vZoneT[i]->getCorner(j)->x()>=x1) ||
                        //            (vZoneT[i]->getCorner(j)->x()>=x2 && vZoneT[i]->getCorner(j)->x()<=x1)))
                        //{
                       // qDebug() << x1 << y1 << x2 << y2 << x << y << pow(x1 - x + (x2-x1)*t,2) << pow(y1 - y + (y2-y1)*t,2) << t << " l="<< l << " inside " << k;

                        if(l<1){
                           // QGraphicsTextItem *zero_p = this->addText(QString::number(k));
                           // zero_p->setDefaultTextColor(Qt::blue);
                           // zero_p->setPos(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y());
                            bcs_vec.push_back(std::make_pair(k,1));
                            map[k]=z;
                            //qDebug()<<map[k]<<k;
                            //qDebug() << k << " inside"; //WATCH HERE REDO!!!!!!!!!!!!!
                            break;
                        }

                    }
                }

                for( int z=0; z<gr_cond_points.size(); ++z )
                {
                    for( int q=0; q<gr_cond_points.at(z)->count(); ++q )
                    {
                      //  qDebug() << z << q << " inside";
                        double x1,x2,y1,y2;
                        if(q<gr_cond_points.at(z)->count()-1){
                                x1 = gr_cond_points.at(z)->at(q).x();
                                x2 = gr_cond_points.at(z)->at(q+1).x();
                                y1 = gr_cond_points.at(z)->at(q).y();
                                y2 = gr_cond_points.at(z)->at(q+1).y();
                        } else {
                                x1 = gr_cond_points.at(z)->at(q).x();
                                x2 = gr_cond_points.at(z)->at(0).x();
                                y1 = gr_cond_points.at(z)->at(q).y();
                                y2 = gr_cond_points.at(z)->at(0).y();

                        }
                        //if(((vZoneT[i]->getCorner(j)->x() == x1) && (vZoneT[i]->getCorner(j)->y() >= y1)
                          //      && (vZoneT[i]->getCorner(j)->y() <= y2)) ||
                            //    ((vZoneT[i]->getCorner(j)->y() == y1) && (vZoneT[i]->getCorner(j)->x() >= x1)
                              //                                  && (vZoneT[i]->getCorner(j)->x() <= x2)))
                        //qDebug() << (vZoneT[i]->getCorner(j)->x()-x1)*(y2-y1) << (vZoneT[i]->getCorner(j)->y()-y1)*(x2-x1) << k;
                        double x = vZoneT[i]->getCorner(j)->x();
                        double y = vZoneT[i]->getCorner(j)->y();

                        double t = ((x-x1)*(x2-x1)+(y-y1)*(y2-y1))/
                                (pow((x2-x1),2)+pow((y2-y1),2));
                        if(t<0) t=0;
                        else if(t>1) t = 1;

                        double l = sqrt(pow(x1 - x + (x2-x1)*t,2) +
                                        pow(y1 - y + (y2-y1)*t,2));

                        //if(abs((vZoneT[i]->getCorner(j)->x()-x1)*(y2-y1)-(vZoneT[i]->getCorner(j)->y()-y1)*(x2-x1)) <= 0.001
                        //        && ((vZoneT[i]->getCorner(j)->x()<=x2 && vZoneT[i]->getCorner(j)->x()>=x1) ||
                        //            (vZoneT[i]->getCorner(j)->x()>=x2 && vZoneT[i]->getCorner(j)->x()<=x1)))
                        //{
                       // qDebug() << x1 << y1 << x2 << y2 << x << y << pow(x1 - x + (x2-x1)*t,2) << pow(y1 - y + (y2-y1)*t,2) << t << " l="<< l << " inside " << k;

                        if(l<1){
                           // QGraphicsTextItem *zero_p = this->addText(QString::number(k));
                           // zero_p->setDefaultTextColor(Qt::blue);
                           // zero_p->setPos(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y());
                            bcs_vec.push_back(std::make_pair(k,0));
                            map[k]=z;
                            //qDebug()<<map[k]<<k;
                            //qDebug() << k << " inside"; //WATCH HERE REDO!!!!!!!!!!!!!
                            break;
                        }

                    }
                }
                /*for( int z=0; z<m_points.count(); ++z )
                {
                    if(abs(m_points.at(z).x()-vZoneT[i]->getCorner(j)->x())<=2 ||
                       abs(m_points.at(z).y()-vZoneT[i]->getCorner(j)->y())<=2 )
                    {
                       // qDebug() << m_points.at(z).x()-vZoneT[i]->getCorner(j)->x() << m_points.at(z).y()-vZoneT[i]->getCorner(j)->y();
                        bcs_vec.push_back(std::make_pair(k,0));
                        //qDebug() << k;
                        break;
                    }
                }*/
                k=k+1;
            } else {
                trs_vec.push_back(nodes_vec.indexOf(std::make_pair(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y())));
            }
            //nodes(k,0) = vZoneT[i]->getCorner(j)->x();
            //nodes(k,1) = vZoneT[i]->getCorner(j)->y();
            //trs(i, j) = k+1;
            //qDebug() <<vZoneT[i]->getCorner(j)->x() << vZoneT[i]->getCorner(j)->y();

            //if (!b) bcs_vec.push_back(std::make_pair(k+1,1));

            // проверить тут значения, по возможности добавить вектор, куда все складывать
            if(j==2){
                 this->addLine(vZoneT[i]->getCorner(j-1)->x(), vZoneT[i]->getCorner(j-1)->y(), vZoneT[i]->getCorner(0)->x(), vZoneT[i]->getCorner(0)->y(), pen);
            }
            else
                this->addLine(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y(), vZoneT[i]->getCorner(j+1)->x(), vZoneT[i]->getCorner(j+1)->y(), pen);
        }
        //trs << endr;
    }
   // qDebug() << bcs_vec;
    // Нахождение треугольников в зоне диэлектрика(только для прямоугольников) ////////////////////////////////////////////
    for (int i=0; i<dielectric_points.size();i++)
    {
    for(int k=0; k<trs_vec.size()/3;k++)
    {
        domains.push_back(std::make_pair(k,1));
        int rez=0;

            double Xmax,Xmin,Ymax,Ymin;
            Xmax=dielectric_points.at(i)->at(0).x();
            Xmin=dielectric_points.at(i)->at(0).x();
            Ymax=dielectric_points.at(i)->at(0).y();
            Ymin=dielectric_points.at(i)->at(0).y();
            //qDebug()<<dielectric_points.at(i)->size();
            for (int j=0; j<dielectric_points.at(i)->size();j++)
            {
                if (dielectric_points.at(i)->at(j).x()>Xmax)
                    Xmax=dielectric_points.at(i)->at(j).x();
                if (dielectric_points.at(i)->at(j).x()<Xmin)
                    Xmin=dielectric_points.at(i)->at(j).x();
                if (dielectric_points.at(i)->at(j).y()>Ymax)
                    Ymax=dielectric_points.at(i)->at(j).y();
                if (dielectric_points.at(i)->at(j).y()<Ymin)
                    Ymin=dielectric_points.at(i)->at(j).y();
                //qDebug()<<Xmax<<" "<<Xmin;
            }
            if (nodes_vec.at(trs_vec.at(k*3)).first>=Xmin && nodes_vec.at(trs_vec.at(k*3)).first<=Xmax)
            {
                if (nodes_vec.at(trs_vec.at(k*3)).second>=Ymin && nodes_vec.at(trs_vec.at(k*3)).second<=Ymax)
                    rez++;
            }
            if (nodes_vec.at(trs_vec.at(k*3+1)).first>=Xmin && nodes_vec.at(trs_vec.at(k*3+1)).first<=Xmax)
            {
                if (nodes_vec.at(trs_vec.at(k*3+1)).second>=Ymin && nodes_vec.at(trs_vec.at(k*3+1)).second<=Ymax)
                    rez++;
            }
            if (nodes_vec.at(trs_vec.at(k*3+2)).first>=Xmin && nodes_vec.at(trs_vec.at(k*3+2)).first<=Xmax)
            {
                if (nodes_vec.at(trs_vec.at(k*3+2)).second>=Ymin && nodes_vec.at(trs_vec.at(k*3+2)).second<=Ymax)
                    rez++;
            }
            if (rez==3)
            {
                if(domains.at(k).second!=2)
                domains.at(k).second=i+2;
            }
            rez=0;
        }

    }
    for (int i=0; i<domains.size(); i++)
    {

            qDebug()<<domains.at(i);

    }


    std::vector<std::pair<double, double>> bcs_vec_temp;
    bcs_vec_temp = bcs_vec;

}

