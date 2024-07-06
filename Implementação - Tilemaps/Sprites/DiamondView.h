#ifndef DiamondView_h
#define DiamondView_h

#include "TilemapView.h"
#include <iostream>
using namespace std;

class DiamondView : public TilemapView {
public:
    void computeDrawPosition(const int col, const int row, const float tw, const float th, float& targetx, float& targety) const {
        targetx = col * (tw / 2.0f) + row * (tw / 2.0f);
        targety = col * (th / 2.0f) - row * (th / 2.0f);
    }

    void computeMouseMap(int& col, int& row, const float tw, const float th, const float mx, const float my) const {
        float tw2 = tw / 2.0f;
        float th2 = th / 2.0f;

        col = static_cast<int>(floor((mx / tw2 + my / th2) / 2));
        row = static_cast<int>(floor((my / th2 - mx / tw2) / 2));

        row = -row;
    }

    void computeTileWalking(int& col, int& row, const int direction) const {
        switch (direction) {
        case DIRECTION_NORTH:
            row += 2;
            break;
        case DIRECTION_SOUTH:
            row -= 2;
            break;
        case DIRECTION_EAST:
            col++;
            row--;
            break;
        case DIRECTION_WEST:
            col--;
            row++;
            break;
        case DIRECTION_NORTHEAST:
            col++;
            break;
        case DIRECTION_SOUTHEAST:
            row--;
            break;
        case DIRECTION_SOUTHWEST:
            col--;
            break;
        case DIRECTION_NORTHWEST:
            row++;
            break;
        }
    }

    bool pointInDiamond(float px, float py, float x, float y, float tw, float th) const {
        float ax = x;
        float ay = y + th / 2;
        float bx = x + tw / 2;
        float by = y + th;
        float cx = x + tw;
        float cy = y + th / 2;
        float dx = x + tw / 2;
        float dy = y;

        return pointInTriangle(px, py, ax, ay, bx, by, dx, dy) || pointInTriangle(px, py, bx, by, cx, cy, dx, dy);
    }

    bool pointInTriangle(float px, float py, float ax, float ay, float bx, float by, float cx, float cy) const {
        float d1 = (px - bx) * (ay - by) - (ax - bx) * (py - by);
        float d2 = (px - cx) * (by - cy) - (bx - cx) * (py - cy);
        float d3 = (px - ax) * (cy - ay) - (cx - ax) * (py - ay);

        bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

        return !(has_neg && has_pos);
    }
};

#endif
