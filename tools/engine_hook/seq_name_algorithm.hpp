#pragma once

// 0x49BE30
inline u32 SeqResourceNameHash(const char *seqFileName)
{
    u32 hashId = 0;

    size_t seqFileNameLength = strlen(seqFileName) - 1;
    if (seqFileNameLength > 8)
    {
        seqFileNameLength = 8;
    }

    size_t index = 0;
    if (seqFileNameLength)
    {
        do
        {
            char letter = seqFileName[index];
            if (letter == '.')
            {
                break;
            }

            const u32 temp = 10 * hashId;
            if (letter < '0' || letter > '9')
            {
                if (letter >= 'a')
                {
                    if (letter <= 'z')
                    {
                        letter -= ' ';
                    }
                }
                hashId = letter % 10 + temp;
            }
            else
            {
                hashId = index || *seqFileName != '0' ? temp + letter - '0' : temp + 9;
            }
            ++index;
        } while (index < seqFileNameLength);
    }
    return hashId;
}
