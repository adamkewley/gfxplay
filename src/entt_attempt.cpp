#include "app.hpp"

#include <entt.hpp>
#include <iostream>

struct Position final {
    float x;
    float y;
};
static std::ostream& operator<<(std::ostream& o, Position const& p) {
    return o << "px = " << p.x << ", py = " << p.y;
}

struct Velocity final {
    float x;
    float y;
};
static std::ostream& operator<<(std::ostream& o, Velocity const& v) {
    return o << "vx = " << v.x << ", vy = " << v.y;
}

static void update(entt::registry& registry) {
    auto view = registry.view<const Position, Velocity>();
    for (auto [ent, pos, vel] : view.each()) {
        std::cout << pos << ' ' << vel << '\n';
    }
}

int main(int, char*[]) {
    entt::registry registry;

    for (auto i = 0u; i < 10u; ++i) {
        auto const ent = registry.create();
        registry.emplace<Position>(ent, 1.0f * i, 1.0f * i);
        if (i % 2 == 0) {
            registry.emplace<Velocity>(ent, 0.1f * i, 0.1f * i);
        }
    }

    update(registry);

    return 0;
}
