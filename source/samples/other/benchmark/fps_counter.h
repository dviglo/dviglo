// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/common/primitive_types.h>

using namespace dviglo::PrimitiveTypes;

class FpsCounter
{
private:
    // Results
    int resultMinFps_;
    int resultMaxFps_;
    int resultFps_;
    int resultNumFrames_; // Number of frames after warm-up
    float resultTime_; // Time after warm-up
    float totalTime_; // Time with warm-up

    // Current FPS updated every half second
    int currentFps_;
    int frameCounter_;
    float timeCounter_;

public:
    void Clear()
    {
        resultMinFps_ = -1;
        resultMaxFps_ = -1;
        resultFps_ = -1;
        resultNumFrames_ = 0;
        resultTime_ = 0.f;
        totalTime_ = 0.f;
        
        currentFps_ = -1;
        frameCounter_ = 0;
        timeCounter_ = 0.f;
    }

    FpsCounter()
    {
        Clear();
    }

    int GetResultMinFps() const { return resultMinFps_; }
    int GetResultMaxFps() const { return resultMaxFps_; }
    int GetResultFps() const { return resultFps_; }
    float GetTotalTime() const { return totalTime_; }

    int GetCurrentFps() const { return currentFps_; }

    void Update(float timeStep);
};
