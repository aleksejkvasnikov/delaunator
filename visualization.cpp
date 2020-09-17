#include "visualization.h"
#include "ui_visualization.h"

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
