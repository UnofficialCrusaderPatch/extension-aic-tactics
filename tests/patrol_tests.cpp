#include <cassert>
#include "aic_tactics/patrol_geometry.hpp"

using namespace AicTactics;

static TilePoint at(int x, int y)
{
    TilePoint point;
    point.x = x;
    point.y = y;
    return point;
}

static bool same(TilePoint a, int x, int y) { return a.x == x && a.y == y; }

static void octants()
{
    assert(patrolOctant(0, 0) == -1);
    assert(patrolOctant(10, 0) == 0 && patrolOctant(10, 4) == 0 && patrolOctant(10, 5) == 1);
    assert(patrolOctant(0, 10) == 2 && patrolOctant(-10, 10) == 3 && patrolOctant(-10, 0) == 4);
    assert(patrolOctant(-7, -7) == 5 && patrolOctant(0, -10) == 6 && patrolOctant(7, -7) == 7);
    assert(patrolOctant(4, 10) == 2 && patrolOctant(5, 10) == 1);
    int ux = 9, uy = 9;
    patrolOctantVector(8, ux, uy);
    assert(ux == 0 && uy == 0);
    patrolOctantVector(3, ux, uy);
    assert(ux == -1 && uy == 1);
    assert(patrolDistance(at(1, 1), at(4, -2)) == 3);
}

static void wing()
{
    TilePoint out[2];
    // Target east of the first group: stations to the north and south.
    assert(wingPatrolStations(at(100, 100), at(150, 100), at(98, 110), 12, out) == 2);
    assert(same(out[0], 100, 112) && same(out[1], 100, 88));
    // Equal distance keeps the left (north) station first.
    assert(wingPatrolStations(at(100, 100), at(150, 100), at(100, 100), 12, out) == 2);
    assert(same(out[0], 100, 88) && same(out[1], 100, 112));
    // Diagonal approach uses diagonal flanks.
    assert(wingPatrolStations(at(100, 100), at(150, 150), at(90, 90), 12, out) == 2);
    assert(same(out[0], 109, 91) && same(out[1], 91, 109));
    assert(wingPatrolStations(at(100, 100), at(150, 100), at(1, 1), 0, out) == 0);
    assert(wingPatrolStations(at(100, 100), at(100, 100), at(1, 1), 12, out) == 0);
}

static void mirror()
{
    TilePoint out[2];
    // Home south of the keep: reflect across the vertical axis.
    assert(mirrorPatrolStations(at(170, 190), at(200, 200), at(200, 300), out) == 2);
    assert(same(out[0], 230, 190) && same(out[1], 230, 210));
    // Home east: horizontal axis.
    assert(mirrorPatrolStations(at(170, 190), at(200, 200), at(300, 200), out) == 2);
    assert(same(out[0], 170, 210));
    // Home south-east: diagonal y = x.
    assert(mirrorPatrolStations(at(170, 190), at(200, 200), at(300, 300), out) == 2);
    assert(same(out[0], 190, 170));
    // Home south-west: diagonal y = -x.
    assert(mirrorPatrolStations(at(170, 190), at(200, 200), at(100, 300), out) == 2);
    assert(same(out[0], 210, 230));
    // Slightly off the home axis: the reflection would sit beside the first
    // group, so the keep's flanks are used.
    assert(mirrorPatrolStations(at(203, 170), at(200, 200), at(200, 300), out) == 2);
    assert(same(out[0], 230, 203) && same(out[1], 170, 197));
    // Approaching along the axis: use both flanks of the keep.
    assert(mirrorPatrolStations(at(200, 170), at(200, 200), at(200, 300), out) == 2);
    assert(same(out[0], 230, 200) && same(out[1], 170, 200));
    // Too close to the keep, or no home direction.
    assert(mirrorPatrolStations(at(202, 201), at(200, 200), at(200, 300), out) == 0);
    assert(mirrorPatrolStations(at(170, 190), at(200, 200), at(200, 200), out) == 0);
    // Still marching from far away: no mirror yet.
    assert(mirrorPatrolStations(at(151, 200), at(200, 200), at(200, 300), out) == 0);
    assert(mirrorPatrolStations(at(152, 200), at(200, 200), at(200, 300), out) == 2);
}

static void rings()
{
    int ux = 0, uy = 0;
    patrolRingVector(0, ux, uy);
    assert(ux == 1024 && uy == 0);
    patrolRingVector(4, ux, uy);
    assert(ux == 0 && uy == 1024);
    patrolRingVector(-2, ux, uy);
    assert(ux == 724 && uy == -724);
    patrolRingVector(18, ux, uy);
    assert(ux == 724 && uy == 724);
    // Every vector has length 1024 within rounding.
    for (int direction = 0; direction < PatrolRingDirections; ++direction) {
        patrolRingVector(direction, ux, uy);
        const int length = ux * ux + uy * uy;
        assert(length > 1023 * 1023 && length < 1025 * 1025);
    }
    // Rounding is half away from zero and symmetric.
    assert(same(patrolRingStep(at(100, 100), 2, 17), 112, 112));
    assert(same(patrolRingStep(at(100, 100), 10, 17), 88, 88));
    assert(same(patrolRingStep(at(100, 100), 1, 13), 112, 105));
    assert(same(patrolRingStep(at(100, 100), 9, 13), 88, 95));
    assert(same(patrolRingStep(at(100, 100), 5, 13), 95, 112));
    assert(patrolRingDirection(0, 0) == -1);
    assert(patrolRingDirection(10, 0) == 0 && patrolRingDirection(0, 200) == 4);
    assert(patrolRingDirection(-10, -10) == 10 && patrolRingDirection(10, -4) == 15);
    for (int direction = 0; direction < PatrolRingDirections; ++direction) {
        patrolRingVector(direction, ux, uy);
        assert(patrolRingDirection(ux, uy) == direction);
    }
    assert(patrolRingWrap(-1) == 15 && patrolRingWrap(16) == 0 && patrolRingWrap(-17) == 15);
}

static void routes()
{
    int point = 15, step = 1;
    nextRoutePoint(ClockwiseRoute, point, step);
    assert(point == 0 && step == 1);
    step = -1;
    nextRoutePoint(CounterclockwiseRoute, point, step);
    assert(point == 15 && step == -1);
    // A sweep turns at either end of its five positions.
    point = 3;
    step = 1;
    nextRoutePoint(FrontSweepRoute, point, step);
    assert(point == 4 && step == 1);
    nextRoutePoint(FrontSweepRoute, point, step);
    assert(point == 3 && step == -1);
    point = 0;
    nextRoutePoint(FrontSweepRoute, point, step);
    assert(point == 1 && step == 1);
    assert(frontSweepDirection(4, 0) == 2 && frontSweepDirection(4, 4) == 6);
    assert(frontSweepDirection(0, 0) == 14 && frontSweepDirection(15, 4) == 1);
}

static void screens()
{
    TilePoint out = at(0, 0);
    // Four tiles from the engine toward the keep.
    assert(siegeScreenStation(at(100, 140), at(100, 100), 4, out) && same(out, 100, 136));
    // Diagonal steps are shortened like the wing gap.
    assert(siegeScreenStation(at(140, 140), at(100, 100), 7, out) && same(out, 135, 135));
    // Never more than half way to the keep.
    assert(siegeScreenStation(at(100, 106), at(100, 100), 10, out) && same(out, 100, 103));
    assert(!siegeScreenStation(at(100, 101), at(100, 100), 4, out));
    assert(!siegeScreenStation(at(100, 140), at(100, 100), 0, out));
    assert(!siegeScreenStation(at(100, 100), at(100, 100), 4, out));
}

int main()
{
    octants();
    wing();
    mirror();
    rings();
    routes();
    screens();
    return 0;
}
