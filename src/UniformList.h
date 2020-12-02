#pragma once

struct UniformList {
    float iResolution[3] = {};
    float iTime = 0.0f;
    float iTimeDelta = 0.0f;
    int iFrame = 0;
    float iFrameRate = 0;

    int iTimeLocation;
    int iResolutionLocation;
    int iTimeDeltaLocation;
    int iFrameLocation;
    int iFrameRateLocation;
};

