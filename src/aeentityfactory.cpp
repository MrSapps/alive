#include <map>

#include "gridmap.hpp"
#include "aeentityfactory.hpp"

#include "oddlib/stream.hpp"

#include "core/entity.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/transformcomponent.hpp"

struct AeContinuePoint
{
    const static u8 kSizeInBytes = 20;

    u16 mScale;
    u16 mSaveFileId;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mSaveFileId = ReadU16(s);
    }
};

struct AePathTransition
{
    const static u8 kSizeInBytes = 28;

    u16 mLevel;
    u16 mPath;
    u16 mCamera;
    u16 mMovie;
    u16 mWipe;
    u16 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mLevel = ReadU16(s);
        mPath = ReadU16(s);
        mCamera = ReadU16(s);
        mMovie = ReadU16(s);
        mWipe = ReadU16(s);
        mScale = ReadU16(s);
    }
};

struct AeHoist
{
    const static u8 kSizeInBytes = 24;

    u16 mHoistType;
    u16 mEdgeType;
    u16 mId;
    u16 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mHoistType = ReadU16(s);
        mEdgeType = ReadU16(s);
        mId = ReadU16(s);
        mScale = ReadU16(s);
    }
};

struct AeEdge
{
    const static u8 kSizeInBytes = 24;

    u16 mType;
    u16 mCanGrab;
    u32 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mType = ReadU16(s);
        mCanGrab = ReadU16(s);
        mScale = ReadU32(s);
    }
};

struct AeDeathDrop
{
    const static u8 kSizeInBytes = 28;

    u16 mAnimation;
    u16 mSound;
    u16 mId;
    u16 mAction;
    u32 mValue;

    void Deserialize(Oddlib::IStream& s)
    {
        mAnimation = ReadU16(s);
        mSound = ReadU16(s);
        mId = ReadU16(s);
        mAction = ReadU16(s);
        mValue = ReadU32(s);
    }
};

struct AeDoor
{
    const static u8 kSizeInBytes = 68;

    u16 mLevel;
    u16 mPath;
    u16 mCamera;
    u16 mScale;
    u16 mDoorNumber;
    u16 mId;
    u16 mTargetDoorNumber;
    u16 mType;
    u16 mStartState;
    u16 mHubId1;
    u16 mHubId2;
    u16 mHubId3;
    u16 mHubId4;
    u16 mHubId5;
    u16 mHubId6;
    u16 mHubId7;
    u16 mHubId8;
    u16 mWipeEffect;
    u16 mMovieNumber;
    u16 mXOffset;
    u16 mYOffset;
    u16 mWipeXOrigin;
    u16 mWipeYOrigin;
    u16 mAbeDirection;
    u16 mCloseAfterUse;
    u16 mCancelThrowables;

    void Deserialize(Oddlib::IStream& s)
    {
        mLevel = ReadU16(s);
        mPath = ReadU16(s);
        mCamera = ReadU16(s);
        mScale = ReadU16(s);
        mDoorNumber = ReadU16(s);
        mId = ReadU16(s);
        mTargetDoorNumber = ReadU16(s);
        mType = ReadU16(s);
        mStartState = ReadU16(s);
        mHubId1 = ReadU16(s);
        mHubId2 = ReadU16(s);
        mHubId3 = ReadU16(s);
        mHubId4 = ReadU16(s);
        mHubId5 = ReadU16(s);
        mHubId6 = ReadU16(s);
        mHubId7 = ReadU16(s);
        mHubId8 = ReadU16(s);
        mWipeEffect = ReadU16(s);
        mMovieNumber = ReadU16(s);
        mXOffset = ReadU16(s);
        mYOffset = ReadU16(s);
        mWipeXOrigin = ReadU16(s);
        mWipeYOrigin = ReadU16(s);
        mAbeDirection = ReadU16(s);
        mCloseAfterUse = ReadU16(s);
        mCancelThrowables = ReadU16(s);
    }
};

struct AeShadow
{
    const static u8 kSizeInBytes = 32;

    u16 mCenterW;
    u16 mCenterH;
    u8 mRed1;
    u8 mGreen1;
    u8 mBlue1;
    u8 mRed2;
    u8 mGreen2;
    u8 mBlue2;
    u16 mId;
    u32 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mCenterW = ReadU16(s);
        mCenterH = ReadU16(s);
        mRed1 = ReadU8(s);
        mGreen1 = ReadU8(s);
        mBlue1 = ReadU8(s);
        mRed2 = ReadU8(s);
        mGreen2 = ReadU8(s);
        mBlue2 = ReadU8(s);
        mId = ReadU16(s);
        mScale = ReadU32(s);
    }
};

struct AeLiftPoint
{
    const static u8 kSizeInBytes = 28;

    u16 mId;
    u16 mIsStartPosition;
    u16 mLiftType;
    u16 mLiftStopType;
    u16 mScale;
    u16 mIgnoreLiftMover;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mIsStartPosition = ReadU16(s);
        mLiftType = ReadU16(s);
        mLiftStopType = ReadU16(s);
        mScale = ReadU16(s);
        mIgnoreLiftMover = ReadU16(s);
    }
};

struct AeWellLocal
{
    const static u8 kSizeInBytes = 40;

    u16 mScale;
    u16 mTriggerId;
    u16 mWellId;
    u16 mResourceId;
    u16 mOffDeltaX;
    u16 mOffDeltaY;
    u16 mOnDeltaX;
    u16 mOnDeltaY;
    u16 mLeavesWhenOn;
    u16 mLeafX;
    u32 mLeafY;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mTriggerId = ReadU16(s);
        mWellId = ReadU16(s);
        mResourceId = ReadU16(s);
        mOffDeltaX = ReadU16(s);
        mOffDeltaY = ReadU16(s);
        mOnDeltaX = ReadU16(s);
        mOnDeltaY = ReadU16(s);
        mLeavesWhenOn = ReadU16(s);
        mLeafX = ReadU16(s);
        mLeafY = ReadU32(s);
    }
};

struct AeDove
{
    const static u8 kSizeInBytes = 24;

    u16 mNumberOfBirds;
    u16 mPixelPerfect;
    u16 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mNumberOfBirds = ReadU16(s);
        mPixelPerfect = ReadU16(s);
        mScale = ReadU16(s);
    }
};

struct AeRockSack
{
    const static u8 kSizeInBytes = 28;

    u16 mSide;
    u16 mXVel;
    u16 mYVel;
    u16 mScale;
    u16 mNumberOfRocks;

    void Deserialize(Oddlib::IStream& s)
    {
        mSide = ReadU16(s);
        mXVel = ReadU16(s);
        mYVel = ReadU16(s);
        mScale = ReadU16(s);
        mNumberOfRocks = ReadU16(s);
    }
};

struct AeFallingItem
{
    const static u8 kSizeInBytes = 28;

    u16 mId;
    u16 mScale;
    u16 mDelayTime;
    u16 mNumberOfItems;
    u16 mResetId;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mScale = ReadU16(s);
        mDelayTime = ReadU16(s);
        mNumberOfItems = ReadU16(s);
        mResetId = ReadU16(s);
    }
};

struct AePullRingRope
{
    const static u8 kSizeInBytes = 32;

    u16 mId;
    u16 mTargetAction;
    u16 mLengthOfRope;
    u16 mScale;
    u16 mOnSound;
    u16 mOffSound;
    u32 mSoundDirection;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mTargetAction = ReadU16(s);
        mLengthOfRope = ReadU16(s);
        mScale = ReadU16(s);
        mOnSound = ReadU16(s);
        mOffSound = ReadU16(s);
        mSoundDirection = ReadU32(s);
    }
};

struct AeBackgroundAnimation
{
    const static u8 kSizeInBytes = 28;

    u16 mAnimationRes;
    u16 mIsSemiTrans;
    u16 mSemiTransMode;
    u16 mSoundEffect;
    u16 mId;
    u16 mLayer;

    void Deserialize(Oddlib::IStream& s)
    {
        mAnimationRes = ReadU16(s);
        mIsSemiTrans = ReadU16(s);
        mSemiTransMode = ReadU16(s);
        mSoundEffect = ReadU16(s);
        mId = ReadU16(s);
        mLayer = ReadU16(s);
    }
};

struct AeTimedMine
{
    const static u8 kSizeInBytes = 28;

    u16 mId;
    u16 mState;
    u16 mScale;
    u16 mTicksBeforeExplode;
    u32 mDisableResources;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mState = ReadU16(s);
        mScale = ReadU16(s);
        mTicksBeforeExplode = ReadU16(s);
        mDisableResources = ReadU32(s);
    }
};

struct AeSlig
{
    const static u8 kSizeInBytes = 80;

    u16 mScale;
    u16 mStartState;
    u16 mPauseTime;
    u16 mPauseLeftMin;
    u16 mPauseLeftMax;
    u16 mPauseRightMin;
    u16 mPauseRightMax;
    u16 mChalNumber;
    u16 mChalTimer;
    u16 mNumberOfTimesToShoot;
    u16 mUnknown;
    u16 mCode1;
    u16 mCode2;
    u16 mChaseAbe;
    u16 mStartDirection;
    u16 mPanicTimeout;
    u16 mNumPanicSounds;
    u16 mPanicSoundTimeout;
    u16 mStopChaseDelay;
    u16 mTimeToWaitBeforeChase;
    u16 mSligID;
    u16 mListenTime;
    u16 mSayWhat;
    u16 mBeatMud;
    u16 mTalkToAbe;
    u16 mDontShoot;
    u16 mZShootDelay;
    u16 mStayAwake;
    u16 mDisableResources;
    u16 mNoiseWakeUpDistance;
    u32 mId;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mStartState = ReadU16(s);
        mPauseTime = ReadU16(s);
        mPauseLeftMin = ReadU16(s);
        mPauseLeftMax = ReadU16(s);
        mPauseRightMin = ReadU16(s);
        mPauseRightMax = ReadU16(s);
        mChalNumber = ReadU16(s);
        mChalTimer = ReadU16(s);
        mNumberOfTimesToShoot = ReadU16(s);
        mUnknown = ReadU16(s);
        mCode1 = ReadU16(s);
        mCode2 = ReadU16(s);
        mChaseAbe = ReadU16(s);
        mStartDirection = ReadU16(s);
        mPanicTimeout = ReadU16(s);
        mNumPanicSounds = ReadU16(s);
        mPanicSoundTimeout = ReadU16(s);
        mStopChaseDelay = ReadU16(s);
        mTimeToWaitBeforeChase = ReadU16(s);
        mSligID = ReadU16(s);
        mListenTime = ReadU16(s);
        mSayWhat = ReadU16(s);
        mBeatMud = ReadU16(s);
        mTalkToAbe = ReadU16(s);
        mDontShoot = ReadU16(s);
        mZShootDelay = ReadU16(s);
        mStayAwake = ReadU16(s);
        mDisableResources = ReadU16(s);
        mNoiseWakeUpDistance = ReadU16(s);
        mId = ReadU32(s);
    }
};

struct AeSlog
{
    const static u8 kSizeInBytes = 36;

    u16 mScale;
    u16 mDirection;
    u16 mAsleep;
    u16 mWakeUpAnger;
    u16 mBarkAnger;
    u16 mChaseAnger;
    u16 mJumpDelay;
    u16 mDisableResources;
    u16 mAngryId;
    u16 mBoneEatingTime;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mDirection = ReadU16(s);
        mAsleep = ReadU16(s);
        mWakeUpAnger = ReadU16(s);
        mBarkAnger = ReadU16(s);
        mChaseAnger = ReadU16(s);
        mJumpDelay = ReadU16(s);
        mDisableResources = ReadU16(s);
        mAngryId = ReadU16(s);
        mBoneEatingTime = ReadU16(s);
    }
};

struct AeSwitch
{
    const static u8 kSizeInBytes = 32;

    u16 mTargetAction;
    u16 mScale;
    u16 mOnSound;
    u16 mOffSound;
    u16 mSoundDirection;
    u16 mTriggerId;
    u32 mPersistOffscreen;

    void Deserialize(Oddlib::IStream& s)
    {
        mTargetAction = ReadU16(s);
        mScale = ReadU16(s);
        mOnSound = ReadU16(s);
        mOffSound = ReadU16(s);
        mSoundDirection = ReadU16(s);
        mTriggerId = ReadU16(s);
        mPersistOffscreen = ReadU32(s);
    }
};

struct AeSecurityEye
{
    const static u8 kSizeInBytes = 20;

    u32 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU32(s);
    }
};

struct AePulley
{
    const static u8 kSizeInBytes = 20;

    u32 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU32(s);
    }
};

struct AeAbeStart
{
    const static u8 kSizeInBytes = 20;

    u32 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU32(s);
    }
};

struct AeWellExpress
{
    const static u8 kSizeInBytes = 52;

    u16 mScale;
    u16 mId;
    u16 mWellId;
    u16 mResourceId;
    u16 mExitX;
    u16 mExitY;
    u16 mOffLevel;
    u16 mOffPath;
    u16 mOffCamera;
    u16 mOffWell;
    u16 mOnLevel;
    u16 mOnPath;
    u16 mOnCamera;
    u16 mOnWell;
    u16 mLeavesWhenOn;
    u16 mLeafX;
    u16 mLeafY;
    u16 mMovie;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mId = ReadU16(s);
        mWellId = ReadU16(s);
        mResourceId = ReadU16(s);
        mExitX = ReadU16(s);
        mExitY = ReadU16(s);
        mOffLevel = ReadU16(s);
        mOffPath = ReadU16(s);
        mOffCamera = ReadU16(s);
        mOffWell = ReadU16(s);
        mOnLevel = ReadU16(s);
        mOnPath = ReadU16(s);
        mOnCamera = ReadU16(s);
        mOnWell = ReadU16(s);
        mLeavesWhenOn = ReadU16(s);
        mLeafX = ReadU16(s);
        mLeafY = ReadU16(s);
        mMovie = ReadU16(s);
    }
};

struct AeMine
{
    const static u8 kSizeInBytes = 28;

    u16 mNumPatterns;
    u16 mPattern;
    u16 mScale;
    u16 mDisabledResources;
    u32 mPersistsOffscreen;

    void Deserialize(Oddlib::IStream& s)
    {
        mNumPatterns = ReadU16(s);
        mPattern = ReadU16(s);
        mScale = ReadU16(s);
        mDisabledResources = ReadU16(s);
        mPersistsOffscreen = ReadU32(s);
    }
};

struct AeUxb
{
    const static u8 kSizeInBytes = 28;

    u16 mNumPatterns;
    u16 mPattern;
    u16 mScale;
    u16 mState;
    u32 mDisabledResources;

    void Deserialize(Oddlib::IStream& s)
    {
        mNumPatterns = ReadU16(s);
        mPattern = ReadU16(s);
        mScale = ReadU16(s);
        mState = ReadU16(s);
        mDisabledResources = ReadU32(s);
    }
};

struct AeParamite
{
    const static u8 kSizeInBytes = 40;

    u16 mScale;
    u16 mEntrance;
    u16 mAttackDelay;
    u16 mDropDelay;
    u16 mMeatEatingTime;
    u16 mAttackDuration;
    u16 mDisableResources;
    u16 mId;
    u16 mHissBeforeAttack;
    u16 mDeleteWhenFarAway;
    u16 mDeadlyScratch;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mEntrance = ReadU16(s);
        mAttackDelay = ReadU16(s);
        mDropDelay = ReadU16(s);
        mMeatEatingTime = ReadU16(s);
        mAttackDuration = ReadU16(s);
        mDisableResources = ReadU16(s);
        mId = ReadU16(s);
        mHissBeforeAttack = ReadU16(s);
        mDeleteWhenFarAway = ReadU16(s);
        mDeadlyScratch = ReadU16(s);
    }
};

struct AeMovieStone
{
    const static u8 kSizeInBytes = 24;

    u16 mMovieNumber;
    u16 mScale;
    u16 mId;

    void Deserialize(Oddlib::IStream& s)
    {
        mMovieNumber = ReadU16(s);
        mScale = ReadU16(s);
        mId = ReadU16(s);
    }
};

struct AeBirdPortal
{
    const static u8 kSizeInBytes = 36;

    u16 mSide;
    u16 mDestLevel;
    u16 mDestPath;
    u16 mDestCamera;
    u16 mScale;
    u16 mMovie;
    u16 mPortalType;
    u16 mNumMudsForShrykll;
    u16 mCreateId;
    u16 mDeleteId;

    void Deserialize(Oddlib::IStream& s)
    {
        mSide = ReadU16(s);
        mDestLevel = ReadU16(s);
        mDestPath = ReadU16(s);
        mDestCamera = ReadU16(s);
        mScale = ReadU16(s);
        mMovie = ReadU16(s);
        mPortalType = ReadU16(s);
        mNumMudsForShrykll = ReadU16(s);
        mCreateId = ReadU16(s);
        mDeleteId = ReadU16(s);
    }
};

struct AePortalExit
{
    const static u8 kSizeInBytes = 20;

    u16 mSide;
    u16 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mSide = ReadU16(s);
        mScale = ReadU16(s);
    }
};

struct AeTrapDoor
{
    const static u8 kSizeInBytes = 32;

    u16 mId;
    u16 mStartState;
    u16 mSelfClosing;
    u16 mScale;
    u16 mDestLevel;
    u16 mDirection;
    u16 mAnimOffset;
    u16 mStayOpenTime;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mStartState = ReadU16(s);
        mSelfClosing = ReadU16(s);
        mScale = ReadU16(s);
        mDestLevel = ReadU16(s);
        mDirection = ReadU16(s);
        mAnimOffset = ReadU16(s);
        mStayOpenTime = ReadU16(s);
    }
};

struct AeRollingBall
{
    const static u8 kSizeInBytes = 28;

    u16 mScale;
    u16 mRollDirection;
    u16 mId;
    u16 mSpeed;
    u32 mAcceleration;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mRollDirection = ReadU16(s);
        mId = ReadU16(s);
        mSpeed = ReadU16(s);
        mAcceleration = ReadU32(s);
    }
};

struct AeSligLeftBound
{
    const static u8 kSizeInBytes = 20;

    u16 mSligId;
    u16 mDisableResources;

    void Deserialize(Oddlib::IStream& s)
    {
        mSligId = ReadU16(s);
        mDisableResources = ReadU16(s);
    }
};

struct AeInvisibleZone
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeFootSwitch
{
    const static u8 kSizeInBytes = 24;

    u16 mId;
    u16 mScale;
    u16 mAction;
    u16 mTriggerBy;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mScale = ReadU16(s);
        mAction = ReadU16(s);
        mTriggerBy = ReadU16(s);
    }
};

struct AeSecurityOrb
{
    const static u8 kSizeInBytes = 24;

    u32 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU32(s);
    }
};

struct AeMotionDetector
{
    const static u8 kSizeInBytes = 36;

    u16 mScale;
    u16 mDeviceX;
    u16 mDeviceY;
    u16 mSpeedX256;
    u16 mStartOn;
    u16 mDrawFlare;
    u16 mDisableId;
    u16 mAlarmId;
    u16 mAlarmTicks;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mDeviceX = ReadU16(s);
        mDeviceY = ReadU16(s);
        mSpeedX256 = ReadU16(s);
        mStartOn = ReadU16(s);
        mDrawFlare = ReadU16(s);
        mDisableId = ReadU16(s);
        mAlarmId = ReadU16(s);
        mAlarmTicks = ReadU16(s);
    }
};

struct AeSligSpawner
{
    const static u8 kSizeInBytes = 80;

    u16 mScale;
    u16 mStartState;
    u16 mPauseTime;
    u16 mPauseLeftMin;
    u16 mPauseLeftMax;
    u16 mPauseRightMin;
    u16 mPauseRightMax;
    u16 mChalNumber;
    u16 mChalTimer;
    u16 mNumberOfTimesToShoot;
    u16 mUnknown;
    u16 mCode1;
    u16 mCode2;
    u16 mChaseAbe;
    u16 mStartDirection;
    u16 mPanicTimeout;
    u16 mNumPanicSounds;
    u16 mPanicSoundTimeout;
    u16 mStopChaseDelay;
    u16 mTimeToWaitBeforeChase;
    u16 mSligId;
    u16 mListenTime;
    u16 mSayWhat;
    u16 mBeatMud;
    u16 mTalkToAbe;
    u16 mDontShoot;
    u16 mZShootDelay;
    u16 mStayAwake;
    u16 mDisableResources;
    u16 mNoiseWakeUpDistance;
    u16 mId;
    u16 mSpawnMany;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mStartState = ReadU16(s);
        mPauseTime = ReadU16(s);
        mPauseLeftMin = ReadU16(s);
        mPauseLeftMax = ReadU16(s);
        mPauseRightMin = ReadU16(s);
        mPauseRightMax = ReadU16(s);
        mChalNumber = ReadU16(s);
        mChalTimer = ReadU16(s);
        mNumberOfTimesToShoot = ReadU16(s);
        mUnknown = ReadU16(s);
        mCode1 = ReadU16(s);
        mCode2 = ReadU16(s);
        mChaseAbe = ReadU16(s);
        mStartDirection = ReadU16(s);
        mPanicTimeout = ReadU16(s);
        mNumPanicSounds = ReadU16(s);
        mPanicSoundTimeout = ReadU16(s);
        mStopChaseDelay = ReadU16(s);
        mTimeToWaitBeforeChase = ReadU16(s);
        mSligId = ReadU16(s);
        mListenTime = ReadU16(s);
        mSayWhat = ReadU16(s);
        mBeatMud = ReadU16(s);
        mTalkToAbe = ReadU16(s);
        mDontShoot = ReadU16(s);
        mZShootDelay = ReadU16(s);
        mStayAwake = ReadU16(s);
        mDisableResources = ReadU16(s);
        mNoiseWakeUpDistance = ReadU16(s);
        mId = ReadU16(s);
        mSpawnMany = ReadU16(s);
    }
};

struct AeElectricWall
{
    const static u8 kSizeInBytes = 24;

    u16 mScale;
    u16 mId;
    u16 mStart;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mId = ReadU16(s);
        mStart = ReadU16(s);
    }
};

struct AeLiftMover
{
    const static u8 kSizeInBytes = 24;

    u16 mSwitchId;
    u16 mLiftId;
    u16 mDirection;

    void Deserialize(Oddlib::IStream& s)
    {
        mSwitchId = ReadU16(s);
        mLiftId = ReadU16(s);
        mDirection = ReadU16(s);
    }
};

struct AeMeatSack
{
    const static u8 kSizeInBytes = 28;

    u16 mSide;
    u16 mXVel;
    u16 mYVel;
    u16 mScale;
    u16 mNumberOfItems;

    void Deserialize(Oddlib::IStream& s)
    {
        mSide = ReadU16(s);
        mXVel = ReadU16(s);
        mYVel = ReadU16(s);
        mScale = ReadU16(s);
        mNumberOfItems = ReadU16(s);
    }
};

struct AeScrab
{
    const static u8 kSizeInBytes = 44;

    u16 mScale;
    u16 mAttackDelay;
    u16 mPatrolType;
    u16 mLeftMinDelay;
    u16 mLeftMaxDelay;
    u16 mRightMinDelay;
    u16 mRightMaxDelay;
    u16 mAttackDuration;
    u16 mDisableResources;
    u16 mRoarRandomly;
    u16 mPersistant;
    u16 mWhirlAttackDuration;
    u16 mWhirlAttackRecharge;
    u16 mKillCloseFleech;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mAttackDelay = ReadU16(s);
        mPatrolType = ReadU16(s);
        mLeftMinDelay = ReadU16(s);
        mLeftMaxDelay = ReadU16(s);
        mRightMinDelay = ReadU16(s);
        mRightMaxDelay = ReadU16(s);
        mAttackDuration = ReadU16(s);
        mDisableResources = ReadU16(s);
        mRoarRandomly = ReadU16(s);
        mPersistant = ReadU16(s);
        mWhirlAttackDuration = ReadU16(s);
        mWhirlAttackRecharge = ReadU16(s);
        mKillCloseFleech = ReadU16(s);
    }
};

struct AeScrabLeftBound
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeScrabRightBound
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeSligRightBound
{
    const static u8 kSizeInBytes = 20;

    u16 mSligId;
    u16 mDisableResources;

    void Deserialize(Oddlib::IStream& s)
    {
        mSligId = ReadU16(s);
        mDisableResources = ReadU16(s);
    }
};

struct AeSligPersist
{
    const static u8 kSizeInBytes = 20;

    u16 mSligId;
    u16 mDisableResources;

    void Deserialize(Oddlib::IStream& s)
    {
        mSligId = ReadU16(s);
        mDisableResources = ReadU16(s);
    }
};

struct AeEnemyStopper
{
    const static u8 kSizeInBytes = 20;

    u16 mStopDirection;
    u16 mId;

    void Deserialize(Oddlib::IStream& s)
    {
        mStopDirection = ReadU16(s);
        mId = ReadU16(s);
    }
};

struct AeInvisibleSwitch
{
    const static u8 kSizeInBytes = 28;

    u16 mId;
    u16 mIdAction;
    u16 mDelay;
    u16 mSetOffAlarm;
    u16 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mIdAction = ReadU16(s);
        mDelay = ReadU16(s);
        mSetOffAlarm = ReadU16(s);
        mScale = ReadU16(s);
    }
};

struct AeMudokon
{
    const static u8 kSizeInBytes = 48;

    u16 mScale;
    u16 mState;
    u16 mDirection;
    u16 mVoicePitch;
    u16 mRescueId;
    u16 mDeaf;
    u16 mDisableResources;
    u16 mSaveState;
    u16 mEmotion;
    u16 mBlind;
    u16 mAngryTrigger;
    u16 mStopTrigger;
    u16 mGetsDepressed;
    u16 mRingTimeout;
    u32 mInstantPowerup;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mState = ReadU16(s);
        mDirection = ReadU16(s);
        mVoicePitch = ReadU16(s);
        mRescueId = ReadU16(s);
        mDeaf = ReadU16(s);
        mDisableResources = ReadU16(s);
        mSaveState = ReadU16(s);
        mEmotion = ReadU16(s);
        mBlind = ReadU16(s);
        mAngryTrigger = ReadU16(s);
        mStopTrigger = ReadU16(s);
        mGetsDepressed = ReadU16(s);
        mRingTimeout = ReadU16(s);
        mInstantPowerup = ReadU32(s);
    }
};

struct AeZSligCover
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeDoorFlame
{
    const static u8 kSizeInBytes = 24;

    u16 mId;
    u16 mScale;
    u32 mColour;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mScale = ReadU16(s);
        mColour = ReadU32(s);
    }
};

struct AeMovingBomb
{
    const static u8 kSizeInBytes = 32;

    u16 mSpeed;
    u16 mId;
    u16 mStartType;
    u16 mScale;
    u16 mMaxRise;
    u16 mDisableResources;
    u16 mStartSpeed;
    u16 mPersistOffscreen;

    void Deserialize(Oddlib::IStream& s)
    {
        mSpeed = ReadU16(s);
        mId = ReadU16(s);
        mStartType = ReadU16(s);
        mScale = ReadU16(s);
        mMaxRise = ReadU16(s);
        mDisableResources = ReadU16(s);
        mStartSpeed = ReadU16(s);
        mPersistOffscreen = ReadU16(s);
    }
};

struct AeMenuController
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeTimerTrigger
{
    const static u8 kSizeInBytes = 28;

    u16 mId;
    u16 mDelayTicks;
    u16 mId1;
    u16 mId2;
    u16 mId3;
    u16 mId4;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mDelayTicks = ReadU16(s);
        mId1 = ReadU16(s);
        mId2 = ReadU16(s);
        mId3 = ReadU16(s);
        mId4 = ReadU16(s);
    }
};

struct AeSligVoiceLock
{
    const static u8 kSizeInBytes = 28;

    u16 mScale;
    u16 mId;
    u16 mCode1;
    u16 mCode2;
    u16 mXPos;
    u16 mYPos;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mId = ReadU16(s);
        mCode1 = ReadU16(s);
        mCode2 = ReadU16(s);
        mXPos = ReadU16(s);
        mYPos = ReadU16(s);
    }
};

struct AeGrenadeMachine
{
    const static u8 kSizeInBytes = 24;

    u16 mScale;
    u16 mSpoutSide;
    u16 mDisabledResources;
    u16 mNumberOfGrenades;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mSpoutSide = ReadU16(s);
        mDisabledResources = ReadU16(s);
        mNumberOfGrenades = ReadU16(s);
    }
};

struct AeLcdScreen
{
    const static u8 kSizeInBytes = 28;

    u16 mMessage1Id;
    u16 mMessageRandMin;
    u16 mMessageRandMax;
    u16 mMessage2Id;
    u32 mIdToSwitchMessageSets;

    void Deserialize(Oddlib::IStream& s)
    {
        mMessage1Id = ReadU16(s);
        mMessageRandMin = ReadU16(s);
        mMessageRandMax = ReadU16(s);
        mMessage2Id = ReadU16(s);
        mIdToSwitchMessageSets = ReadU32(s);
    }
};

struct AeHandStone
{
    const static u8 kSizeInBytes = 28;

    u16 mScale;
    u16 mCamera1;
    u16 mCamera2;
    u16 mCamera3;
    u32 mTriggerId;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mCamera1 = ReadU16(s);
        mCamera2 = ReadU16(s);
        mCamera3 = ReadU16(s);
        mTriggerId = ReadU32(s);
    }
};

struct AeCreditsController
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeNullObject1
{
    const static u8 kSizeInBytes = 20;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeLcdStatusBoard
{
    const static u8 kSizeInBytes = 24;

    u16 mNumberOfMuds;
    u16 mZulagNumber;
    u32 mHidden;

    void Deserialize(Oddlib::IStream& s)
    {
        mNumberOfMuds = ReadU16(s);
        mZulagNumber = ReadU16(s);
        mHidden = ReadU32(s);
    }
};

struct AeWorkWheelSyncer
{
    const static u8 kSizeInBytes = 32;

    u16 mId1;
    u16 mId2;
    u16 mTriggerId;
    u16 mAction;
    u16 mId3;
    u16 mId4;
    u16 mId5;
    u16 mId6;

    void Deserialize(Oddlib::IStream& s)
    {
        mId1 = ReadU16(s);
        mId2 = ReadU16(s);
        mTriggerId = ReadU16(s);
        mAction = ReadU16(s);
        mId3 = ReadU16(s);
        mId4 = ReadU16(s);
        mId5 = ReadU16(s);
        mId6 = ReadU16(s);
    }
};

struct AeMusic
{
    const static u8 kSizeInBytes = 24;

    u16 mType;
    u16 mStart;
    u32 mTimeTicks;

    void Deserialize(Oddlib::IStream& s)
    {
        mType = ReadU16(s);
        mStart = ReadU16(s);
        mTimeTicks = ReadU32(s);
    }
};

struct AeLightEffect
{
    const static u8 kSizeInBytes = 24;

    u16 mType;
    u16 mSize;
    u16 mId;
    u16 mFlipX;

    void Deserialize(Oddlib::IStream& s)
    {
        mType = ReadU16(s);
        mSize = ReadU16(s);
        mId = ReadU16(s);
        mFlipX = ReadU16(s);
    }
};

struct AeSlogSpawner
{
    const static u8 kSizeInBytes = 32;

    u16 mScale;
    u16 mNumberOfSlogs;
    u16 mAtATime;
    u16 mDirection;
    u16 mTicksBetweenSlogs;
    u16 mId;
    u16 mListenToSligs;
    u16 mJumpAttackDelay;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mNumberOfSlogs = ReadU16(s);
        mAtATime = ReadU16(s);
        mDirection = ReadU16(s);
        mTicksBetweenSlogs = ReadU16(s);
        mId = ReadU16(s);
        mListenToSligs = ReadU16(s);
        mJumpAttackDelay = ReadU16(s);
    }
};

struct AeDeathClock
{
    const static u8 kSizeInBytes = 24;

    u16 mStartTriggerId;
    u16 mTime;
    u32 mStopTriggerId;

    void Deserialize(Oddlib::IStream& s)
    {
        mStartTriggerId = ReadU16(s);
        mTime = ReadU16(s);
        mStopTriggerId = ReadU32(s);
    }
};

struct AeGasEmitter
{
    const static u8 kSizeInBytes = 20;

    u16 mPortId;
    u16 mGasColour;

    void Deserialize(Oddlib::IStream& s)
    {
        mPortId = ReadU16(s);
        mGasColour = ReadU16(s);
    }
};

struct AeSlogHut
{
    const static u8 kSizeInBytes = 24;

    u16 mScale;
    u16 mId;
    u32 mTicksBetweenZs;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mId = ReadU16(s);
        mTicksBetweenZs = ReadU32(s);
    }
};

struct AeGlukkon
{
    const static u8 kSizeInBytes = 44;

    u16 mScale;
    u16 mDirection;
    u16 mCalmMotion;
    u16 mPreAlarmDelay;
    u16 mPostAlarmDelay;
    u16 mSpawnId;
    u16 mSpawnDirection;
    u16 mSpawnDelay;
    u16 mGlukkonType;
    u16 mStartGasId;
    u16 mPlayMovieId;
    u16 mMovieId;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mDirection = ReadU16(s);
        mCalmMotion = ReadU16(s);
        mPreAlarmDelay = ReadU16(s);
        mPostAlarmDelay = ReadU16(s);
        mSpawnId = ReadU16(s);
        mSpawnDirection = ReadU16(s);
        mSpawnDelay = ReadU16(s);
        mGlukkonType = ReadU16(s);
        mStartGasId = ReadU16(s);
        mPlayMovieId = ReadU16(s);
        mMovieId = ReadU16(s);
    }
};

struct AeKillUnsavedMuds
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeSoftLanding
{
    const static u8 kSizeInBytes = 20;

    u32 mId;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU32(s);
    }
};

struct AeNullObject2
{
    const static u8 kSizeInBytes = 32;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeWater
{
    const static u8 kSizeInBytes = 28;

    u16 mMaxDrops;
    u16 mId;
    u16 mSplashTime;
    u16 mSplashXVelocity;
    u16 mSplashYVelocity;
    u16 mTimeout;

    void Deserialize(Oddlib::IStream& s)
    {
        mMaxDrops = ReadU16(s);
        mId = ReadU16(s);
        mSplashTime = ReadU16(s);
        mSplashXVelocity = ReadU16(s);
        mSplashYVelocity = ReadU16(s);
        mTimeout = ReadU16(s);
    }
};

struct AeWorkWheel
{
    const static u8 kSizeInBytes = 28;

    u16 mScale;
    u16 mId;
    u16 mDuration;
    u16 mOffTime;
    u32 mOffWhenStopped;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mId = ReadU16(s);
        mDuration = ReadU16(s);
        mOffTime = ReadU16(s);
        mOffWhenStopped = ReadU32(s);
    }
};

struct AeLaughingGas
{
    const static u8 kSizeInBytes = 28;

    u16 mIsLaughingGas;
    u16 mGasId;
    u16 mRedPercent;
    u16 mGreenPercent;
    u32 mBluePercent;

    void Deserialize(Oddlib::IStream& s)
    {
        mIsLaughingGas = ReadU16(s);
        mGasId = ReadU16(s);
        mRedPercent = ReadU16(s);
        mGreenPercent = ReadU16(s);
        mBluePercent = ReadU32(s);
    }
};

struct AeFlyingSlig
{
    const static u8 kSizeInBytes = 48;

    u16 mScale;
    u16 mState;
    u16 mHiPauseTime;
    u16 mPatrolPauseMin;
    u16 mPatrolPauseMax;
    u16 mDirection;
    u16 mPanicDelay;
    u16 mGiveUpChaseDelay;
    u16 mPrechaseDelay;
    u16 mSligId;
    u16 mListenTime;
    u16 mTriggerId;
    u16 mGrenadeDelay;
    u16 mMaxVelocity;
    u16 mLaunchId;
    u16 mPersistant;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mState = ReadU16(s);
        mHiPauseTime = ReadU16(s);
        mPatrolPauseMin = ReadU16(s);
        mPatrolPauseMax = ReadU16(s);
        mDirection = ReadU16(s);
        mPanicDelay = ReadU16(s);
        mGiveUpChaseDelay = ReadU16(s);
        mPrechaseDelay = ReadU16(s);
        mSligId = ReadU16(s);
        mListenTime = ReadU16(s);
        mTriggerId = ReadU16(s);
        mGrenadeDelay = ReadU16(s);
        mMaxVelocity = ReadU16(s);
        mLaunchId = ReadU16(s);
        mPersistant = ReadU16(s);
    }
};

struct AeFleech
{
    const static u8 kSizeInBytes = 48;

    u16 mScale;
    u16 mDirection;
    u16 mAsleep;
    u16 mWakeUp;
    u16 mNotUsed;
    u16 mAttackAnger;
    u16 mAttackDelay;
    u16 mWakeUpId;
    u16 mHanging;
    u16 mLostTargetTimeout;
    u16 mGoesToSleep;
    u16 mPatrolRange;
    u16 mUnused;
    u16 mAllowWakeUpId;
    u16 mPersistant;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mDirection = ReadU16(s);
        mAsleep = ReadU16(s);
        mWakeUp = ReadU16(s);
        mNotUsed = ReadU16(s);
        mAttackAnger = ReadU16(s);
        mAttackDelay = ReadU16(s);
        mWakeUpId = ReadU16(s);
        mHanging = ReadU16(s);
        mLostTargetTimeout = ReadU16(s);
        mGoesToSleep = ReadU16(s);
        mPatrolRange = ReadU16(s);
        mUnused = ReadU16(s);
        mAllowWakeUpId = ReadU16(s);
        mPersistant = ReadU16(s);
    }
};

struct AeSlurgs
{
    const static u8 kSizeInBytes = 24;

    u16 mPauseDelay;
    u16 mDirection;
    u16 mScale;
    u16 mId;

    void Deserialize(Oddlib::IStream& s)
    {
        mPauseDelay = ReadU16(s);
        mDirection = ReadU16(s);
        mScale = ReadU16(s);
        mId = ReadU16(s);
    }
};

struct AeSlamDoor
{
    const static u8 kSizeInBytes = 28;

    u16 mStartsShut;
    u16 mHalfScale;
    u16 mId;
    u16 mInverted;
    u32 mDelete;

    void Deserialize(Oddlib::IStream& s)
    {
        mStartsShut = ReadU16(s);
        mHalfScale = ReadU16(s);
        mId = ReadU16(s);
        mInverted = ReadU16(s);
        mDelete = ReadU32(s);
    }
};

struct AeLevelLoader
{
    const static u8 kSizeInBytes = 28;

    u16 mId;
    u16 mDestLevel;
    u16 mDestPath;
    u16 mDestCamera;
    u32 mMovie;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mDestLevel = ReadU16(s);
        mDestPath = ReadU16(s);
        mDestCamera = ReadU16(s);
        mMovie = ReadU32(s);
    }
};

struct AeDemoSpawnPoint
{
    const static u8 kSizeInBytes = 16;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeTeleporter
{
    const static u8 kSizeInBytes = 40;

    u16 mId;
    u16 mTargetId;
    u16 mCamera;
    u16 mPath;
    u16 mLevel;
    u16 mTriggerId;
    u16 mHalfScale;
    u16 mWipe;
    u16 mMovieNumber;
    u16 mElectricX;
    u32 mElectricY;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mTargetId = ReadU16(s);
        mCamera = ReadU16(s);
        mPath = ReadU16(s);
        mLevel = ReadU16(s);
        mTriggerId = ReadU16(s);
        mHalfScale = ReadU16(s);
        mWipe = ReadU16(s);
        mMovieNumber = ReadU16(s);
        mElectricX = ReadU16(s);
        mElectricY = ReadU32(s);
    }
};

struct AeSlurgSpawner
{
    const static u8 kSizeInBytes = 32;

    u16 mPauseDelay;
    u16 mDirection;
    u16 mScale;
    u16 mId;
    u16 mDelayBetweenSlurgs;
    u16 mMaxSlurgs;
    u32 mSpawnerId;

    void Deserialize(Oddlib::IStream& s)
    {
        mPauseDelay = ReadU16(s);
        mDirection = ReadU16(s);
        mScale = ReadU16(s);
        mId = ReadU16(s);
        mDelayBetweenSlurgs = ReadU16(s);
        mMaxSlurgs = ReadU16(s);
        mSpawnerId = ReadU32(s);
    }
};

struct AeMineDrill
{
    const static u8 kSizeInBytes = 40;

    u16 mScale;
    u16 mMinOffTime;
    u16 mMaxOffTime;
    u16 mId;
    u16 mBehavior;
    u16 mSpeed;
    u16 mStartState;
    u16 mOffSpeed;
    u16 mMinOffTime2;
    u16 mMaxOffTime2;
    u16 mStartPosition;
    u16 mDirection;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mMinOffTime = ReadU16(s);
        mMaxOffTime = ReadU16(s);
        mId = ReadU16(s);
        mBehavior = ReadU16(s);
        mSpeed = ReadU16(s);
        mStartState = ReadU16(s);
        mOffSpeed = ReadU16(s);
        mMinOffTime2 = ReadU16(s);
        mMaxOffTime2 = ReadU16(s);
        mStartPosition = ReadU16(s);
        mDirection = ReadU16(s);
    }
};

struct AeColorfulMeter
{
    const static u8 kSizeInBytes = 24;

    u16 mId;
    u16 mNumberOfMeterBars;
    u16 mTimer;
    u16 mStartsFull;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mNumberOfMeterBars = ReadU16(s);
        mTimer = ReadU16(s);
        mStartsFull = ReadU16(s);
    }
};

struct AeFlyingSligSpawner
{
    const static u8 kSizeInBytes = 48;

    u16 mScale;
    u16 mState;
    u16 mHiPauseTime;
    u16 mPatrolPauseMin;
    u16 mPatrolPauseMax;
    u16 mDirection;
    u16 mPanicDelay;
    u16 mGiveUpChaseDelay;
    u16 mPrechaseDelay;
    u16 mSligId;
    u16 mListenTime;
    u16 mTriggerId;
    u16 mGrenadeDelay;
    u16 mMaxVelocity;
    u16 mLaunchId;
    u16 mPersistant;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mState = ReadU16(s);
        mHiPauseTime = ReadU16(s);
        mPatrolPauseMin = ReadU16(s);
        mPatrolPauseMax = ReadU16(s);
        mDirection = ReadU16(s);
        mPanicDelay = ReadU16(s);
        mGiveUpChaseDelay = ReadU16(s);
        mPrechaseDelay = ReadU16(s);
        mSligId = ReadU16(s);
        mListenTime = ReadU16(s);
        mTriggerId = ReadU16(s);
        mGrenadeDelay = ReadU16(s);
        mMaxVelocity = ReadU16(s);
        mLaunchId = ReadU16(s);
        mPersistant = ReadU16(s);
    }
};

struct AeMineCar
{
    const static u8 kSizeInBytes = 20;

    u16 mScale;
    u16 mMaxDamage;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mMaxDamage = ReadU16(s);
    }
};

struct AeBoneBag
{
    const static u8 kSizeInBytes = 28;

    u16 mSide;
    u16 mXVel;
    u16 mYVel;
    u16 mScale;
    u16 mNumberOfBones;

    void Deserialize(Oddlib::IStream& s)
    {
        mSide = ReadU16(s);
        mXVel = ReadU16(s);
        mYVel = ReadU16(s);
        mScale = ReadU16(s);
        mNumberOfBones = ReadU16(s);
    }
};

struct AeExplosionSet
{
    const static u8 kSizeInBytes = 36;

    u16 mStartInstantly;
    u16 mId;
    u16 mBigRocks;
    u16 mStartDelay;
    u16 mSequnceDirection;
    u16 mSequnceDelay;
    u16 mSpacing;
    u16 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mStartInstantly = ReadU16(s);
        mId = ReadU16(s);
        mBigRocks = ReadU16(s);
        mStartDelay = ReadU16(s);
        mSequnceDirection = ReadU16(s);
        mSequnceDelay = ReadU16(s);
        mSpacing = ReadU16(s);
        mScale = ReadU16(s);
    }
};

struct AeMultiswitchController
{
    const static u8 kSizeInBytes = 36;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeRedGreenStatusLight
{
    const static u8 kSizeInBytes = 32;

    u16 mId;
    u16 mScale;
    u16 mOtherId1;
    u16 mOtherId2;
    u16 mOtherId3;
    u16 mOtherId4;
    u16 mOtherId5;
    u16 mSnapToGrid;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mScale = ReadU16(s);
        mOtherId1 = ReadU16(s);
        mOtherId2 = ReadU16(s);
        mOtherId3 = ReadU16(s);
        mOtherId4 = ReadU16(s);
        mOtherId5 = ReadU16(s);
        mSnapToGrid = ReadU16(s);
    }
};

struct AeGhostLock
{
    const static u8 kSizeInBytes = 32;

    u16 mScale;
    u16 mTargetTombId1;
    u16 mTargetTombId2;
    u16 mIsPersistant;
    u16 mHasGhost;
    u16 mHasPowerup;
    u16 mPowerupId;
    u16 mOptionId;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mTargetTombId1 = ReadU16(s);
        mTargetTombId2 = ReadU16(s);
        mIsPersistant = ReadU16(s);
        mHasGhost = ReadU16(s);
        mHasPowerup = ReadU16(s);
        mPowerupId = ReadU16(s);
        mOptionId = ReadU16(s);
    }
};

struct AeParamiteNet
{
    const static u8 kSizeInBytes = 20;

    u16 mScale;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
    }
};

struct AeAlarm
{
    const static u8 kSizeInBytes = 20;

    u16 mId;
    u16 mDuration;

    void Deserialize(Oddlib::IStream& s)
    {
        mId = ReadU16(s);
        mDuration = ReadU16(s);
    }
};

struct AeFartMachine
{
    const static u8 kSizeInBytes = 20;

    u16 mNumberOfBrews;

    void Deserialize(Oddlib::IStream& s)
    {
        mNumberOfBrews = ReadU16(s);
    }
};

struct AeScrabSpawner
{
    const static u8 kSizeInBytes = 48;

    u16 mScale;
    u16 mAttackDelay;
    u16 mPatrolType;
    u16 mLeftMinDelay;
    u16 mLeftMaxDelay;
    u16 mRightMinDelay;
    u16 mRightMaxDelay;
    u16 mAttackDuration;
    u16 mDisableResources;
    u16 mRoarRandomly;
    u16 mPersistant;
    u16 mWhirlAttackDuration;
    u16 mWhirlAttackRecharge;
    u16 mKillCloseFleech;
    u16 mId;
    u16 mAppearFrom;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mAttackDelay = ReadU16(s);
        mPatrolType = ReadU16(s);
        mLeftMinDelay = ReadU16(s);
        mLeftMaxDelay = ReadU16(s);
        mRightMinDelay = ReadU16(s);
        mRightMaxDelay = ReadU16(s);
        mAttackDuration = ReadU16(s);
        mDisableResources = ReadU16(s);
        mRoarRandomly = ReadU16(s);
        mPersistant = ReadU16(s);
        mWhirlAttackDuration = ReadU16(s);
        mWhirlAttackRecharge = ReadU16(s);
        mKillCloseFleech = ReadU16(s);
        mId = ReadU16(s);
        mAppearFrom = ReadU16(s);
    }
};

struct AeCrawlingSlig
{
    const static u8 kSizeInBytes = 28;

    u16 mScale;
    u16 mDirection;
    u16 mState;
    u16 mLockerDirection;
    u16 mPanicId;
    u16 mResetOnDeath;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mDirection = ReadU16(s);
        mState = ReadU16(s);
        mLockerDirection = ReadU16(s);
        mPanicId = ReadU16(s);
        mResetOnDeath = ReadU16(s);
    }
};

struct AeSligGetPants
{
    const static u8 kSizeInBytes = 80;

    void Deserialize(Oddlib::IStream& s)
    {
        std::ignore = s;
    }
};

struct AeSligGetWings
{
    const static u8 kSizeInBytes = 48;

    u16 mScale;
    u16 mState;
    u16 mHiPauseTime;
    u16 mPatrolPauseMin;
    u16 mPatrolPauseMax;
    u16 mDirection;
    u16 mPanicDelay;
    u16 mGiveUpChaseDelay;
    u16 mPrechaseDelay;
    u16 mSligId;
    u16 mListenTime;
    u16 mTriggerId;
    u16 mGrenadeDelay;
    u16 mMaxVelocity;
    u16 mLaunchId;
    u16 mPersistant;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mState = ReadU16(s);
        mHiPauseTime = ReadU16(s);
        mPatrolPauseMin = ReadU16(s);
        mPatrolPauseMax = ReadU16(s);
        mDirection = ReadU16(s);
        mPanicDelay = ReadU16(s);
        mGiveUpChaseDelay = ReadU16(s);
        mPrechaseDelay = ReadU16(s);
        mSligId = ReadU16(s);
        mListenTime = ReadU16(s);
        mTriggerId = ReadU16(s);
        mGrenadeDelay = ReadU16(s);
        mMaxVelocity = ReadU16(s);
        mLaunchId = ReadU16(s);
        mPersistant = ReadU16(s);
    }
};

struct AeGreeter
{
    const static u8 kSizeInBytes = 24;

    u16 mScale;
    u16 mMotionDetectorSpeed;
    u16 mDirection;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mMotionDetectorSpeed = ReadU16(s);
        mDirection = ReadU16(s);
    }
};

struct AeCrawlingSligButton
{
    const static u8 kSizeInBytes = 28;

    u16 mScale;
    u16 mId;
    u16 mIdAction;
    u16 mOnSound;
    u16 mOffSound;
    u16 mSoundDirection;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mId = ReadU16(s);
        mIdAction = ReadU16(s);
        mOnSound = ReadU16(s);
        mOffSound = ReadU16(s);
        mSoundDirection = ReadU16(s);
    }
};

struct AeGlukkonSecurityFone
{
    const static u8 kSizeInBytes = 28;

    u16 mScale;
    u16 mOkId;
    u16 mFailId;
    u16 mXPos;
    u32 mYPos;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mOkId = ReadU16(s);
        mFailId = ReadU16(s);
        mXPos = ReadU16(s);
        mYPos = ReadU32(s);
    }
};

struct AeDoorBlocker
{
    const static u8 kSizeInBytes = 20;

    u16 mScale;
    u16 mId;

    void Deserialize(Oddlib::IStream& s)
    {
        mScale = ReadU16(s);
        mId = ReadU16(s);
    }
};

struct AeTorturedMudokon
{
    const static u8 kSizeInBytes = 20;

    u16 mSpeedId;
    u16 mReleaseId;

    void Deserialize(Oddlib::IStream& s)
    {
        mSpeedId = ReadU16(s);
        mReleaseId = ReadU16(s);
    }
};

struct AeTrainDoor
{
    const static u8 kSizeInBytes = 20;

    u32 mXFlip;

    void Deserialize(Oddlib::IStream& s)
    {
        mXFlip = ReadU32(s);
    }
};

static Entity MakeContinuePoint(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeContinuePoint continuePoint;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    continuePoint.Deserialize(ms);

    return entity;
}

static Entity MakePathTransition(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AePathTransition pathTransition;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    pathTransition.Deserialize(ms);

    return entity;
}

static Entity MakeHoist(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeHoist hoist;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    hoist.Deserialize(ms);

    return entity;
}

static Entity MakeEdge(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeEdge edge;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    edge.Deserialize(ms);

    return entity;
}

static Entity MakeDeathDrop(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeDeathDrop deathDrop;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    deathDrop.Deserialize(ms);

    return entity;
}

static Entity MakeDoor(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeDoor door;

    door.Deserialize(ms);

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + door.mXOffset + 5, static_cast<float>(object.mRectTopLeft.mY) + object.mRectBottomRight.mY - static_cast<float>(object.mRectTopLeft.mY) + door.mYOffset);
    entity.GetComponent<AnimationComponent>()->Change("DoorClosed_Barracks");

    return entity;
}

static Entity MakeShadow(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeShadow shadow;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    shadow.Deserialize(ms);

    return entity;
}

static Entity MakeLiftPoint(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeLiftPoint liftPoint;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    liftPoint.Deserialize(ms);

    return entity;
}

static Entity MakeWellLocal(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeWellLocal wellLocal;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    wellLocal.Deserialize(ms);

    return entity;
}

static Entity MakeDove(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeDove dove;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("DOVBASIC.BAN_60_AePc_2");
    dove.Deserialize(ms);

    return entity;
}

static Entity MakeRockSack(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeRockSack rockSack;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("MIP04C01.CAM_1002_AePc_2");
    rockSack.Deserialize(ms);

    return entity;
}

static Entity MakeFallingItem(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeFallingItem fallingItem;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("FALLBONZ.BAN_2007_AePc_0");
    fallingItem.Deserialize(ms);

    return entity;
}

static Entity MakePullRingRope(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AePullRingRope pullRingRope;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("PULLRING.BAN_1014_AePc_1");
    pullRingRope.Deserialize(ms);

    return entity;
}

static Entity MakeBackgroundAnimation(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeBackgroundAnimation backgroundAnimation;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));

    backgroundAnimation.Deserialize(ms);

    switch (backgroundAnimation.mAnimationRes)
    {
    case 1201:
        entity.GetComponent<AnimationComponent>()->Change("BAP01C06.CAM_1201_AePc_0");
        entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + 5, static_cast<float>(object.mRectTopLeft.mY) + 3);
        break;
    case 1202:
        entity.GetComponent<AnimationComponent>()->Change("FARTFAN.BAN_1202_AePc_0");
        break;
    default:
        entity.GetComponent<AnimationComponent>()->Change("AbeStandSpeak1");
    }

    return entity;
}

static Entity MakeTimedMine(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeTimedMine timedMine;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BOMB.BND_1005_AePc_1");
    timedMine.Deserialize(ms);

    return entity;
}

static Entity MakeSlig(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSlig slig;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("SligStandIdle");
    slig.Deserialize(ms);

    return entity;
}

static Entity MakeSlog(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSlog slog;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("SLOG.BND_570_AePc_3");
    slog.Deserialize(ms);

    return entity;
}

static Entity MakeSwitch(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSwitch switchObj;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + 37, static_cast<float>(object.mRectTopLeft.mY) - 5);
    entity.GetComponent<AnimationComponent>()->Change("SwitchIdle");
    switchObj.Deserialize(ms);

    return entity;
}

static Entity MakeSecurityEye(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSecurityEye securityEye;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("MAIMORB.BAN_2006_AePc_0");
    securityEye.Deserialize(ms);

    return entity;
}

static Entity MakePulley(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AePulley pulley;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BALIFT.BND_1001_AePc_0");
    pulley.Deserialize(ms);

    return entity;
}

static Entity MakeAbeStart(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeAbeStart abeStart;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    abeStart.Deserialize(ms);

    return entity;
}

static Entity MakeWellExpress(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeWellExpress wellExpress;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    wellExpress.Deserialize(ms);

    return entity;
}

static Entity MakeMine(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeMine mine;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("LANDMINE.BAN_1036_AePc_0");
    mine.Deserialize(ms);

    return entity;
}

static Entity MakeUxb(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeUxb uxb;

    uxb.Deserialize(ms);

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));

    if (uxb.mState == 0) // On
    {
        entity.GetComponent<AnimationComponent>()->Change("TBOMB.BAN_1037_AePc_1");
    }
    else if (uxb.mState == 1) // Off
    {
        entity.GetComponent<AnimationComponent>()->Change("TBOMB.BAN_1037_AePc_0");
    }

    return entity;
}

static Entity MakeParamite(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeParamite paramite;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("PARAMITE.BND_600_AePc_4");
    paramite.Deserialize(ms);

    return entity;
}

static Entity MakeMovieStone(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeMovieStone movieStone;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    movieStone.Deserialize(ms);

    return entity;
}

static Entity MakeBirdPortal(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeBirdPortal birdPortal;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    birdPortal.Deserialize(ms);

    return entity;
}

static Entity MakePortalExit(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AePortalExit portalExit;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("PORTAL.BND_351_AePc_0");
    portalExit.Deserialize(ms);

    return entity;
}

static Entity MakeTrapDoor(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeTrapDoor trapDoor;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));

    trapDoor.Deserialize(ms);

    if (trapDoor.mStartState == 0) // closed
    {
        entity.GetComponent<AnimationComponent>()->Change("TRAPDOOR.BAN_1004_AePc_1");
    }
    else if (trapDoor.mStartState == 1) // open
    {
        entity.GetComponent<AnimationComponent>()->Change("TRAPDOOR.BAN_1004_AePc_2");
    }

    return entity;
}

static Entity MakeRollingBall(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeRollingBall rollingBall;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    rollingBall.Deserialize(ms);

    return entity;
}

static Entity MakeSligLeftBound(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSligLeftBound sligLeftBound;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    sligLeftBound.Deserialize(ms);

    return entity;
}

static Entity MakeInvisibleZone(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeInvisibleZone invisibleZone;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    invisibleZone.Deserialize(ms);

    return entity;
}

static Entity MakeFootSwitch(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeFootSwitch footSwitch;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("TRIGGER.BAN_2010_AePc_0");
    footSwitch.Deserialize(ms);

    return entity;
}

static Entity MakeSecurityOrb(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSecurityOrb securityOrb;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    securityOrb.Deserialize(ms);

    return entity;
}

static Entity MakeMotionDetector(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeMotionDetector motionDetector;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("MOTION.BAN_6001_AePc_0");
    motionDetector.Deserialize(ms);

    return entity;
}

static Entity MakeSligSpawner(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSligSpawner sligSpawner;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    sligSpawner.Deserialize(ms);

    return entity;
}

static Entity MakeElectricWall(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeElectricWall electricWall;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("ELECWALL.BAN_6000_AePc_0");
    electricWall.Deserialize(ms);

    return entity;
}

static Entity MakeLiftMover(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeLiftMover liftMover;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BALIFT.BND_1001_AePc_1");
    liftMover.Deserialize(ms);

    return entity;
}

static Entity MakeMeatSack(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeMeatSack meatSack;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BONEBAG.BAN_590_AePc_2");
    meatSack.Deserialize(ms);

    return entity;
}

static Entity MakeScrab(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeScrab scrab;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("SCRAB.BND_700_AePc_2");
    scrab.Deserialize(ms);

    return entity;
}

static Entity MakeScrabLeftBound(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeScrabLeftBound scrabLeftBound;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    scrabLeftBound.Deserialize(ms);

    return entity;
}

static Entity MakeScrabRightBound(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeScrabRightBound scrabRightBound;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    scrabRightBound.Deserialize(ms);

    return entity;
}

static Entity MakeSligRightBound(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSligRightBound sligRightBound;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    sligRightBound.Deserialize(ms);

    return entity;
}

static Entity MakeSligPersist(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSligPersist sligPersist;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    sligPersist.Deserialize(ms);

    return entity;
}

static Entity MakeEnemyStopper(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeEnemyStopper enemyStopper;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    enemyStopper.Deserialize(ms);

    return entity;
}

static Entity MakeInvisibleSwitch(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeInvisibleSwitch invisibleSwitch;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    invisibleSwitch.Deserialize(ms);

    return entity;
}

static Entity MakeMudokon(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeMudokon mudokon;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("MUDCHSL.BAN_511_AePc_1");
    mudokon.Deserialize(ms);

    return entity;
}

static Entity MakeZSligCover(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeZSligCover zSligCover;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    zSligCover.Deserialize(ms);

    return entity;
}

static Entity MakeDoorFlame(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeDoorFlame doorFlame;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("FIRE.BAN_304_AePc_0");
    doorFlame.Deserialize(ms);

    return entity;
}

static Entity MakeMovingBomb(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeMovingBomb movingBomb;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("MOVEBOMB.BAN_3006_AePc_0");
    movingBomb.Deserialize(ms);

    return entity;
}

static Entity MakeMenuController(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeMenuController menuController;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    menuController.Deserialize(ms);

    return entity;
}

static Entity MakeTimerTrigger(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeTimerTrigger timerTrigger;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    timerTrigger.Deserialize(ms);

    return entity;
}

static Entity MakeSligVoiceLock(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSligVoiceLock sligVoiceLock;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("SECDOOR.BAN_6027_AePc_1");
    sligVoiceLock.Deserialize(ms);

    return entity;
}

static Entity MakeGrenadeMachine(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeGrenadeMachine grenadeMachine;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BAP01C13.CAM_6008_AePc_1");
    grenadeMachine.Deserialize(ms);

    return entity;
}

static Entity MakeLcdScreen(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeLcdScreen lcdScreen;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    lcdScreen.Deserialize(ms);

    return entity;
}

static Entity MakeHandStone(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeHandStone handStone;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    handStone.Deserialize(ms);

    return entity;
}

static Entity MakeCreditsController(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeCreditsController creditsController;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    creditsController.Deserialize(ms);

    return entity;
}

static Entity MakeNullObject1(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeNullObject1 nullObject;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    nullObject.Deserialize(ms);

    return entity;
}

static Entity MakeLcdStatusBoard(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeLcdStatusBoard lcdStatusBoard;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    lcdStatusBoard.Deserialize(ms);

    return entity;
}

static Entity MakeWorkWheelSyncer(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeWorkWheelSyncer workWheelSyncer;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    workWheelSyncer.Deserialize(ms);

    return entity;
}

static Entity MakeMusic(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeMusic music;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    music.Deserialize(ms);

    return entity;
}

static Entity MakeLightEffect(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeLightEffect lightEffect;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    lightEffect.Deserialize(ms);

    return entity;
}

static Entity MakeSlogSpawner(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSlogSpawner slogSpawner;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    slogSpawner.Deserialize(ms);

    return entity;
}

static Entity MakeDeathClock(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeDeathClock deathClock;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    deathClock.Deserialize(ms);

    return entity;
}

static Entity MakeGasEmitter(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeGasEmitter gasEmitter;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    gasEmitter.Deserialize(ms);

    return entity;
}

static Entity MakeSlogHut(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSlogHut slogHut;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    slogHut.Deserialize(ms);

    return entity;
}

static Entity MakeGlukkon(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeGlukkon glukkon;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("GLUKKON.BND_800_AePc_1");
    glukkon.Deserialize(ms);

    return entity;
}

static Entity MakeKillUnsavedMuds(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeKillUnsavedMuds killUnsavedMuds;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    killUnsavedMuds.Deserialize(ms);

    return entity;
}

static Entity MakeSoftLanding(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSoftLanding softLanding;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    softLanding.Deserialize(ms);

    return entity;
}

static Entity MakeNullObject2(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeNullObject2 nullObject;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    nullObject.Deserialize(ms);

    return entity;
}

static Entity MakeWater(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeWater water;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    water.Deserialize(ms);

    return entity;
}

static Entity MakeWorkWheel(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeWorkWheel workWheel;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("WORKWHEL.BAN_320_AePc_1");
    workWheel.Deserialize(ms);

    return entity;
}

static Entity MakeLaughingGas(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeLaughingGas laughingGas;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    laughingGas.Deserialize(ms);

    return entity;
}

static Entity MakeFlyingSlig(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeFlyingSlig flyingSlig;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("FLYSLIG.BND_450_AePc_1");
    flyingSlig.Deserialize(ms);

    return entity;
}

static Entity MakeFleech(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeFleech fleech;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("FLEECH.BAN_900_AePc_1");
    fleech.Deserialize(ms);

    return entity;
}

static Entity MakeSlurgs(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSlurgs slurgs;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("SLURG.BAN_306_AePc_3");
    slurgs.Deserialize(ms);

    return entity;
}

static Entity MakeSlamDoor(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSlamDoor slamDoor;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + 13, static_cast<float>(object.mRectTopLeft.mY) + 18);
    entity.GetComponent<AnimationComponent>()->Change("SLAM.BAN_2020_AePc_1");
    slamDoor.Deserialize(ms);

    return entity;
}

static Entity MakeLevelLoader(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeLevelLoader levelLoader;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    levelLoader.Deserialize(ms);

    return entity;
}

static Entity MakeDemoSpawnPoint(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeDemoSpawnPoint demoSpawnPoint;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    demoSpawnPoint.Deserialize(ms);

    return entity;
}

static Entity MakeTeleporter(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeTeleporter teleporter;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    teleporter.Deserialize(ms);

    return entity;
}

static Entity MakeSlurgSpawner(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeSlurgSpawner slurgSpawner;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    slurgSpawner.Deserialize(ms);

    return entity;
}

static Entity MakeMineDrill(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeMineDrill mineDrill;

    mineDrill.Deserialize(ms);

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));

    if (mineDrill.mDirection == 0) // Down
    {
        if (mineDrill.mStartState == 0) // Off
        {
            entity.GetComponent<AnimationComponent>()->Change("DRILL.BAN_6004_AePc_1");
        }
        else if (mineDrill.mStartState == 1) // On
        {
            entity.GetComponent<AnimationComponent>()->Change("DRILL.BAN_6004_AePc_2");
        }
    }
    else if (mineDrill.mStartState == 1 || mineDrill.mStartState == 2) // Right || Left
    {
        if (mineDrill.mStartState == 0) // Off
        {
            entity.GetComponent<AnimationComponent>()->Change("DRILL.BAN_6004_AePc_0");
        }
        else if (mineDrill.mStartState == 1) // On
        {
            entity.GetComponent<AnimationComponent>()->Change("DRILL.BAN_6004_AePc_4");
        }
    }

    return entity;
}

static Entity MakeColorfulMeter(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeColorfulMeter colorfulMeter;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    colorfulMeter.Deserialize(ms);

    return entity;
}

static Entity MakeFlyingSligSpawner(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeFlyingSligSpawner flyingSligSpawner;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    flyingSligSpawner.Deserialize(ms);

    return entity;
}

static Entity MakeMineCar(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeMineCar mineCar;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BAYROLL.BAN_6013_AePc_2");
    mineCar.Deserialize(ms);

    return entity;
}

static Entity MakeBoneBag(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeBoneBag boneBag;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BONEBAG.BAN_590_AePc_2");
    boneBag.Deserialize(ms);

    return entity;
}

static Entity MakeExplosionSet(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeExplosionSet explosionSet;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    explosionSet.Deserialize(ms);

    return entity;
}

static Entity MakeMultiswitchController(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeMultiswitchController multiswitchController;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    multiswitchController.Deserialize(ms);

    return entity;
}

static Entity MakeRedGreenStatusLight(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeRedGreenStatusLight redGreenStatusLight;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("STATUSLT.BAN_373_AePc_1");
    redGreenStatusLight.Deserialize(ms);

    return entity;
}

static Entity MakeGhostLock(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeGhostLock ghostLock;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("GHOSTTRP.BAN_1053_AePc_0");
    ghostLock.Deserialize(ms);

    return entity;
}

static Entity MakeParamiteNet(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeParamiteNet paramiteNet;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("WEB.BAN_2034_AePc_0");
    paramiteNet.Deserialize(ms);

    return entity;
}

static Entity MakeAlarm(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeAlarm alarm;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    alarm.Deserialize(ms);

    return entity;
}

static Entity MakeFartMachine(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeFartMachine fartMachine;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("BREWBTN.BAN_6016_AePc_0");
    fartMachine.Deserialize(ms);

    return entity;
}

static Entity MakeScrabSpawner(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeScrabSpawner scrabSpawner;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    scrabSpawner.Deserialize(ms);

    return entity;
}

static Entity MakeCrawlingSlig(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeCrawlingSlig crawlingSlig;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("CRAWLSLG.BND_449_AePc_3");
    crawlingSlig.Deserialize(ms);

    return entity;
}

static Entity MakeSligGetPants(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSligGetPants sligGetPants;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("LOCKER.BAN_448_AePc_1");
    sligGetPants.Deserialize(ms);

    return entity;
}

static Entity MakeSligGetWings(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeSligGetWings sligGetWings;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("LOCKER.BAN_448_AePc_1");
    sligGetWings.Deserialize(ms);

    return entity;
}

static Entity MakeGreeter(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeGreeter greeter;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("GREETER.BAN_307_AePc_0");
    greeter.Deserialize(ms);

    return entity;
}

static Entity MakeCrawlingSligButton(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeCrawlingSligButton crawlingSligButton;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("CSLGBUTN.BAN_1057_AePc_0");
    crawlingSligButton.Deserialize(ms);

    return entity;
}

static Entity MakeGlukkonSecurityFone(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeGlukkonSecurityFone glukkonSecurityFone;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("SECDOOR.BAN_6027_AePc_1");
    glukkonSecurityFone.Deserialize(ms);

    return entity;
}

static Entity MakeDoorBlocker(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent>();
    AeDoorBlocker doorBlocker;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    doorBlocker.Deserialize(ms);

    return entity;
}

static Entity MakeTorturedMudokon(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeTorturedMudokon torturedMudokon;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("MUDTORT.BAN_518_AePc_2");
    torturedMudokon.Deserialize(ms);

    return entity;
}

static Entity MakeTrainDoor(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    auto entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
    AeTrainDoor trainDoor;

    entity.GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY), static_cast<float>(object.mRectBottomRight.mX - object.mRectTopLeft.mX), static_cast<float>(object.mRectBottomRight.mY - object.mRectTopLeft.mY));
    entity.GetComponent<AnimationComponent>()->Change("TRAINDOR.BAN_2013_AePc_1");
    trainDoor.Deserialize(ms);

    return entity;
}

static std::map<const u32, std::function<Entity(const Oddlib::Path::MapObject&, EntityManager&, Oddlib::MemoryStream&)>> sEnumMap =
    {
        { ObjectTypesAe::eContinuePoint, MakeContinuePoint },
        { ObjectTypesAe::ePathTransition, MakePathTransition },
        { ObjectTypesAe::eHoist, MakeHoist },
        { ObjectTypesAe::eEdge, MakeEdge },
        { ObjectTypesAe::eDeathDrop, MakeDeathDrop },
        { ObjectTypesAe::eDoor, MakeDoor },
        { ObjectTypesAe::eShadow, MakeShadow },
        { ObjectTypesAe::eLiftPoint, MakeLiftPoint },
        { ObjectTypesAe::eWellLocal, MakeWellLocal },
        { ObjectTypesAe::eDove, MakeDove },
        { ObjectTypesAe::eRockSack, MakeRockSack },
        { ObjectTypesAe::eFallingItem, MakeFallingItem },
        { ObjectTypesAe::ePullRingRope, MakePullRingRope },
        { ObjectTypesAe::eBackgroundAnimation, MakeBackgroundAnimation },
        { ObjectTypesAe::eTimedMine, MakeTimedMine },
        { ObjectTypesAe::eSlig, MakeSlig },
        { ObjectTypesAe::eSlog, MakeSlog },
        { ObjectTypesAe::eSwitch, MakeSwitch },
        { ObjectTypesAe::eSecurityEye, MakeSecurityEye },
        { ObjectTypesAe::ePulley, MakePulley },
        { ObjectTypesAe::eAbeStart, MakeAbeStart },
        { ObjectTypesAe::eWellExpress, MakeWellExpress },
        { ObjectTypesAe::eMine, MakeMine },
        { ObjectTypesAe::eUxb, MakeUxb },
        { ObjectTypesAe::eParamite, MakeParamite },
        { ObjectTypesAe::eMovieStone, MakeMovieStone },
        { ObjectTypesAe::eBirdPortal, MakeBirdPortal },
        { ObjectTypesAe::ePortalExit, MakePortalExit },
        { ObjectTypesAe::eTrapDoor, MakeTrapDoor },
        { ObjectTypesAe::eRollingBall, MakeRollingBall },
        { ObjectTypesAe::eSligLeftBound, MakeSligLeftBound },
        { ObjectTypesAe::eInvisibleZone, MakeInvisibleZone },
        { ObjectTypesAe::eFootSwitch, MakeFootSwitch },
        { ObjectTypesAe::eSecurityOrb, MakeSecurityOrb },
        { ObjectTypesAe::eMotionDetector, MakeMotionDetector },
        { ObjectTypesAe::eSligSpawner, MakeSligSpawner },
        { ObjectTypesAe::eElectricWall, MakeElectricWall },
        { ObjectTypesAe::eLiftMover, MakeLiftMover },
        { ObjectTypesAe::eMeatSack, MakeMeatSack },
        { ObjectTypesAe::eScrab, MakeScrab },
        { ObjectTypesAe::eScrabLeftBound, MakeScrabLeftBound },
        { ObjectTypesAe::eScrabRightBound, MakeScrabRightBound },
        { ObjectTypesAe::eSligRightBound, MakeSligRightBound },
        { ObjectTypesAe::eSligPersist, MakeSligPersist },
        { ObjectTypesAe::eEnemyStopper, MakeEnemyStopper },
        { ObjectTypesAe::eInvisibleSwitch, MakeInvisibleSwitch },
        { ObjectTypesAe::eMudokon, MakeMudokon },
        { ObjectTypesAe::eZSligCover, MakeZSligCover },
        { ObjectTypesAe::eDoorFlame, MakeDoorFlame },
        { ObjectTypesAe::eMovingBomb, MakeMovingBomb },
        { ObjectTypesAe::eMenuController, MakeMenuController },
        { ObjectTypesAe::eTimerTrigger, MakeTimerTrigger },
        { ObjectTypesAe::eSligVoiceLock, MakeSligVoiceLock },
        { ObjectTypesAe::eGrenadeMachine, MakeGrenadeMachine },
        { ObjectTypesAe::eLcdScreen, MakeLcdScreen },
        { ObjectTypesAe::eHandStone, MakeHandStone },
        { ObjectTypesAe::eCreditsController, MakeCreditsController },
        { ObjectTypesAe::eNullObject1, MakeNullObject1 },
        { ObjectTypesAe::eLcdStatusBoard, MakeLcdStatusBoard },
        { ObjectTypesAe::eWorkWheelSyncer, MakeWorkWheelSyncer },
        { ObjectTypesAe::eMusic, MakeMusic },
        { ObjectTypesAe::eLightEffect, MakeLightEffect },
        { ObjectTypesAe::eSlogSpawner, MakeSlogSpawner },
        { ObjectTypesAe::eDeathClock, MakeDeathClock },
        { ObjectTypesAe::eGasEmitter, MakeGasEmitter },
        { ObjectTypesAe::eSlogHut, MakeSlogHut },
        { ObjectTypesAe::eGlukkon, MakeGlukkon },
        { ObjectTypesAe::eKillUnsavedMuds, MakeKillUnsavedMuds },
        { ObjectTypesAe::eSoftLanding, MakeSoftLanding },
        { ObjectTypesAe::eNullObject2, MakeNullObject2 },
        { ObjectTypesAe::eWater, MakeWater },
        { ObjectTypesAe::eWorkWheel, MakeWorkWheel },
        { ObjectTypesAe::eLaughingGas, MakeLaughingGas },
        { ObjectTypesAe::eFlyingSlig, MakeFlyingSlig },
        { ObjectTypesAe::eFleech, MakeFleech },
        { ObjectTypesAe::eSlurgs, MakeSlurgs },
        { ObjectTypesAe::eSlamDoor, MakeSlamDoor },
        { ObjectTypesAe::eLevelLoader, MakeLevelLoader },
        { ObjectTypesAe::eDemoSpawnPoint, MakeDemoSpawnPoint },
        { ObjectTypesAe::eTeleporter, MakeTeleporter },
        { ObjectTypesAe::eSlurgSpawner, MakeSlurgSpawner },
        { ObjectTypesAe::eMineDrill, MakeMineDrill },
        { ObjectTypesAe::eColorfulMeter, MakeColorfulMeter },
        { ObjectTypesAe::eFlyingSligSpawner, MakeFlyingSligSpawner },
        { ObjectTypesAe::eMineCar, MakeMineCar },
        { ObjectTypesAe::eBoneBag, MakeBoneBag },
        { ObjectTypesAe::eExplosionSet, MakeExplosionSet },
        { ObjectTypesAe::eMultiswitchController, MakeMultiswitchController },
        { ObjectTypesAe::eRedGreenStatusLight, MakeRedGreenStatusLight },
        { ObjectTypesAe::eGhostLock, MakeGhostLock },
        { ObjectTypesAe::eParamiteNet, MakeParamiteNet },
        { ObjectTypesAe::eAlarm, MakeAlarm },
        { ObjectTypesAe::eFartMachine, MakeFartMachine },
        { ObjectTypesAe::eScrabSpawner, MakeScrabSpawner },
        { ObjectTypesAe::eCrawlingSlig, MakeCrawlingSlig },
        { ObjectTypesAe::eSligGetPants, MakeSligGetPants },
        { ObjectTypesAe::eSligGetWings, MakeSligGetWings },
        { ObjectTypesAe::eGreeter, MakeGreeter },
        { ObjectTypesAe::eCrawlingSligButton, MakeCrawlingSligButton },
        { ObjectTypesAe::eGlukkonSecurityFone, MakeGlukkonSecurityFone },
        { ObjectTypesAe::eDoorBlocker, MakeDoorBlocker },
        { ObjectTypesAe::eTorturedMudokon, MakeTorturedMudokon },
        { ObjectTypesAe::eTrainDoor, MakeTrainDoor }
    };

Entity AeEntityFactory::Create(const PathsJson::PathTheme* /*pathTheme*/, const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    // TODO: Use pathTheme to set correct "skin" for various things

    return sEnumMap[object.mType](object, entityManager, ms);
}
