#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "grid.h"
#include <QMouseEvent>
#include <thread>
#include <atomic>
#include <QtNetwork>
#include <QDebug>
using namespace std;

#define ODDFN  ":/img/Odd.png"
#define EVENFN  ":/img/Even.png"
#define WCFN  ":/img/white_chess.png"
#define BCFN  ":/img/black_chess.png"
//extern int playerListenPort[2];
//extern int currentPlayer;
enum{PLAYER1, PLAYER2};
//extern int playerType[2];
enum{HUMAN, COMPUTER, REMOTE};

namespace Ui {
class MainWindow;
}
class ChessBlock: public PaintBlock
{
public:
    ChessBlock(QWidget* parent): PaintBlock(parent){}
    bool isOccupied = false;
protected:
    void mousePressEvent(QMouseEvent *e);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, size_t row = 6,  size_t col = 6);
    ~MainWindow();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::MainWindow *ui;
    size_t row;
    size_t col;
    Grid<ChessBlock>* grid;
    std::thread* control_thread;
    QUdpSocket* receiver;
    atomic_bool exit;
    QTimer *pTimer = nullptr;
    void processMove(size_t this_row, size_t this_col);
    void test_func()
    {
        qDebug() << "test func";
        grid->setPic(0,0, BCFN);
    }
    void control_func();
    void resetBoard();
    void setCurrentPlayer(int player);
};

#endif // MAINWINDOW_H
