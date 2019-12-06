#ifndef TR_OBJECT_H
#define TR_OBJECT_H
#include <QPointF>
#include <QList>

class tr_object
{
public:
    tr_object();
    void set_points(QList <QPointF> *points){
        this->m_points = *points;
    }
private:
    QList <QPointF> m_points;
};

#endif // TR_OBJECT_H
