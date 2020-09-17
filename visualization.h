#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <QDialog>

namespace Ui {
class visualization;
}

class visualization : public QDialog
{
    Q_OBJECT

public:
    explicit visualization(QWidget *parent = nullptr);
    ~visualization();

private:
    Ui::visualization *ui;
};

#endif // VISUALIZATION_H
