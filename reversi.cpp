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

    auto iid = getXY(id);
    int x = iid.first;
    int y = iid.second;
//    auto [x, y] = getXY(id);
    for(int d = 0; d < 8; ++d)
    {
        int i = x;
        int j = y;
        int fx = -1;
        int fy = -1;
//        auto dd = checkDirection[d];
        auto dx = checkDirection[d][0];
        auto dy = checkDirection[d][1];
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
                    ret[getID(i,j)] = (uint8_t)color;
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
        if(s[id] != EMPTY)
        {
            ret[id] = 0;
            continue;
        }
        auto iid = getXY(id);
        int x = iid.first;
        int y = iid.second;
//        auto [x, y] = getXY(id);
        for(int d = 0; d < 8; ++d)
        {
            int this_d = 0;
//            auto [dx, dy] = checkDirection[d];
            auto dx = checkDirection[d][0];
            auto dy = checkDirection[d][1];
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

static const int values[8][8] =
{{ 30, -25, 10, 5, 5, 10, -25,  30,},
{-25, -25,  1, 1, 1,  1, -25, -25,},
{ 10,   1,  5, 2, 2,  5,   1,  10,},
{  5,   1,  2, 1, 1,  2,   1,   5,},
{  5,   1,  2, 1, 1,  2,   1,   5,},
{ 10,   1,  5, 2, 2,  5,   1,  10,},
{-25, -25,  1, 1, 1,  1, -25, -25,},
{ 30, -25, 10, 5, 5, 10, -25,  30,},};

static const int * pvalues = ((const int *)values);


int positionalValue(const State& s, int color)
{
    if(GAMESCALE != 8) throw;
    int ret = 0;
    auto avi = getAvail(s, color);
    int mobility = 0;
    for(const auto& i : avi) if(i)mobility++;
    for(int i = 0; i < 64; ++i)
    {
        if(s[i] == EMPTY) continue;
        else if(s[i] == BLACK) ret += *(pvalues + i);
        else ret -= *(pvalues + i);
    }
    return (ret + mobility * (color == BLACK? 10 : -10));
}

int MinMax(const State& s, int color, int limit, const valueFunc& V, int & bestMove)
{
    if(limit == 0) return V(s, color);
    auto avil = getAvail(s, color);
    bool isTerminal = true;
    int bestValue = 0;
    int lbestMove;
    bool isFirstTime = true;
    for(size_t i = 0; i < avil.size(); ++i)
    {
        if(avil[i] == 0) continue;
        isTerminal = false;
        auto ns = s;
        ns[i] = (uint8_t)color;
        int nextColor = (color == BLACK) ? WHITE : BLACK;
        auto ret = MinMax(ns, nextColor, limit - 1, V, lbestMove);
        if(limit == 7) qDebug() << ret;
        if((color == BLACK && ret > bestValue) || (color == WHITE && ret < bestValue) || isFirstTime)
        {
            bestValue = ret;
            bestMove = i;
        }
        isFirstTime = false;
    }
    if(isTerminal) return V(s, color);
    return bestValue;
}

int AlphaBeta(const State& s, int color, int limit, const valueFunc& V, int alpha, int beta, int & bestMove)
{
    if(limit == 0) return V(s, color);
    auto avil = getAvail(s, color);
    bool isTerminal = true;
    int lbestMove;
    if(color == BLACK)
    {
        for(size_t i = 0; i < avil.size(); ++i)
        {
            if(avil[i] == 0) continue;
            isTerminal = false;
            auto ns = s;
            ns[i] = (uint8_t)color;
            auto new_alpha = AlphaBeta(ns, !color, limit - 1, V, alpha, beta, lbestMove);
            if(new_alpha > alpha)
            {
                alpha = new_alpha;
                bestMove = i;
            }
            if(beta <= alpha)
                break;
        }
        if(isTerminal) return V(s, color);
        else return alpha;
    }
    else
    {
        for(size_t i = 0; i < avil.size(); ++i)
        {
            if(avil[i] == 0) continue;
            isTerminal = false;
            auto ns = s;
            ns[i] = (uint8_t)color;
            auto new_beta = AlphaBeta(ns, !color, limit - 1, V, alpha, beta, lbestMove);
            if(new_beta < beta)
            {
                beta = new_beta;
                bestMove = i;
            }
            if(beta <= alpha)
                break;
        }
        if(isTerminal) return V(s, color);
        else return beta;
    }
}
