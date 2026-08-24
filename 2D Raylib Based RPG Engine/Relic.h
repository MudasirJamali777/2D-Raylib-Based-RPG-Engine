#pragma once
#include "raylib.h"
#include <string>

enum class RelicType {
    OverclockCore,
    RazorPrism,
    NanoforgeHeart,
    PhaseBoots,
    SentryKernel,
    EMPCapacitor,
    BloodCircuit
};

struct RelicChoice {
    RelicType type = RelicType::OverclockCore;
    std::string name;
    std::string description;
    Color color = WHITE;
};

inline RelicChoice GetRelicData(RelicType type) {
    switch (type) {
    case RelicType::OverclockCore:
        return { type, "OVERCLOCK CORE", "+Attack speed. Stacks.", { 255, 170, 80, 255 } };
    case RelicType::RazorPrism:
        return { type, "RAZOR PRISM", "+Crit chance and crit damage.", { 255, 90, 190, 255 } };
    case RelicType::NanoforgeHeart:
        return { type, "NANOFORGE HEART", "+Max HP and stronger healing.", { 100, 255, 180, 255 } };
    case RelicType::PhaseBoots:
        return { type, "PHASE BOOTS", "+Move speed and dash recovery.", { 100, 220, 255, 255 } };
    case RelicType::SentryKernel:
        return { type, "SENTRY KERNEL", "+Turret damage and duration.", { 120, 160, 255, 255 } };
    case RelicType::EMPCapacitor:
        return { type, "EMP CAPACITOR", "+EMP damage and radius.", { 80, 255, 255, 255 } };
    case RelicType::BloodCircuit:
        return { type, "BLOOD CIRCUIT", "Hits restore a little HP.", { 255, 85, 110, 255 } };
    }

    return { RelicType::OverclockCore, "UNKNOWN", "", WHITE };
}
