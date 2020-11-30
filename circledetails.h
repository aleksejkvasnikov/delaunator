#ifndef CIRCLEDETAILS_H
#define CIRCLEDETAILS_H

#include <QWidget>

namespace Ui {
class circledetails;
}

class circledetails : public QWidget
{
    Q_OBJECT

public:
    explicit circledetails(QWidget *parent = 0);
    int getPointNumber(){return pointNumber;}
    double getX0(){return x0;}
    double getX1(){return x1;}
    double getY0(){return y0;}
    double getY1(){return y1;}
    int getStatus(){return gr_checked;}
    ~circledetails();
    void setMinMax(double x0, double y0, double x1, double y1);
signals:
    void detail_entered();
private slots:
    void on_cancelButton_clicked();

    void on_chargedButt_clicked();

    void on_groundedButt_clicked();

    void on_okButton_clicked();

    void on_dielectricButton_clicked();


private:
    Ui::circledetails *ui;
    int pointNumber = 30;
    double x0, y0, x1, y1;
    int gr_checked = 0;
};

#endif // CIRCLEDETAILS_H
