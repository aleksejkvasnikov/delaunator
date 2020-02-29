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
private:
    Ui::MainWindow *ui;

protected:
//    bool
private slots:
    void move_scene_slot(bool mode);
    void addTringlesData(int tr);
    void update_position(QPointF pos);
    void doFEMcalc(bool mode);
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
};

#endif // MAINWINDOW_H
