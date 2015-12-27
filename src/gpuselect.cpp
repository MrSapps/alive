
// Defaulting to DGPU instead of IGPU unless explicitly profiled otherwise.
#ifdef _WIN32
extern "C"
{
    // For NVIDIA
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

    // For AMD
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
