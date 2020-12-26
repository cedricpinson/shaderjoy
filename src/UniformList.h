#pragma once

struct UniformList {
    float iResolution[3] = {};
    float iMouse[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float iTime = 0.0f;
    float iTimeDelta = 0.0f;
    int iFrame = 0;

    int iMouseLocation;
    int iTimeLocation;
    int iResolutionLocation;
    int iTimeDeltaLocation;
    int iFrameLocation;
    int iFrameRateLocation;
};
