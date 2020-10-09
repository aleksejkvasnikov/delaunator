#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "graphicscene.h"
#include "pointszone.h"
#include "QMouseEvent"
#include "QDialog"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    GraphicScene *scene;
    PointsZone *pointszone;
    double get_tri2d_cap(mat v, double nr_nodes, double nr_trs, mat nds, mat trs,mat domains);
    mat bcs_F, nodes_F, trs_F, domains_F,v_F,matrix_C;
    void FEM_Bandeson ();
    double W_Bandeson (mat noExt, mat noInt,mat no2xy,mat el2no);
    double C_F,nr_nodes_F,nr_trs_F,nr_trs_it=0,del,del1,del2;
private:
    Ui::MainWindow *ui;

protected:
//    bool
private slots:
    void move_scene_slot(bool mode);
    void addTringlesData(int tr);
    void update_position(QPointF pos);
    void doFEMcalc(bool mode);
    mat CmpElMtx_Bandeson(mat xy, double elInx);
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pointButton_clicked();
    void on_rectButton_clicked();
    void on_pushButton_8_clicked();
    void on_circleButton_clicked();
    void on_femCalc_clicked();
    void on_make_1_button_clicked();
    void on_showMesh_button_clicked();
    void pointszone_entered();
    void on_saveButton_clicked();
    void on_loadButton_clicked();
    void on_ReMesh_clicked();
    void RefinementTRS(double nr_trs, double nr_nds);
    void on_showE_button_clicked();
    void result_text(double C_F, double nr_trs_F,double nr_nodes_F,double times, double L, double Z1);
    void result_text(mat matrix_C,double nr_trs_F,double nr_nodes_F, mat L, double times);
    void on_pushButton_5_clicked();

    void on_Rect_Mesh_button_clicked();
    void on_FemBandesonButton_clicked();
    void on_feild_but_clicked();
};

#endif // MAINWINDOW_H
