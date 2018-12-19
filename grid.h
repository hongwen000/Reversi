#ifndef GRID_H
#define GRID_H
#include <QGridLayout>
#include <vector>
#include <QLabel>
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
    vector<QLabel*> pLabels;
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
        for(auto& l: pLabels)
        {
            if(l)
            {
                layout->removeWidget(l);
                if(l) delete l;
            }
        }
        pLabels.clear();
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
        for(size_t oi = 0; oi < row + 1; ++oi)
        {
            vector<T*> vBlocks;
            for(size_t oj = 0; oj < col + 1; ++oj)
            {
                if(oi == 0 && oj == 0)
                {
                    continue;
                }
                else if(oi == 0)
                {
#ifdef USE_LETTER_LABEL
                    pLabels.push_back(new QLabel(QString((int)(oj + 'A' - 1)),parent));
#else
                    pLabels.push_back(new QLabel(QString::number(oj-1)));
#endif
                    layout->addWidget(pLabels.back(),oi, oj);
                }
                else if(oj == 0)
                {
                    pLabels.push_back(new QLabel(QString::number(oi-1),parent));
                    layout->addWidget(pLabels.back(),oi, oj);
                }
                else
                {
                    int i = oi - 1;
                    int j = oj - 1;
                    auto pBlock = new T(parent);
                    pBlock->id = i * row + j;
                    vBlocks.push_back(pBlock);
                    layout->addWidget(pBlock, oi, oj);
                }
            }
            if(!vBlocks.empty())
                pBlocks.push_back(vBlocks);
        }
//        for(size_t i = 0; i < row; ++i)
//        {
//            vector<T*> vBlocks;
//            for(size_t j = 0; j < col; ++j)
//            {
//                auto pBlock = new T(parent);
//                pBlock->id = i * row + j;
//                vBlocks.push_back(pBlock);
//                layout->addWidget(pBlock, i, j);
//            }
//            pBlocks.push_back(vBlocks);
//        }
    }
};

#endif // GRID_H
