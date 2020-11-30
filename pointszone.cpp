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
    ui->dielectricButton->setChecked(false);
    gr_checked = 1;
}


void PointsZone::on_groundedButt_clicked()
{
    ui->chargedButt->setChecked(false);
    ui->dielectricButton->setChecked(false);
    gr_checked = 0;
}


void PointsZone::on_buttonBox_rejected()
{
    hide();
}

void PointsZone::on_dielectricButton_clicked()
{
    ui->chargedButt->setChecked(false);
    ui->groundedButt->setChecked(false);
    gr_checked = 2;
}
