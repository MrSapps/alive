#include "resourcemapper.hpp"
#include "fmv.hpp"
#include "oddlib/bits_factory.hpp"
#include "oddlib/audio/vab.hpp"
#include <cmath>
#include "oddlib/audio/SequencePlayer.h"

Animation::AnimationSetHolder::AnimationSetHolder(std::shared_ptr<Oddlib::LvlArchive> sLvlPtr, std::shared_ptr<Oddlib::AnimationSet> sAnimSetPtr, u32 animIdx) : mLvlPtr(sLvlPtr), mAnimSetPtr(sAnimSetPtr)
{
    mAnim = mAnimSetPtr->AnimationAt(animIdx);
}

const Oddlib::Animation& Animation::AnimationSetHolder::Animation() const
{
    return *mAnim;
}

u32 Animation::AnimationSetHolder::MaxW() const
{
    return mAnimSetPtr->MaxW();
}

u32 Animation::AnimationSetHolder::MaxH() const
{
    return mAnimSetPtr->MaxH();
}

Animation::Animation(AnimationSetHolder anim, bool isPsx, bool scaleFrameOffsets, u32 defaultBlendingMode, const std::string& sourceDataSet) : mAnim(anim), mIsPsx(isPsx), mScaleFrameOffsets(scaleFrameOffsets), mSourceDataSet(sourceDataSet)
{
    switch (defaultBlendingMode)
    {
    case 0:
        mBlendingMode = AbstractRenderer::eBlendModes::eNormal;
        break;
    case 1:
        mBlendingMode = AbstractRenderer::eBlendModes::eB100F100;
        break;
    default:
        // TODO: Other required blending modes
        mBlendingMode = AbstractRenderer::eBlendModes::eNormal;
        LOG_WARNING("Unknown blending mode: " << defaultBlendingMode);
    }
}

s32 Animation::FrameCounter() const
{
    return mCounter;
}

bool Animation::Update()
{
    bool ret = false;
    mCounter++;
    if (mCounter >= mFrameDelay)
    {
        ret = true;
        mFrameDelay = mAnim.Animation().Fps(); // Because mFrameDelay is 1 initially and Fps() can be > 1
        mCounter = 0;
        mFrameNum++;
        if (mFrameNum >= mAnim.Animation().NumFrames())
        {
            if (mAnim.Animation().Loop())
            {
                mFrameNum = mAnim.Animation().LoopStartFrame();
            }
            else
            {
                mFrameNum = mAnim.Animation().NumFrames() - 1;
            }

            // Reached the final frame, animation has completed 1 cycle
            mCompleted = true;
        }

        // Are we *on* the last frame?
        mIsLastFrame = (mFrameNum == mAnim.Animation().NumFrames() - 1);
    }
    return ret;
}

bool Animation::IsLastFrame() const
{
    return mIsLastFrame;
}

bool Animation::IsComplete() const
{
    return mCompleted;
}

void Animation::Render(AbstractRenderer& rend, bool flipX, int layer, AbstractRenderer::eCoordinateSystem coordinateSystem /*= AbstractRenderer::eWorld*/) const
{
    // TODO: Position calculation should be refactored

    /*
    static std::string msg;
    std::stringstream s;
    s << "Render frame number: " << mFrameNum;
    if (s.str() != msg)
    {
        LOG_INFO(s.str());
    }
    msg = s.str();
    */

    const Oddlib::Animation::Frame& frame = mAnim.Animation().GetFrame(mFrameNum == -1 ? 0 : mFrameNum);

    f32 xFrameOffset = (mScaleFrameOffsets ? static_cast<f32>(frame.mOffX / kPcToPsxScaleFactor) : static_cast<f32>(frame.mOffX)) * mScale;
    const f32 yFrameOffset = static_cast<f32>(frame.mOffY) * mScale;

    const f32 xpos = static_cast<f32>(mXPos);
    const f32 ypos = static_cast<f32>(mYPos);

    if (flipX)
    {
        xFrameOffset = -xFrameOffset;
    }
    // Render sprite as textured quad
    const TextureHandle textureId = rend.CreateTexture(AbstractRenderer::eTextureFormats::eRGBA, frame.mFrame->w, frame.mFrame->h, AbstractRenderer::eTextureFormats::eRGBA, frame.mFrame->pixels, true);
    rend.TexturedQuad(
        textureId,
        xpos + xFrameOffset,
        ypos + yFrameOffset,
        static_cast<f32>(frame.mFrame->w) * (flipX ? -ScaleX() : ScaleX()),
        static_cast<f32>(frame.mFrame->h) * mScale,
        layer,
        ColourU8{ 255, 255, 255, 255 },
        AbstractRenderer::eNormal,
        coordinateSystem
    );
    rend.DestroyTexture(textureId);

    if (Debugging().mAnimBoundingBoxes)
    {
        // Render bounding box
        const ColourU8 boundingBoxColour{ 255, 0, 255, 255 };
        const f32 width = static_cast<f32>(std::abs(frame.mTopLeft.x - frame.mBottomRight.x)) * mScale;

        const glm::vec4 rectScreen(rend.WorldToScreenRect(xpos + (static_cast<f32>(flipX ? -frame.mTopLeft.x : frame.mTopLeft.x) * mScale),
            ypos + (static_cast<f32>(frame.mTopLeft.y) * mScale),
            flipX ? -width : width,
            static_cast<f32>(std::abs(frame.mTopLeft.y - frame.mBottomRight.y)) * mScale));

        rend.Rect(rectScreen.x, rectScreen.y, rectScreen.z, rectScreen.w,
            layer,
            boundingBoxColour,
            AbstractRenderer::eBlendModes::eNormal,
            AbstractRenderer::eCoordinateSystem::eScreen);
    }

    if (Debugging().mAnimDebugStrings)
    {
        // Render frame pos and frame number
        const glm::vec2 xyposScreen(rend.WorldToScreen(glm::vec2(xpos, ypos)));
        rend.Text(xyposScreen.x, xyposScreen.y,
            24.0f,
            (mSourceDataSet
                + " x: " + std::to_string(xpos)
                + " y: " + std::to_string(ypos)
                + " f: " + std::to_string(FrameNumber())
                ).c_str(),
            ColourU8{ 255,255,255,255 },
            AbstractRenderer::eLayers::eFmv,
            AbstractRenderer::eBlendModes::eNormal,
            AbstractRenderer::eCoordinateSystem::eScreen);
    }
}

void Animation::SetFrame(u32 frame)
{
    mCounter = 0;
    mFrameDelay = 1; // Force change frame on first Update()
    mFrameNum = frame;
    mIsLastFrame = false;
    mCompleted = false;
}

void Animation::Restart()
{
    mCounter = 0;
    mFrameDelay = 1; // Force change frame on first Update()
    mFrameNum = -1;
    mIsLastFrame = false;
    mCompleted = false;
}

bool Animation::Collision(s32 x, s32 y) const
{
    const Oddlib::Animation::Frame& frame = mAnim.Animation().GetFrame(FrameNumber());

    // TODO: Refactor rect calcs
    f32 xpos = mScaleFrameOffsets ? static_cast<f32>(frame.mOffX / kPcToPsxScaleFactor) : static_cast<f32>(frame.mOffX);
    f32 ypos = static_cast<f32>(frame.mOffY);

    ypos = mYPos + (ypos * mScale);
    xpos = mXPos + (xpos * mScale);

    f32 w = static_cast<f32>(frame.mFrame->w) * ScaleX();
    f32 h = static_cast<f32>(frame.mFrame->h) * mScale;


    return PointInRect(x, y, static_cast<s32>(xpos), static_cast<s32>(ypos), static_cast<s32>(w), static_cast<s32>(h));
}

void Animation::SetXPos(s32 xpos)
{
    mXPos = xpos;
}

void Animation::SetYPos(s32 ypos)
{
    mYPos = ypos;
}

s32 Animation::XPos() const
{
    return mXPos;
}

s32 Animation::YPos() const
{
    return mYPos;
}

u32 Animation::MaxW() const
{
    return static_cast<u32>(mAnim.MaxW()*ScaleX());
}

u32 Animation::MaxH() const
{
    return static_cast<u32>(mAnim.MaxH()*mScale);
}

s32 Animation::FrameNumber() const
{
    return mFrameNum;
}

u32 Animation::NumberOfFrames() const
{
    return mAnim.Animation().NumFrames();
}

void Animation::SetScale(f32 scale)
{
    mScale = scale;
}

f32 Animation::ScaleX() const
{
    // PC sprites have a bigger width as they are higher resolution
    return mIsPsx ? (mScale) : (mScale / kPcToPsxScaleFactor);
}

const /*static*/ f32 Animation::kPcToPsxScaleFactor = 1.73913043478f;

ResourceMapper::ResourceMapper(IFileSystem& fileSystem, 
    const char* dataSetContentsFile,
    const char* animationResourceFile,
    const char* soundResourceMapFile,
    const char* pathsResourceMapFile,
    const char* fmvsResourceMapFile)
{
    auto dataSetContentStream = fileSystem.Open(dataSetContentsFile);
    assert(dataSetContentStream != nullptr);
    const auto dataSetContentsJsonData = dataSetContentStream->LoadAllToString();
    ParseDataSetContentsJson(dataSetContentsJsonData);

    auto animationResourcesStream = fileSystem.Open(animationResourceFile);
    assert(animationResourcesStream != nullptr);
    const auto animationJson = animationResourcesStream->LoadAllToString();
    ParseAnimationResourcesJson(animationJson);

    auto soundResourcesStream = fileSystem.Open(soundResourceMapFile);
    assert(soundResourcesStream != nullptr);
    const auto soundJsonData = soundResourcesStream->LoadAllToString();
    mSoundResources.Parse(soundJsonData);

    auto pathResourcesStream = fileSystem.Open(pathsResourceMapFile);
    assert(pathResourcesStream != nullptr);
    const auto pathJsonData = pathResourcesStream->LoadAllToString();
    ParsePathResourceJson(pathJsonData);

    auto fmvResourcesStream = fileSystem.Open(fmvsResourceMapFile);
    assert(fmvResourcesStream != nullptr);
    const auto fmvJsonData = fmvResourcesStream->LoadAllToString();
    ParseFmvResourceJson(fmvJsonData);
}

std::vector<std::tuple<const char*, const char*, bool>> ResourceMapper::DebugUi(const char* dataSetFilter, const char* nameFilter)
{
    // Collect the UI data/state
    if (mUi.mItems.empty())
    {
        for (const auto& animMap : mAnimMaps)
        {
            // "Resource" name
            UiItem item;
            std::string dataSets = " (";

            for (const AnimFileLocations& mapping : animMap.second.mLocations)
            {
                // Each dataset this resource lives in
                item.mItems.push_back(mapping.mDataSetName);
                dataSets += mapping.mDataSetName + " ";
            }
            dataSets += ")";
            item.mLabel = animMap.first + dataSets;
            item.mResourceName = animMap.first;

            mUi.mItems.push_back(item);
        }
    }

    // Render it
    std::vector<std::tuple<const char*, const char*, bool>> ret;
    for (UiItem& item : mUi.mItems)
    {
        bool found = false;
        if (dataSetFilter[0] != '\0')
        {
            for (const std::string& subItem : item.mItems)
            {
                if (string_util::StringFilter(subItem.c_str(), dataSetFilter))
                {
                    found = true;
                    break;
                }
            }
        }
        else
        {
            found = true;
        }

        if (found)
        {
            if (string_util::StringFilter(item.mResourceName.c_str(), nameFilter))
            {
                if (ImGui::Checkbox(item.mLabel.c_str(), &item.mLoad))
                {
                    for (const std::string& subItem : item.mItems)
                    {
                        ret.emplace_back(std::make_tuple(subItem.c_str(), item.mResourceName.c_str(), item.mLoad));
                    }
                }
            }
        }
    }
    return ret;
}

const MusicTheme* ResourceMapper::FindSoundTheme(const char* themeName)
{
    return mSoundResources.FindMusicTheme(themeName);
}

const std::vector<SoundResource>& ResourceMapper::GetSoundResources() const
{
    return mSoundResources.mSounds;
}

const SoundBankLocation* ResourceMapper::FindSoundBank(const std::string& soundBank)
{
    return mSoundResources.FindSoundBank(soundBank);
}

ResourceLocator::ResourceLocator(ResourceMapper&& resourceMapper, DataPaths&& dataPaths)
    : mResMapper(std::move(resourceMapper)), mDataPaths(std::move(dataPaths))
{

}

ResourceLocator::~ResourceLocator()
{

}

std::vector<std::tuple<const char*, const char*, bool>> ResourceLocator::DebugUi(const char* dataSetFilter, const char* nameFilter)
{
    return mResMapper.DebugUi(dataSetFilter, nameFilter);
}

std::string ResourceLocator::LocateScript(const char* scriptName)
{
    // Look for the engine built-in script first
    std::string fileName = std::string("{GameDir}\\data\\scripts\\") + scriptName;
    if (mDataPaths.GameFs().FileExists(fileName))
    {
        return mDataPaths.GameFs().Open(fileName)->LoadAllToString();
    }

    // TODO: Look in the mods for script overrides or new scripts
    LOG_ERROR("Script not found: " << scriptName);

    return "";
}

std::unique_ptr<ISound> ResourceLocator::DoLoadSoundMusic(const char* resourceName, const DataPaths::FileSystemInfo& fs, const std::string& strSb, const MusicResource& musicRes)
{
    const SoundBankLocation* sbl = mResMapper.FindSoundBank(strSb);

    // Only look in this file system if its mapped to the same dataset as what the sound bank lives in
    if (fs.mDataSetName != sbl->mDataSetName)
    {
        return nullptr;
    }

    const std::vector<ResourceMapper::DataSetFileAttributes>* bsqFileLocationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), sbl->mSeqFileName.c_str());
    if (!bsqFileLocationsInThisDataSet)
    {
        return nullptr;
    }

    for (const ResourceMapper::DataSetFileAttributes& bsqFileAttributes : *bsqFileLocationsInThisDataSet)
    {
        std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, bsqFileAttributes.mLvlName);
        if (lvl)
        {
            std::string vh = sbl->mSoundBankName + ".VH";
            std::string vb = sbl->mSoundBankName + ".VB";
            auto seqFile = lvl->FileByName(sbl->mSeqFileName);
            auto vhFile = lvl->FileByName(vh);
            auto vbFile = lvl->FileByName(vb);
            if (seqFile && vhFile && vbFile)
            { 
                // Get SEQ block
                auto seqChunk = seqFile->ChunkById(musicRes.mResourceId);
                if (seqChunk)
                {
                    auto vab = std::make_unique<Vab>();

                    auto seqStream = seqChunk->Stream();

                    // Read VH
                    auto vhStream = vhFile->ChunkByIndex(0)->Stream();
                    vab->ReadVh(*vhStream, bsqFileAttributes.mIsPsx);

                    // Get sounds.dat for VB if required
                    std::string soundsDatFileName = "sounds.dat";
                    const bool useSoundsDat = bsqFileAttributes.mIsAo == false && bsqFileAttributes.mIsPsx == false && fs.mFileSystem->FileExists(soundsDatFileName);
                    std::unique_ptr<Oddlib::IStream> soundsDatStream;
                    if (useSoundsDat)
                    {
                        soundsDatStream = fs.mFileSystem->Open(soundsDatFileName);
                    }

                    // Read VB
                    auto vbStream = vbFile->ChunkByIndex(0)->Stream();
                    vab->ReadVb(*vbStream, bsqFileAttributes.mIsPsx, useSoundsDat, soundsDatStream.get());

                    LOG_INFO("Using sound bank: " << sbl->mName);

                    return std::make_unique<SeqSound>(resourceName, std::move(vab), std::move(seqStream));
                }
            }
        }
    }
    return nullptr;
}

std::unique_ptr<ISound> ResourceLocator::DoLoadSoundEffect(const char* resourceName, const DataPaths::FileSystemInfo& fs, const std::string& strSb, const SoundEffectResource& sfxRes, const SoundEffectResourceLocation& sfxResLoc)
{
    const SoundBankLocation* sbl = mResMapper.FindSoundBank(strSb);

    // Only look in this file system if its mapped to the same dataset as what the sound bank lives in
    if (fs.mDataSetName != sbl->mDataSetName)
    {
        return nullptr;
    }

    const std::vector<ResourceMapper::DataSetFileAttributes>* bsqFileLocationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), sbl->mSeqFileName.c_str());
    if (!bsqFileLocationsInThisDataSet)
    {
        return nullptr;
    }

    for (const ResourceMapper::DataSetFileAttributes& bsqFileAttributes : *bsqFileLocationsInThisDataSet)
    {
        std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, bsqFileAttributes.mLvlName);
        if (lvl)
        {
            std::string vh = sbl->mSoundBankName + ".VH";
            std::string vb = sbl->mSoundBankName + ".VB";
            auto vhFile = lvl->FileByName(vh);
            auto vbFile = lvl->FileByName(vb);
            if (vhFile && vbFile)
            {
                auto vab = std::make_unique<Vab>();

                // Read VH
                auto vhStream = vhFile->ChunkByIndex(0)->Stream();
                vab->ReadVh(*vhStream, bsqFileAttributes.mIsPsx);

                // Get sounds.dat for VB if required
                std::string soundsDatFileName = "sounds.dat";
                const bool useSoundsDat = bsqFileAttributes.mIsAo == false && bsqFileAttributes.mIsPsx == false && fs.mFileSystem->FileExists(soundsDatFileName);
                std::unique_ptr<Oddlib::IStream> soundsDatStream;
                if (useSoundsDat)
                {
                    soundsDatStream = fs.mFileSystem->Open(soundsDatFileName);
                }

                // Read VB
                auto vbStream = vbFile->ChunkByIndex(0)->Stream();
                vab->ReadVb(*vbStream, bsqFileAttributes.mIsPsx, useSoundsDat, soundsDatStream.get());

                LOG_INFO("Using sound bank: " << sbl->mName);

                return std::make_unique<SingleSeqSampleSound>(resourceName,
                    std::move(vab),
                    sfxResLoc.mProgram,
                    sfxResLoc.mTone,
                    sfxRes.mMinPitch,
                    sfxRes.mMaxPitch,
                    sfxRes.mVolume);
            }
        }
    }
    return nullptr;
}

std::unique_ptr<ISound> ResourceLocator::LocateSound(const char* resourceName, const char* explicitSoundBankName /*= nullptr*/, bool useMusicRec /*= true*/, bool useSfxRec /*= true*/)
{
    const SoundResource* sr = mResMapper.FindSound(resourceName);
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod)
        {
            // TODO: Mod path
        }
        else
        {
            if (sr)
            {
                if (useMusicRec && !sr->mMusic.mSoundBanks.empty())
                {
                    const std::set<std::string>& soundBanks = explicitSoundBankName ? std::set<std::string> { explicitSoundBankName } : sr->mMusic.mSoundBanks;
                    for (const std::string& sb : soundBanks)
                    {
                        auto ret = DoLoadSoundMusic(resourceName, fs, sb, sr->mMusic);
                        if (ret)
                        {
                            return ret;
                        }
                    }
                }

                if (useSfxRec && !sr->mSoundEffect.mSoundBanks.empty())
                {
                    for (const SoundEffectResourceLocation& loc : sr->mSoundEffect.mSoundBanks)
                    {
                        const bool explicitSoundBankNameExists = explicitSoundBankName && loc.mSoundBanks.find(explicitSoundBankName) != std::end(loc.mSoundBanks);
                        if (explicitSoundBankName && !explicitSoundBankNameExists)
                        {
                            break;
                        }

                        const std::set<std::string>& soundBanks = explicitSoundBankName ? std::set<std::string> { explicitSoundBankName } : loc.mSoundBanks;
                        for (const std::string& sb : soundBanks)
                        {
                            auto ret = DoLoadSoundEffect(resourceName, fs, sb, sr->mSoundEffect, loc);
                            if (ret)
                            {
                                return ret;
                            }
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

up_future_UP_Path ResourceLocator::LocatePath(const std::string& resourceName)
{
    return std::make_unique<future_UP_Path>(std::async(std::launch::async, [=]() -> Oddlib::UP_Path
    {
        const ResourceMapper::PathMapping* mapping = mResMapper.FindPath(resourceName.c_str());
        if (mapping)
        {
            for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
            {
                if (fs.mIsMod)
                {
                    // TODO: Mod path
                }
                else
                {
                    const ResourceMapper::PathLocation* pathLocation = mapping->Find(fs.mDataSetName);
                    if (pathLocation)
                    {
                        const std::vector<ResourceMapper::DataSetFileAttributes>* locationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), pathLocation->mDataSetFileName.c_str());
                        if (locationsInThisDataSet)
                        {
                            for (const ResourceMapper::DataSetFileAttributes& attributes : *locationsInThisDataSet)
                            {
                                std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, attributes.mLvlName);
                                if (lvl)
                                {
                                    auto lvlFile = lvl->FileByName(pathLocation->mDataSetFileName);
                                    if (lvlFile)
                                    {
                                        auto chunk = lvlFile->ChunkById(mapping->mId);
                                        auto stream = chunk->Stream();
                                        return std::make_unique<Oddlib::Path>(mapping->mMusicTheme, *stream,
                                            mapping->mCollisionOffset,
                                            mapping->mIndexTableOffset,
                                            mapping->mObjectOffset,
                                            mapping->mNumberOfScreensX,
                                            mapping->mNumberOfScreensY,
                                            attributes.mIsAo);
                                    }
                                }

                            }
                        }
                    }
                }
            }
        }
        return nullptr;
    }));
}

std::unique_ptr<Oddlib::IBits> ResourceLocator::LocateCamera(const char* resourceName)
{
    LOG_INFO("Requesting camera " << resourceName);
    return DoLocateCamera(resourceName, false);
}

static bool CanDeltaBeApplied(int camW, int camH, int deltaW, int deltaH)
{
    if (camW == 640 && deltaW == 1440 && camH == 240 && deltaH == 1080)
    {
        return true;
    }
    return false;
}

static void ApplyDelta(SDL_Surface* deltaSurface, SDL_Surface* originalCameraSurface)
{
    auto dst = static_cast<u8*>(deltaSurface->pixels);
    auto w = static_cast<unsigned int>(deltaSurface->w);
    auto h = static_cast<unsigned int>(deltaSurface->h);

    uint8_t *src = static_cast<uint8_t*>(originalCameraSurface->pixels);
    // Apply delta image over linearly interpolated game cam image
    for (auto y = 0u; y < h; ++y)
    {
        const f32 src_rel_y = 1.f*y / (h - 1); // 0..1
        for (auto x = 0u; x < w; ++x)
        {
            const f32 src_rel_x = 1.f*x / (w - 1); // 0..1

            int src_x = (int)std::floor(src_rel_x*originalCameraSurface->w - 0.5f);
            int src_y = (int)std::floor(src_rel_y*originalCameraSurface->h - 0.5f);

            int src_x_plus = src_x + 1;
            int src_y_plus = src_y + 1;

            f32 lerp_x = src_rel_x*originalCameraSurface->w - (src_x + 0.5f);
            f32 lerp_y = src_rel_y*originalCameraSurface->h - (src_y + 0.5f);
            assert(lerp_x >= 0.0f);
            assert(lerp_x <= 1.0f);
            assert(lerp_y >= 0.0f);
            assert(lerp_y <= 1.0f);

            // Limit source pixels inside image
            src_x = std::max(src_x, 0);
            src_y = std::max(src_y, 0);
            src_x_plus = std::min(src_x_plus, originalCameraSurface->w - 1);
            src_y_plus = std::min(src_y_plus, originalCameraSurface->h - 1);

            // Indices to four neighboring pixels
            const int src_indices[4] =
            {
                src_x * 3 + originalCameraSurface->pitch*src_y,
                src_x_plus * 3 + originalCameraSurface->pitch*src_y,
                src_x * 3 + originalCameraSurface->pitch*src_y_plus,
                src_x_plus * 3 + originalCameraSurface->pitch*src_y_plus
            };

            const int dst_ix = (x + w*y) * 3;

            for (int comp = 0; comp < 3; ++comp)
            {
                // 4 neighboring texels
                f32 a = src[src_indices[0] + comp] / 255.f;
                f32 b = src[src_indices[1] + comp] / 255.f;
                f32 c = src[src_indices[2] + comp] / 255.f;
                f32 d = src[src_indices[3] + comp] / 255.f;

                // 2d linear interpolation
                f32 orig = (a*(1 - lerp_x) + b*lerp_x)*(1 - lerp_y) + (c*(1 - lerp_x) + d*lerp_x)*lerp_y;
                f32 delta = dst[dst_ix + comp] / 255.f;

                // "Grain extract" has been used in creating the delta image
                f32 merged = orig + delta - 0.5f;
                dst[dst_ix + comp] = (uint8_t)(std::max(std::min(merged * 255 + 0.5f, 255.f), 0.0f));
            }
        }
    }
}

std::unique_ptr<Oddlib::IBits> ResourceLocator::DoLocateCamera(const char* resourceName, bool ignoreMods)
{
    std::string deltaName;
    std::string modName;

    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod && !ignoreMods)
        {
            if (modName.empty())
            {
                modName = std::string(resourceName) + ".png";
            }

            // Check for mod trying to fully replace camera with its own, or simply a new camera
            if (fs.mFileSystem->FileExists(modName))
            {
                auto stream = fs.mFileSystem->Open(modName);
                auto surface = SDLHelpers::LoadPng(*stream, false);
                if (surface)
                {
                    LOG_INFO("Loaded new or replacement camera from mod " << fs.mDataSetName);
                    return Oddlib::MakeBits(std::move(surface));
                }
            }

            if (deltaName.empty())
            {
                deltaName = std::string(resourceName) + ".cam.bmp.png"; // TODO: Rename to something sane
            }

            if (fs.mFileSystem->FileExists(deltaName))
            {
                auto cam = DoLocateCamera(resourceName, true);
                if (cam)
                {
                    auto originalCameraSurface = cam->GetSurface();
                    auto deltaPngStream = fs.mFileSystem->Open(deltaName);
                    auto deltaSurface = SDLHelpers::LoadPng(*deltaPngStream, false);
                    if (deltaSurface)
                    {
                        if (CanDeltaBeApplied(originalCameraSurface->w, originalCameraSurface->h, deltaSurface->w, deltaSurface->h))
                        {
                            ApplyDelta(deltaSurface.get(), originalCameraSurface);
                            LOG_INFO("Applied camera upscaling delta from " << fs.mDataSetName);
                            return Oddlib::MakeBits(std::move(deltaSurface));
                        }
                    }
                }
            }
        }
        else
        {
            const std::vector<ResourceMapper::DataSetFileAttributes>* locationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), resourceName);
            if (locationsInThisDataSet)
            {
                for (const ResourceMapper::DataSetFileAttributes& attributes : *locationsInThisDataSet)
                {
                    std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, attributes.mLvlName);
                    if (lvl)
                    {
                        auto lvlFile = lvl->FileByName(resourceName);
                        if (lvlFile)
                        {
                            auto bitsChunk = lvlFile->ChunkByType(Oddlib::MakeType("Bits"));
                            auto bitsStream = bitsChunk->Stream();
                      
                            auto fg1Chunk = lvlFile->ChunkByType(Oddlib::MakeType("FG1 "));
                            std::unique_ptr<Oddlib::IStream> fg1Stream;
                            if (fg1Chunk)
                            {
                                fg1Stream = fg1Chunk->Stream();
                            }

                            LOG_INFO("Loaded original camera from " << fs.mDataSetName << " has foreground layer: " << (fg1Stream ? "true" : "false"));
                            return Oddlib::MakeBits(*bitsStream, fg1Stream.get());
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

std::unique_ptr<IMovie> ResourceLocator::LocateFmv(IAudioController& audioController, const char* resourceName, const ResourceMapper::FmvFileLocation* location)
{
    // Try from explicitly passed in location
    if (location)
    {
        for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
        {
            if (fs.mDataSetName == location->mDataSetName)
            {
                auto ret = DoLocateFmvFromFileLocation(*location, fs, resourceName, audioController);
                if (ret)
                {
                    return ret;
                }
            }
        }
        return nullptr;
    }

    const ResourceMapper::FmvMapping* fmvMapping = mResMapper.FindFmv(resourceName);
    if (!fmvMapping)
    {
        return nullptr;
    }

    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod)
        {
            // TODO: Look up the override in the mod fs

        }
        else
        {
            auto ret = DoLocateFmv(audioController, resourceName, fs, *fmvMapping);
            if (ret)
            {
                return ret;
            }
        }
    }

    return nullptr;
}


std::unique_ptr<IMovie> ResourceLocator::DoLocateFmv(IAudioController& audioController, const char* resourceName, const DataPaths::FileSystemInfo& fs, const ResourceMapper::FmvMapping& fmvMapping)
{
    // Each each mapping in the resource record that has matched resourceName
    for (const ResourceMapper::FmvFileLocation& location : fmvMapping.mLocations)
    {
        // Check if the mapping applies to the data set that fs is
        if (location.mDataSetName == fs.mDataSetName)
        {
            auto ret = DoLocateFmvFromFileLocation(location, fs, resourceName, audioController);
            if (ret)
            {
                return ret;
            }
        }
    }
    return nullptr;
}


std::unique_ptr<IMovie> ResourceLocator::DoLocateFmvFromFileLocation(const ResourceMapper::FmvFileLocation& location, const DataPaths::FileSystemInfo& fs, const char* resourceName, IAudioController& audioController)
{
    std::string locationFileName = location.mFileName;
    if (fs.mFileSystem->FileExists(locationFileName))
    {
        auto stream = fs.mFileSystem->Open(locationFileName);
        if (stream)
        {
            std::unique_ptr<SubTitleParser> subTitles;
            std::string subTitleFileName = "{GameDir}/data/subtitles/" + std::string(resourceName) + ".SRT";
            if (mDataPaths.GameFs().FileExists(subTitleFileName))
            {
                auto subsStream = mDataPaths.GameFs().Open(subTitleFileName);
                if (subsStream)
                {
                    subTitles = std::make_unique<SubTitleParser>(std::move(subsStream));
                }
            }
            return IMovie::Factory(resourceName, audioController, std::move(stream), std::move(subTitles), location.mStartSector, location.mEndSector);
        }
    }
    return nullptr;
}

std::unique_ptr<Animation> ResourceLocator::LocateAnimation(const char* resourceName)
{
    const ResourceMapper::AnimMapping* animMapping = mResMapper.FindAnimation(resourceName);
    if (!animMapping)
    {
        return nullptr;
    }

    // For each data set attempt to find resourceName by mapping
    // to a LVL/file/chunk. Or in the case of a mod dataset something else.
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod)
        {
            // TODO: Look up the override in the mod fs

            // If this name is not a known resource then it is a new resource for this mod

            // TODO: Handle special case overrides that still need the real file (i.e cam deltas)
        }
        else
        {
            auto ret = DoLocateAnimation(fs, resourceName, *animMapping);
            if (ret)
            {
                return ret;
            }
        }
    }
    return nullptr;
}

std::unique_ptr<Animation> ResourceLocator::LocateAnimation(const char* resourceName, const char* dataSetName)
{
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mDataSetName == dataSetName)
        {
            const ResourceMapper::AnimMapping* animMapping = mResMapper.FindAnimation(resourceName);
            if (!animMapping)
            {
                return nullptr;
            }

            auto ret = DoLocateAnimation(fs, resourceName, *animMapping);
            if (ret)
            {
                return ret;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<Oddlib::LvlArchive> ResourceLocator::OpenLvl(IFileSystem& fs, const std::string& dataSetName, const std::string& lvlName)
{
    auto lvlPtr = mCache.GetLvl(dataSetName, lvlName);
    if (!lvlPtr)
    {
        // Try to open new lvl since it wasn't in the cache
        auto lvlStream = fs.Open(lvlName);
        if (lvlStream)
        {
            // Cache this lvl
            auto lvl = std::make_unique<Oddlib::LvlArchive>(std::move(lvlStream));
            lvlPtr = mCache.AddLvl(std::move(lvl), dataSetName, lvlName);
        }
    }
    return lvlPtr;
}

const std::vector<SoundResource>& ResourceLocator::GetSoundResources() const
{
    return mResMapper.GetSoundResources();
}

const MusicTheme* ResourceLocator::LocateSoundTheme(const char* themeName)
{
    return mResMapper.FindSoundTheme(themeName);
}

std::unique_ptr<Animation> ResourceLocator::DoLocateAnimation(const DataPaths::FileSystemInfo& fs, const char* resourceName, const ResourceMapper::AnimMapping& animMapping)
{
    // Each each mapping in the resource record that has matched resourceName
    for (const ResourceMapper::AnimFileLocations& location : animMapping.mLocations)
    {
        // Check if the mapping applies to the data set that fs is
        if (location.mDataSetName == fs.mDataSetName)
        {
            // Loop through all the locations in the data set where resourceName lives
            for (const ResourceMapper::AnimFile& animFile : location.mFiles)
            {
                // Now find all of the LVLs where animFile lives
                const std::vector<ResourceMapper::DataSetFileAttributes>* fileLocations = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), animFile.mFile.c_str());
                if (fileLocations)
                {
                    // Loop through each LVL and see if animFile exists there
                    for (const ResourceMapper::DataSetFileAttributes& dataSetFileAttributes : *fileLocations)
                    {
                        auto lvlPtr = OpenLvl(*fs.mFileSystem, fs.mDataSetName, dataSetFileAttributes.mLvlName);
                        if (lvlPtr)
                        {
                            auto animSetPtr = mCache.GetAnimSet(fs.mDataSetName, dataSetFileAttributes.mLvlName, animFile.mFile, animFile.mId);
                            if (!animSetPtr)
                            {
                                // Open the file within the archive
                                auto lvlFile = lvlPtr->FileByName(animFile.mFile);
                                if (lvlFile)
                                {
                                    // Get the chunk within the file that lives in the lvl
                                    Oddlib::LvlArchive::FileChunk* chunk = lvlFile->ChunkById(animFile.mId);
                                    if (chunk)
                                    {
                                        LOG_INFO(resourceName
                                            << " located in data set " << fs.mDataSetName
                                            << " mapped to " << fs.mFileSystem->FsPath()
                                            << " in lvl archive " << dataSetFileAttributes.mLvlName
                                            << " in lvl file " << animFile.mFile
                                            << " with lvl file chunk id " << animFile.mId
                                            << " at anim index " << animFile.mAnimationIndex
                                            << " is psx " << dataSetFileAttributes.mIsPsx
                                            << " scale frame offsets " << dataSetFileAttributes.mScaleFrameOffsets);

                                        auto stream = chunk->Stream();
                                        Oddlib::AnimSerializer as(*stream, dataSetFileAttributes.mIsPsx);
                                        animSetPtr = mCache.AddAnimSet(std::make_unique<Oddlib::AnimationSet>(as), fs.mDataSetName, dataSetFileAttributes.mLvlName, animFile.mFile, animFile.mId);
                                    }
                                }
                            }

                            // Construct the animation from the chunk bytes
                            return std::make_unique<Animation>(
                                Animation::AnimationSetHolder(lvlPtr, animSetPtr, animFile.mAnimationIndex),
                                dataSetFileAttributes.mIsPsx,
                                dataSetFileAttributes.mScaleFrameOffsets,
                                animMapping.mBlendingMode,
                                fs.mDataSetName);

                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

BaseSeqSound::BaseSeqSound(const char* soundName, std::unique_ptr<Vab> vab)
    : mVab(std::move(vab)), mSoundName(soundName)
{

}

void BaseSeqSound::DebugUi()
{
    mSeqPlayer->DebugUi();
}

void BaseSeqSound::Play(f32* stream, u32 len)
{
    mSeqPlayer->Play(stream, len);
}

bool BaseSeqSound::AtEnd() const
{
    return mSeqPlayer->AtEnd();
}

void BaseSeqSound::Restart()
{
    mSeqPlayer->Restart();
}

void BaseSeqSound::Update()
{
    mSeqPlayer->Update();
}

const std::string& BaseSeqSound::Name() const
{
    return mSoundName;
}

SingleSeqSampleSound::SingleSeqSampleSound(const char* soundName, std::unique_ptr<Vab> vab, u32 program, u32 note, u32 minPitch, u32 maxPitch, u32 /*vol*/)
    : BaseSeqSound(soundName, std::move(vab)), mProgram(program), mNote(note), mMinPitch(minPitch), mMaxPitch(maxPitch)
{

}

static float RandFloat(float a, float b)
{
    return ((b - a)*((float)rand() / RAND_MAX)) + a;
}

void SingleSeqSampleSound::Load()
{
    mSeqPlayer = std::make_unique<SequencePlayer>(mSoundName.c_str(), *mVab);
    mSeqPlayer->NoteOnSingleShot(mProgram, mNote, 127, 0.0f, RandFloat(static_cast<f32>(mMinPitch), static_cast<f32>(mMaxPitch)));
}

SeqSound::SeqSound(const char* soundName, std::unique_ptr<Vab> vab, std::unique_ptr<Oddlib::IStream> seq)
    : BaseSeqSound(soundName, std::move(vab)), mSeqData(std::move(seq))
{

}

void SeqSound::Load()
{
    mSeqPlayer = std::make_unique<SequencePlayer>(mSoundName.c_str(), *mVab);
    mSeqPlayer->LoadSequenceStream(*mSeqData);
    mSeqPlayer->PlaySequence();
}
