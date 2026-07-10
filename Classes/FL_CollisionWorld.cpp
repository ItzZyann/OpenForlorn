#include "FL_CollisionWorld.h"

#include <algorithm>
#include <cmath>

#if defined(__clang__) || defined(__GNUC__)
    // The macOS compatibility build may force-load the same source from its
    // auxiliary archive while Xcode also compiles it directly. Weak definitions
    // keep that legacy arrangement link-safe without moving code back to headers.
    #define FL_CPP_WEAK __attribute__((weak))
#else
    #define FL_CPP_WEAK
#endif

USING_NS_CC;

FL_CPP_WEAK FL_CollisionWorld::Segment::Segment()
    : start(CCPointZero)
    , end(CCPointZero) {
}

FL_CPP_WEAK FL_CollisionWorld::Segment::Segment(
    const CCPoint& segmentStart,
    const CCPoint& segmentEnd
)
    : start(segmentStart)
    , end(segmentEnd) {
}

FL_CPP_WEAK FL_CollisionWorld::MoveResult::MoveResult()
    : grounded(false)
    , hitCeiling(false)
    , hitWall(false) {
}

namespace {

float clampFloat(float value, float minimum, float maximum) {
    return std::max(minimum, std::min(maximum, value));
}

CCRect playerRect(const CCPoint& center, float halfWidth, float halfHeight) {
    return CCRect(
        center.x - halfWidth,
        center.y - halfHeight,
        halfWidth * 2.0f,
        halfHeight * 2.0f
    );
}

} // namespace

FL_CPP_WEAK FL_CollisionWorld::FL_CollisionWorld() {
}

FL_CPP_WEAK float FL_CollisionWorld::cross(const CCPoint& a, const CCPoint& b, const CCPoint& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

FL_CPP_WEAK float FL_CollisionWorld::dot(const CCPoint& left, const CCPoint& right) {
    return left.x * right.x + left.y * right.y;
}

FL_CPP_WEAK float FL_CollisionWorld::length(const CCPoint& value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

FL_CPP_WEAK CCPoint FL_CollisionWorld::normalized(const CCPoint& value) {
    const float vectorLength = length(value);
    if (vectorLength < 0.00001f) return CCPointZero;
    return ccp(value.x / vectorLength, value.y / vectorLength);
}

FL_CPP_WEAK float FL_CollisionWorld::signedArea(const std::vector<CCPoint>& points) {
    if (points.size() < 3) return 0.0f;
    double twiceArea = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index) {
        const CCPoint& current = points[index];
        const CCPoint& next = points[(index + 1) % points.size()];
        twiceArea += static_cast<double>(current.x) * next.y -
            static_cast<double>(next.x) * current.y;
    }
    return static_cast<float>(twiceArea * 0.5);
}

FL_CPP_WEAK CCRect FL_CollisionWorld::boundsForPoints(const std::vector<CCPoint>& points) {
    if (points.empty()) return CCRect(0.0f, 0.0f, 0.0f, 0.0f);
    float minimumX = points.front().x;
    float maximumX = points.front().x;
    float minimumY = points.front().y;
    float maximumY = points.front().y;
    for (std::size_t index = 1; index < points.size(); ++index) {
        minimumX = std::min(minimumX, points[index].x);
        maximumX = std::max(maximumX, points[index].x);
        minimumY = std::min(minimumY, points[index].y);
        maximumY = std::max(maximumY, points[index].y);
    }
    return CCRect(minimumX, minimumY, maximumX - minimumX, maximumY - minimumY);
}

FL_CPP_WEAK bool FL_CollisionWorld::rectsOverlapStrict(const CCRect& left, const CCRect& right) {
    const float epsilon = 0.001f;
    return left.getMaxX() > right.getMinX() + epsilon &&
        left.getMinX() < right.getMaxX() - epsilon &&
        left.getMaxY() > right.getMinY() + epsilon &&
        left.getMinY() < right.getMaxY() - epsilon;
}

FL_CPP_WEAK bool FL_CollisionWorld::pointInsideRectStrict(const CCPoint& point, const CCRect& rect) {
    const float epsilon = 0.001f;
    return point.x > rect.getMinX() + epsilon && point.x < rect.getMaxX() - epsilon &&
        point.y > rect.getMinY() + epsilon && point.y < rect.getMaxY() - epsilon;
}

FL_CPP_WEAK bool FL_CollisionWorld::pointOnSegment(
    const CCPoint& point,
    const CCPoint& start,
    const CCPoint& end
) {
    if (std::fabs(cross(start, end, point)) > 0.001f) return false;
    const float minimumX = std::min(start.x, end.x) - 0.001f;
    const float maximumX = std::max(start.x, end.x) + 0.001f;
    const float minimumY = std::min(start.y, end.y) - 0.001f;
    const float maximumY = std::max(start.y, end.y) + 0.001f;
    return point.x >= minimumX && point.x <= maximumX &&
        point.y >= minimumY && point.y <= maximumY;
}

FL_CPP_WEAK bool FL_CollisionWorld::pointInsidePolygonStrict(
    const CCPoint& point,
    const std::vector<CCPoint>& polygon
) {
    if (polygon.size() < 3) return false;
    bool inside = false;
    for (std::size_t index = 0, previous = polygon.size() - 1;
         index < polygon.size(); previous = index++) {
        const CCPoint& current = polygon[index];
        const CCPoint& prior = polygon[previous];
        if (pointOnSegment(point, prior, current)) return false;

        const bool crossesRay = (current.y > point.y) != (prior.y > point.y);
        if (!crossesRay) continue;
        const float xAtPointY = (prior.x - current.x) * (point.y - current.y) /
            (prior.y - current.y) + current.x;
        if (point.x < xAtPointY) inside = !inside;
    }
    return inside;
}

FL_CPP_WEAK bool FL_CollisionWorld::segmentsProperlyIntersect(
    const CCPoint& a,
    const CCPoint& b,
    const CCPoint& c,
    const CCPoint& d
) {
    const float first = cross(a, b, c);
    const float second = cross(a, b, d);
    const float third = cross(c, d, a);
    const float fourth = cross(c, d, b);
    const float epsilon = 0.0001f;
    return ((first > epsilon && second < -epsilon) || (first < -epsilon && second > epsilon)) &&
        ((third > epsilon && fourth < -epsilon) || (third < -epsilon && fourth > epsilon));
}

FL_CPP_WEAK bool FL_CollisionWorld::rectIntersectsPolygon(
    const CCPoint& center,
    float halfWidth,
    float halfHeight,
    const Polygon& polygon
) {
    const CCRect rectangle = playerRect(center, halfWidth, halfHeight);
    if (!rectsOverlapStrict(rectangle, polygon.bounds)) return false;

    // When a narrow solid object fits exactly inside the player's AABB, all
    // corners may lie on polygon edges. Corner-only tests then report no contact.
    // The AABB center catches that containment case and makes thin walls/doors
    // from sprite collision block movement reliably.
    if (pointInsidePolygonStrict(center, polygon.points)) return true;

    const CCPoint corners[] = {
        ccp(rectangle.getMinX(), rectangle.getMinY()),
        ccp(rectangle.getMaxX(), rectangle.getMinY()),
        ccp(rectangle.getMaxX(), rectangle.getMaxY()),
        ccp(rectangle.getMinX(), rectangle.getMaxY())
    };

    for (unsigned int index = 0; index < 4; ++index) {
        if (pointInsidePolygonStrict(corners[index], polygon.points)) return true;
    }
    for (std::size_t index = 0; index < polygon.points.size(); ++index) {
        if (pointInsideRectStrict(polygon.points[index], rectangle)) return true;
    }

    for (std::size_t polygonIndex = 0; polygonIndex < polygon.points.size(); ++polygonIndex) {
        const CCPoint& polygonStart = polygon.points[polygonIndex];
        const CCPoint& polygonEnd = polygon.points[(polygonIndex + 1) % polygon.points.size()];
        for (unsigned int rectangleIndex = 0; rectangleIndex < 4; ++rectangleIndex) {
            const CCPoint& rectangleStart = corners[rectangleIndex];
            const CCPoint& rectangleEnd = corners[(rectangleIndex + 1) % 4];
            if (segmentsProperlyIntersect(polygonStart, polygonEnd, rectangleStart, rectangleEnd)) {
                return true;
            }
        }
    }
    return false;
}

FL_CPP_WEAK bool FL_CollisionWorld::segmentHeightAtX(const Segment& segment, float x, float& y) {
    const float minimumX = std::min(segment.start.x, segment.end.x) - 0.5f;
    const float maximumX = std::max(segment.start.x, segment.end.x) + 0.5f;
    if (x < minimumX || x > maximumX) return false;

    const float dx = segment.end.x - segment.start.x;
    if (std::fabs(dx) < 0.001f) return false;
    const float progress = (x - segment.start.x) / dx;
    if (progress < -0.01f || progress > 1.01f) return false;
    y = segment.start.y + (segment.end.y - segment.start.y) * progress;
    return true;
}

FL_CPP_WEAK void FL_CollisionWorld::appendUniqueDirection(
    std::vector<CCPoint>& directions,
    const CCPoint& direction
) {
    const CCPoint unit = normalized(direction);
    if (length(unit) < 0.5f) return;
    for (std::size_t index = 0; index < directions.size(); ++index) {
        if (dot(unit, directions[index]) > 0.9995f) return;
    }
    directions.push_back(unit);
}

FL_CPP_WEAK void FL_CollisionWorld::appendPolylineSegments(
    const std::vector<CCPoint>& points,
    std::vector<Segment>& output
) {
    if (points.size() < 2) return;
    for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        CCPoint start = points[index];
        CCPoint end = points[index + 1];
        if (start.x > end.x) std::swap(start, end);
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;
        const float segmentLength = std::sqrt(dx * dx + dy * dy);
        if (segmentLength < 1.0f || std::fabs(dx) / segmentLength < 0.25f) continue;
        output.push_back(Segment(start, end));
    }
}

FL_CPP_WEAK void FL_CollisionWorld::appendUpperFacingSegments(
    const std::vector<CCPoint>& points,
    std::vector<Segment>& output
) {
    if (points.size() < 2) return;
    if (points.size() == 2) {
        CCPoint start = points[0];
        CCPoint end = points[1];
        if (start.x > end.x) std::swap(start, end);
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;
        const float segmentLength = std::sqrt(dx * dx + dy * dy);
        if (segmentLength >= 1.0f && std::fabs(dx) / segmentLength >= 0.25f) {
            output.push_back(Segment(start, end));
        }
        return;
    }

    const float area = signedArea(points);
    if (std::fabs(area) < 0.001f) {
        for (std::size_t index = 0; index + 1 < points.size(); ++index) {
            std::vector<CCPoint> pair;
            pair.push_back(points[index]);
            pair.push_back(points[index + 1]);
            appendUpperFacingSegments(pair, output);
        }
        return;
    }

    for (std::size_t index = 0; index < points.size(); ++index) {
        const CCPoint& current = points[index];
        const CCPoint& next = points[(index + 1) % points.size()];
        const float dx = next.x - current.x;
        const float dy = next.y - current.y;
        const float segmentLength = std::sqrt(dx * dx + dy * dy);
        if (segmentLength < 1.0f) continue;

        const float outwardY = area < 0.0f ? dx : -dx;
        if (outwardY / segmentLength <= 0.18f) continue;
        output.push_back(Segment(current, next));
    }
}

FL_CPP_WEAK void FL_CollisionWorld::addShape(const std::vector<CCPoint>& sourcePoints, bool passable) {
    std::vector<CCPoint> points;
    points.reserve(sourcePoints.size());
    for (std::size_t index = 0; index < sourcePoints.size(); ++index) {
        if (!points.empty()) {
            const CCPoint difference = ccp(
                sourcePoints[index].x - points.back().x,
                sourcePoints[index].y - points.back().y
            );
            if (length(difference) < 0.01f) continue;
        }
        points.push_back(sourcePoints[index]);
    }
    if (points.size() >= 2) {
        const CCPoint closingDifference = ccp(
            points.front().x - points.back().x,
            points.front().y - points.back().y
        );
        if (length(closingDifference) < 0.01f) points.pop_back();
    }
    if (points.size() < 2) return;

    const float area = std::fabs(signedArea(points));
    const CCRect shapeBounds = boundsForPoints(points);
    const float boundsArea = shapeBounds.size.width * shapeBounds.size.height;
    const bool openPolyline = points.size() == 2 || area < 0.001f ||
        (boundsArea > 0.001f && area / boundsArea < 0.08f);

    if (openPolyline) {
        appendPolylineSegments(points, m_oneWaySegments);
        appendPolylineSegments(points, m_groundSegments);
        return;
    }
    if (passable) {
        appendUpperFacingSegments(points, m_oneWaySegments);
        appendUpperFacingSegments(points, m_groundSegments);
        return;
    }

    Polygon polygon;
    polygon.points = points;
    polygon.bounds = shapeBounds;
    appendUpperFacingSegments(points, polygon.upperSegments);
    m_groundSegments.insert(
        m_groundSegments.end(),
        polygon.upperSegments.begin(),
        polygon.upperSegments.end()
    );
    m_solidPolygons.push_back(polygon);
}

FL_CPP_WEAK bool FL_CollisionWorld::separationForPolygon(
    const CCPoint& center,
    const CCPoint& velocity,
    float halfWidth,
    float halfHeight,
    const Polygon& polygon,
    CCPoint& normal,
    float& distance
) const {
    if (!rectIntersectsPolygon(center, halfWidth, halfHeight, polygon)) return false;

    std::vector<CCPoint> directions;
    directions.reserve(polygon.points.size() * 2 + 4);
    appendUniqueDirection(directions, ccp(1.0f, 0.0f));
    appendUniqueDirection(directions, ccp(-1.0f, 0.0f));
    appendUniqueDirection(directions, ccp(0.0f, 1.0f));
    appendUniqueDirection(directions, ccp(0.0f, -1.0f));
    for (std::size_t index = 0; index < polygon.points.size(); ++index) {
        const CCPoint& start = polygon.points[index];
        const CCPoint& end = polygon.points[(index + 1) % polygon.points.size()];
        const CCPoint edge = ccp(end.x - start.x, end.y - start.y);
        const CCPoint edgeNormal = normalized(ccp(-edge.y, edge.x));
        appendUniqueDirection(directions, edgeNormal);
        appendUniqueDirection(directions, ccp(-edgeNormal.x, -edgeNormal.y));
    }

    const float maximumDistance = std::max(
        64.0f,
        polygon.bounds.size.width + polygon.bounds.size.height +
            halfWidth * 4.0f + halfHeight * 4.0f
    );
    const float velocityLength = length(velocity);
    const CCPoint incoming = velocityLength > 0.001f
        ? ccp(-velocity.x / velocityLength, -velocity.y / velocityLength)
        : CCPointZero;

    bool found = false;
    float bestDistance = 1.0e30f;
    float bestApproach = -1.0e30f;
    CCPoint bestNormal = CCPointZero;

    for (std::size_t directionIndex = 0; directionIndex < directions.size(); ++directionIndex) {
        const CCPoint& direction = directions[directionIndex];
        float high = 0.25f;
        while (high <= maximumDistance && rectIntersectsPolygon(
            ccp(center.x + direction.x * high, center.y + direction.y * high),
            halfWidth,
            halfHeight,
            polygon
        )) {
            high *= 2.0f;
        }
        if (high > maximumDistance) continue;

        float low = 0.0f;
        for (int iteration = 0; iteration < 14; ++iteration) {
            const float middle = (low + high) * 0.5f;
            if (rectIntersectsPolygon(
                ccp(center.x + direction.x * middle, center.y + direction.y * middle),
                halfWidth,
                halfHeight,
                polygon
            )) {
                low = middle;
            } else {
                high = middle;
            }
        }

        const float approach = dot(direction, incoming);
        if (!found || high < bestDistance - 0.02f ||
            (std::fabs(high - bestDistance) <= 0.02f && approach > bestApproach)) {
            found = true;
            bestDistance = high;
            bestApproach = approach;
            bestNormal = direction;
        }
    }

    if (!found) return false;
    normal = bestNormal;
    distance = bestDistance + 0.015f;
    return true;
}

FL_CPP_WEAK bool FL_CollisionWorld::separationDistanceInDirection(
    const CCPoint& center,
    float halfWidth,
    float halfHeight,
    const Polygon& polygon,
    const CCPoint& requestedDirection,
    float& distance
) const {
    if (!rectIntersectsPolygon(center, halfWidth, halfHeight, polygon)) return false;

    const CCPoint direction = normalized(requestedDirection);
    if (length(direction) < 0.5f) return false;

    const float maximumDistance = std::max(
        64.0f,
        polygon.bounds.size.width + polygon.bounds.size.height +
            halfWidth * 4.0f + halfHeight * 4.0f
    );

    float high = 0.25f;
    while (high <= maximumDistance && rectIntersectsPolygon(
        ccp(center.x + direction.x * high, center.y + direction.y * high),
        halfWidth,
        halfHeight,
        polygon
    )) {
        high *= 2.0f;
    }
    if (high > maximumDistance) return false;

    float low = 0.0f;
    for (int iteration = 0; iteration < 16; ++iteration) {
        const float middle = (low + high) * 0.5f;
        if (rectIntersectsPolygon(
            ccp(center.x + direction.x * middle, center.y + direction.y * middle),
            halfWidth,
            halfHeight,
            polygon
        )) {
            low = middle;
        } else {
            high = middle;
        }
    }

    distance = high + 0.015f;
    return true;
}

FL_CPP_WEAK bool FL_CollisionWorld::findTraversableTop(
    const Polygon& polygon,
    float footY,
    float centerX,
    float halfWidth,
    float movementDirection,
    float& surfaceY
) const {
    if (polygon.upperSegments.empty()) return false;

    const float direction = movementDirection < 0.0f ? -1.0f : 1.0f;
    const float probes[] = {
        centerX + direction * halfWidth * 0.98f,
        centerX + direction * halfWidth * 0.78f,
        centerX + direction * halfWidth * 0.52f,
        centerX,
        centerX - direction * halfWidth * 0.55f
    };
    const float maximumStepUp = 18.0f;
    const float maximumDrop = 8.0f;
    bool found = false;
    float highest = -1.0e30f;

    for (std::size_t segmentIndex = 0;
         segmentIndex < polygon.upperSegments.size(); ++segmentIndex) {
        for (unsigned int probeIndex = 0;
             probeIndex < sizeof(probes) / sizeof(probes[0]); ++probeIndex) {
            float candidateY = 0.0f;
            if (!segmentHeightAtX(
                polygon.upperSegments[segmentIndex],
                probes[probeIndex],
                candidateY
            )) continue;
            if (candidateY < footY - maximumDrop ||
                candidateY > footY + maximumStepUp) continue;
            if (!found || candidateY > highest) {
                found = true;
                highest = candidateY;
            }
        }
    }

    if (found) surfaceY = highest;
    return found;
}

FL_CPP_WEAK bool FL_CollisionWorld::findLandingOnSegments(
    const std::vector<Segment>& segments,
    float oldFootY,
    float newFootY,
    float centerX,
    float halfWidth,
    float& surfaceY
) const {
    // Probe slightly inside the player's feet. Using the exact AABB corners at
    // adjoining polygons makes a shared endpoint look like a vertical wall.
    const float probes[] = {
        centerX,
        centerX - halfWidth * 0.98f,
        centerX + halfWidth * 0.98f,
        centerX - halfWidth * 0.72f,
        centerX + halfWidth * 0.72f,
        centerX - halfWidth * 0.34f,
        centerX + halfWidth * 0.34f
    };
    const float stepUpAllowance = 18.0f;
    const float landingSkin = 2.5f;
    bool found = false;
    float highest = -1.0e30f;

    for (std::size_t segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex) {
        for (unsigned int probeIndex = 0;
             probeIndex < sizeof(probes) / sizeof(probes[0]); ++probeIndex) {
            float candidateY = 0.0f;
            if (!segmentHeightAtX(segments[segmentIndex], probes[probeIndex], candidateY)) continue;
            if (oldFootY < candidateY - stepUpAllowance) continue;
            if (newFootY > candidateY + landingSkin) continue;
            if (!found || candidateY > highest) {
                found = true;
                highest = candidateY;
            }
        }
    }

    if (found) surfaceY = highest;
    return found;
}

FL_CPP_WEAK bool FL_CollisionWorld::findOneWayLanding(
    float oldFootY,
    float newFootY,
    float centerX,
    float halfWidth,
    float& surfaceY
) const {
    return findLandingOnSegments(
        m_oneWaySegments,
        oldFootY,
        newFootY,
        centerX,
        halfWidth,
        surfaceY
    );
}

FL_CPP_WEAK bool FL_CollisionWorld::moveAabb(
    const CCPoint& oldCenter,
    CCPoint& newCenter,
    CCPoint& velocity,
    float halfWidth,
    float halfHeight,
    MoveResult& result
) const {
    result = MoveResult();
    bool changed = false;

    // Resolve the supporting surface before solid polygon sides. Adjacent level
    // pieces overlap heavily, so their internal vertical edges must not behave
    // like walls while the player is already standing on their upper boundary.
    if (velocity.y <= 0.0f) {
        const float oldFootY = oldCenter.y - halfHeight;
        const float newFootY = newCenter.y - halfHeight;
        float surfaceY = 0.0f;
        if (findLandingOnSegments(
            m_groundSegments,
            oldFootY,
            newFootY,
            newCenter.x,
            halfWidth,
            surfaceY
        )) {
            newCenter.y = surfaceY + halfHeight;
            velocity.y = 0.0f;
            result.grounded = true;
            changed = true;
        }
    }

    // Polygon contacts are resolved only along a world axis. Projecting the
    // velocity onto an arbitrary edge normal made floor/slope contacts remove
    // horizontal speed every substep, which looked like slow sliding at seams.
    for (int pass = 0; pass < 8; ++pass) {
        bool foundContact = false;
        bool foundFloorContact = false;
        float bestDistance = 1.0e30f;
        CCPoint bestAxis = CCPointZero;
        int bestKind = 0; // 1=floor, 2=ceiling, 3=wall

        const CCRect playerBounds = playerRect(
            newCenter,
            halfWidth,
            halfHeight
        );
        const float footY = newCenter.y - halfHeight;

        for (std::size_t polygonIndex = 0;
             polygonIndex < m_solidPolygons.size(); ++polygonIndex) {
            const Polygon& polygon = m_solidPolygons[polygonIndex];
            if (!rectsOverlapStrict(playerBounds, polygon.bounds)) continue;
            if (!rectIntersectsPolygon(newCenter, halfWidth, halfHeight, polygon)) continue;

            // If this polygon has a walkable top directly below the player's
            // footprint and it is within step height, its side is an internal
            // seam, not a blocking wall. This is common where an object collision
            // overlaps the landscape collision by several pixels.
            if (result.grounded) {
                float traversableY = 0.0f;
                if (findTraversableTop(
                    polygon,
                    footY,
                    newCenter.x,
                    halfWidth,
                    velocity.x,
                    traversableY
                )) {
                    const float desiredCenterY = traversableY + halfHeight;
                    if (desiredCenterY > newCenter.y + 0.001f) {
                        newCenter.y = desiredCenterY;
                        changed = true;
                    }
                    // Once the feet are aligned with this top boundary, ignore
                    // the polygon's buried/internal side edges for this substep.
                    continue;
                }
            }

            CCPoint contactNormal = CCPointZero;
            float contactDistance = 0.0f;
            if (!separationForPolygon(
                newCenter,
                velocity,
                halfWidth,
                halfHeight,
                polygon,
                contactNormal,
                contactDistance
            )) continue;

            int contactKind = 3;
            CCPoint contactAxis = CCPointZero;
            float axisDistance = 0.0f;
            if (contactNormal.y > 0.30f && velocity.y <= 0.0f) {
                contactKind = 1;
                contactAxis = ccp(0.0f, 1.0f);
                if (!separationDistanceInDirection(
                    newCenter,
                    halfWidth,
                    halfHeight,
                    polygon,
                    contactAxis,
                    axisDistance
                )) continue;
            } else if (contactNormal.y < -0.45f && velocity.y >= 0.0f) {
                contactKind = 2;
                contactAxis = ccp(0.0f, -1.0f);
                if (!separationDistanceInDirection(
                    newCenter,
                    halfWidth,
                    halfHeight,
                    polygon,
                    contactAxis,
                    axisDistance
                )) continue;
            } else {
                contactKind = 3;
                float leftDistance = 0.0f;
                float rightDistance = 0.0f;
                const bool canMoveLeft = separationDistanceInDirection(
                    newCenter,
                    halfWidth,
                    halfHeight,
                    polygon,
                    ccp(-1.0f, 0.0f),
                    leftDistance
                );
                const bool canMoveRight = separationDistanceInDirection(
                    newCenter,
                    halfWidth,
                    halfHeight,
                    polygon,
                    ccp(1.0f, 0.0f),
                    rightDistance
                );
                if (!canMoveLeft && !canMoveRight) continue;

                if (velocity.x > 0.001f) {
                    // A polygon whose nearest exit is to the right is behind the
                    // player. It may still overlap the trailing edge of the AABB,
                    // but must not pull the player backwards while moving right.
                    if (canMoveRight && (!canMoveLeft || rightDistance < leftDistance)) continue;
                    contactAxis = ccp(-1.0f, 0.0f);
                    axisDistance = leftDistance;
                } else if (velocity.x < -0.001f) {
                    if (canMoveLeft && (!canMoveRight || leftDistance < rightDistance)) continue;
                    contactAxis = ccp(1.0f, 0.0f);
                    axisDistance = rightDistance;
                } else if (!canMoveRight || (canMoveLeft && leftDistance <= rightDistance)) {
                    contactAxis = ccp(-1.0f, 0.0f);
                    axisDistance = leftDistance;
                } else {
                    contactAxis = ccp(1.0f, 0.0f);
                    axisDistance = rightDistance;
                }
            }

            // While grounded, floor correction must happen before a wall at the
            // same corner. Otherwise the wall correction can move the player back
            // a fraction of a pixel on every physics substep.
            const bool floorPriority = result.grounded && contactKind == 1;
            const bool shouldReplace = !foundContact ||
                (floorPriority && !foundFloorContact) ||
                (floorPriority == foundFloorContact && axisDistance < bestDistance);
            if (shouldReplace) {
                foundContact = true;
                foundFloorContact = floorPriority;
                bestDistance = axisDistance;
                bestAxis = contactAxis;
                bestKind = contactKind;
            }
        }

        if (!foundContact) break;

        newCenter.x += bestAxis.x * bestDistance;
        newCenter.y += bestAxis.y * bestDistance;
        changed = true;

        if (bestKind == 1) {
            if (velocity.y < 0.0f) velocity.y = 0.0f;
            result.grounded = true;
        } else if (bestKind == 2) {
            if (velocity.y > 0.0f) velocity.y = 0.0f;
            result.hitCeiling = true;
        } else {
            if ((bestAxis.x > 0.0f && velocity.x < 0.0f) ||
                (bestAxis.x < 0.0f && velocity.x > 0.0f)) {
                velocity.x = 0.0f;
            }
            result.hitWall = true;
        }
    }

    // Recheck one-way platforms after solid correction because an axis-only
    // correction may place the feet over a platform in the same substep.
    if (velocity.y <= 0.0f) {
        const float oldFootY = oldCenter.y - halfHeight;
        const float newFootY = newCenter.y - halfHeight;
        float surfaceY = 0.0f;
        if (findOneWayLanding(oldFootY, newFootY, newCenter.x, halfWidth, surfaceY)) {
            const float landedCenterY = surfaceY + halfHeight;
            if (newCenter.y <= landedCenterY + 2.5f) {
                newCenter.y = landedCenterY;
                velocity.y = 0.0f;
                result.grounded = true;
                changed = true;
            }
        }
    }

    return changed;
}

FL_CPP_WEAK bool FL_CollisionWorld::snapToGround(
    CCPoint& center,
    float halfWidth,
    float halfHeight,
    float maximumDrop,
    float maximumRise
) const {
    const float probes[] = {
        center.x,
        center.x - halfWidth * 0.68f,
        center.x + halfWidth * 0.68f
    };
    const float footY = center.y - halfHeight;
    bool found = false;
    float highest = -1.0e30f;

    for (std::size_t segmentIndex = 0; segmentIndex < m_groundSegments.size(); ++segmentIndex) {
        for (unsigned int probeIndex = 0;
             probeIndex < sizeof(probes) / sizeof(probes[0]); ++probeIndex) {
            float candidateY = 0.0f;
            if (!segmentHeightAtX(m_groundSegments[segmentIndex], probes[probeIndex], candidateY)) continue;
            if (candidateY < footY - maximumDrop || candidateY > footY + maximumRise) continue;
            if (!found || candidateY > highest) {
                found = true;
                highest = candidateY;
            }
        }
    }

    if (found) center.y = highest + halfHeight;
    return found;
}

FL_CPP_WEAK std::size_t FL_CollisionWorld::solidShapeCount() const {
    return m_solidPolygons.size();
}

FL_CPP_WEAK std::size_t FL_CollisionWorld::oneWaySegmentCount() const {
    return m_oneWaySegments.size();
}

#undef FL_CPP_WEAK
