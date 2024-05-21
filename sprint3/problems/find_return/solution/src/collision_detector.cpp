#include "collision_detector.h"
#include <cassert>
#include <optional>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> detected_events;
    static auto eq_pt = [](geom::Point2D p1, geom::Point2D p2) {
        return p1.x == p2.x && p1.y == p2.y;
    };
    for (size_t g = 0; g < provider.GatherersCount(); ++g) {
        Gatherer gatherer = provider.GetGatherer(g);
        if (eq_pt(gatherer.start_pos_, gatherer.end_pos_)) {
            continue;
        }
        for (size_t i = 0; i < provider.ItemsCount(); ++i) {
            Item item = provider.GetItem(i);
            auto collect_result = TryCollectPoint(gatherer.start_pos_, gatherer.end_pos_, item.position_);

            if (collect_result.IsCollected(gatherer.width_ + item.width_)) {
                GatheringEvent evt{.item_id_ = i,
                                   .gatherer_id_ = g,
                                   .sq_distance_ = collect_result.sq_distance,
                                   .time_ = collect_result.proj_ratio};
                detected_events.push_back(evt);
            }
        }
    }
    std::sort(detected_events.begin(), detected_events.end(),
              [](const GatheringEvent& e_l, const GatheringEvent& e_r) { return e_l.time_ < e_r.time_; });
    return detected_events;
}

std::optional<LineSegment> Intersect(LineSegment s1, LineSegment s2) {
    double left = std::max(s1.x1, s2.x1);
    double right = std::min(s1.x2, s2.x2);
    if (right < left) {
        return std::nullopt;
    }
    return LineSegment{.x1 = left, .x2 = right};
}

LineSegment ProjectX(Rectangle r) {
    return LineSegment{.x1 = r.x, .x2 = r.x + r.w};
}

LineSegment ProjectY(Rectangle r) {
    return LineSegment{.x1 = r.y, .x2 = r.y + r.h};
}

std::optional<Rectangle> Intersect(Rectangle r1, Rectangle r2) {
    auto px = Intersect(ProjectX(r1), ProjectX(r2));
    auto py = Intersect(ProjectY(r1), ProjectY(r2));
    if (!px || !py) {
        return std::nullopt;
    }
    return Rectangle(px->x1, py->x1, px->x2 - px->x1, py->x2 - py->x1);
}

std::vector<OfficeSaveEvent> FindOfficeSaveEvents(const OfficeSaveProvider& provider) {
    std::vector<OfficeSaveEvent> detected_events;
    auto eq_pt = [](geom::Point2D p1, geom::Point2D p2) {
        return p1.x == p2.x && p1.y == p2.y;
    };
    for (size_t g = 0; g < provider.GatherersCount(); ++g) {
        Gatherer gatherer = provider.GetGatherer(g);
        if (eq_pt(gatherer.start_pos_, gatherer.end_pos_)) {
            continue;
        }
        for (size_t i = 0; i < provider.RectsCount(); ++i) {
            Rectangle office = provider.GetRect(i);
            Rectangle gatherer_path(geom::Point2D(gatherer.start_pos_.x, gatherer.start_pos_.y),
                                    geom::Point2D(gatherer.end_pos_.x, gatherer.end_pos_.y), gatherer.width_);
            auto is_intersects = Intersect(gatherer_path, office);
            if (!is_intersects) {
                continue;
            }
            double min_ratio = 1.01;
            for (auto vertex : is_intersects->GetVertices()) {
                auto collect_result = TryCollectPoint(gatherer.start_pos_, gatherer.end_pos_, vertex);
                if (collect_result.IsCollected(gatherer.width_) && collect_result.proj_ratio < min_ratio) {
                    min_ratio = collect_result.proj_ratio;
                }
            }
            OfficeSaveEvent evt{.office_id_ = i, .gatherer_id_ = g, .time_ = min_ratio};
            detected_events.push_back(evt);
        }
    }
    std::sort(detected_events.begin(), detected_events.end(),
              [](const OfficeSaveEvent& e_l, const OfficeSaveEvent& e_r) { return e_l.time_ < e_r.time_; });
    return detected_events;
}

bool operator<(const AllIvents& a, const AllIvents& b) {
    double a_time, b_time;
    std::visit([&](auto&& arg) { a_time = arg.time_; }, a);
    std::visit([&](auto&& arg) { b_time = arg.time_; }, b);
    return a_time < b_time;
}

}  // namespace collision_detector