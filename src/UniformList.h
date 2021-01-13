#pragma once

struct UniformList {
    float iResolution[3] = {};
    float iMouse[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float iTime = 0.0f;
    float iTimeDelta = 0.0f;
    int iFrame = 0;
    int iChannel[4] = {0, 1, 2, 3};
    float iChannelResolution[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    int iChannelResolutionLocation;
    int iChannelLocation[4];
    int iMouseLocation;
    int iTimeLocation;
    int iResolutionLocation;
    int iTimeDeltaLocation;
    int iFrameLocation;
    int iFrameRateLocation;
};
