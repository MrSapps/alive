#pragma once

#include <vector>
#include <memory>

namespace Oddlib
{
    

    class Anim
    {
    public:
    };

    class Frame
    {
    public:
    };

    class Animation
    {
    public:
        std::vector<std::shared_ptr<Frame>> mFrames;
    };

    class AnimationFactory
    {
    public:

    };
}
