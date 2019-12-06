#include "pointszone.h"
#include "ui_pointszone.h"

PointsZone::PointsZone(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PointsZone)
{
    ui->setupUi(this);
}

PointsZone::~PointsZone()
{
    delete ui;
}

void PointsZone::on_buttonBox_accepted()
{
    emit zone_entered();
    hide();
}

void PointsZone::on_chargedButt_clicked()
{
    ui->groundedButt->setChecked(false);
    gr_checked = true;
}


void PointsZone::on_groundedButt_clicked()
{
    ui->chargedButt->setChecked(false);
    gr_checked = false;
}


void PointsZone::on_buttonBox_rejected()
{
    hide();
}
