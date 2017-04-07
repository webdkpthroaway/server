/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#include "WorldEvent.h"
#include "../ObjectGuid.h"
#include <chrono> // todo
#include <unordered_map>
#include <vector>
#include <cstdint>

class EmeraldDragons final : public WorldEvent
{
public: // swap
    static constexpr uint16_t EVENT_ID = 66; // todo, move
    static constexpr uint32_t MIN_RESPAWN_TIME = 3600 * 24 * 4;
    static constexpr uint32_t MAX_RESPAWN_TIME = 3600 * 24 * 7;

    enum class DeathState
    {
        DEAD, ALIVE
    };

    enum class Dragon : uint32_t
    {
        YSONDRE = 14887,
        LETHON  = 14888,
        EMERISS = 14889,
        TAERAR  = 14890,
        END     = 14891
    };

    enum class Zone
    { 
        SERADANE,
        BOUGH_SHADOW,
        DREAM_BOUGH,
        TWILIGHT_GROVE
    };

    struct State
    {
        Zone zone;
        ObjectGuid guid;
        DeathState deathState;
    };

    struct Coordinates
    {
        uint32_t map;
        float x, y, z, o;
    };

    EmeraldDragons() : WorldEvent(EVENT_ID) {}

    void Enable() override;
    void Update() override;
    void Disable() override;

private: // swap
    const static std::unordered_map<Zone, Coordinates> spawns;

    std::unordered_map<Dragon, State> state;

    void LoadState();
    void PermutateDragons();
    void UpdateRespawnTimeForDeadDragons(std::vector<ObjectGuid>& dragons, time_t respawnTime);

    static uint32_t CheckSingleVariable(uint32_t index, uint32_t default); // todo, move out of class
};