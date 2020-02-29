#include "rectdetails.h"
#include "ui_rectdetails.h"
#include "QDebug"
rectDetails::rectDetails(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::rectDetails)
{
    ui->setupUi(this);
    ui->groundedButt->setChecked(true);
}

void rectDetails::setMinMax(double x, double y, double w, double h)
{
    ui->xMinEdit->setText(QString::number(x));
    ui->xMaxEdit->setText(QString::number(x+w));
    ui->yMinEdit->setText(QString::number((y+h)*-1));
    ui->yMaxEdit->setText(QString::number(y*-1));
}

rectDetails::~rectDetails()
{
    delete ui;
}

void rectDetails::on_okButton_clicked()
{
    //qDebug() << "HERE";
    minX = ui->xMinEdit->text().toDouble();
    maxX = ui->xMaxEdit->text().toDouble() - ui->xMinEdit->text().toDouble();
    minY = ui->yMaxEdit->text().toDouble() * -1;
    maxY = (ui->yMinEdit->text().toDouble()) * -1 - minY;
    emit detail_entered();
    hide();
}

void rectDetails::on_chargedButt_clicked()
{
    ui->groundedButt->setChecked(false);
    ui->dielectricButton->setChecked(false);
    ui->calcButton->setChecked(false);
    gr_checked = 1;
}

void rectDetails::on_groundedButt_clicked()
{
    ui->chargedButt->setChecked(false);
    ui->dielectricButton->setChecked(false);
    ui->calcButton->setChecked(false);
    gr_checked = 0;
}

void rectDetails::on_cancelButton_clicked()
{
    hide();
}

void rectDetails::on_dielectricButton_clicked()
{
    ui->groundedButt->setChecked(false);
    ui->chargedButt->setChecked(false);
    ui->calcButton->setChecked(false);
    gr_checked = 2;
}

void rectDetails::on_calcButton_clicked()
{
    ui->groundedButt->setChecked(false);
    ui->chargedButt->setChecked(false);
    ui->dielectricButton->setChecked(false);
    gr_checked = 3;
}
