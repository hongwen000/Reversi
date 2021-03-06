#include "grid.h"

void PaintBlock::setPic(const QString &filen)
{
    pic.load(filen);
    pic_ori = pic;
    update();
}

void PaintBlock::setOverlayPic(const QString& filen)
{
    picover.load(filen);
    picover_ori = picover;
    update();
}

void PaintBlock::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if(pic.isNull())
    {
        QPen(Qt::GlobalColor::white, 10);
        rectangle.setSize({50,50});
        painter.drawRect(rectangle);
    }
    else
    {
        painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
        painter.drawPixmap(rect(), pic);
        if(!picover.isNull())
        {
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawPixmap(rect(), picover);
        }
    }
}

