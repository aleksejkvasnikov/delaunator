#ifndef POINTSZONE_H
#define POINTSZONE_H

#include <QDialog>
#include <QWidget>

namespace Ui {
class PointsZone;
}

class PointsZone : public QDialog
{
    Q_OBJECT

public:
    explicit PointsZone(QWidget *parent = 0);
    ~PointsZone();
    bool getStatus(){return gr_checked;}

signals:
    void zone_entered();

private slots:
    void on_buttonBox_accepted();

    void on_groundedButt_clicked();

    void on_chargedButt_clicked();

    void on_buttonBox_rejected();

private:
    Ui::PointsZone *ui;
    bool gr_checked = false;
};

#endif // POINTSZONE_H
