#include "circledetails.h"
#include "ui_circledetails.h"

circledetails::circledetails(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::circledetails)
{
    ui->setupUi(this);
    ui->groundedButt->setChecked(true);
}

circledetails::~circledetails()
{
    delete ui;
}

void circledetails::setMinMax(double x0, double y0, double x1, double y1)
{
    ui->x0Edit->setText(QString::number(x0));
    ui->y0Edit->setText(QString::number(y0*-1));
    ui->x1Edit->setText(QString::number(x1));
    ui->y1Edit->setText(QString::number(y1*-1));
}

void circledetails::on_cancelButton_clicked()
{
    hide();
}

void circledetails::on_chargedButt_clicked()
{
    ui->groundedButt->setChecked(false);
    gr_checked = true;
}

void circledetails::on_groundedButt_clicked()
{
    ui->chargedButt->setChecked(false);
    gr_checked = false;
}

void circledetails::on_okButton_clicked()
{
    x0 = ui->x0Edit->text().toDouble();
    x1 = ui->x1Edit->text().toDouble();
    y0 = ui->y0Edit->text().toDouble() * -1;
    y1 = ui->y1Edit->text().toDouble() * -1;
    pointNumber = ui->pointsEdit->text().toInt();
    emit detail_entered();
    hide();
}
