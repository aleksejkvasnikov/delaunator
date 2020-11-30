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
    int getStatus(){return gr_checked;}

signals:
    void zone_entered();

private slots:
    void on_buttonBox_accepted();

    void on_groundedButt_clicked();

    void on_chargedButt_clicked();

    void on_buttonBox_rejected();

    void on_dielectricButton_clicked();

private:
    Ui::PointsZone *ui;
    int gr_checked = 0;
};

#endif // POINTSZONE_H
