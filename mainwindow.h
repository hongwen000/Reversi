#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "grid.h"
#include <QMouseEvent>

#define ODDFN  ":/img/Odd.png"
#define EVENFN  ":/img/Even.png"
#define WCFN  ":/img/white_chess.png"
#define BCFN  ":/img/black_chess.png"

namespace Ui {
class MainWindow;
}
class ChessBlock: public PaintBlock
{
public:
    ChessBlock(QWidget* parent): PaintBlock(parent){}
    void setWhite();
    void setBlack();
protected:
    void mousePressEvent(QMouseEvent *e);
    bool white;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, size_t row = 6,  size_t col = 6);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Grid<ChessBlock>* grid;
    size_t row;
    size_t col;
    void resetBoard()
    {
        for(size_t i = 0; i < row; ++i)
        {
            for(size_t j = 0; j < col; ++j)
            {
                bool ii = i % 2;
                bool jj = j % 2;
                if(ii && jj)
                    grid->setPic(i, j, ODDFN);
                else if(ii && !jj)
                    grid->setPic(i, j, EVENFN);
                else if (!ii && jj)
                    grid->setPic(i, j, EVENFN);
                else
                    grid->setPic(i, j, ODDFN);
            }
        }
    }
};

#endif // MAINWINDOW_H
