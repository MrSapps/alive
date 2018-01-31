#include "mapobject.hpp"
#include "debug.hpp"
#include "engine.hpp"
#include "physics.hpp"
#include "collisionline.hpp"
#include "gridmap.hpp"
#include "resourcemapper.hpp"

MapObject::Loader::Loader(MapObject&)
{

}

bool MapObject::Loader::Load()
{
    return false;
}

void MapObject::Loader::LoadAnimations()
{

}

void MapObject::Loader::LoadSounds()
{

}

void MapObject::Loader::SetState(MapObject::Loader::LoaderStates)
{

}

MapObject::~MapObject()
{
}

void MapObject::LoadAnimation(const std::string&)
{

}

bool MapObject::Init()
{
	return false;
}

void MapObject::Update(const InputState&)
{

}

bool MapObject::WallCollision(IMap&, f32, f32) const
{
	return false;
}

bool MapObject::CellingCollision(IMap&, f32, f32) const
{
	return false;
}

CollisionResult MapObject::FloorCollision(IMap&) const
{
	return {};
}

MapObject* MapObject::AddChildObject()
{
	return nullptr;
}

u32 MapObject::ChildCount() const
{
	return 0;
}

void MapObject::RemoveChild(u32)
{

}

void MapObject::LoadScript()
{

}

bool MapObject::AnimationComplete() const
{
    return false;
}

void MapObject::SetAnimation(const std::string&)
{

}

void MapObject::SetAnimationFrame(s32)
{

}

void MapObject::SetAnimationAtFrame(const std::string&, u32)
{

}

bool MapObject::AnimUpdate()
{
    return false;
}

s32 MapObject::FrameCounter() const
{
    return 0;
}

s32 MapObject::NumberOfFrames() const
{
    return 0;
}

bool MapObject::IsLastFrame() const
{
    return false;
}

s32 MapObject::FrameNumber() const
{
    return 0;
}

void MapObject::ReloadScript()
{

}

void MapObject::Render(AbstractRenderer&, int, int, float, int) const
{

}

bool MapObject::ContainsPoint(s32, s32) const
{
	return false;
}

float SnapXToGrid(float)
{
   return 0;
}

void MapObject::SnapXToGrid()
{

}