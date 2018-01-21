#include "mapobject.hpp"
#include "debug.hpp"
#include "engine.hpp"
#include "physics.hpp"
#include "collisionline.hpp"
#include "gridmap.hpp"
#include "resourcemapper.hpp"

/*static*/ void MapObject::RegisterScriptBindings()
{
    {
        Sqrat::Class<MapObject, Sqrat::NoConstructor<MapObject>> c(Sqrat::DefaultVM::Get(), "MapObject");
        
        c.Func("AddChildObject", &MapObject::AddChildObject);
        c.Func("ChildCount", &MapObject::ChildCount);
        c.Func("ChildAt", &MapObject::ChildAt);
        c.Func("RemoveChild", &MapObject::RemoveChild);

        c.Func("SetScriptInstance", &MapObject::SetScriptInstance);
        c.Func("LoadAnimation", &MapObject::LoadAnimation);
        c.Func("SetAnimation", &MapObject::SetAnimation);
        c.Func("SetAnimationFrame", &MapObject::SetAnimationFrame);
        c.Func("FrameNumber", &MapObject::FrameNumber);
        c.Func("IsLastFrame", &MapObject::IsLastFrame);
        c.Func("AnimUpdate", &MapObject::AnimUpdate);
        c.Func("SetAnimationAtFrame", &MapObject::SetAnimationAtFrame);
        c.Func("AnimationComplete", &MapObject::AnimationComplete);
        c.Func("NumberOfFrames", &MapObject::NumberOfFrames);
        c.Func("FrameCounter", &MapObject::FrameCounter);

        c.Func("WallCollision", &MapObject::WallCollision);
        c.Func("CellingCollision", &MapObject::CellingCollision);
        c.Func("FloorCollision", &MapObject::FloorCollision);

        c.Func("SnapXToGrid", &MapObject::SnapXToGrid);
        c.Func("FacingLeft", &MapObject::FacingLeft);

        c.Func("FacingRight", &MapObject::FacingRight);
        c.Func("FlipXDirection", &MapObject::FlipXDirection);
        c.Var("mXPos", &MapObject::mXPos);
        c.Var("mYPos", &MapObject::mYPos);
        c.Var("mName", &MapObject::mName);
        Sqrat::RootTable().Bind("MapObject", c);
    }

    {
        Sqrat::Class<CollisionResult> cr(Sqrat::DefaultVM::Get(), "CollisionResult");
        cr.Var("collision", &CollisionResult::collision);
        cr.Var("x", &CollisionResult::x);
        cr.Var("y", &CollisionResult::y);
        cr.Var("distance", &CollisionResult::distance);
        Sqrat::RootTable().Bind("CollisionResult", cr);
    }
}

static bool GetArray(Sqrat::Object& sqObj, const char* arrayName, Sqrat::Array& sqArray)
{
    sqArray = sqObj[arrayName];
    if (!sqArray.IsNull())
    {
        const SQInteger arraySize = sqArray.GetSize();
        if (arraySize > 0)
        {
            return true;
        }
    }
    return false;
}

MapObject::Loader::Loader(MapObject& obj)
    : mMapObj(obj)
{

}

bool MapObject::Loader::Load()
{
    switch (mState)
    {
    case LoaderStates::eInit:
        SetState(LoaderStates::eLoadAnimations);
        break;

    case LoaderStates::eLoadAnimations:
        LoadAnimations();
        break;

    case LoaderStates::eLoadSounds:
        LoadSounds();
        break;
    }

    if (mState == LoaderStates::eInit)
    {
        return true;
    }

    return false;
}

void MapObject::Loader::LoadAnimations()
{
    Sqrat::Array animsArray;
    if (GetArray(mMapObj.mScriptObject, "kAnimationResources", animsArray))
    {
        if (mForLoop.IterateTimeBoxed(kMaxExecutionTimeMs, animsArray.GetSize(), [&]()
        {
            Sqrat::SharedPtr<std::string> item = animsArray.GetValue<std::string>(static_cast<int>(mForLoop.Value()));
            if (item)
            {
                mMapObj.LoadAnimation(*item);
            }
        }))
        {
            SetState(LoaderStates::eLoadSounds);
        }
    }
    else
    {
        SetState(LoaderStates::eLoadSounds);
    }
}

void MapObject::Loader::LoadSounds()
{
    Sqrat::Array soundsArray;
    if (GetArray(mMapObj.mScriptObject, "kSoundResources", soundsArray))
    {
        if (mForLoop.IterateTimeBoxed(kMaxExecutionTimeMs, soundsArray.GetSize(), [&]()
        {
            Sqrat::SharedPtr<std::string> item = soundsArray.GetValue<std::string>(static_cast<int>(mForLoop.Value()));
            if (item)
            {
                // TODO: Now that all sfx are cached this probably shouldn't be needed.. unless used as hook for loading mod fx?
                LOG_INFO(*item);
            }
        }))
        {
            SetState(LoaderStates::eInit);
        }
    }
    else
    {
        SetState(LoaderStates::eInit);
    }
}

void MapObject::Loader::SetState(MapObject::Loader::LoaderStates state)
{
    if (state != mState)
    {
        mState = state;
    }
}

MapObject::~MapObject()
{
}

void MapObject::LoadAnimation(const std::string& name)
{
    mAnims[name] = mLocator.LocateAnimation(name).get();
}

bool MapObject::Init()
{
    if (!mLoader)
    {
        mLoader = std::make_unique<Loader>(*this);
    }

    if (mLoader->Load())
    {
        mLoader = nullptr;
        return true;
    }

    return false;
}

bool MapObject::WallCollision(IMap& map, f32 dx, f32 dy) const
{
    // The game checks for both kinds of walls no matter the direction
    // ddcheat into a tunnel and the "inside out" wall will still force
    // a crouch.
    return
        CollisionLine::RayCast<2>(map.Lines(),
            glm::vec2(mXPos, mYPos + dy),
            glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos + dy),
            { 1u, 2u }, nullptr);
}

bool MapObject::CellingCollision(IMap& map, f32 dx, f32 dy) const
{
    return CollisionLine::RayCast<1>(map.Lines(),
        glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos - 2), // avoid collision if we are standing on a celling
        glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos + dy),
        { 3u }, nullptr);
}

CollisionResult MapObject::FloorCollision(IMap& map) const
{
    Physics::raycast_collision c;
    if (CollisionLine::RayCast<1>(map.Lines(),
        glm::vec2(mXPos, mYPos),
        glm::vec2(mXPos, mYPos + 260 * 3), // Check up to 3 screen down
        { 0u }, &c))
    {
        const f32 distance = glm::distance(mYPos, c.intersection.y);
        return{ true, c.intersection.x, c.intersection.y, distance };
    }
    return{};
}

MapObject* MapObject::AddChildObject()
{
    auto ptr = std::make_unique<MapObject>(mLocator, mRect);
    MapObject* pRaw = ptr.get();
    mChildren.push_back(std::move(ptr));
    return pRaw;
}

u32 MapObject::ChildCount() const
{
    return static_cast<u32>(mChildren.size());
}

Sqrat::Object& MapObject::ChildAt(u32 index)
{
    if (index < mChildren.size())
    {
        return mChildren[index]->mScriptObject;
    }
    throw Oddlib::Exception("ChildAt() index out of bounds");
}

void MapObject::RemoveChild(u32 index)
{
    if (index < mChildren.size())
    {
        mChildren.erase(mChildren.begin() + index);
    }
}

void MapObject::LoadScript()
{
    /*
    // Load FSM script
    const std::string script = mLocator.LocateScript(mScriptName.c_str());
    try
    {
    mLuaState.script(script);
    }
    catch (const sol::error& ex)
    {
    //LOG_ERROR(ex.what());
    return;
    }

    // Set initial state
    try
    {
    sol::protected_function f = mLuaState["init"];
    auto ret = f(this);
    if (!ret.valid())
    {
    sol::error err = ret;
    std::string what = err.what();
    LOG_ERROR(what);
    }

    }
    catch (const sol::error& e)
    {
    LOG_ERROR(e.what());
    }
    */
}

void MapObject::Update(const InputState& input)
{
    //TRACE_ENTRYEXIT;

    Debugging().mDebugObj = this;
    if (Debugging().mSingleStepObject && !Debugging().mDoSingleStepObject)
    {
        return;
    }

    //if (mAnim)
    {

        Sqrat::Function updateFn(mScriptObject, "Update");
        updateFn.Execute(input.Mapping().GetActions());
        SquirrelVm::CheckError();

        /*
        sol::protected_function f = mLuaState["update"];
        auto ret = f(this, input.Mapping().GetActions());
        if (!ret.valid())
        {
        sol::error err = ret;
        std::string what = err.what();
        LOG_ERROR(what);
        }*/
    }

    //::Sleep(300);

    static float prevX = 0.0f;
    static float prevY = 0.0f;
    if (prevX != mXPos || prevY != mYPos)
    {
        //LOG_INFO("Player X Delta " << mXPos - prevX << " Y Delta " << mYPos - prevY << " frame " << mAnim->FrameNumber());
    }
    prevX = mXPos;
    prevY = mYPos;

    Debugging().mInfo.mXPos = mXPos;
    Debugging().mInfo.mYPos = mYPos;
    Debugging().mInfo.mFrameToRender = FrameNumber();

    if (Debugging().mSingleStepObject && Debugging().mDoSingleStepObject)
    {
        // Step is done - no more updates till the user requests it
        Debugging().mDoSingleStepObject = false;
    }
}

bool MapObject::AnimationComplete() const
{
    if (!mAnim) { return false; }
    return mAnim->IsComplete();
}

void MapObject::SetAnimation(const std::string& animation)
{
    if (animation.empty())
    {
        mAnim = nullptr;
    }
    else
    {
        if (mAnims.find(animation) == std::end(mAnims))
        {
            abort();
            /*
            auto anim = mLocator.LocateAnimation(animation.c_str());
            if (!anim)
            {
            LOG_ERROR("Animation " << animation << " not found");
            abort();
            }
            mAnims[animation] = std::move(anim);
            */
        }
        mAnim = mAnims[animation].get();
        mAnim->Restart();
    }
}

void MapObject::SetAnimationFrame(s32 frame)
{
    if (mAnim)
    {
        mAnim->SetFrame(frame);
    }
}

void MapObject::SetAnimationAtFrame(const std::string& animation, u32 frame)
{
    SetAnimation(animation);
    mAnim->SetFrame(frame);
}

bool MapObject::AnimUpdate()
{
    return mAnim->Update();
}

s32 MapObject::FrameCounter() const
{
    return mAnim->FrameCounter();
}

s32 MapObject::NumberOfFrames() const
{
    return mAnim->NumberOfFrames();
}

bool MapObject::IsLastFrame() const
{
    return mAnim->IsLastFrame();
}

s32 MapObject::FrameNumber() const
{
    if (!mAnim) { return 0; }
    return mAnim->FrameNumber();
}

void MapObject::ReloadScript()
{
    LoadScript();
    SnapXToGrid();
}

void MapObject::Render(AbstractRenderer& rend, int x, int y, float scale, int layer) const
{
    if (mAnim)
    {
        mAnim->SetXPos(static_cast<s32>(mXPos) + x);
        mAnim->SetYPos(static_cast<s32>(mYPos) + y);
        mAnim->SetScale(scale);
        mAnim->Render(rend, mFlipX, layer);
    }
    for (auto& child : mChildren)
    {
        child->Render(rend, x, y, scale, layer);
    }
}

bool MapObject::ContainsPoint(s32 x, s32 y) const
{
    if (!mAnim)
    {
        // For animationless objects use the object rect
        return PointInRect(x, y, mRect.x, mRect.y, mRect.w, mRect.h);
    }

    return mAnim->Collision(x, y);
}

float SnapXToGrid(float toSnap)
{
    //25x20 grid hack
    const float oldX = toSnap;
    const s32 xpos = static_cast<s32>(toSnap);
    const s32 gridPos = (xpos - 12) % 25;
    if (gridPos >= 13)
    {
        toSnap = static_cast<float>(xpos - gridPos + 25);
    }
    else
    {
        toSnap = static_cast<float>(xpos - gridPos);
    }

    LOG_INFO("SnapX: " << oldX << " to " << toSnap);
    return toSnap;
}

void MapObject::SnapXToGrid()
{
    mXPos = ::SnapXToGrid(mXPos);
}
