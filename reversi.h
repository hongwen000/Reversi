#ifndef REVERSI_H
#define REVERSI_H
#include <cstdint>
#include <array>
using namespace std;
#define GAMESCALE 8
#include <utility>
enum {EMPTY = 0, BLACK, WHITE};

using State = std::array<uint8_t, GAMESCALE * GAMESCALE>;

extern const int checkDirection[8][2];
bool inBoundary(int x, int y);
pair<int,int> getXY(int id);
int getID(int x, int y);
State getNextState(const State& s, int id, int color);
std::array<int, GAMESCALE * GAMESCALE> getAvail(const State& s, int color);

#endif // REVERSI_H
