#include "reversi.h"
#include <QDebug>

const int checkDirection[8][2] = {
    {-1, 0},
    {-1, 1},
    { 0, 1},
    { 1, 1},
    { 1, 0},
    {1, -1},
    {0, -1},
    {-1,-1},
};
bool inBoundary(int x, int y)
{
    return (x >= 0) && (x < GAMESCALE) && (y >= 0) && (y < GAMESCALE);
}

pair<int, int> getXY(int id)
{
    return {id / GAMESCALE, id % GAMESCALE};
}

int getID(int x, int y)
{
    return x * GAMESCALE + y;
}

State getNextState(const State &s, int id, int color)
{
    auto ret = s;
    auto [x, y] = getXY(id);
    for(int d = 0; d < 8; ++d)
    {
        int i = x;
        int j = y;
        qDebug() << "Click on " << i << " " << j;
        int fx = -1;
        int fy = -1;
        auto [dx, dy] = checkDirection[d];
        while(inBoundary(i += dx, j += dy))
        {
            if(color == s[getID(i,j)])
            {
                fx = i;
                fy = j;
//                qDebug() << "fx, fy: " << fx << " " << fy;
            }
            if(s[getID(i,j)] == EMPTY)
            {
                break;
            }
        }
        if(fx != -1 && fy != -1)
        {
            i = x;
            j = y;
            while(inBoundary(i += dx, j += dy))
            {
                if(i == fx && j == fy)
                    break;
                if(s[getID(i,j)] != EMPTY)
                {
//                    qDebug() << "fx, fy: " << fx << " " << fy;
//                    qDebug() << "set i, j " << i << " " << j;
                    ret[getID(i,j)] = color;
                }
            }
        }
    }
    return ret;
}

bool sameColorChess(int c1, int c2)
{
    if(c1 == EMPTY || c2 == EMPTY)
        return false;
    if(c1 == c2)
        return true;
    return false;
}

bool diffColorChess(int c1, int c2)
{
    if(c1 == EMPTY || c2 == EMPTY)
        return false;
    if(c1 != c2)
        return true;
    return false;
}
std::array<int, GAMESCALE * GAMESCALE> getAvail(const State& s, int color)
{
    std::array<int, GAMESCALE * GAMESCALE> ret;
    for(auto & i: ret) i = 0;
    for(int id = 0; id < GAMESCALE * GAMESCALE; ++id)
    {
        if(s[id] != EMPTY) ret[id] = 0;
        auto [x, y] = getXY(id);
        for(int d = 0; d < 8; ++d)
        {
            int this_d = 0;
            auto [dx, dy] = checkDirection[d];
            int x2 = x + dx;
            int y2 = y + dy;
            bool flag = false;
            auto id2 = getID(x2, y2);
            if(inBoundary(x2, y2) && diffColorChess(color, s[id2]))
            {
                this_d += 1;
                while(inBoundary(x2 += dx, y2 += dy))
                {
                    id2 = getID(x2, y2);
                    if(sameColorChess(color, s[id2]))
                    {
                        flag = true;
                        break;
                    }
                    else if(diffColorChess(color, s[id2]))
                    {
                        this_d++;
                    }
                    else if(s[id2] == EMPTY)
                    {
                        break;
                    }
                }
                if(!flag) this_d = 0;
            }
            ret[id] += this_d;
        }
    }
    return ret;
}
