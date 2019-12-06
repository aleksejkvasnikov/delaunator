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
    this->setBackgroundBrush(Qt::gray);

    QPen penZero(Qt::darkGreen, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen penLine(Qt::darkGray, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
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
    /*QGraphicsTextItem *f= this->addText("1");
    f->setPos(40, 1);
    QGraphicsTextItem *s= this->addText("2");
    s->setPos(60, 1);
    QGraphicsTextItem *t= this->addText("3");
    t->setPos(70, 70);
    QGraphicsTextItem *fo= this->addText("4");
    fo->setPos(30, 70);
    QGraphicsTextItem *fiv = this->addText("5");
    fiv->setPos(0, 0);
    QGraphicsTextItem *six = this->addText("6");
    six->setPos(100, 0);
    QGraphicsTextItem *sev = this->addText("7");
    sev->setPos(100, 100);
    QGraphicsTextItem *eig = this->addText("8");
    eig->setPos(0, 100); */
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

            ellipse->setBrush(Qt::white);
            m_points.append(me->scenePos());
        }
        if(this->activeDrawer == 4)
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

void GraphicScene::drawFinalRect()
{
    //qDebug () << "here";
    QGraphicsRectItem * rect = this->addRect(rectDet->getMinX(), rectDet->getMinY(), rectDet->getMaxX(), rectDet->getMaxY());
    rect->setBrush(Qt::transparent);
    QPen red_pen(Qt::red);
    QPen blue_pen(Qt::blue);
    QRectF qrf = rect->rect();
    // LOOK HERE 16.10
    QList <QPointF> *temp_points = new QList <QPointF>;
    temp_points->append(qrf.topLeft());
    temp_points->append(qrf.topRight());
    temp_points->append(qrf.bottomRight());
    temp_points->append(qrf.bottomLeft());
    if(rectDet->getStatus()){
        qDebug () << "inside";
        rect->setPen(blue_pen);
        inside_points.push_back(temp_points);
    } else {
        qDebug () << "outside";
        rect->setPen(red_pen);
        outside_points.push_back(temp_points);
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
    if(circleDet->getStatus()){
        qDebug () << "inside";
        inside_points.push_back(temp_points);
    } else {
        qDebug () << "outside";
        outside_points.push_back(temp_points);
    }
}

void GraphicScene::connectPoints()
{
    QList <QPointF> *temp_points = new QList <QPointF>;
    QPen red(Qt::red,Qt::DashLine);
    QPen blue(Qt::blue,Qt::DashLine);

    if (pointszone->getStatus())
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
    else
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
    m_points.clear();
    setActiveDrawer(0);
}

void GraphicScene::eraseAll()
{
    m_points.clear();
    outside_points.clear();
    inside_points.clear();
    this->clear();
    QPen penZero(Qt::darkGreen, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen penLine(Qt::darkGray, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
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
    E_x = abs(E_x)/E_x.max();
    E_y = abs(E_y)/E_y.max();


    for (int i=0; i<nr_trs; i++){
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        //qDebug() << n1 << n2 << n3 << E_mag(i,0)/E_mag.max();
        QPolygonF poly;
        poly << QPointF(nds(n1,0), nds(n1,1)) << QPointF(nds(n2,0), nds(n2,1)) << QPointF(nds(n3,0), nds(n3,1))<< QPointF(nds(n1,0), nds(n1,1));
        QPen pen(Qt::black);
        this->addPolygon(poly, pen, interpolate(E_mag(i,0), E_mag.max()));
        QBrush brush(Qt::red);
    }

}

void GraphicScene::show_mesh(mat v, double nr_trs, mat nds, mat trs)
{
    //cout << v;
    double max = 0;
    for (int i=0; i<nr_trs; i++){
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        if((v(n1)+v(n2)+v(n3)/3) > max) max = (v(n1)+v(n2)+v(n3)/3);
    }

    for (int i=0; i<nr_trs; i++){
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        //qDebug() << n1 << n2 << n3 << E_mag(i,0)/E_mag.max();
        QPolygonF poly;
        poly << QPointF(nds(n1,0), nds(n1,1)) << QPointF(nds(n2,0), nds(n2,1)) << QPointF(nds(n3,0), nds(n3,1))<< QPointF(nds(n1,0), nds(n1,1));
        QPen pen(Qt::black);
        this->addPolygon(poly, pen, interpolate(((v(n1)+v(n2)+v(n3)/3)), max));
        //qDebug() << abs((v(n1)+v(n2)+v(n3)/3))/v.max();
        QBrush brush(Qt::red);
    }
}
QColor GraphicScene::interpolate(double value, double max){
    if(value <0) value = 0;
    double ratio = value/max;
    QColor start = Qt::red;
    QColor end = Qt::blue;
    int r = (int)(ratio*start.red() + (1-ratio)*end.red());
    int g = (int)(ratio*start.green() + (1-ratio)*end.green());
    int b = (int)(ratio*start.blue() + (1-ratio)*end.blue());
    return QColor::fromRgb(r,g,b);
}
QColor GraphicScene::interpolate1(double ratio, double max)
{
    int r , g ,b;
    if (ratio<0) ratio=0;
    double val = ratio/max;
    qDebug() << ratio << max << ratio/max;
    if(val>=0 && val <0.2){
         qDebug () << 1;
         QColor start = Qt::cyan;  QColor mid = Qt::blue;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.2 && val <0.4){
        qDebug () << 2;
         QColor start = Qt::green;  QColor mid = Qt::cyan;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.4 && val <0.6){
        qDebug () << 3;
         QColor start = Qt::yellow;  QColor mid = Qt::green;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.6 && val <0.8){
        qDebug () << 4;
         QColor start = QColor( 0xFF, 0xA0, 0x00 );  QColor mid = Qt::yellow;
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    else if(val>=0.8 && val <=1){
        qDebug () << 5;
         QColor start =  Qt::red;  QColor mid =QColor( 0xFF, 0xA0, 0x00 );
         r = (int)(val*start.red() + (1-val)*mid.red());
         g = (int)(val*start.green() + (1-val)*mid.green());
         b = (int)(val*start.blue() + (1-val)*mid.blue());
    }
    return QColor::fromRgb(r,g,b);
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
void GraphicScene::doTriangles()
{
    // реализовать проход по всем объектам и каким-то образом сделать выбор режима областей (булевы функции) (?)
    this->clear();
// REDO
    QGraphicsTextItem *zero_p = this->addText("0");
    zero_p->setDefaultTextColor(Qt::red);
    zero_p->setPos(0, 0);
    QPen penZero(Qt::darkGreen, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen penLine(Qt::darkGray, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
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


    for (int i; i<all_Outside_points.size();i++)
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
       // qDebug() <<all_outside_vSegments1.size();
    //tut
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

    Zone2* iZone(zoneDifference(outsideSumZone,insideSumZone));
    Zone2* pZone(iZone->convertToBoundedZone());
    MeshGenParams params(pZone);
    //std::cout << minAngle << minEdgeLen << maxEdgeLen  << std::endl;
    dt.refine(pZone,minAngle,minEdgeLen,maxEdgeLen,true); // max should be max 1/3 of max

    //std::vector<Triangle2*> vZoneT;
    vZoneT.clear();
    pZone->getTriangles(vZoneT);
    nodes.clear();
    trs.clear();
    nodes.set_size(vZoneT.size()*3, 2);
    trs.set_size(vZoneT.size(),3);
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
            if(nodes_vec.indexOf(std::make_pair(vZoneT[i]->getCorner(j)->x(), vZoneT[i]->getCorner(j)->y()))==-1){
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
}