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
#include <complex>
#include <QTime>
#include <QStorageInfo>




#define M_PI 3.1416
using namespace arma;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scene = new GraphicScene(ui->centralWidget);
    pointszone = new PointsZone;
    visual = new visualization;
    scene->visual = visual;
    ui->graphicsView->setScene(scene);

 //   ui->graphicsView->setAlignment(Qt::AlignBottom|Qt::AlignLeft);
    ui->graphicsView->setSceneRect(-6000, -3600, 12000, 7200);

    ui->graphicsView->show();
   // this->resize(600, 600);
    Graphics_view_zoom* z = new Graphics_view_zoom(ui->graphicsView);
    z->set_modifiers(Qt::NoModifier);
    matrix_C.set_size(2,2);
    matrix_C.zeros();
 //   connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(on_some_pushButton_clicked()));
    //ui->graphicsView->installEventFilter();
    QObject::connect(scene, SIGNAL(move_scene_sig(bool)), this, SLOT(move_scene_slot(bool)));
    QObject::connect(scene, SIGNAL(send_triangles(int)), this, SLOT(addTringlesData(int)));
    QObject::connect(scene, SIGNAL(mouse_positionChanged(QPointF)), this, SLOT(update_position(QPointF)));
    QObject::connect(scene->pointszone, SIGNAL(zone_entered()),this, SLOT(pointszone_entered()));
    QObject::connect(scene, SIGNAL(RefinementDone (double, double)), this, SLOT(RefinementTRS(double, double)));
    QObject::connect(visual, SIGNAL(plot_redy()), scene, SLOT(potential_line_calc()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::doFEMcalc(bool mode)
{
    visual->clear();
    QTime time;
    time.start();
    mat bcs, nodes, trs, domains;
    double tol;
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
    cout<<domains;
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
        double eps = 1;

        if (!domains.empty())
        {
        if(domains(i,1)==2)
            eps=ui->dielectric_edit->text().toDouble();
        else if (domains(i,1)==3)
            eps=ui->dielectric_edit_2->text().toDouble();
        else
            eps=1;
        }


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
    if (scene->getInsideSize() == 1)
    {
        QVector <mat> V_all;
   mat v;

    v = solve(mat(a),f);
    V_all.push_back(v);
    visual->Set_Data(V_all,trs,nodes, scene->getInsideSize());
  // cout << v;
    double W;
    W=get_tri2d_cap(v, nr_nodes, nr_trs, nodes, trs, domains);
    double C;
    C = 2*W;
    double mu0 = 4.0 * M_PI * pow(10.0, -7);
    int c0=299792456;
    double eps0=1/(mu0*c0*c0);
    double L = (mu0 * eps0)/ C;
    double Z1 = pow((L/C),0.5);
    C = C*1e12;
    L = L*1e6;
    qDebug()<<"trs["<<nr_trs<<"] nodes["<<nr_nodes<<"]";

   /* if(!mode)
              scene->get_tri2d_E(v, nr_nodes, nr_trs, nodes, trs);
          else scene->show_mesh(v, nr_trs, nodes, trs,ui->ReMeshEdit->text().toDouble());*/
    if(!mode){
    v_F = v;
    C_F=C;
    bcs_F = bcs;
    trs_F = trs;
    nodes_F = nodes;
    nr_trs_F=nr_trs;
    nr_nodes_F=nr_nodes;
    domains_F = domains;
    }
    double times;
    times = time.elapsed();


        emit result_text(C_F,nr_trs_F,nr_nodes_F,times, L, Z1);
    }

    if ( scene->getInsideSize() == 2)
    {
        QVector <mat> V_all;

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
        mat v11,v22,v12;
        v11 = solve(mat(a),f1);
        v22 = solve(mat(a),f2);
        v12 = solve(mat(a),f);
        V_all.push_back(v11);
        V_all.push_back(v22);
        V_all.push_back(v12);
        double W11,W12,W22;
       W11 = get_tri2d_cap(v11, nr_nodes, nr_trs, nodes, trs,domains);
       W22 = get_tri2d_cap(v22, nr_nodes, nr_trs, nodes, trs,domains);
       W12 = get_tri2d_cap(v12, nr_nodes, nr_trs, nodes, trs, domains);
       visual->Set_Data(V_all,trs,nodes, scene->getInsideSize());
       double C11, C12, C22;
       C11 = 2 * W11;
       C22 = 2 * W22;
       C12 = W12 - (C11+C22)/2;
       mat C;
       C.set_size(2,2);
       C(0,0) = C11;
       C(0,1) = C12;
       C(1,0) = C12;
       C(1,1) = C22;
        //qDebug()<<"****************************";
       //qDebug()<<"["<<"W11="<<W11<<", "<<"W12="<<W12<<", "<<"W22="<<W22<<"]";
      // qDebug()<<endl;
     //  qDebug()<<C11<<", "<<C12;
    //   qDebug()<<C12<<", "<<C22;
   //    qDebug()<<endl;
  //     cout<<C;
        mat L(2,2,fill::zeros);

       if (!mode){
           //cout << matrix_C;
           del=abs(matrix_C(0,0)-C(0,0))/abs(matrix_C(0,0));
           del1=abs(matrix_C(0,1)-C(0,1))/abs(matrix_C(0,1));
           del2=abs(matrix_C(1,1)-C(1,1))/abs(matrix_C(1,1));
           matrix_C=C;
           v_F = v11;
           bcs_F = bcs;
           trs_F = trs;
           nodes_F = nodes;
           nr_trs_F=nr_trs;
           nr_nodes_F=nr_nodes;


           double times;
           times = time.elapsed();
           emit result_text(C,nr_trs_F,nr_nodes_F, L,times);
       }

    }
    else if (scene->getInsideSize() > 2) {
        QVector <mat> V_all;
        double N;
        mat vv;
        N=scene->getInsideSize();
        mat C(N,N,fill::zeros);
        for (int i=0; i<N; i++)
        {
            mat f_temp;
            f_temp = f;
            for (int j=0; j<f.n_rows; j++){
            if (scene->getMapData(j)!=i)
                f_temp(j,0)=0;
            }
            mat vii;
            vii = solve(mat(a),f_temp);
            V_all.push_back(vii);

            double Wii, Cii;
            Wii = get_tri2d_cap(vii, nr_nodes, nr_trs, nodes, trs, domains);
            Cii = 2 * Wii;
            C(i,i) = Cii;
        }
        //cout<<C;

        for (int i=0; i < N-1; i++)
        {
            for (int j=i+1; j<N; j++)
            {
                mat f_temp;
                f_temp = f;
                for (int k=0; k<f.n_rows; k++){
                if (scene->getMapData(k)!=i && scene->getMapData(k)!=j)
                    f_temp(k,0)=0;
                }
                mat vij;
                vij = solve(mat(a),f_temp);
                vv=vij;
                V_all.push_back(vij);

                double Wij, Cij;
                Wij = get_tri2d_cap(vij, nr_nodes, nr_trs, nodes, trs, domains);
                Cij = Wij - (C(i,i)+C(j,j))/2;
                C(i,j) = Cij;
                C(j,i) = Cij;
            }
        }
        //scene->show_mesh(vv, nr_trs, nodes, trs,ui->ReMeshEdit->text().toDouble(),domains);

        //v_F =vv;
        visual->Set_Data(V_all,trs,nodes, scene->getInsideSize());
        v_F = V_all.at(1);
        nodes_F = nodes;
        trs_F = trs;

        qDebug()<<"trs="<<nr_trs;
        C=C*1e12;
        //C.save("C.txt",arma_ascii);
        cout<<C;
        cout<<"-------------";
    }



qDebug("Time elapsed: %d ms", time.elapsed());
}

mat MainWindow::CmpElMtx_Bandeson(mat xy, double elInx)
{
    double eps = 1;
    if (!scene->domains.empty())
    {
    if(scene->domains.at(elInx).second==2)
        eps=ui->dielectric_edit->text().toDouble();
    else if (scene->domains.at(elInx).second==3)
        eps=ui->dielectric_edit_2->text().toDouble();
    else
        eps=1;
    }

    mat s1,s2,s3;
    s1=xy.col(2)-xy.col(1);
    s2=xy.col(0)-xy.col(2);
    s3=xy.col(1)-xy.col(0);
    double Atot;
    Atot = 0.5 * (s2(0)*s3(1)-s2(1)*s3(0));

    if (Atot < 0)
      qDebug()<<"The nodes of the element given in wrong order";

   // qDebug()<<Atot;
    mat grad_phi1e, grad_phi2e, grad_phi3e, grad_phi;
    grad_phi1e.set_size(2,1);
    grad_phi2e.set_size(2,1);
    grad_phi3e.set_size(2,1);

    grad_phi1e(0)=-s1(1);
    grad_phi1e(1)=s1(0);
    grad_phi1e = grad_phi1e / (2*Atot);

    grad_phi2e(0)=-s2(1);
    grad_phi2e(1)=s2(0);
    grad_phi2e = grad_phi2e / (2*Atot);

    grad_phi3e(0)=-s3(1);
    grad_phi3e(1)=s3(0);
    grad_phi3e = grad_phi3e / (2*Atot);

    grad_phi.set_size(2,3);
    grad_phi.col(0)=grad_phi1e;
    grad_phi.col(1)=grad_phi2e;
    grad_phi.col(2)=grad_phi3e;

    mat Ae,I,J;
    mat T(1,1,fill::zeros);
    mat E(1,1,fill::zeros);
   // E = T * T;
    Ae.set_size(3,3);
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
        {
            I = grad_phi.col(i).t();
            J = grad_phi.col(j);
            T = I * J * Atot*eps;
            Ae(i,j) = T(0);
        }

    return Ae;
}

double MainWindow::get_tri2d_cap(mat v, double nr_nodes, double nr_trs, mat nds, mat trs, mat domains)
{
    qDebug()<<nr_trs;
    qDebug()<<trs.n_rows;
    qDebug()<<domains.n_rows;
    qDebug()<<v.n_rows;
    qDebug()<<nds.n_rows;
    double eps0 = (1.0/(36.0*M_PI))*pow(10.0,-9);
    double mu0 = 4.0 * M_PI * pow(10.0, -7);
    double U = 0.0; double c1,c2,c3,b1,b2,b3;
    double eps = 1;
    //cout << v;
    mat d(3,3, fill::ones);
    for(int i=0; i<nr_trs; i++){
       if (!domains.empty())
       {
        if (domains(i,1)==2)
            eps=ui->dielectric_edit->text().toDouble();
        else if (domains(i,1)==3)
            eps=ui->dielectric_edit_2->text().toDouble();
        else
            eps=1;
        }
       // qDebug()<<eps;
        //qDebug()<<"--------------------------";
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
    qDebug()<<"tut";
    U=U*eps0/2.0;
    double C = 2*U;

    QString text;
    qDebug() << "C = " << C;
    double L = (mu0 * eps0)/ C;
    qDebug() << "L = " << L;
    double Z1 = pow((L/C),0.5);
    qDebug() << "Z1 = " << Z1;
    //qDebug() << scene->getInsideSize();
    text="C = "+QString::number(C);
    ui->textBrowser->setText(text);
    return U;
}

void MainWindow::FEM_Bandeson()
{
    QTime time;
    time.start();
    double mu0 = 4.0 * M_PI * pow(10.0, -7);
   // qDebug()<<mu0;
    int c0=299792456;
    double eps0=1/(mu0*c0*c0);
    mat bcs, nodes, trs, domains;
    nodes.clear();
    nodes.set_size(2,scene->nodes_vec.size());

    for(int i=0; i<scene->nodes_vec.size(); i++){
        nodes(0,i)=scene->nodes_vec.at(i).first;
        nodes(1,i)=scene->nodes_vec.at(i).second;
    }
    trs.clear();
    trs.set_size(3,scene->trs_vec.size()/3);
    for(int i=0; i<scene->trs_vec.size()/3; i++){
        trs(0,i)=scene->trs_vec.at(3*i);
        trs(1,i)=scene->trs_vec.at(3*i+1);
        trs(2,i)=scene->trs_vec.at(3*i+2);
        //qDebug () << scene->trs_vec.at(i) << scene->trs_vec.at(i+1) << scene->trs_vec.at(i+2);
    }
    //cout << trs;
    mat noExt, noInt, no2xy, el2no;
    noExt.clear();
    noInt.clear();
    no2xy.clear();
    el2no.clear();
    no2xy = nodes;
    el2no  = trs;

    if (scene->getInsideSize() == 1)
    {

    QVector<double> TempExt;
    QVector<double> TempInt;
    for (int i =0; i < scene->bcs_vec.size(); i++){
        if (scene->bcs_vec.at(i).second==1.0)
            TempInt.push_back(scene->bcs_vec.at(i).first);
        else
            TempExt.push_back(scene->bcs_vec.at(i).first);
    }
    noInt.set_size(1,TempInt.size());
    for (int i=0; i < TempInt.size(); i++)
    {
        noInt(i) = TempInt.at(i);
    }

    noExt.set_size(1,TempExt.size());
    for (int i=0; i<TempExt.size(); i++)
    {
        noExt(i) = TempExt.at(i);
    }


    double W;
    W = W_Bandeson(noExt,noInt,no2xy,el2no);
    //qDebug()<<eps0;
    double C;
    C = 2*W;
    qDebug()<<C;
    qDebug("Time elapsed: %d ms", time.elapsed());
    double times;
    times = time.elapsed();
    double L = (mu0 * eps0)/ C;
    double Z1 = pow((L/C),0.5);
    C = C*pow(10.0,12);
    L = L*1e6;
        emit result_text(C,nr_trs_F,nr_nodes_F, times, L, Z1);
    }

    else if (scene->getInsideSize() == 2)
    {
        QVector<double> TempExt;
        QVector<double> TempInt;
        QVector<double> TempInt1;
        QVector<double> TempInt2;
        for (int i =0; i < scene->bcs_vec.size(); i++){
            if (scene->bcs_vec.at(i).second==1.0)
            {
                TempInt.push_back(scene->bcs_vec.at(i).first);
                if (scene->getMapData(scene->bcs_vec.at(i).first)==1)
                {
                    TempInt1.push_back(scene->bcs_vec.at(i).first);
                }
                else if (scene->getMapData(scene->bcs_vec.at(i).first)==0)
                {
                    TempInt2.push_back(scene->bcs_vec.at(i).first);
                }
            }
            else
                TempExt.push_back(scene->bcs_vec.at(i).first);
        }

        noInt.set_size(1,TempInt.size());
        for (int i=0; i < TempInt.size(); i++)
        {
            noInt(i) = TempInt.at(i);
        }

        mat noInt1;
        noInt1.set_size(1,TempInt1.size());
        for (int i=0; i < TempInt1.size(); i++)
        {
            noInt1(i) = TempInt1.at(i);
        }


        mat noInt2;
        noInt2.set_size(1,TempInt2.size());
        for (int i=0; i < TempInt2.size(); i++)
        {
            noInt2(i) = TempInt2.at(i);
        }

        noExt.set_size(1,TempExt.size());
        for (int i=0; i<TempExt.size(); i++)
        {
            noExt(i) = TempExt.at(i);
        }

        double W11, W22, W12;
        W11 = W_Bandeson(noExt,noInt1,no2xy,el2no);
        W22 = W_Bandeson(noExt,noInt2,no2xy,el2no);
        W12 = W_Bandeson(noExt,noInt,no2xy,el2no);
        qDebug()<<W11<<W22<<W12;

        double C11, C12, C22;
        C11 = 2 * W11;
        C22 = 2 * W22;
        C12 = W12 - (C11+C22)/2;

        mat C;
        C.set_size(2,2);
        C(0,0) = C11;
        C(0,1) = C12;
        C(1,0) = C12;
        C(1,1) = C22;
        mat C0;
        C0 = inv(C);

        mat L(2,2);
        L = eps0*mu0*C0;

        cout<<L;
        double times;
        times = time.elapsed();
        emit result_text(C,1,1, L,times);

    }

    else
    {
        noInt.clear();
        noExt.clear();
        double N;
        mat vv;
        N=scene->getInsideSize();
        mat C(N,N,fill::zeros);
        for (int i=0; i<N; i++)
        {
            QVector<double> TempExt;
            QVector<double> TempInt;
            for (int j =0; j < scene->bcs_vec.size(); j++)
            {
                if (scene->bcs_vec.at(j).second==1.0)
                {
                    if (scene->getMapData(scene->bcs_vec.at(j).first)==i)
                    {
                        TempInt.push_back(scene->bcs_vec.at(j).first);
                    }
                }
                else
                    TempExt.push_back(scene->bcs_vec.at(j).first);
            }

            noInt.set_size(1,TempInt.size());
            for (int i=0; i < TempInt.size(); i++)
            {
                noInt(i) = TempInt.at(i);
            }

            noExt.set_size(1,TempExt.size());
            for (int i=0; i<TempExt.size(); i++)
            {
                noExt(i) = TempExt.at(i);
            }
            double Wii, Cii;
            Wii = W_Bandeson(noExt,noInt,no2xy,el2no);
            Cii = 2 * Wii;
            C(i,i) = Cii;
        }
        for (int i=0; i < N-1; i++)
                {
                    for (int j=i+1; j<N; j++)
                    {
                        QVector<double> TempExt;
                        QVector<double> TempInt;
                        for (int k =0; k < scene->bcs_vec.size(); k++)
                        {
                            if (scene->bcs_vec.at(k).second==1.0)
                            {
                                if (scene->getMapData(scene->bcs_vec.at(k).first)==i || scene->getMapData(scene->bcs_vec.at(k).first)==j)
                                {
                                    TempInt.push_back(scene->bcs_vec.at(k).first);
                                }
                            }
                            else
                                TempExt.push_back(scene->bcs_vec.at(k).first);
                        }
                        noInt.clear();
                        noExt.clear();

                        noInt.set_size(1,TempInt.size());
                        for (int i=0; i < TempInt.size(); i++)
                        {
                            noInt(i) = TempInt.at(i);
                        }

                        noExt.set_size(1,TempExt.size());
                        for (int i=0; i<TempExt.size(); i++)
                        {
                            noExt(i) = TempExt.at(i);
                        }

                        double Wij, Cij;
                        Wij = W_Bandeson(noExt,noInt,no2xy,el2no);
                        Cij = Wij - (C(i,i)+C(j,j))/2;
                        C(i,j) = Cij;
                        C(j,i) = Cij;
                    }
                }
        C = C*1e12;
        cout<<C;
        double times;
        times = time.elapsed();
        qDebug()<<times;

    }
}

double MainWindow::W_Bandeson(mat noExt,mat noInt,mat no2xy,mat el2no)
{
    double mu0 = 4.0 * M_PI * pow(10.0, -7);
   // qDebug()<<mu0;
    int c0=299792456;
    double eps0=1/(mu0*c0*c0);
    double noNum, elNum;
    noNum = no2xy.n_cols;
    elNum = el2no.n_cols;
   // qDebug()<<elNum;
   // cout<<no2xy;
    no2xy = pow(10.0, -2) * no2xy;
   // cout << no2xy;
    mat A(noNum, noNum, fill::zeros);
    mat b(noNum, 1, fill::zeros);

    for (int elIdx = 0; elIdx < elNum; elIdx++)
    {
        mat no, xy;
        no=el2no.col(elIdx);

        xy.set_size(2,3);
        xy.col(0)=no2xy.col(no(0));
        xy.col(1)=no2xy.col(no(1));
        xy.col(2)=no2xy.col(no(2));

        mat A_el;
       A_el = CmpElMtx_Bandeson(xy,elIdx);



        for (int i=0; i<3; i++)
            for (int j=0; j<3; j++)
            {
                A(no(j),no(i)) = A(no(j),no(i)) +  A_el(i,j);
            }
    }
    //cout<<A;
   // A.save("A.txt",arma_ascii);




    mat no_ess_temp(1,noInt.n_elem+noExt.n_elem);

    for (int i=0; i<noInt.n_elem; i++)
        no_ess_temp(i) = noInt(i);
    for (int i=0; i<noExt.n_elem; i++)
        no_ess_temp(noInt.n_elem+i) = noExt(i);

   // cout<<no_ess_temp;
    mat no_ess;
    no_ess = unique (no_ess_temp);
    no_ess = no_ess.t();
  //  cout<<no_ess;


    mat no_all(1,noNum);
    for (int i=0; i<noNum; i++)
        no_all(i) = i;
    //cout<<no_all;

    mat no_nat;
    no_nat = no_all;
    for (int i=0; i<no_ess.n_elem; i++)
    {
        no_nat(no_ess(i))=0;
    }
    no_nat = nonzeros(no_nat);
    no_nat = no_nat.t();
   // cout<<no_nat;


    mat A_ess(no_nat.n_elem,no_ess.n_elem);
    for (int i = 0; i < no_nat.n_elem; i++)
    {
        for (int j = 0; j < no_ess.n_elem; j++)
        {
            A_ess(i,j) = A(no_nat(i),no_ess(j));
        }
    }
   // cout<<A_ess;

    mat A_nat (no_nat.n_elem,no_nat.n_elem);
    for (int i = 0; i<no_nat.n_elem; i++)
    {
        for(int j = 0; j<no_nat.n_elem; j++)
        {
            A_nat(i,j) = A(no_nat(i),no_nat(j));
        }
    }
    mat B(no_nat.n_elem,1);
    for (int i = 0; i<no_nat.n_elem; i++)
    {
        B(i) = b(no_nat(i));
    }

    mat z(no_all.n_elem,1,fill::zeros);
    for (int i = 0; i<noInt.n_elem; i++)
    {
        z(noInt(i)) = 1;
    }

    mat z_ess(no_ess.n_elem,1);
    for (int i=0; i<no_ess.n_elem; i++)
    {
        z_ess(i) = z(no_ess(i));
    }


    mat z_nat;
    mat temp;
    temp = (B - A_ess * z_ess);
    z_nat = solve(A_nat,temp);
   // cout<<z_nat;

    mat Z(no_all.n_elem,1,fill::zeros);
    for (int i = 0; i<no_ess.n_elem;i++)
    {
        Z(no_ess(i)) = z_ess(i);
    }
    for (int i = 0; i<no_nat.n_elem;i++)
    {
        Z(no_nat(i)) = z_nat(i);
    }

    //cout<<Z;
    mat Zi(1,Z.n_elem);
    for (int i = 0; i<Z.n_elem; i++)
    {
        Zi(i)=Z(i);
    }

    mat temp1;
    temp1 = Zi*A*Z;

    double W;
    W = 0.5 * eps0 * as_scalar(temp1);
    return W;
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
    ui->showE_button->setEnabled(true);
    ui->showMesh_button->setEnabled(true);
}

void MainWindow::on_make_1_button_clicked()
{
    scene->setActiveDrawer(4);
}



void MainWindow::on_showMesh_button_clicked()
{
   scene->show_mesh(v_F, nr_trs_F, nodes_F, trs_F,ui->ReMeshEdit->text().toDouble(),domains_F);
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
    QString fileName = QFileDialog::getSaveFileName(this,tr("Save file"));
    if (fileName.isEmpty())
             return;
         else {
             QFile file(fileName+".str");
             if (!file.open(QIODevice::WriteOnly)) {
                 QMessageBox::information(this, tr("Unable to open file"),
                     file.errorString());
                 return;
             }
             QDataStream out(&file);
                    //  out.setVersion(QDataStream::Qt_5_10);
                      QVector<QList <QPointF> *> inside1,outside1,dielectric1;
                      QVector<QList <QPointF> > inside,outside,dielectric;
                      inside1 = scene->getInside();
                      for(int i=0; i<scene->getInside().size(); i++){
                          QList <QPointF> List;
                          for(int j=0; j<scene->getInside().at(i)->size(); j++){
                              QPointF a = inside1.at(i)->at(j);
                              List.push_back(a);
                          }
                          inside.push_back(List);
                      }
                      outside1 = scene->getOutside();
                      for(int i=0; i<scene->getOutside().size(); i++){
                          QList <QPointF> List;
                          for(int j=0; j<scene->getOutside().at(i)->size(); j++){
                              QPointF a = outside1.at(i)->at(j);
                              List.push_back(a);
                          }
                          outside.push_back(List);
                      }
                      dielectric1 = scene->getDielectric();
                      for(int i=0; i<scene->getDielectric().size(); i++){
                          QList <QPointF> List;
                          for(int j=0; j<scene->getDielectric().at(i)->size(); j++){
                              QPointF a = dielectric1.at(i)->at(j);
                              List.push_back(a);
                          }
                          dielectric.push_back(List);
                      }
                      out<<inside<<outside<<dielectric;
    }
}

void MainWindow::on_loadButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
             tr("Open structure file"), "",
             tr("Structure (*.str);;All Files (*)"));
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
          //  in.setVersion(QDataStream::Qt_5_10);
            QVector<QList <QPointF>> inside,outside,dielectric;
            in >> inside>>outside>>dielectric;
            QVector<QList <QPointF> *> inside1,outside1,dielectric1;

            for(int i=0; i<inside.size(); i++){
                QList <QPointF>* List = new QList<QPointF>;
                for(int j=0; j<inside.at(i).size(); j++){
                    QPointF a = inside.at(i).at(j);
                    List->push_back(a);
                }
                inside1.push_back(List);
            }
            for(int i=0; i<outside.size(); i++){
                QList <QPointF>* List = new QList<QPointF>;
                for(int j=0; j<outside.at(i).size(); j++){
                    QPointF a = outside.at(i).at(j);
                    List->push_back(a);
                }
                outside1.push_back(List);
            }
            for(int i=0; i<dielectric.size(); i++){
                QList <QPointF>* List = new QList<QPointF>;
                for(int j=0; j<dielectric.at(i).size(); j++){
                    QPointF a = dielectric.at(i).at(j);
                    List->push_back(a);
                }
                dielectric1.push_back(List);
            }
            scene->SetInside(inside1);
            scene->SetOutside(outside1);
            scene->SetDielectric(dielectric1);
            scene->drawLoad();
    }
}

void MainWindow::on_ReMesh_clicked()
{
    //if (nr_trs_it<nr_trs_F){
    //    qDebug()<<nr_trs_it<<"  "<<nr_trs_F;
    nr_trs_it = nr_trs_F;
    scene->RefinementMesh(trs_F,v_F,nodes_F, ui->ReMeshEdit->text().toDouble());
    ui->showE_button->setEnabled(false);
    ui->showMesh_button->setEnabled(false);
   // }
   // else {
   // ui->textBrowser->setText("Refinement done");
   // }
}

void MainWindow::RefinementTRS(double nr_trs, double nr_nds)
{
    ui->textBrowser->setText("trs["+QString::number(nr_trs)+"]  nodes["+QString::number(nr_nds)+"]");
}

void MainWindow::on_showE_button_clicked()
{
    scene->get_tri2d_E(v_F, nr_nodes_F, nr_trs_F, nodes_F, trs_F);
}

void MainWindow::result_text(double C_F, double nr_trs_F, double nr_nodes_F, double times,double L, double Z1)
{
    ui->textBrowser->clear();
    ui->textBrowser->setText("C="+QString::number(C_F)+"["+"пФ"+"]");
    ui->textBrowser->append("L="+QString::number(L));
    ui->textBrowser->append("Z1="+QString::number(Z1));
    ui->textBrowser->append("trs["+QString::number(nr_trs_F)+"]  nodes["+QString::number(nr_nodes_F)+"]");
    ui->textBrowser->append("time=" + QString::number(times));
}

void MainWindow::result_text(mat matrix_C, double nr_trs_F, double nr_nodes_F, mat L, double times)
{
    ui->textBrowser->clear();
    ui->textBrowser->setText("C=|"+QString::number(matrix_C(0,0)* 1e12)+"  "+QString::number(matrix_C(0,1)* 1e12)+"|" +"   "+ "["+"пФ"+"]");
    ui->textBrowser->append("    |"+QString::number(matrix_C(0,1)* 1e12)+"  "+QString::number(matrix_C(1,1)* 1e12)+"|");
    ui->textBrowser->append("trs["+QString::number(nr_trs_F)+"]  nodes["+QString::number(nr_nodes_F)+"]");
    //ui->textBrowser->append("del11["+QString::number(del)+"]  del12["+QString::number(del1)+"]"+"]  del22["+QString::number(del2)+"]");
    ui->textBrowser->append("L=|"+QString::number(L(0,0)* 1e9)+"  "+QString::number(L(0,1)* 1e9)+"|");
    ui->textBrowser->append("    |"+QString::number(L(0,1)* 1e9)+"  "+QString::number(L(1,1)* 1e9)+"|");
    ui->textBrowser->append("time=" + QString::number(times));
}

void MainWindow::on_pushButton_5_clicked()
{
    //scene->potential_line_calc(v_F,nodes_F,trs_F);
}

void MainWindow::on_Rect_Mesh_button_clicked()
{
    scene->RectMesh2(trs_F,nodes_F,v_F);
}

void MainWindow::on_FemBandesonButton_clicked()
{
    FEM_Bandeson();
}

void MainWindow::on_feild_but_clicked()
{
    visual->show();

}
