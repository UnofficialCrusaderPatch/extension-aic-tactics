#include "aic_tactics/patrol_geometry.hpp"

namespace AicTactics {

namespace {
int magnitude(int value) { return value < 0 ? -value : value; }
const int octantX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const int octantY[8] = {0, 1, 1, 1, 0, -1, -1, -1};

TilePoint point(int x, int y)
{
    TilePoint result;
    result.x = x;
    result.y = y;
    return result;
}
} // namespace

int patrolOctant(int dx, int dy)
{
    if (dx == 0 && dy == 0) return -1;
    const int ax = magnitude(dx), ay = magnitude(dy);
    // tan(22.5 degrees) is about 0.414; 2/5 keeps the test integral.
    if (5 * ay <= 2 * ax) return dx > 0 ? 0 : 4;
    if (5 * ax <= 2 * ay) return dy > 0 ? 2 : 6;
    if (dx > 0) return dy > 0 ? 1 : 7;
    return dy > 0 ? 3 : 5;
}

void patrolOctantVector(int octant, int& ux, int& uy)
{
    if (octant < 0 || octant > 7) { ux = 0; uy = 0; return; }
    ux = octantX[octant];
    uy = octantY[octant];
}

int patrolDistance(TilePoint a, TilePoint b)
{
    const int dx = magnitude(a.x - b.x), dy = magnitude(a.y - b.y);
    return dx > dy ? dx : dy;
}

int wingPatrolStations(TilePoint first, TilePoint keep, TilePoint second, int spacing, TilePoint out[2])
{
    if (spacing <= 0) return 0;
    const int direction = patrolOctant(keep.x - first.x, keep.y - first.y);
    if (direction < 0) return 0;
    int lx, ly, rx, ry;
    patrolOctantVector((direction + 6) % 8, lx, ly);
    patrolOctantVector((direction + 2) % 8, rx, ry);
    // A diagonal step covers about 1.4 tiles; keep the gap close to `spacing`.
    if (direction % 2) spacing = (spacing * 5 + 3) / 7;
    const TilePoint left = point(first.x + lx * spacing, first.y + ly * spacing);
    const TilePoint right = point(first.x + rx * spacing, first.y + ry * spacing);
    const bool rightFirst = patrolDistance(right, second) < patrolDistance(left, second);
    out[0] = rightFirst ? right : left;
    out[1] = rightFirst ? left : right;
    return 2;
}

int mirrorPatrolStations(TilePoint first, TilePoint keep, TilePoint home, TilePoint out[2])
{
    const int distance = patrolDistance(first, keep);
    if (distance < 4 || distance > MirrorPatrolRadius) return 0;
    const int axis = patrolOctant(home.x - keep.x, home.y - keep.y);
    if (axis < 0) return 0;
    const int vx = first.x - keep.x, vy = first.y - keep.y;
    int mx, my;
    switch (axis % 4) {
    case 0: mx = vx; my = -vy; break;   // horizontal axis
    case 2: mx = -vx; my = vy; break;   // vertical axis
    case 1: mx = vy; my = vx; break;    // diagonal y = x
    default: mx = -vy; my = -vx; break; // diagonal y = -x
    }
    // Near the home axis the reflection lands beside the first group (twice
    // its sideways offset away). Use the keep's flanks instead.
    if (3 * patrolDistance(point(mx, my), point(vx, vy)) < 2 * distance) {
        out[0] = point(keep.x - vy, keep.y + vx);
        out[1] = point(keep.x + vy, keep.y - vx);
        return 2;
    }
    out[0] = point(keep.x + mx, keep.y + my);
    out[1] = point(keep.x - vx, keep.y - vy);
    return 2;
}

namespace {
// 1024 * (cos, sin) of direction * 22.5 degrees, rounded.
const int ringX[PatrolRingDirections] = {1024, 946, 724, 392, 0, -392, -724, -946,
    -1024, -946, -724, -392, 0, 392, 724, 946};
const int ringY[PatrolRingDirections] = {0, 392, 724, 946, 1024, 946, 724, 392,
    0, -392, -724, -946, -1024, -946, -724, -392};

int scaled(int unit, int step)
{
    const int product = unit * step;
    return (product + (product >= 0 ? 512 : -512)) / 1024;
}
} // namespace

int patrolRingWrap(int direction)
{
    direction %= PatrolRingDirections;
    return direction < 0 ? direction + PatrolRingDirections : direction;
}

void patrolRingVector(int direction, int& ux, int& uy)
{
    direction = patrolRingWrap(direction);
    ux = ringX[direction];
    uy = ringY[direction];
}

TilePoint patrolRingStep(TilePoint origin, int direction, int step)
{
    int ux, uy;
    patrolRingVector(direction, ux, uy);
    return point(origin.x + scaled(ux, step), origin.y + scaled(uy, step));
}

int patrolRingDirection(int dx, int dy)
{
    if (dx == 0 && dy == 0) return -1;
    int best = 0, bestDot = dx * ringX[0] + dy * ringY[0];
    for (int direction = 1; direction < PatrolRingDirections; ++direction) {
        const int dot = dx * ringX[direction] + dy * ringY[direction];
        if (dot > bestDot) { best = direction; bestDot = dot; }
    }
    return best;
}

int frontSweepDirection(int front, int position)
{
    return patrolRingWrap(front - FrontSweepSpan + position);
}

void nextRoutePoint(int kind, int& point, int& step)
{
    if (kind != FrontSweepRoute) {
        point = patrolRingWrap(point + step);
        return;
    }
    if (point + step < 0 || point + step >= FrontSweepPositions) step = -step;
    point += step;
}

bool siegeScreenStation(TilePoint engine, TilePoint keep, int forward, TilePoint& out)
{
    const int distance = patrolDistance(engine, keep);
    const int direction = patrolOctant(keep.x - engine.x, keep.y - engine.y);
    if (forward <= 0 || distance < 2 || direction < 0) return false;
    if (forward > distance / 2) forward = distance / 2;
    int ux, uy;
    patrolOctantVector(direction, ux, uy);
    if (direction % 2) forward = (forward * 5 + 3) / 7;
    if (forward <= 0) return false;
    out = point(engine.x + ux * forward, engine.y + uy * forward);
    return true;
}

} // namespace AicTactics
