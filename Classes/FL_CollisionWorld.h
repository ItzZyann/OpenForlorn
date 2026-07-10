#pragma once

#include "incl.h"

#include <cstddef>
#include <vector>

// Static collision geometry built from level collision points and from the
// transformed bounds of block objects that use sprite-based collision.
class FL_CollisionWorld {
public:
    struct Segment {
        CCPoint start;
        CCPoint end;

        Segment();
        Segment(const CCPoint& segmentStart, const CCPoint& segmentEnd);
    };

    struct MoveResult {
        bool grounded;
        bool hitCeiling;
        bool hitWall;

        MoveResult();
    };

    FL_CollisionWorld();

    // passable=true creates a one-way platform. A non-passable shape is solid
    // on every side and blocks floors, walls and ceilings.
    void addShape(const std::vector<CCPoint>& points, bool passable);

    bool moveAabb(
        const CCPoint& oldCenter,
        CCPoint& newCenter,
        CCPoint& velocity,
        float halfWidth,
        float halfHeight,
        MoveResult& result
    ) const;

    bool snapToGround(
        CCPoint& center,
        float halfWidth,
        float halfHeight,
        float maximumDrop,
        float maximumRise
    ) const;

    std::size_t solidShapeCount() const;
    std::size_t oneWaySegmentCount() const;

private:
    struct Polygon {
        std::vector<CCPoint> points;
        std::vector<Segment> upperSegments;
        CCRect bounds;
    };

    std::vector<Polygon> m_solidPolygons;
    std::vector<Segment> m_oneWaySegments;
    std::vector<Segment> m_groundSegments;

    static float cross(const CCPoint& a, const CCPoint& b, const CCPoint& c);
    static float dot(const CCPoint& left, const CCPoint& right);
    static float length(const CCPoint& value);
    static CCPoint normalized(const CCPoint& value);
    static float signedArea(const std::vector<CCPoint>& points);
    static CCRect boundsForPoints(const std::vector<CCPoint>& points);
    static bool rectsOverlapStrict(const CCRect& left, const CCRect& right);
    static bool pointInsideRectStrict(const CCPoint& point, const CCRect& rect);
    static bool pointOnSegment(const CCPoint& point, const CCPoint& start, const CCPoint& end);
    static bool pointInsidePolygonStrict(const CCPoint& point, const std::vector<CCPoint>& polygon);
    static bool segmentsProperlyIntersect(
        const CCPoint& a,
        const CCPoint& b,
        const CCPoint& c,
        const CCPoint& d
    );
    static bool rectIntersectsPolygon(
        const CCPoint& center,
        float halfWidth,
        float halfHeight,
        const Polygon& polygon
    );
    static bool segmentHeightAtX(const Segment& segment, float x, float& y);
    static void appendUniqueDirection(std::vector<CCPoint>& directions, const CCPoint& direction);
    static void appendPolylineSegments(
        const std::vector<CCPoint>& points,
        std::vector<Segment>& output
    );
    static void appendUpperFacingSegments(
        const std::vector<CCPoint>& points,
        std::vector<Segment>& output
    );

    bool separationForPolygon(
        const CCPoint& center,
        const CCPoint& velocity,
        float halfWidth,
        float halfHeight,
        const Polygon& polygon,
        CCPoint& normal,
        float& distance
    ) const;

    bool separationDistanceInDirection(
        const CCPoint& center,
        float halfWidth,
        float halfHeight,
        const Polygon& polygon,
        const CCPoint& direction,
        float& distance
    ) const;

    bool findTraversableTop(
        const Polygon& polygon,
        float footY,
        float centerX,
        float halfWidth,
        float movementDirection,
        float& surfaceY
    ) const;

    bool findLandingOnSegments(
        const std::vector<Segment>& segments,
        float oldFootY,
        float newFootY,
        float centerX,
        float halfWidth,
        float& surfaceY
    ) const;

    bool findOneWayLanding(
        float oldFootY,
        float newFootY,
        float centerX,
        float halfWidth,
        float& surfaceY
    ) const;
};
