#pragma once

#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <sstream>
#include "core/audiobuffer.hpp"
#include "oddlib/path.hpp"
#include "fsm.hpp"
#include "abstractrenderer.hpp"
#include "collisionline.hpp"
#include "imgui/imgui.h"
#include "iterativeforloop.hpp"
#include "core/entitymanager.hpp"

class AbstractRenderer;
class ResourceLocator;
class InputState;

namespace Oddlib
{
class LvlArchive;
class IBits;
}

class Animation;

class Sound;

class GridScreen
{
public:
    GridScreen(const GridScreen&) = delete;
    GridScreen& operator=(const GridScreen&) = delete;
    GridScreen(const Oddlib::Path::Camera& camera, ResourceLocator& locator);
    ~GridScreen();

public:
    const std::string& FileName() const
    { return mFileName; }
    void LoadTextures(AbstractRenderer& rend);
    void UnLoadTextures(AbstractRenderer& rend);
    bool hasTexture() const;
    const Oddlib::Path::Camera& getCamera() const
    { return mCamera; }
    void Render(AbstractRenderer& rend, float x, float y, float w, float h);

private:
    std::string mFileName;
    TextureHandle mCameraTexture;
    TextureHandle mFG1Texture;
    Oddlib::Path::Camera mCamera; // TODO: This is not the in-game format
    std::unique_ptr<Oddlib::IBits> mCam; // Temp hack to prevent constant reloading of LVLs
    ResourceLocator& mLocator;
};

#define NO_MOVE_OR_MOVE_ASSIGN(x)  x(x&&) = delete; x& operator = (x&&) = delete // TODO: move this out

constexpr u32 kSwitchTimeMs = 300;

class WorldState;

class GridMap
{
public:
    GridMap(const GridMap&) = delete;
    GridMap& operator=(const GridMap&) = delete;
    GridMap(CoordinateSpace& coords, WorldState& state, EntityManager &entityManager);
    ~GridMap();

public:
    bool LoadMap(const Oddlib::Path& path, ResourceLocator& locator);
    void UnloadMap(AbstractRenderer& renderer);

public:
    EntityManager &mRoot;

private:
    class Loader
    {
    public:
        explicit Loader(GridMap& gm);

    public:
        bool Load(const Oddlib::Path& path, ResourceLocator& locator);

    private:
        enum class LoaderStates
        {
            eInit,
            eSetupAndConvertCollisionItems,
            eAllocateCameraMemory,
            eLoadCameras,
            eLoadEntities,
        };
        enum ObjectTypesAe : u8
        {
            eContinuePoint = 1,
            eHoist = 2,
            eEdge = 3,
            eDeathDrop = 4,
            eDoor = 5,
            eShadow = 6,
            eLiftPoint = 7,
            eWellLocal = 8,
            eDove = 9,
            eRockSack = 10,
            eFallingItem = 11,
            ePullRingRope = 12,
            eBackgroundAnimation = 13,
            eTimedMine = 14,
            eSlig = 15,
            eSlog = 16,
            eSwitch = 17,
            eSecurityEye = 19,
            ePulley = 21,
            eAbeStart = 22,
            eWellExpress = 23,
            eMine = 24,
            eUxb = 25,
            eParamite = 26,
            eMovieStone = 27,
            eBirdPortal = 28,
            ePortalExit = 29,
            eTrapDoor = 30,
            eRollingBall = 31,
            eSligLeftBound = 32,
            eInvisibleZone = 33,
            eFootSwitch = 34,
            eSecurityOrb = 35,
            eMotionDetector = 36,
            eSligSpawner = 37,
            eElectricWall = 38,
            eLiftMover = 39,
            eMeatSack = 40,
            eScrab = 41,
            eScrabLeftBound = 43,
            eScrabRightBound = 44,
            eSligRightBound = 45,
            eSligPersist = 46,
            eEnemyStopper = 47,
            eInvisibleSwitch = 48,
            eMud = 49,
            eZSligCover = 50,
            eDoorFlame = 51,
            eMovingBomb = 52,
            eMenuController = 54,
            eTimerTrigger = 57,
            eSecurityDoor = 58,
            eGrenadeMachine = 59,
            eLcdScreen = 60,
            eHandStone = 61,
            eCreditsController = 62,
            eLcdStatusBoard = 64,
            eWheelSyncer = 65,
            eMusic = 66,
            eLightEffect = 67,
            eSlogSpawner = 68,
            eDeathClock = 69,
            eGasEmitter = 71,
            eSlogHut = 72,
            eGlukkon = 73,
            eKillUnsavedMuds = 74,
            eSoftLanding = 75,
            eWater = 77,
            eWheel = 79,
            eLaughingGas = 81,
            eFlyingSlig = 82,
            eFleech = 83,
            eSlurgs = 84,
            eSlamDoor = 85,
            eLevelLoader = 86,
            eDemoSpawnPoint = 87,
            eTeleporter = 88,
            eSlurgSpawner = 89,
            eGrinder = 90,
            eColorfulMeter = 91,
            eFlyingSligSpawner = 92,
            eMineCar = 93,
            eSlogFoodSack = 94,
            eExplosionSet = 95,
            eMultiswitchController = 96,
            eRedGreenStatusLight = 97,
            eSlapLock = 98,
            eParamiteNet = 99,
            eAlarm = 100,
            eFartMachine = 101,
            eScrabSpawner = 102,
            eCrawlingSlig = 103,
            eSligGetPants = 104,
            eSligGetWings = 105,
            eGreeter = 106,
            eCrawlingSligButton = 107,
            eGlukkonSecurityDoor = 108,
            eDoorBlocker = 109,
            eTorturedMudokon = 110,
            eTrainDoor = 111
        };

    private:
        void SetState(LoaderStates state);
        void SetupAndConvertCollisionItems(const Oddlib::Path& path);
        void HandleAllocateCameraMemory(const Oddlib::Path& path);
        void HandleLoadCameras(const Oddlib::Path& path, ResourceLocator& locator);
        void HandleLoadEntities(const Oddlib::Path& path);

    private:
        GridMap& mGm;
        LoaderStates mState = LoaderStates::eInit;
        IterativeForLoopU32 mXForLoop;
        IterativeForLoopU32 mYForLoop;
        IterativeForLoopU32 mIForLoop;
    };

private:
    Loader mLoader;
    WorldState& mWorldState;
};