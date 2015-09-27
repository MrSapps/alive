#include "filesystem.hpp"
#include "logger.hpp"
#include "string_util.hpp"
#include "SDL.h"
#include "oddlib/stream.hpp"
#include "oddlib/cdromfilesystem.hpp"
#include <algorithm>
#include <fstream>
#include "jsonxx/jsonxx.h"
#include "imgui/imgui.h"

#ifdef _WIN32
const char kDirSeperator = '\\';
#else
const char kDirSeperator = '/';
#endif

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

static bool FileExists(const std::string& fileName)
{
    std::ifstream fileStream;
    fileStream.open(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (fileStream)
    {
        LOG_INFO("File: " << fileName << " found");
        return true;
    }
    LOG_WARNING("File: " << fileName << " not found");
    return false;
}

FileSystem::Directory::Directory(const std::string& path, int priority)
    : IResourcePathAbstraction(path, priority)
{
}

std::unique_ptr<Oddlib::IStream> FileSystem::Directory::Open(const std::string& fileName)
{
    const auto fullFileName = GetFilePath(Path(), fileName);
    return std::make_unique<Oddlib::Stream>(fullFileName);
}

bool FileSystem::Directory::Exists(const std::string& fileName) const
{
    const auto fullFileName = GetFilePath(Path(), fileName);
    return FileExists(fullFileName);
}

FileSystem::RawCdImagePath::RawCdImagePath(const std::string& path, int priority) 
    : IResourcePathAbstraction(path, priority)
{
    mCdImage = std::make_unique<RawCdImage>(path);
    mCdImage->LogTree();
}

std::unique_ptr<Oddlib::IStream> FileSystem::RawCdImagePath::Open(const std::string& fileName)
{
    return mCdImage->ReadFile(fileName, true);
}

bool FileSystem::RawCdImagePath::Exists(const std::string& fileName) const
{
    return mCdImage->FileExists(fileName);
}

FileSystem::FileSystem()
{

}

FileSystem::~FileSystem()
{

}

bool FileSystem::Init()
{
    InitBasePath();
    InitResourcePaths();
    CopyPerUserFiles();
    return true;
}

void FileSystem::AddResourcePath(const std::string& path, int priority)
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

bool FileSystem::Exists(const std::string& name) const
{
    return FileExists(GetFilePath(mBasePath,name));
}

bool FileSystem::ResourceExists(const std::string& name) const
{
    // Look in each resource path by priority
    for (auto& resourceLocation : mResourcePaths)
    {
        if (resourceLocation->Exists(name))
        {
            return true;
        }
    }
    return false;
}

/*
* Rules:
* 1. Zero length files are ignored.
* 2. File systems are searched in priority ordering.
* 3. The alternate file name (PSX file name) is treated the same as the original file name.
* 4. The larger PSX file name wins even if its lower priority - this is to handle a case where BR.MOV in CD1 isn't 0 bytes
* and we need to make sure we pick the CD2 BR.MOV.
*/
std::unique_ptr<Oddlib::IStream> FileSystem::OpenResource(const std::string& name)
{
    LOG_INFO("Opening resource: " << name);

    if (mResourcePaths.empty())
    {
        throw Oddlib::Exception("No resource paths configured");
    }

    // Look in each resource path by priority
    for (auto& resourceLocation : mResourcePaths)
    {
        if (resourceLocation->Exists(name))
        {
            return resourceLocation->Open(name);
        }
    }
  
    throw Oddlib::Exception("Missing resource");
}

void FileSystem::DebugUi()
{
    static bool bSet = false;
    if (!bSet)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 40));
        bSet = true;
    }
    ImGui::Begin("Resource paths", nullptr, ImVec2(700, 200), 1.0f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    
    ImGui::GetStyle().WindowMinSize = ImVec2(260, 200);
    ImGui::GetStyle().WindowTitleAlign = ImGuiAlign_Center;


    static int idx = -1;
    static std::vector<const char*> items;
    if (items.empty())
    {
        for (size_t i = 0; i < mResourcePaths.size(); i++)
        {
            items.push_back(mResourcePaths[i]->Path().c_str());
        }
    }

    static char pathBuffer[255] = {};
    static char priorityBuffer[255] = {};
    if (ImGui::ListBoxHeader("##", ImVec2(ImGui::GetWindowWidth()-15, ImGui::GetWindowSize().y - 137)))
    {
        for (size_t i = 0; i < items.size(); i++)
        {
            if (ImGui::Selectable(items[i], static_cast<int>(i) == idx))
            {
                idx = i;
                memset(pathBuffer, 0, sizeof(pathBuffer));
                strncpy(pathBuffer, items[i], sizeof(pathBuffer));

                memset(priorityBuffer, 0, sizeof(priorityBuffer));
                strncpy(priorityBuffer, std::to_string(mResourcePaths[i]->Priority()).c_str(), sizeof(priorityBuffer));
            }
        }
        ImGui::ListBoxFooter();
    }
    

    ImGui::InputText("Path", pathBuffer, sizeof(pathBuffer));
    ImGui::InputText("Priority", priorityBuffer, sizeof(priorityBuffer));

   
    if (ImGui::Button("Add/Update", ImVec2(ImGui::GetWindowWidth(), 20)))
    {
       
        try
        {
            const std::string resPath = pathBuffer;
            const int priority = std::stoi(priorityBuffer);
            AddResourcePath(resPath, priority);
            items.clear();
        }
        catch (const std::exception&)
        {

        }
    }


    if (ImGui::Button("Delete selected", ImVec2(ImGui::GetWindowWidth(), 20)))
    {
        if (idx >= 0 && idx < static_cast<int>(mResourcePaths.size()))
        {
            mResourcePaths.erase(mResourcePaths.begin() + idx);
        }
        items.clear();
    }

    ImGui::End();
}

void FileSystem::CopyPerUserFiles()
{
    // TODO: Copy config files to user profile so the user can provide their own/edit them
    // this is also where save files will be located, will differ per OS.
}

void FileSystem::SortPaths()
{
    std::stable_sort(mResourcePaths.begin(), mResourcePaths.end(),
        [](const std::unique_ptr<IResourcePathAbstraction>& lhs, const std::unique_ptr<IResourcePathAbstraction>& rhs)
    {
        return lhs->Priority() < rhs->Priority();
    });
}

std::unique_ptr<FileSystem::IResourcePathAbstraction> FileSystem::MakeResourcePath(std::string path, int priority)
{
    TRACE_ENTRYEXIT;
    if (string_util::ends_with(path, ".bin", true))
    {
        LOG_INFO("Adding CD image: " << path);
        return std::make_unique<FileSystem::RawCdImagePath>(path, priority);
    }
    LOG_INFO("Adding directory: " << path);
    return std::make_unique<FileSystem::Directory>(path, priority);
}

std::unique_ptr<Oddlib::IStream> FileSystem::Open(const std::string& name)
{
    const auto fileName = GetFilePath(mBasePath, name);
    LOG_INFO("Opening file: " << fileName);
    return std::make_unique<Oddlib::Stream>(fileName);
}

void FileSystem::InitBasePath()
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
        mBasePath = "." + kDirSeperator;
        LOG_ERROR("SDL_GetBasePath failed, falling back to ." << kDirSeperator);
    }
    LOG_INFO("basePath is " << mBasePath);
}

void FileSystem::InitResourcePaths()
{
    TRACE_ENTRYEXIT;

    auto stream = Open("data/resource_paths.json");
    const std::string jsonFileContents = stream->LoadAllToString();

    jsonxx::Object rootJsonObject;
    rootJsonObject.parse(jsonFileContents);
    if (rootJsonObject.has<jsonxx::Array>("ResourcePaths"))
    {
        const jsonxx::Array& resourcePaths = rootJsonObject.get<jsonxx::Array>("ResourcePaths");

        for (size_t i = 0; i < resourcePaths.size(); i++)
        {
            const jsonxx::Object& pathAndPriority = resourcePaths.get<jsonxx::Object>(i);

            const auto& path = pathAndPriority.get<jsonxx::String>("path");
            const auto& priority = pathAndPriority.get<jsonxx::Number>("priority");

            AddResourcePath(path,static_cast<int>(priority));
        }
    }
}
