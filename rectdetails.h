#ifndef RECTDETAILS_H
#define RECTDETAILS_H

#include <QWidget>

namespace Ui {
class rectDetails;
}

class rectDetails : public QWidget
{
    Q_OBJECT

public:
    explicit rectDetails(QWidget *parent = 0);
    void setMinMax(double x, double y, double w, double h);
    ~rectDetails();
    double getMinX(){ return minX;}
    double getMaxX(){ return maxX;}
    double getMinY(){ return minY;}
    double getMaxY(){ return maxY;}
    bool getStatus(){ return gr_checked;}
signals:
    void detail_entered();
private slots:
    void on_okButton_clicked();

    void on_chargedButt_clicked();

    void on_groundedButt_clicked();

    void on_cancelButton_clicked();

private:
    Ui::rectDetails *ui;
    double minX, maxX, minY, maxY;
    bool gr_checked = false;
};

#endif // RECTDETAILS_H
