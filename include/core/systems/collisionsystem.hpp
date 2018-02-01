#pragma once

#include <memory>
#include <vector>

#include "collisionline.hpp"
#include "oddlib/path.hpp"
#include "core/system.hpp"

class CollisionSystem final : public System
{
public:
    DECLARE_SYSTEM(CollisionSystem);

public:
    void Update() final;

public:
    void ConvertCollisionItems(const std::vector<Oddlib::Path::CollisionItem>& items);
    void Clear();

public:
    class RaycastHit final
    {
    public:
        RaycastHit() = default;
        RaycastHit(float distance, const glm::vec2& point, const glm::vec2& origin);

    public:
        explicit operator bool() const;

    private:
        float mDistance = -1.0f;
        glm::vec2 mPoint = {0.0f, 0.0f};
        glm::vec2 mOrigin = {0.0f, 0.0f};
    };

public:
    RaycastHit Raycast(glm::vec2 origin, glm::vec2 direction, std::initializer_list<CollisionLine::eLineTypes> lineMask) const;

private:
    std::vector<std::unique_ptr<CollisionLine>> mCollisionLines;  // CollisionLine contains raw pointers to other CollisionLine objects. Hence the vector has unique_ptrs so that adding or removing to this vector won't cause the raw pointers to dangle.
};