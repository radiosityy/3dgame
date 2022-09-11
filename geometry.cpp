#include "geometry.h"

#if 0
bool LineSegment::intersects(const LineSegment& rhs) const noexcept
{
    return intersects(*this, rhs);
}

bool LineSegment::intersects(const LineSegment& lhs, const LineSegment& rhs) noexcept
{
    Point2D r = lhs.p1 - lhs.p0;
    Point2D s = rhs.p1 - rhs.p0;
    Point2D qmp = rhs.p0 - lhs.p0;
    float rxs = r.cross(s);
    float qmpxr = qmp.cross(r);

    if(rxs == 0)
    {
        if(qmpxr == 0)
            return true;
        else
            return false;
    }
    else
    {
        float qmpxs = qmp.x*s.y - qmp.y*s.x;
        float u = qmpxr / rxs;
        float t = qmpxs / rxs;
        if((t >= 0.0f) && (t <= 1.0f) && (u >= 0.0f) && (u <= 1.0f))
            return true;
        else
            return false;
    }
}
#endif
