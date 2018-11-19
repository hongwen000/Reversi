#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent, size_t row, size_t col) :
    QMainWindow(parent), ui(new Ui::MainWindow), row(row), col(col)
{
    ui->setupUi(this);
    grid = new Grid<ChessBlock>(this, ui->gridLayout_2, row, col);
    resetBoard();
    ui->statusBar->showMessage("状态：6666端口已打开，但未和任何远程玩家连接");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void ChessBlock::setWhite(){white = true;}

void ChessBlock::setBlack(){white = false;}

void ChessBlock::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if(white)
        setPic(WCFN);
    else
        setPic(BCFN);
}
