/*
#pragma once

#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

namespace OpenApoc
{
    class BattleUnit;
    enum BattleUnit::Stance;
    class BattleTile;
    class TileMap;

    class BattleUnitMission
    {
    public:

        enum class MissionType
        {
            GotoLocation,
            Snooze,
            ChangeStance
        };

        // Methods used in pathfinding etc.
        bool getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest);
        void update(GameState &state, BattleUnit &u, unsigned int ticks);
        bool isFinished(GameState &state, BattleUnit &u);
        void start(GameState &state, BattleUnit &u);
        void setPathTo(Vehicle &v, Vec3<int> target, int maxIterations = 500);
        bool advanceAlongPath(Vec3<float> &dest);

        // Methods to create new missions
        static BattleUnitMission *gotoLocation(BattleUnit &u, Vec3<int> target);
        static BattleUnitMission *snooze(BattleUnit &u, unsigned int ticks);
        static BattleUnitMission *changeStance(BattleUnit &u, BattleUnit::Stance stance);

        UString getName();

        MissionType type = MissionType::GotoLocation;

        // GotoLocation
        Vec3<int> targetLocation = { 0, 0, 0 };
        // Snooze
        unsigned int timeToSnooze = 0;

        std::list<Vec3<int>> currentPlannedPath;
    };
} // namespace OpenApoc

*/