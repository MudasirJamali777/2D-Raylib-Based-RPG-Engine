#pragma once
#include <raylib.h>
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
        return { type, "QUICKENING CHARM", "+Faster swings. Stacks.", { 255, 170, 80, 255 } };
    case RelicType::RazorPrism:
        return { type, "HUNTER'S PRISM", "+Crit chance and crit might.", { 255, 90, 190, 255 } };
    case RelicType::NanoforgeHeart:
        return { type, "OAKHEART RELIC", "+Vitality and stronger healing.", { 100, 255, 180, 255 } };
    case RelicType::PhaseBoots:
        return { type, "WINDSTRIDER BOOTS", "+Move speed and dash recovery.", { 100, 220, 255, 255 } };
    case RelicType::SentryKernel:
        return { type, "WATCHER TOTEM", "+Totem damage and duration.", { 120, 160, 255, 255 } };
    case RelicType::EMPCapacitor:
        return { type, "STORM ORB", "+Nova damage and reach.", { 80, 255, 255, 255 } };
    case RelicType::BloodCircuit:
        return { type, "BLOODTHORN SIGIL", "Strikes restore a little HP.", { 255, 85, 110, 255 } };
    }

    return { RelicType::OverclockCore, "UNKNOWN", "", WHITE };
}
