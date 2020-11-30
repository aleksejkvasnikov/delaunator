#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <QDialog>
#include "armadillo"
#include <QMap>

using namespace arma;

namespace Ui {
class visualization;
}

class visualization : public QDialog
{
    Q_OBJECT

public:
    explicit visualization(QWidget *parent = nullptr);
    ~visualization();
    void Set_Data (QVector<mat> V, mat trs, mat nds, int Num);
    bool get_line (){return line;}
    bool get_color(){return color;}
    bool get_mesh(){return mesh;}
    void get_settings(mat &V, mat &trs, mat &nds, double &start, double &end, double &step);
    void clear();
signals:
    void plot_redy();

private slots:
    void on_radioButton_2_clicked();

    void on_radioButton_clicked();

    void on_buttonBox_accepted();

    void on_doubleSpinBox_3_valueChanged(double arg1);

    void on_doubleSpinBox_valueChanged(double arg1);

    void on_doubleSpinBox_2_valueChanged(double arg1);

    void on_checkBox_stateChanged(int arg1);

    void on_checkBox_3_stateChanged(int arg1);

    void on_checkBox_2_stateChanged(int arg1);

    void on_comboBox_currentIndexChanged(int index);

    void on_buttonBox_rejected();

private:
    bool line;
    bool color;
    bool mesh;
    double start_line;
    double end_line;
    double step_line;
    Ui::visualization *ui;
    QVector<mat> V_all;
    mat TRS;
    mat NDS;
    mat V_select;
    void set_default();
};

#endif // VISUALIZATION_H
