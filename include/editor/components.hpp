#pragma once
#include "serialization/serialization.hpp"

// Component to designate the camera considered primary. Used for editor.
struct MainCameraComp {
    bool _dummy;

    REGISTER_SERIALIZABLE(MainCameraComp, MainCamera)
};
