#include "filesystem.hpp"
#include "logger.hpp"
#include "string_util.hpp"
#include "SDL.h"
#include "oddlib/stream.hpp"
#include "oddlib/cdromfilesystem.hpp"
#include <algorithm>
#include <fstream>
#include "jsonxx/jsonxx.h"
#include "gui.hpp"

#ifdef _WIN32
const char kDirSeperator = '\\';
#else
const char kDirSeperator = '/';
#endif

// Helpers
static std::string GetFilePath(const std::string& basePath, const std::string& fileName)
{
    std::string fullFileName;
    if (string_util::ends_with(basePath, std::string(1, kDirSeperator)))
    {
        fullFileName = basePath + fileName;
    }
    else
    {
        fullFileName = basePath + kDirSeperator + fileName;
    }
    return fullFileName;
}

static Sint64 FileExists(const std::string& fileName)
{
    std::ifstream fileStream;
    fileStream.open(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (fileStream)
    {
        LOG_INFO("File: " << fileName << " found");
        return fileStream.tellg();
    }
    LOG_WARNING("File: " << fileName << " not found");
    return -1;
}

// Directory
ResourcePathAndModsFs::Directory::Directory(const std::string& path, int priority)
    : IResourcePathAbstraction(path, priority)
{
}

std::unique_ptr<Oddlib::IStream> ResourcePathAndModsFs::Directory::Open(const std::string& fileName)
{
    const auto fullFileName = GetFilePath(Path(), fileName);
    return std::make_unique<Oddlib::Stream>(fullFileName);
}

Sint64 ResourcePathAndModsFs::Directory::Exists(const std::string& fileName) const
{
    const auto fullFileName = GetFilePath(Path(), fileName);
    return FileExists(fullFileName);
}

// RawCdImagePath
ResourcePathAndModsFs::RawCdImagePath::RawCdImagePath(const std::string& path, int priority)
    : IResourcePathAbstraction(path, priority)
{
    mCdImage = std::make_unique<RawCdImage>(path);
    mCdImage->LogTree();
}

std::unique_ptr<Oddlib::IStream> ResourcePathAndModsFs::RawCdImagePath::Open(const std::string& fileName)
{
    // Only PSX FMV's need raw sector reading, everything else is a "normal" file
    return mCdImage->ReadFile(fileName, string_util::ends_with(fileName, ".MOV", false));
}

Sint64 ResourcePathAndModsFs::RawCdImagePath::Exists(const std::string& fileName) const
{
    return mCdImage->FileExists(fileName);
}

// GameDataFs
void GameDataFs::InitBasePath()
{
    char* pBasePath = SDL_GetBasePath();
    if (pBasePath)
    {
        mBasePath = std::string(pBasePath);
        SDL_free(pBasePath);

        // If it looks like we're running from the IDE/dev build then attempt to fix up the path to the correct location to save
        // manually setting the correct working directory.
        const bool bIsDebugPath = string_util::contains(mBasePath, "/alive/bin/") || string_util::contains(mBasePath, "\\alive\\bin\\");
        if (bIsDebugPath)
        {
            if (string_util::contains(mBasePath, "/alive/bin/"))
            {
                LOG_WARNING("We appear to be running from the IDE (Linux) - fixing up basePath to be ../");
                mBasePath += "../";
            }
            else
            {
                LOG_WARNING("We appear to be running from the IDE (Win32) - fixing up basePath to be ../");
                mBasePath += "..\\..\\";
            }
        }
    }
    else
    {
        mBasePath = std::string(".") + kDirSeperator;
        LOG_ERROR("SDL_GetBasePath failed, falling back to ." << kDirSeperator);
    }
    LOG_INFO("basePath is " << mBasePath);
}

std::unique_ptr<Oddlib::IStream> GameDataFs::Open(const std::string& name)
{
    const auto fileName = GetFilePath(mBasePath, name);
    LOG_INFO("Opening file: " << fileName);
    try
    {
        return std::make_unique<Oddlib::Stream>(fileName);
    }
    catch (Oddlib::Exception& ex)
    {
        LOG_ERROR(ex.what());
        return nullptr;
    }
}

// ResourcePathAndModsFs
void ResourcePathAndModsFs::AddResourcePath(const std::string& path, int priority)
{
    try
    {
        for (auto& resPath : mResourcePaths)
        {
            if (resPath->Path() == path)
            {
                resPath->SetPriority(priority);
                SortPaths();
                return;
            }
        }

        mResourcePaths.emplace_back(MakeResourcePath(path, priority));
        SortPaths();
    }
    catch (const Oddlib::Exception& ex)
    {
        LOG_ERROR("Failed to add resource path: " << path << " with priority: " << priority << " err: " << ex.what());
    }
}

void ResourcePathAndModsFs::ClearAllResourcePaths()
{
    mResourcePaths.clear();
}

void ResourcePathAndModsFs::AddPcToPsxMapping(const std::string& pcName, const std::string& psxName)
{
    mPcToPsxMappings[pcName] = psxName;
}

void ResourcePathAndModsFs::SortPaths()
{
    std::stable_sort(mResourcePaths.begin(), mResourcePaths.end(),
        [](const std::unique_ptr<IResourcePathAbstraction>& lhs, 
           const std::unique_ptr<IResourcePathAbstraction>& rhs)
    {
        return lhs->Priority() < rhs->Priority();
    });
}

std::unique_ptr<ResourcePathAndModsFs::IResourcePathAbstraction> ResourcePathAndModsFs::MakeResourcePath(std::string path, int priority)
{
    TRACE_ENTRYEXIT;
    if (string_util::ends_with(path, ".bin", true))
    {
        LOG_INFO("Adding CD image: " << path);
        return std::make_unique<ResourcePathAndModsFs::RawCdImagePath>(path, priority);
    }
    LOG_INFO("Adding directory: " << path);
    return std::make_unique<ResourcePathAndModsFs::Directory>(path, priority);
}

std::unique_ptr<Oddlib::IStream> ResourcePathAndModsFs::Open(const std::string& name)
{
    if (mResourcePaths.empty())
    {
        throw Oddlib::Exception("No resource paths configured");
    }

    return FindFile(GetAlternateNames(name), false);
}

std::unique_ptr<Oddlib::IStream> ResourcePathAndModsFs::OpenFmv(const std::string& name, bool pickBiggest /*= false*/)
{
    if (mResourcePaths.empty())
    {
        throw Oddlib::Exception("No resource paths configured");
    }
    return FindFile(GetAlternateNames(name), pickBiggest);
}

ResourcePathAndModsFs::IResourcePathAbstraction* ResourcePathAndModsFs::FindFile(const std::string& name, bool pickBiggest)
{
    IResourcePathAbstraction* biggest = nullptr;
    Sint64 lastSize = 0;
    for (auto& resourceLocation : mResourcePaths)
    {
        const auto fileSize = resourceLocation->Exists(name);
        if (fileSize > 0)
        {
            if (pickBiggest)
            {
                // Search through till we find the biggest file
                if (fileSize > lastSize)
                {
                    lastSize = fileSize;
                    biggest = resourceLocation.get();
                }
            }
            else
            {
                // Return the first location that has the file
                return resourceLocation.get();
            }
        }
    }
    return biggest;
}

std::unique_ptr<Oddlib::IStream> ResourcePathAndModsFs::FindFile(const std::vector<std::string>& names, bool pickBiggestOfSameName)
{
    // Find all resource paths that matches the input names, for each set of resource paths that
    // have each name more than once, optionally pick the biggest.
    std::vector<std::pair<IResourcePathAbstraction*, std::string>> resourcePaths;
    for (const auto& name : names)
    {
        auto resourcePath = FindFile(name, pickBiggestOfSameName);
        if (resourcePath)
        {
            resourcePaths.emplace_back(std::make_pair(resourcePath, name));
        }
    }

    // Grab the resource path with the lowest (i.e the highest) priority
    std::vector<std::pair<IResourcePathAbstraction*, std::string>>::iterator lowestPriority = resourcePaths.end();
    if (!resourcePaths.empty())
    {
        for (auto resPathResult = resourcePaths.begin(); resPathResult != resourcePaths.end(); resPathResult++)
        {
            if (lowestPriority == std::end(resourcePaths) || resPathResult->first->Priority() < lowestPriority->first->Priority())
            {
                lowestPriority = resPathResult;
            }
        }
    }
   
    // Open the file from the lowest priority resource path
    if (lowestPriority != resourcePaths.end())
    {
        return lowestPriority->first->Open(lowestPriority->second);
    }

    return nullptr;
}

// TODO: Change LVL archive interface so we can keep it open/cached here
std::unique_ptr<Oddlib::IStream> ResourcePathAndModsFs::OpenLvlFileChunkById(const std::string& lvl, const std::string& name, Uint32 id)
{
    auto stream = FindFile(GetAlternateNames(lvl), true);
    if (stream)
    {
        Oddlib::LvlArchive lvlArchive(std::move(stream));
        auto file = lvlArchive.FileByName(name);
        if (file)
        {
            auto chunk = file->ChunkById(id);
            if (chunk)
            {
                return chunk->Stream();
            }
        }
    }
    return nullptr;
}

std::unique_ptr<Oddlib::IStream> ResourcePathAndModsFs::OpenLvlFileChunkByType(const std::string& lvl, const std::string& name, Uint32 type)
{
    auto stream = FindFile(GetAlternateNames(lvl), true);
    if (stream)
    {
        Oddlib::LvlArchive lvlArchive(std::move(stream));
        auto file = lvlArchive.FileByName(name);
        if (file)
        {
            auto chunk = file->ChunkByType(type);
            return chunk->Stream();
        }
    }
    return nullptr;
}

std::vector<std::string> ResourcePathAndModsFs::GetAlternateNames(const std::string& name)
{
    std::vector<std::string> ret{ name };
    auto psxName = mPcToPsxMappings.find(name);
    if (psxName != std::end(mPcToPsxMappings))
    {
        ret.emplace_back(psxName->second);
    }
    else
    {
        // Check for a match using lowercase copy of the name
        std::string nameCopy = name;
        std::transform(nameCopy.begin(), nameCopy.end(), nameCopy.begin(), string_util::c_tolower);
        psxName = mPcToPsxMappings.find(nameCopy);
        if (psxName != std::end(mPcToPsxMappings))
        {
            ret.emplace_back(psxName->second);
        }
    }

    // TODO: Derive mod(s) name/path

    return ret;
}

// FileSystem
bool FileSystem::Init()
{
    mGameDataFs.Init();
    mUserSettingsFs.Init();
    InitResourcePaths();
    return true;
}

void FileSystem::DebugUi(GuiContext &gui)
{
    gui_begin_window(&gui, "Resource paths", v2i(700, 200));

    //ImGui::GetStyle().WindowMinSize = ImVec2(260, 200);
    //ImGui::GetStyle().WindowTitleAlign = ImGuiAlign_Center;
    static int idx = -1;
    static std::vector<const char*> items;
    if (items.empty())
    {
        for (size_t i = 0; i < mResourceandModsFs.mResourcePaths.size(); i++)
        {
            items.push_back(mResourceandModsFs.mResourcePaths[i]->Path().c_str());
        }
    }
    static char pathBuffer[255] = {};
    static char priorityBuffer[255] = {};
    gui_begin_listbox(&gui, "path_listbox");//, ImVec2(ImGui::GetWindowWidth()-15, ImGui::GetWindowSize().y - 137)))
    {
        for (size_t i = 0; i < items.size(); i++)
        {
            if (gui_selectable(&gui, items[i], static_cast<int>(i) == idx))
            {
                idx = static_cast<int>(i);
                memset(pathBuffer, 0, sizeof(pathBuffer));
                strncpy(pathBuffer, items[i], sizeof(pathBuffer));
                memset(priorityBuffer, 0, sizeof(priorityBuffer));
                strncpy(priorityBuffer, std::to_string(mResourceandModsFs.mResourcePaths[i]->Priority()).c_str(), sizeof(priorityBuffer));
            }
        }
    }
    gui_end_listbox(&gui);

    gui_textfield(&gui, "Path", pathBuffer, sizeof(pathBuffer));
    gui_textfield(&gui, "Priority", priorityBuffer, sizeof(priorityBuffer));

    if (gui_button(&gui, "Add/Update"))//, v2i(gui_window_client_size(&gui).x, 20)))
    {

        try
        {
            const std::string resPath = pathBuffer;
            const int priority = std::stoi(priorityBuffer);
            mResourceandModsFs.AddResourcePath(resPath, priority);
            items.clear();
        }
        catch (const std::exception&)
        {
        }
    }
    if (gui_button(&gui, "Delete selected")) //ImVec2(ImGui::GetWindowWidth(), 20)))
    {
        if (idx >= 0 && idx < static_cast<int>(mResourceandModsFs.mResourcePaths.size()))
        {
            mResourceandModsFs.mResourcePaths.erase(mResourceandModsFs.mResourcePaths.begin() + idx);
        }
        items.clear();
    }
    gui_end_window(&gui);
}

void FileSystem::InitResourcePaths()
{
    TRACE_ENTRYEXIT;
    
    auto stream = mGameDataFs.Open("data/resource_paths.json");
    const std::string jsonFileContents = stream->LoadAllToString();

    jsonxx::Object rootJsonObject;
    rootJsonObject.parse(jsonFileContents);
    if (rootJsonObject.has<jsonxx::Array>("ResourcePaths"))
    {
        const jsonxx::Array& resourcePaths = rootJsonObject.get<jsonxx::Array>("ResourcePaths");

        for (size_t i = 0; i < resourcePaths.size(); i++)
        {
            const jsonxx::Object& pathAndPriority = resourcePaths.get<jsonxx::Object>(static_cast<Uint32>(i));

            const auto& path = pathAndPriority.get<jsonxx::String>("path");
            const auto& priority = pathAndPriority.get<jsonxx::Number>("priority");

            mResourceandModsFs.AddResourcePath(path,static_cast<int>(priority));
        }
    }
}
