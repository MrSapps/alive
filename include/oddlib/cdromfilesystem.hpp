#pragma once

class InvalidCdImageException : public Oddlib::Exception
{
public:
    explicit InvalidCdImageException(const char* msg)
        : Exception(msg)
    {

    }
};

const Uint32 kRawSectorSize = 2352;
const Uint32 kFileSystemStartSector = 16;

class RawCdImage
{
public:
    RawCdImage(const RawCdImage&) = delete;
    RawCdImage& operator = (const RawCdImage&) = delete;
    RawCdImage(Oddlib::Stream& stream)
        : mStream(stream)
    {
        ReadFileSystem();
    }

    void LogTree()
    {
        mRoot.Log(1);
    }

private:

    // Each sector is 2352 bytes
#pragma pack(push)
#pragma pack(1)
    struct RawSectorHeader
    {
        Uint8 mSync[12]; // Sync bytes of 0xFF
        Uint8 mMin;
        Uint8 mSecond;
        Uint8 mFrame; // aka sector number
        Uint8 mMode;
        Uint8 mData[2336];
    };

    struct both_endian_32
    {
        Uint32 little;
        Uint32 big;
    };

    struct both_endian_16
    {
        Uint16 little;
        Uint16 big;
    };

    struct date_time
    {
        Uint8 year[4];
        Uint8 month[2];
        Uint8 day[2];
        Uint8 hour[2];
        Uint8 minute[2];
        Uint8 second[2];
        Uint8 mil[2];
        Uint8 gmt;
    };

    struct volume_descriptor
    {
        Uint8 mType;
        Uint8 mMagic[5];
        Uint8 mVersion;
        Uint8 mUnused;
        Uint8 mSys_id[32];
        Uint8 mVol_id[32];
        Uint8 mUnused2[8];
        both_endian_32 vol_size;
        Uint8 mUnused3[32];

        both_endian_16 vol_count;
        both_endian_16 vol_index;
        both_endian_16 logical_block_size;
        both_endian_32 path_table_size;

        Uint32 path_table_location_LSB;
        Uint32 path_table_optional_location_LSB;
        Uint32 path_table_location_MSB;
        Uint32 path_table_optional_location_MSB;

        Uint8 root_entry[34];

        Uint8 vol_set_id[128];
        Uint8 publisher_id[128];
        Uint8 data_preparer_id[128];
        Uint8 app_id[128];
        Uint8 copyright_file[38];
        Uint8 abstract_file[36];
        Uint8 biblio_file[37];

        date_time vol_creation;
        date_time vol_modif;
        date_time vol_expiration;
        date_time vol_effective;

        Uint8 file_structure_version;
        Uint8 unused4;

        Uint8 extra_data[512];
        Uint8 reserved[653];
    };


    struct path_entry
    {
        Uint8 name_length;
        Uint8 extended_length;
        Uint32 location;
        Uint16 parent;
    };

    struct directory_record
    {
        Uint8 length;
        Uint8 extended_length;
        both_endian_32 location;
        both_endian_32 data_length;
        Uint8 date[7]; //irregular
        Uint8 flags;
        Uint8 unit_size;
        Uint8 gap_size;
        both_endian_16 sequence_number;
        Uint8 length_file_id; //files end with ;1
        //file id
        //padding
        //system use
    };

    struct CDXASector
    {
        uint8_t sync[12];
        uint8_t header[4];
        struct CDXASubHeader
        {
            uint8_t file_number;
            uint8_t channel;
            uint8_t submode;
            uint8_t coding_info;
            uint8_t file_number_copy;
            uint8_t channel_number_copy;
            uint8_t submode_copy;
            uint8_t coding_info_copy;
        } subheader;
        uint8_t data[2328];
    };
#pragma pack(pop)

    class Sector
    {
    public:
        Sector(int sectorNumber, Oddlib::Stream& stream)
        {
            stream.Seek((kRawSectorSize*sectorNumber));
            stream.ReadBytes(reinterpret_cast<Uint8*>(&mData), kRawSectorSize);

            RawSectorHeader* rawHeader = reinterpret_cast<RawSectorHeader*>(&mData);
            if (rawHeader->mMode != 2)
            {
                throw InvalidCdImageException(("Only mode 2 sectors supported got (" + std::to_string(rawHeader->mMode) + ")").c_str());
            }

            // To tell the different Mode 2s apart you have to examine bytes 16 - 23 of the sector
            // (the first 8 bytes of Mode Data).If bytes 16 - 19 are not the same as 20 - 23, 
            // then it is Mode 2. If they are equal and bit 5 is on(0x20), then it is Mode 2 Form 2. 
            // Otherwise it is Mode 2 Form 1.
            CDXASector* xaHeader = reinterpret_cast<CDXASector*>(&mData);
            if (xaHeader->subheader.file_number == xaHeader->subheader.file_number_copy &&
                xaHeader->subheader.channel == xaHeader->subheader.channel_number_copy &&
                xaHeader->subheader.submode == xaHeader->subheader.submode_copy &&
                xaHeader->subheader.coding_info == xaHeader->subheader.coding_info_copy &&
                xaHeader->subheader.submode & 0x20
                )
            {
                //  Mode 2 Form 2
                mMode2Form1 = false;
            }
            else
            {
                //  Mode 2 Form 1
                mMode2Form1 = true;
            }
        }

        Uint8* DataPtr()
        {
            if (mMode2Form1)
            {
                return &mData.mData[8];
            }
            else
            {
                return &mData.mData[0];
            }
        }

        Uint32 DataLength()
        {
            return mMode2Form1 ? 2048 : 2336;
        }

    private:
        RawSectorHeader mData;
        bool mMode2Form1 = false;
    };

    const char* NamePointer(directory_record* dr)
    {
        return ((const char*)&dr->length_file_id) + 1;
    }

    bool IsDots(directory_record* dr)
    {
        return (dr->length_file_id == 1 && (NamePointer(dr)[0] == 0x0 || NamePointer(dr)[0] == 0x1));
    }

    void ReadFile(directory_record* dr)
    {
        auto dataSize = dr->data_length.little;
        auto dataSector = dr->location.little;

        std::vector<Uint8> data;
        data.reserve(dataSize);
        do
        {
            auto sizeToRead = dataSize;
            Sector sector(dataSector, mStream);
            if (sizeToRead > sector.DataLength())
            {
                sizeToRead = sector.DataLength();
                dataSize -= sector.DataLength();
                dataSector++;
            }
            else
            {
                sizeToRead = dataSize;
                dataSize -= sizeToRead;
            }

            const Uint8* ptr = sector.DataPtr();
            for (size_t i = 0; i < sizeToRead; i++)
            {
                data.emplace_back(*ptr);
                ptr++;
            }
        } while (dataSize > 0);

        std::cout << "Data size is " << data.size() << std::endl;

        std::string str(reinterpret_cast<char*>(data.data()), data.size());
        std::cout << "Data is: " << str.c_str() << std::endl;
    }

    struct Directory;

    void ReadDirectory(directory_record* rec, Directory* d)
    {
        const auto dataSize = rec->data_length.little;
        auto sector = rec->location.little;
        size_t totalDataRead = 0;

        while (totalDataRead != dataSize)
        {
            Sector sector(sector++, mStream);
            totalDataRead += sector.DataLength();
            directory_record* dr = (directory_record*)sector.DataPtr();
            while (dr->length)
            {
                if (!IsDots(dr))
                {
                    const std::string name(NamePointer(dr), dr->length_file_id);
                    //std::cout << name.c_str() << std::endl;
                    if ((dr->flags & 2) && dr->location.little != rec->location.little)
                    {
                        auto dir = std::make_unique<Directory>();
                        dir->mDir.mDr = *dr;
                        dir->mDir.mName = name;
                        d->mChildren.push_back(std::move(dir));
                        ReadDirectory(dr, d->mChildren.back().get());
                    }
                    else
                    {
                        d->mFiles.emplace_back(DrWrapper{ *dr, name });
                        // ReadFile(dr);
                    }
                }

                char* ptr = reinterpret_cast<char*>(dr);
                dr = reinterpret_cast<directory_record*>(ptr + dr->length);
            }
        }

    }

    void ReadFileSystem()
    {
        volume_descriptor* volDesc = nullptr;
        auto secNum = kFileSystemStartSector - 1;
        do
        {
            secNum++;
            Sector sector(secNum, mStream);
            volDesc = (volume_descriptor*)sector.DataPtr();
            if (volDesc->mType == 1)
            {
                directory_record* dr = (directory_record*)&volDesc->root_entry[0];
                mRoot.mDir = DrWrapper{ *dr, "" };
                ReadDirectory(dr, &mRoot);
                break;
            }
        } while (volDesc->mType != 1);
    }

    Oddlib::Stream& mStream;

    struct DrWrapper
    {
        directory_record mDr;
        std::string mName;
    };

    struct Directory
    {
        DrWrapper mDir;
        std::vector<DrWrapper> mFiles;
        std::vector<std::unique_ptr<Directory>> mChildren;
        void Log(int level)
        {
            {
                std::string indent(level, '-');
                std::cout << indent.c_str() << "[dir] " << mDir.mName.c_str() << std::endl;
            }

            {
                std::string indent(level + 1, '-');
                for (auto& file : mFiles)
                {
                    std::cout << indent.c_str() << "[file] " << file.mName.c_str() << std::endl;
                }
            }

            for (auto& child : mChildren)
            {
                child->Log(level + 1);
            }

        }
    };

    Directory mRoot;
};
