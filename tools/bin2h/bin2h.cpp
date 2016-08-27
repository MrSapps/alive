#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cout << "bin2h - Convert any file to a header file" << std::endl;
        std::cout << "Usage: Input file, output file, array name" << std::endl;
        return 1;
    }

    const std::string inputFileName(argv[1]);

    // Open input file
    std::fstream inFile(inputFileName.c_str(), std::ios::in | std::ios::binary);
    if (!inFile)
    {
        std::cout << "Failed to open " << inputFileName << " for reading" << std::endl;
        return 1;
    }

    // Get input file size
    inFile.seekg(0, inFile.end);
    const std::streamoff inFileSizeBytes = inFile.tellg();
    inFile.seekg(0, inFile.beg);

    // Read input into a buffer
    std::vector<unsigned char> inFileBytes(static_cast<size_t>(inFileSizeBytes));
    inFile.read(reinterpret_cast<char *>(inFileBytes.data()), inFileBytes.size());

    // Create temporary output file
    const std::string outputFileName(argv[2]);
    std::fstream outFile(outputFileName.c_str(), std::ios::out | std::ios::binary);
    if (!outFile)
    {
        std::cout << "Failed to open " << outputFileName << " for writing" << std::endl;
        return 1;
    }

    const std::string pragmaOnce = "#pragma once";
    outFile.write(pragmaOnce.data(), pragmaOnce.length());
    outFile << std::endl;
    outFile << std::endl;

    const std::string generated = "// This header was generated from " + inputFileName + ", any local changes will be lost";
    outFile.write(generated.data(), generated.length());
    outFile << std::endl;
    outFile << std::endl;

    // Write the generated header
    const std::string headerStart("#include <vector>\n\nstd::vector<unsigned char> get_" + std::string(argv[3]) + "()\n{\n    const static unsigned char temp[] =");
    outFile.write(headerStart.data(), headerStart.length());
    outFile << std::endl;

    const std::string arrayStart = "    {";
    outFile.write(arrayStart.data(), arrayStart.length());
    outFile << std::endl;

    // Write out each byte of the input as hex
    std::stringstream ss;
    size_t breakPos = 0;
    for (size_t idx = 0; idx < inFileBytes.size(); idx++)
    {
        if (breakPos == 0)
        {
            ss << "        ";
        }

        ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(inFileBytes[idx]);
        if (idx+1 != inFileBytes.size())
        {
            ss << ", ";
        }

        breakPos++;
        if (breakPos > 18)
        {
            breakPos = 0;
            ss << "\n";
        }
    }

    const std::string arrayData = ss.str();
    outFile.write(arrayData.data(), arrayData.length());

    outFile << std::endl;
    const std::string arrayEnd = "    };\n    return std::vector<unsigned char>(std::begin(temp), std::end(temp));\n}\n";
    outFile.write(arrayEnd.data(), arrayEnd.length());
    outFile << std::endl;

    return 0;
}
