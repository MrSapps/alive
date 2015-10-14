#pragma once

enum AudioInterpolation {
    AudioInterpolation_none,
    AudioInterpolation_linear, // Causes digitized feel when lowering frequency. Also noise.
    AudioInterpolation_cubic, // Cause hight ringing, which can be removed by filtering.
    AudioInterpolation_hermite, // No digitized feel. No ringing. Just perfect!
};
