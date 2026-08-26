#pragma once
#include <string>

enum class QuestObjective {
    SlayFoes,
    ClearCourts,
    ClaimBlessings,
    DefeatElites,
    DefeatBosses
};

struct QuestDefinition {
    std::string title;
    std::string description;
    QuestObjective objective = QuestObjective::SlayFoes;
    int target = 1;
    int euroReward = 0;
};

inline const char* QuestObjectiveLabel(QuestObjective objective) {
    switch (objective) {
    case QuestObjective::SlayFoes: return "Foes";
    case QuestObjective::ClearCourts: return "Courts";
    case QuestObjective::ClaimBlessings: return "Blessings";
    case QuestObjective::DefeatElites: return "Elites";
    case QuestObjective::DefeatBosses: return "Bosses";
    }
    return "Task";
}
