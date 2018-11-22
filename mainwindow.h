#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "grid.h"
#include <QMouseEvent>
#include <thread>
#include <atomic>
#include <QtNetwork>
#include <QDebug>
#include <QInputDialog>
#include <QLCDNumber>
#include "reversi.h"
using namespace std;

#define ODDFN  ":/img/Even.png"
#define EVENFN  ":/img/Odd.png"
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
    bool isAvailable = false;
    void drawNum(int n);
    int color;
protected:
    void mousePressEvent(QMouseEvent *e);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, size_t row = GAMESCALE,  size_t col = GAMESCALE);
    ~MainWindow();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_comboBox_p2_currentTextChanged(const QString &arg1);

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    size_t row;
    size_t col;
    Grid<ChessBlock>* grid = nullptr;
    std::thread* control_thread = nullptr;
    std::thread* ai_thread = nullptr;
    QUdpSocket* receiver = nullptr;
    atomic_bool exit;
    QTimer *pTimer = nullptr;
    int remoteListenPort = -1;
    void processMove(size_t x, size_t y);
    void test_func()
    {
        qDebug() << "test func";
        grid->setPic(0,0, BCFN);
    }
    int getFirstPlayer();
    int getCurrentPlayerChessColor();
    void control_func();
    void resetBoard();
    void setCurrentPlayer(int player);
    void startGame();
    void drawAvailNum(const array<int, GAMESCALE * GAMESCALE> & avi);
    State toState();
    pair<int, int> fromState(const State& s);
    void processGrid(int x, int y, int color);
signals:
    void remoteChallengeEvent(const QString &);
    void reqRepaint();
private slots:
    void on_reqRepaint();
    void on_remoteChallengeEvent(const QString &str);
};

#endif // MAINWINDOW_H
