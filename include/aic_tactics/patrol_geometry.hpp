#ifndef AIC_TACTICS_PATROL_GEOMETRY_HPP
#define AIC_TACTICS_PATROL_GEOMETRY_HPP

namespace AicTactics {

// Integer-only station geometry for attack patrol and siege-defence groups.
// Directions are quantized to compass octants or ring directions so that every
// peer computes the same tiles without floating point or trigonometry.
struct TilePoint { int x; int y; };

enum PatrolFormation {
    NativePatrolFormation = 0, WingPatrolFormation = 1, MirrorPatrolFormation = 2, RoutesPatrolFormation = 3
};

// Routes: group g (slot 15 is group 1) takes route (g - 1) % 3; each later
// round of three follows the same routes RouteLag ring points behind.
enum PatrolRouteKind { ClockwiseRoute = 0, CounterclockwiseRoute = 1, FrontSweepRoute = 2 };
const int RouteLag = 4;

// 0=E, 1=SE, 2=S, 3=SW, 4=W, 5=NW, 6=N, 7=NE (map y grows southward).
// Returns -1 for a zero vector.
int patrolOctant(int dx, int dy);
void patrolOctantVector(int octant, int& ux, int& uy);
int patrolDistance(TilePoint a, TilePoint b);

// Wing: two stations about `spacing` tiles to either side of the first group's
// leader, perpendicular to its direction toward the target keep (diagonal
// steps are shortened by 5/7 so the gap stays close to `spacing`). The side
// nearer the second group's leader comes first; ties keep the left side first.
int wingPatrolStations(TilePoint first, TilePoint keep, TilePoint second, int spacing, TilePoint out[2]);

// Mirror: the first group's offset from the target keep entrance reflected
// across the compass axis nearest the target-to-home direction; the point
// reflection through the keep is the alternative. Close to that axis the
// keep's two flanks are used instead. A group within four tiles of the keep
// has no meaningful mirror; one farther than MirrorPatrolRadius is still
// marching, and keeps the native search until it closes in.
const int MirrorPatrolRadius = 48;
int mirrorPatrolStations(TilePoint first, TilePoint keep, TilePoint home, TilePoint out[2]);

// Ring routes round the target keep. Sixteen directions, 0 = east and rising
// clockwise on the map (y grows southward), each an integer vector of length
// 1024 so that every peer samples the same tiles.
const int PatrolRingDirections = 16;
void patrolRingVector(int direction, int& ux, int& uy);
// The tile `step` tiles from `origin` along a ring direction, rounded half away from zero.
TilePoint patrolRingStep(TilePoint origin, int direction, int step);
// The ring direction nearest to a vector (largest dot product, ties to the lower
// direction); -1 for a zero vector.
int patrolRingDirection(int dx, int dy);
int patrolRingWrap(int direction);
// A front sweep walks the directions front-2 .. front+2 and turns at either end.
const int FrontSweepSpan = 2;
const int FrontSweepPositions = 2 * FrontSweepSpan + 1;
int frontSweepDirection(int front, int position);
// The route point after `point` for a circling (step +1/-1) or sweeping route.
// A sweep turns at its ends, so `step` may change.
void nextRoutePoint(int kind, int& point, int& step);

// Siege-defence screen: `forward` tiles from an engine's rally point toward the
// target keep, at most half way there (diagonals shortened by 5/7 as for Wing).
// Returns false when the keep is within one tile or `forward` is not positive.
bool siegeScreenStation(TilePoint engine, TilePoint keep, int forward, TilePoint& out);

} // namespace AicTactics
#endif
