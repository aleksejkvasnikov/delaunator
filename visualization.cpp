#include "visualization.h"
#include "ui_visualization.h"
#include "armadillo"

using namespace arma;

visualization::visualization(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::visualization)
{
    ui->setupUi(this);

}

visualization::~visualization()
{
    delete ui;
}

void visualization::Set_Data(QVector<mat> V,mat trs, mat nds, int Num)
{
    V_all = V;
    TRS = trs;
    NDS = nds;
    ui->comboBox->clear();
    for (int i=0; i<Num;i++)
    {
        ui->comboBox->addItem(QString::number(i+1));
    }
    Num = Num - 1;
    int Pr = 1;
    while (Num != 0)
    {
        for (int i=0; i<Num;i++)
        {
            ui->comboBox->addItem(QString::number(Pr)+","+QString::number(i+Pr+1)+" = "+
                                  QString::number(i+Pr+1)+","+QString::number(Pr));
        }
        Num = Num - 1;
        Pr++;
    }
}

void visualization::get_settings(mat &V, mat &trs, mat &nds, double &start, double &end, double &step)
{
    V = V_select;
    trs = TRS;
    nds = NDS;
    start = start_line;
    end = end_line;
    step = step_line;
}

void visualization::clear()
{
     V_all.clear();
     TRS.clear();
     NDS.clear();
     V_select.clear();
}


void visualization::on_radioButton_2_clicked()
{
    ui->radioButton->setChecked(false);
    ui->groupBox_2->setEnabled(true);
    line = ui->checkBox_3->isChecked();
    color = ui->checkBox->isChecked();
    mesh = ui->checkBox_2->isChecked();
    start_line = ui->doubleSpinBox_3->value();
    end_line = ui->doubleSpinBox->value();
    step_line = ui->doubleSpinBox_2->value();
}

void visualization::on_radioButton_clicked()
{
    ui->radioButton_2->setChecked(false);
    ui->groupBox_2->setEnabled(false);
    set_default();
}

void visualization::on_buttonBox_accepted()
{
    emit plot_redy();
    hide();
}

void visualization::set_default()
{
    line = true;
    color = true;
    mesh = false;
    start_line = 0.01;
    end_line = 0.99;
    step_line = 0.1;
}

void visualization::on_doubleSpinBox_3_valueChanged(double arg1)
{
    start_line = arg1;
}

void visualization::on_doubleSpinBox_valueChanged(double arg1)
{
    end_line = arg1;
}

void visualization::on_doubleSpinBox_2_valueChanged(double arg1)
{
    step_line = arg1;
}

void visualization::on_checkBox_stateChanged(int arg1)
{
    color = ui->checkBox->isChecked();
}

void visualization::on_checkBox_3_stateChanged(int arg1)
{
    line = ui->checkBox_3->isChecked();
}

void visualization::on_checkBox_2_stateChanged(int arg1)
{
    mesh = ui->checkBox_2->isChecked();
}

void visualization::on_comboBox_currentIndexChanged(int index)
{
    V_select = V_all.at(index);
}

void visualization::on_buttonBox_rejected()
{
    hide();
}
