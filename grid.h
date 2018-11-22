#ifndef GRID_H
#define GRID_H
#include <QGridLayout>
#include <vector>
#include <QWidget>
#include <QPainter>
#include <QPen>
#include <Qt>
using namespace std;

class PaintBlock: public QWidget
{
    Q_OBJECT
public:
    explicit PaintBlock(QWidget * parent = nullptr): QWidget(parent), pic(":/img/white.png"){}
    void setPic(const QString& filen);
    void setOverlayPic(const QString& filen);
    int id;
protected:
    QPixmap pic;
    QPixmap picover;
    QPixmap pic_ori;
    QPixmap picover_ori;
    QString picfile;
    QRect rectangle;
protected:
    void paintEvent(QPaintEvent *);
};

template <typename T>
class Grid
{
private:
    QWidget* parent;
    QGridLayout* layout;
public:
    Grid(QWidget* parent, QGridLayout* layout, size_t row, size_t col)
        :parent(parent), layout(layout), row(row), col(col)
    {
        if(!layout)
            throw(std::runtime_error("layout is nullptr"));
        layout->setSpacing(0);
        reDraw();
    }
    void setPic(size_t row, size_t col, const QString& filen)
    {
        if(row > pBlocks.size())
            throw(runtime_error("setPic: row > pBlocks.size()"));
        if(col > pBlocks[row].size())
            throw(runtime_error("setPic: col > pBlocks[col].size()"));
        pBlocks[row][col]->setPic(filen);
    }
    void setOverlayPic(size_t row, size_t col, const QString& filen)
    {
        if(row > pBlocks.size())
            throw(runtime_error("setPic: row > pBlocks.size()"));
        if(col > pBlocks[row].size())
            throw(runtime_error("setPic: col > pBlocks[col].size()"));
        pBlocks[row][col]->setOverlayPic(filen);
    }
    vector<vector<T*>> pBlocks;
    size_t row;
    size_t col;
    void reDraw()
    {
        for(auto& v: pBlocks)
        {
            for(auto& p: v)
            {
                if(p)
                {
                    layout->removeWidget(p);
                    if(p) delete p;
                }
            }
        }
        pBlocks.clear();
        for(size_t i = 0; i < row; ++i)
        {
            vector<T*> vBlocks;
            for(size_t j = 0; j < col; ++j)
            {
                auto pBlock = new T(parent);
                pBlock->id = i * row + j;
                vBlocks.push_back(pBlock);
                layout->addWidget(pBlock, i, j);
            }
            pBlocks.push_back(vBlocks);
        }
    }
};

#endif // GRID_H
