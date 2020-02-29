#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "graphicscene.h"
#include <QDebug>
#include <QInputDialog>
#include <QErrorMessage>
#include <QElapsedTimer>
#include <graphics_view_zoom.h>
#include "armadillo"
#include "QColor"
#include <QFileDialog>
#include <QDataStream>
#include <QMessageBox>

int a;
#define M_PI 3.14
using namespace arma;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scene = new GraphicScene(ui->centralWidget);
    pointszone = new PointsZone;
    ui->graphicsView->setScene(scene);
 //   ui->graphicsView->setAlignment(Qt::AlignBottom|Qt::AlignLeft);
    ui->graphicsView->setSceneRect(-1800, -900, 3600, 1800);
    ui->graphicsView->show();
   // this->resize(600, 600);
    Graphics_view_zoom* z = new Graphics_view_zoom(ui->graphicsView);
    z->set_modifiers(Qt::NoModifier);
 //   connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(on_some_pushButton_clicked()));
    //ui->graphicsView->installEventFilter();
    QObject::connect(scene, SIGNAL(move_scene_sig(bool)), this, SLOT(move_scene_slot(bool)));
    QObject::connect(scene, SIGNAL(send_triangles(int)), this, SLOT(addTringlesData(int)));
    QObject::connect(scene, SIGNAL(mouse_positionChanged(QPointF)), this, SLOT(update_position(QPointF)));
    QObject::connect(scene->pointszone, SIGNAL(zone_entered()),this, SLOT(pointszone_entered()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::doFEMcalc(bool mode)
{
    mat AA;
    AA.load("Aq.txt");
    mat bcs, nodes, trs, domains;
    nodes.clear();
    nodes.set_size(scene->nodes_vec.size(), 2);

    for(int i=0; i<scene->nodes_vec.size(); i++){
        nodes(i,0)=scene->nodes_vec.at(i).first;
        nodes(i,1)=scene->nodes_vec.at(i).second;
    }
    trs.clear();
    trs.set_size(scene->trs_vec.size()/3,3);
    for(int i=0; i<scene->trs_vec.size()/3; i++){
        trs(i,0)=scene->trs_vec.at(3*i);
        trs(i,1)=scene->trs_vec.at(3*i+1);
        trs(i,2)=scene->trs_vec.at(3*i+2);
        //qDebug () << scene->trs_vec.at(i) << scene->trs_vec.at(i+1) << scene->trs_vec.at(i+2);
    }
    //cout << trs;
    bcs.clear();
    bcs.set_size(scene->bcs_vec.size(), 2);
    for (int i =0; i < scene->bcs_vec.size(); i++){
        bcs(i,0) = scene->bcs_vec.at(i).first;
        bcs(i,1) = scene->bcs_vec.at(i).second;
    }
    //cout << "\n--------------\n";
    //cout << bcs;
    //qDebug()<<scene->getMapData(bcs(1,0));

    domains.clear();
    domains.set_size(scene->domains.size(),2);
    for (int i=0; i<scene->domains.size(); i++)
    {
        domains(i,0)=scene->domains.at(i).first;
        domains(i,1)=scene->domains.at(i).second;
    }

    int nr_nodes = nodes.n_rows;
    int nr_trs = trs.n_rows;
    int nr_bcs = bcs.n_rows;
   // qDebug() << nr_nodes << nr_trs << nr_bcs;
    mat a(nr_nodes, nr_nodes, fill::zeros);
    mat f(nr_nodes, 1, fill::zeros);
    mat b(3, nr_trs, fill::zeros);
    mat c(3, nr_trs, fill::zeros);
    mat d(3,3, fill::ones);

    //cout << nodes<< bcs << trs;
    for(int i=0; i<nr_trs; i++){
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        //qDebug() << n1 << n2 << n3;
        //qDebug() << i;
        d(0,1) = nodes(n1,0); d(0,2) = nodes(n1,1);
        d(1,1) = nodes(n2,0); d(1,2) = nodes(n2,1);
        d(2,1) = nodes(n3,0); d(2,2) = nodes(n3,1);
        double Area = det(d)/2;
        //qDebug() << Area;
        mat tempest, temp;
        tempest << 1.0 << endr << 0.0 << endr << 0.0;
        temp = solve(d, tempest);
        b(0,i) = temp(1,0);
        c(0,i) = temp(2,0);
        //cout << temp(1,0) << temp(2,0);
        tempest << 0.0 << endr << 1.0 << endr << 0.0;
        temp = solve(d, tempest);
        b(1,i) = temp(1,0);
        c(1,i) = temp(2,0);
        tempest << 0.0 << endr << 0.0 << endr << 1.0;
        temp = solve(d, tempest);
        b(2,i) = temp(1,0);
        c(2,i) = temp(2,0);
        //qDebug() << a(n1,n1) << Area << pow(b(0,i),2) << pow(c(0,i),2);

        double eps;
        if(domains.at(i)==2)
            eps=ui->dielectric_edit->text().toDouble();
        else
            eps=1;



        a(n1,n1) = a(n1,n1) + Area*(pow(b(0,i),2) + pow(c(0,i),2))*(-eps);
        a(n1,n2) = a(n1,n2) + Area*(b(0,i)*b(1,i) + c(0,i)*c(1,i))*(-eps);
        a(n1,n3) = a(n1,n3) + Area*(b(0,i)*b(2,i) + c(0,i)*c(2,i))*(-eps);
        //qDebug() << a(n1,n1) << a(n1,n2) << a(n1,n3);

        a(n2,n2) = a(n2,n2) + Area*(pow(b(1,i),2) + pow(c(1,i),2))*(-eps);
        a(n2,n1) = a(n2,n1) + Area*(b(1,i)*b(0,i) + c(1,i)*c(0,i))*(-eps);
        a(n2,n3) = a(n2,n3) + Area*(b(1,i)*b(2,i) + c(1,i)*c(2,i))*(-eps);
        //qDebug() << a(n2,n2) << a(n2,n1) << a(n2,n3);

        a(n3,n3) = a(n3,n3) + Area*(pow(b(2,i),2) + pow(c(2,i),2))*(-eps);
        a(n3,n1) = a(n3,n1) + Area*(b(2,i)*b(0,i) + c(2,i)*c(0,i))*(-eps);
        a(n3,n2) = a(n3,n2) + Area*(b(2,i)*b(1,i) + c(2,i)*c(1,i))*(-eps);
        //qDebug() << a(n3,n3) << a(n3,n1) << a(n3,n2);
    }
    //cout << a;

    //a.save("After.txt", arma_ascii);
    for(int i=0; i<nr_bcs; i++){
        double n = bcs(i,0);
        //if (bcs(i,1)==1){
        a.row(n).fill(0.0);
        a(n,n) = 1.0;
        f(n,0) = bcs(i,1);
    }


    //cout << bcs;
    for(int i=0; i<nr_nodes; i++){
        if(a(i,i)==0){
            a.row(i).fill(0.0);
            a(i,i) = 1.0;
            f(i,0) = 0.0;
        }
    }
   // a.save("Arrr.txt", arma_ascii);
   // cout << a;
    //cout << "\n--------------\n";
   //cout << f;
    //cout << "\n--------------\n";
    if (scene->calc_trs.size()==0)
    {
   mat v;
    v = solve(mat(a),f);
  // cout << v;
    get_tri2d_cap(v, nr_nodes, nr_trs, nodes, trs, domains);
    qDebug()<<"trs["<<nr_trs<<"] nodes["<<nr_nodes<<"]";
    }

    if (scene->calc_trs.size()==1)
    {
        mat zone_trs;
        zone_trs.clear();
        zone_trs.set_size(scene->calc_trs.at(0).size()/3,3);
        for (int i=0; i<scene->calc_trs.at(0).size()/3;i++){
            zone_trs(i,0)=scene->calc_trs.at(0).at(3*i);
            zone_trs(i,1)=scene->calc_trs.at(0).at(3*i+1);
            zone_trs(i,2)=scene->calc_trs.at(0).at(3*i+2);
        }
        int nr_zone_trs=zone_trs.n_rows;
        mat v;
         v = solve(mat(a),f);
       // cout << v;
         get_tri2d_cap(v, nr_nodes, nr_zone_trs, nodes, zone_trs,domains);
    }

    if (scene->calc_trs.size()==2)
    {
        mat trs_1;
        mat trs_2;
        trs_1.clear();
        trs_1.set_size(scene->calc_trs.at(0).size()/3,3);
        for (int i=0; i<scene->calc_trs.at(0).size()/3;i++){
            trs_1(i,0)=scene->calc_trs.at(0).at(3*i);
            trs_1(i,1)=scene->calc_trs.at(0).at(3*i+1);
            trs_1(i,2)=scene->calc_trs.at(0).at(3*i+2);
        }
        trs_2.clear();
        trs_2.set_size(scene->calc_trs.at(1).size()/3,3);
        for (int i=0; i<scene->calc_trs.at(1).size()/3;i++){
            trs_2(i,0)=scene->calc_trs.at(1).at(3*i);
            trs_2(i,1)=scene->calc_trs.at(1).at(3*i+1);
            trs_2(i,2)=scene->calc_trs.at(1).at(3*i+2);
        }
        qDebug()<<"tut";
        mat f1,f2;
        f1=f;
        f2=f;
        for (int i=0; i<f.n_rows; i++)
        {
            if (scene->getMapData(i)==1)
                f1(i,0)=0;
            else if (scene->getMapData(i)==0)
                f2(i,0)=0;
        }
       // cout << "\n--------------\n";
       // cout << f1;
       // cout << "\n--------------\n";
         //       cout << f2;
        mat v11,v22,v12;
        v11 = solve(mat(a),f1);
        v22 = solve(mat(a),f2);
        v12 = solve(mat(a),f);
        int nr_trs_1=trs_1.n_rows;
        int nr_trs_2=trs_2.n_rows;
        //qDebug()<<"c11'";
        double W11,W12,W22;
       W11 = get_tri2d_cap(v11, nr_nodes, nr_trs, nodes, trs,domains);
        //qDebug()<<"c12";
        //get_tri2d_cap(v1, nr_nodes, nr_trs_2, nodes, trs_2,domains);
        //qDebug()<<"c21";
        //get_tri2d_cap(v2, nr_nodes, nr_trs_1, nodes, trs_1,domains);
      //  qDebug()<<"c22'";
       W22 = get_tri2d_cap(v22, nr_nodes, nr_trs, nodes, trs,domains);
       W12 = get_tri2d_cap(v12, nr_nodes, nr_trs, nodes, trs, domains);
       double C11, C12, C22;
       C11 = 2 * W11;
       C22 = 2 * W22;
       C12 = W12 - (C11+C22)/2;
       mat C;
       C.set_size(2,2);
       C(0,0) = C11 + C12;
       C(0,1) = C12;
       C(1,0) = C12;
       C(1,1) = C22 + C12;
       qDebug()<<"****************************";
       qDebug()<<"trs["<<nr_trs<<"] nodes["<<nr_nodes<<"]";
       qDebug()<<"["<<"W11="<<W11<<", "<<"W12="<<W12<<", "<<"W22="<<W22<<"]";
       qDebug()<<endl;
       qDebug()<<C11<<", "<<C12;
       qDebug()<<C12<<", "<<C22;
       qDebug()<<endl;
       cout<<C;
      /* if(!mode)
          scene->get_tri2d_E(v11, nr_nodes, nr_trs, nodes, trs);
      else scene->show_mesh(v12, nr_trs, nodes, trs);
*/
    }



}
double MainWindow::get_tri2d_cap(mat v, double nr_nodes, double nr_trs, mat nds, mat trs, mat domains)
{
    double eps0 = (1.0/(36.0*M_PI))*pow(10.0,-9);
    double mu0 = 4.0 * M_PI * pow(10.0, -7);
    double U = 0.0; double c1,c2,c3,b1,b2,b3;
    double eps;
    //cout << v;
    mat d(3,3, fill::ones);
    for(int i=0; i<nr_trs; i++){
        if (domains(i,1)==2)
            eps=ui->dielectric_edit->text().toDouble();
        else
            eps=1;
        double n1 = trs(i,0); double n2 = trs(i,1); double n3 = trs(i,2);
        //qDebug() << n1 << n2 << n3;
        //qDebug() << i;
        d(0,1) = nds(n1,0); d(0,2) = nds(n1,1);
        d(1,1) = nds(n2,0); d(1,2) = nds(n2,1);
        d(2,1) = nds(n3,0); d(2,2) = nds(n3,1);
        mat tempest, temp;
        tempest << 1.0 << endr << 0.0 << endr << 0.0;
        temp = solve(d, tempest);
        b1 = temp(1,0);
        c1 = temp(2,0);
        tempest << 0.0 << endr << 1.0 << endr << 0.0;
        temp = solve(d, tempest);
        b2 = temp(1,0);
        c2 = temp(2,0);
        tempest << 0.0 << endr << 0.0 << endr << 1.0;
        temp = solve(d, tempest);
        b3 = temp(1,0);
        c3 = temp(2,0);
        double Area = det(d)/2.0;
        //qDebug() << Area;
        if (Area <=0){
            qDebug() << "ERROR";
        }

        //qDebug() << pow((v(n1,0)*b1 + v(n2,0)*b2 + v(n3,0)*b3),2);
        U = U + Area * ( pow((v(n1,0)*b1 + v(n2,0)*b2 + v(n3,0)*b3),2) + pow((v(n1,0)*c1 + v(n2,0)*c2 + v(n3,0)*c3),2))*eps;
        //qDebug() << U;
    }
    U=U*eps0/2.0;
    double C = 2*U;

    QString text;
    qDebug() << "C = " << C;
    double L = (mu0 * eps0)/ C;
    qDebug() << "L = " << L;
    double Z1 = pow((L/C),0.5);
    qDebug() << "Z1 = " << Z1;
    text="C = "+QString::number(C);
    ui->textBrowser->setText(text);
    return U;
}

void MainWindow::move_scene_slot(bool mode)
{
    scene->setActiveDrawer(0);
    ui->pointButton->setStyleSheet("");
    ui->rectButton->setStyleSheet("");
    ui->circleButton->setStyleSheet("");
    if(mode){
        ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    } else{
        ui->graphicsView->setDragMode(QGraphicsView::NoDrag);
    }
}

void MainWindow::addTringlesData(int tr)
{
    //qDebug() << tr;
    statusBar()->showMessage("Количество треугольников: " + QString::number(tr));
}

void MainWindow::update_position(QPointF pos)
{
    statusBar()->showMessage("x:"+QString::number(pos.x())+" y:"+QString::number(pos.y()*-1));
}

void MainWindow::on_pushButton_clicked()
{
    scene->connectPoints();
    ui->pointButton->setStyleSheet("");
    ui->circleButton->setStyleSheet("");
    ui->rectButton->setStyleSheet("");
}

void MainWindow::on_pushButton_2_clicked() // button New
{
    scene->newObj();
}

void MainWindow::on_pushButton_3_clicked()
{
    scene->eraseAll();
}

void MainWindow::on_pushButton_4_clicked()
{
   // system("start C:/work/talgatfolder/Release/ShieldingModule2.exe");
    if(scene->items().size()==0){
        QErrorMessage *dial = new QErrorMessage(this);
        dial->showMessage("0 точек на канвасе");
    } else{
        scene->setParams(ui->mAngle_edit->text().toDouble(), ui->minEdge_edit->text().toDouble(), ui->maxEdge_edit->text().toDouble());
        QElapsedTimer timer;
        timer.start();
        scene->doTriangles();        
        qDebug() << "The TRIANGULATION operation took" << timer.elapsed() << "milliseconds";
    }
}

void MainWindow::on_pointButton_clicked()
{
    scene->EnterPointsZone();
}

void MainWindow::on_rectButton_clicked()
{
    scene->setActiveDrawer(2);
    ui->pointButton->setStyleSheet("");
    ui->circleButton->setStyleSheet("");
    ui->rectButton->setStyleSheet("background:#A9A9A9;");
   /* QPalette pal = ui->rectButton->palette();
    pal.setColor(QPalette::Button, QColor(Qt::gray));
    ui->rectButton->setAutoFillBackground(true);
    ui->rectButton->setPalette(pal);
    ui->rectButton->update(); */
}

void MainWindow::on_pushButton_8_clicked()
{
    scene->fullRecalc();
   // scene->recalcPoints();
}

void MainWindow::on_circleButton_clicked()
{
    scene->setActiveDrawer(3);
    ui->circleButton->setStyleSheet("background:#A9A9A9;");
    ui->pointButton->setStyleSheet("");
    ui->rectButton->setStyleSheet("");
    //bool ok;
    //int n = QInputDialog::getInt(0,tr("Окружность"),tr("Количество точек"),1,1,360,1,&ok);
    //if (ok){
    //    scene->setQualPoints(n);
    //}
     scene->setActiveDrawer(3);
}

void MainWindow::on_femCalc_clicked()
{
    doFEMcalc(false);
}

void MainWindow::on_make_1_button_clicked()
{
    scene->setActiveDrawer(4);
}



void MainWindow::on_showMesh_button_clicked()
{
    doFEMcalc(true);
}

void MainWindow::pointszone_entered()
{
    scene->setActiveDrawer(1);
    if (scene->pointszone->getStatus())
    {
        ui->pointButton->setStyleSheet("background:#0000ff;");
        ui->rectButton->setStyleSheet("");
        ui->circleButton->setStyleSheet("");
    }
    else
    {
        ui->pointButton->setStyleSheet("background:#ff0000;");
        ui->rectButton->setStyleSheet("");
        ui->circleButton->setStyleSheet("");
    }
}


void MainWindow::on_saveButton_clicked()
{
   /* QString fileName = QFileDialog::getSaveFileName(this,tr("Save file"));
    if (fileName.isEmpty())
             return;
         else {
             QFile file(fileName);
             if (!file.open(QIODevice::WriteOnly)) {
                 QMessageBox::information(this, tr("Unable to open file"),
                     file.errorString());
                 return;
             }
             QDataStream out(&file);
                      out.setVersion(QDataStream::Qt_5_10);
                      QVector<QList <QPointF> *> inside,outside,dielectric;
                      inside = scene->getInside();
                      outside = scene->getOutside();
                      dielectric = scene->getDielectric();
                      out<<inside<<outside<<dielectric;
    }*/
}

void MainWindow::on_loadButton_clicked()
{
   /* QString fileName = QFileDialog::getOpenFileName(this,
             tr("Open Address Book"), "",
             tr("Address Book (*.abk);;All Files (*)"));
    if (fileName.isEmpty())
            return;
        else {

            QFile file(fileName);

            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::information(this, tr("Unable to open file"),
                    file.errorString());
                return;
            }
            QDataStream in(&file);
            in.setVersion(QDataStream::Qt_5_10);
            QVector<QList <QPointF> *> inside,outside,dielectric;
            in >> inside>>outside>>dielectric;
            scene->SetInside(inside);
            scene->SetOutside(outside);
            scene->SetDielectric(dielectric);
            scene->drawLoad();
    }*/
}
