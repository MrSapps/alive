#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/inputsystem.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

DEFINE_COMPONENT(SligMovementComponent);

static const std::string kSligStandTurnAround = "SligStandTurnAround";
static const std::string kSligStandIdle = "SligStandIdle";
static const std::string kSligStandToWalk = "SligStandToWalk";
static const std::string kSligWalking = "SligWalking2";
static const std::string kSligWalkToStand1 = "SligWalkToStand";
static const std::string kSligWalkToStand2 = "SligWalkToStandMidGrid";

void SligMovementComponent::OnLoad()
{
    Component::OnLoad(); // calls OnResolveDependencies

    mStateFnMap[States::eStanding] =            { &SligMovementComponent::PreStanding,  &SligMovementComponent::Standing          };
    mStateFnMap[States::eWalking] =             { &SligMovementComponent::PreWalking,   &SligMovementComponent::Walking           };
    mStateFnMap[States::eStandTurningAround] =  { nullptr,                              &SligMovementComponent::StandTurnAround   };

    SetAnimation(kSligStandIdle);
}

void SligMovementComponent::OnResolveDependencies()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();
    mTransformComponent = mEntity->GetComponent<TransformComponent>();
}

void SligMovementComponent::Update()
{
    auto it = mStateFnMap.find(mData.mState);
    if (it != std::end(mStateFnMap) && it->second.mHandler)
    {
        it->second.mHandler(this);
    }
    else
    {
        ASyncTransition();
    }
}

void SligMovementComponent::Serialize(std::ostream &os) const
{
    // static_assert(std::is_pod<decltype(mData)>::value);
	os.write(static_cast<const char*>(static_cast<const void*>(&mData)), sizeof(decltype(mData)));
}

void SligMovementComponent::Deserialize(std::istream &is)
{
    // static_assert(std::is_pod<decltype(mData)>::value);
	is.read(static_cast<char*>(static_cast<void*>(&mData)), sizeof(decltype(mData)));
}

void SligMovementComponent::ASyncTransition()
{
    if (mAnimationComponent->Complete())
    {
        SetState(mData.mNextState);
    }
}

bool SligMovementComponent::DirectionChanged() const
{
    return (!mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoLeft) || (mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoRight);
}

bool SligMovementComponent::TryMoveLeftOrRight() const
{
    return mData.mGoal == Goal::eGoLeft || mData.mGoal == Goal::eGoRight;
}

void SligMovementComponent::SetAnimation(const std::string& anim)
{
    mAnimationComponent->Change(anim.c_str());
}

void SligMovementComponent::SetState(SligMovementComponent::States state)
{
    auto prevState = mData.mState;
    mData.mState = state;
    auto it = mStateFnMap.find(mData.mState);
    if (it != std::end(mStateFnMap))
    {
        if (it->second.mPreHandler)
        {
            it->second.mPreHandler(this, prevState);
        }
    }
}

void SligMovementComponent::PreStanding(SligMovementComponent::States /*previous*/)
{
    SetAnimation(kSligStandIdle);
    mPhysicsComponent->xSpeed = 0.0f;
    mPhysicsComponent->ySpeed = 0.0f;
}

void SligMovementComponent::Standing()
{
    if (TryMoveLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetAnimation(kSligStandTurnAround);
            SetState(States::eStandTurningAround);
            mData.mNextState = States::eStanding;
        }
        else
        {
            SetAnimation(kSligStandToWalk);
            mData.mNextState = States::eWalking;
            SetXSpeed(kSligWalkSpeed);
            SetState(States::eStandToWalking);
        }
    }
    else if (mData.mGoal == Goal::eChant)
    {
        SetState(States::eChanting);
    }
}

void SligMovementComponent::PreWalking(SligMovementComponent::States /*previous*/)
{
    SetAnimation(kSligWalking);
    SetXSpeed(kSligWalkSpeed);
}

void SligMovementComponent::Walking()
{
    if (mAnimationComponent->FrameNumber() == 5 + 1 || mAnimationComponent->FrameNumber() == 14 + 1)
    {
        mTransformComponent->SnapXToGrid();
    }

    if (DirectionChanged() || !TryMoveLeftOrRight())
    {
        if (mAnimationComponent->FrameNumber() == 2 + 1 || mAnimationComponent->FrameNumber() == 11 + 1)
        {
            SetState(States::eWalkingToStanding);
            mData.mNextState = States::eStanding;
            SetAnimation(mAnimationComponent->FrameNumber() == 2 + 1 ? kSligWalkToStand1 : kSligWalkToStand2);
        }
    }
}

void SligMovementComponent::StandTurnAround()
{
    if (mAnimationComponent->Complete())
    {
        mTransformComponent->SnapXToGrid();
        mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        SetState(States::eStanding);
    }
}

void SligMovementComponent::SetXSpeed(f32 speed)
{
    if (mAnimationComponent->mFlipX)
    {
        mPhysicsComponent->xSpeed = -speed;
    }
    else
    {
        mPhysicsComponent->xSpeed = speed;
    }
}

DEFINE_COMPONENT(SligPlayerControllerComponent);

void SligPlayerControllerComponent::OnResolveDependencies()
{
    mEntity->GetManager()->With<InputSystem>([this](auto, auto inputSystem)
                                             {
                                                 mInputMappingActions = inputSystem->GetActions();
                                             });
	mSligMovement = mEntity->GetComponent<SligMovementComponent>();
}

void SligPlayerControllerComponent::Update()
{
    if (mInputMappingActions->Left(mInputMappingActions->mIsDown) && !mInputMappingActions->Right(mInputMappingActions->mIsDown))
    {
        mSligMovement->mData.mGoal = SligMovementComponent::Goal::eGoLeft;
    }
    else if (mInputMappingActions->Right(mInputMappingActions->mIsDown) && !mInputMappingActions->Left(mInputMappingActions->mIsDown))
    {
        mSligMovement->mData.mGoal = SligMovementComponent::Goal::eGoRight;
    }
    else if (mInputMappingActions->Chant(mInputMappingActions->mIsDown))
    {
        mSligMovement->mData.mGoal = SligMovementComponent::Goal::eChant;
    }
    else
    {
        mSligMovement->mData.mGoal = SligMovementComponent::Goal::eStand;
    }
}