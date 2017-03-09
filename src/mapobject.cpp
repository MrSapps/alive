#include "mapobject.hpp"
#include "debug.hpp"
#include "engine.hpp"

/*
MapObject::MapObject(IMap& map, sol::state& luaState, ResourceLocator& locator, const ObjRect& rect)
: mMap(map), mLuaState(luaState), mLocator(locator), mRect(rect)
{

}

MapObject::MapObject(IMap& map, sol::state& luaState, ResourceLocator& locator, const std::string& scriptName)
: mMap(map), mLuaState(luaState), mLocator(locator), mScriptName(scriptName)
{

}
*/

/*static*/ void MapObject::RegisterScriptBindings(sol::state& state)
{
    Sqrat::Class<MapObject> c(Sqrat::DefaultVM::Get(), "MapObject");
    c.Func("SetScriptInstance", &MapObject::SetScriptInstance);
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
    c.Var("states", &MapObject::mStates);
    c.Var("mXPos", &MapObject::mXPos);
    c.Var("mYPos", &MapObject::mYPos);
    c.Var("mName", &MapObject::mName);
    Sqrat::RootTable().Bind("MapObject", c);

    state.new_usertype<MapObject>("MapObject",
        "SetAnimation", &MapObject::SetAnimation,
        "SetAnimationFrame", &MapObject::SetAnimationFrame,
        "FrameNumber", &MapObject::FrameNumber,
        "IsLastFrame", &MapObject::IsLastFrame,
        "AnimUpdate", &MapObject::AnimUpdate,
        "SetAnimationAtFrame", &MapObject::SetAnimationAtFrame,
        "AnimationComplete", &MapObject::AnimationComplete,
        "NumberOfFrames", &MapObject::NumberOfFrames,
        "FrameCounter", &MapObject::FrameCounter,

        "WallCollision", &MapObject::WallCollision,
        "CellingCollision", &MapObject::CellingCollision,
        "FloorCollision", &MapObject::FloorCollision,

        "SnapXToGrid", &MapObject::SnapXToGrid,
        "FacingLeft", &MapObject::FacingLeft,

        "FacingRight", &MapObject::FacingRight,
        "FlipXDirection", &MapObject::FlipXDirection,
        "states", &MapObject::mStates,
        "mXPos", &MapObject::mXPos,
        "mYPos", &MapObject::mYPos);
}

template<class T, class U>
static void IterateArray(Sqrat::Object& sqObj, const char* arrayName, U callBack)
{
    Sqrat::Array sqArray = sqObj[arrayName];
    if (!sqArray.IsNull())
    {
        const SQInteger arraySize = sqArray.GetSize();
        for (SQInteger i = 0; i < arraySize; i++)
        {
            Sqrat::SharedPtr<T> item = sqArray.GetValue<T>(static_cast<int>(i));
            if (item)
            {
                callBack(*item);
            }
        }
    }
}

MapObject::~MapObject()
{
    TRACE_ENTRYEXIT;
    LOG_INFO("this = " << std::hex << "0x" << static_cast<void*>(this));
}

void MapObject::Init(ResourceLocator& locator)
{
    // Read the kAnimationResources array and kSoundResources
    IterateArray<std::string>(mScriptObject, "kAnimationResources", [&](const std::string& anim)
    {
       mAnims[anim] = locator.LocateAnimation(anim.c_str());
    });

    IterateArray<std::string>(mScriptObject, "kSoundResources", [&](const std::string& anim)
    {
        LOG_INFO(anim);
    });
}

void MapObject::Activate(bool direction)
{
    sol::protected_function f = mStates["Activate"];
    auto ret = f(direction);
    if (!ret.valid())
    {
        sol::error err = ret;
        std::string what = err.what();
        LOG_ERROR(what);
    }
}

bool MapObject::WallCollision(f32 /*dx*/, f32 /*dy*/) const
{
    // The game checks for both kinds of walls no matter the direction
    // ddcheat into a tunnel and the "inside out" wall will still force
    // a crouch.
    /*
    return
    CollisionLine::RayCast<2>(mMap.Lines(),
    glm::vec2(mXPos, mYPos + dy),
    glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos + dy),
    { 1u, 2u }, nullptr);
    */
    return false;
}

bool MapObject::CellingCollision(f32 /*dx*/, f32 /*dy*/) const
{
    /*
    return CollisionLine::RayCast<1>(mMap.Lines(),
    glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos - 2), // avoid collision if we are standing on a celling
    glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos + dy),
    { 3u }, nullptr);
    */
    return false;
}

std::tuple<bool, f32, f32, f32> MapObject::FloorCollision() const
{
    /*
    Physics::raycast_collision c;
    if (CollisionLine::RayCast<1>(mMap.Lines(),
    glm::vec2(mXPos, mYPos),
    glm::vec2(mXPos, mYPos + 260*3), // Check up to 3 screen down
    { 0u }, &c))
    {
    const f32 distance = glm::distance(mYPos, c.intersection.y);
    return std::make_tuple(true, c.intersection.x, c.intersection.y, distance);
    }*/
    return std::make_tuple(false, 0.0f, 0.0f, 0.0f);
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

void MapObject::Render(Renderer& rend, GuiContext& /*gui*/, int x, int y, float scale, int layer) const
{
    if (mAnim)
    {
        mAnim->SetXPos(static_cast<s32>(mXPos) + x);
        mAnim->SetYPos(static_cast<s32>(mYPos) + y);
        mAnim->SetScale(scale);
        mAnim->Render(rend, mFlipX, layer);
    }
}

bool MapObject::ContainsPoint(s32 x, s32 y) const
{
    if (!mAnim)
    {
        // For animationless objects use the object rect
//        return PointInRect(x, y, mRect.x, mRect.y, mRect.w, mRect.h);
    }

    return mAnim->Collision(x, y);
}

void MapObject::SnapXToGrid()
{
    //25x20 grid hack
    const float oldX = mXPos;
    const s32 xpos = static_cast<s32>(mXPos);
    const s32 gridPos = (xpos - 12) % 25;
    if (gridPos >= 13)
    {
        mXPos = static_cast<float>(xpos - gridPos + 25);
    }
    else
    {
        mXPos = static_cast<float>(xpos - gridPos);
    }

    LOG_INFO("SnapX: " << oldX << " to " << mXPos);
}
