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

#include "NightmareDragons.h"
#include "../ObjectMgr.h"
#include <array>
#include <cstdint>

// less than ideal - DB has no good place for storing this
std::unordered_map<EmeraldDragons::Zone, EmeraldDragons::SpawnCoords> EmeraldDragons::spawns
{
    { Zone::BOUGH_SHADOW,   { 1, 0.0, 0.0, 0.0, 0.0 } },
    { Zone::DREAM_BOUGH,    { 1, 0.0, 0.0, 0.0, 0.0 } },
    { Zone::TWILIGHT_GROVE, { 0, 0.0, 0.0, 0.0, 0.0 } },
    { Zone::SERADANE,       { 0, 0.0, 0.0, 0.0, 0.0 } },

};

void EmeraldDragons::Enable()
{
    LoadStates();
}

void EmeraldDragons::Disable()
{
    // despawn the dragons, if they're up
    
}

void EmeraldDragons::Update()
{
    uint32 aliveCount = DEF_ALIVE_COUNT;
    uint32 reqUpdate = DEF_REQ_UPDATE;
    uint32 respawnTimer = time(nullptr) + urand(MIN_RESPAWN_TIME, MAX_RESPAWN_TIME);

    CheckSingleVariable(VAR_ALIVE_COUNT, aliveCount);
    CheckSingleVariable(VAR_REQ_UPDATE, reqUpdate);
    CheckSingleVariable(VAR_RESP_TIME, respawnTimer);

    auto guids = LoadDragons();

    if (guids.size() != NightmareDragons.size())
    {
        sLog.outError("GameEventMgr: [Dragons of Nightmare] Unable to find GUIDs for all dragons! Aborting!");
        return;
    }

    // if at least one is alive and at least one is dead
    // update the respawn timer of dead dragons by some big value
    // thus we are just waiting for all dragons to die
    if (aliveCount && aliveCount < NightmareDragons.size())
        UpdateRespawnTimeForDeadDragons(guids, 9999999999);

    if (!reqUpdate)
    {
        if (!sGameEventMgr.IsActiveEvent(m_eventId))
        {
            if (respawnTimer < time(nullptr))
            {
                if (!aliveCount)
                {
                    aliveCount = DEF_ALIVE_COUNT;
                    sObjectMgr.SetSavedVariable(VAR_ALIVE_COUNT, DEF_ALIVE_COUNT, true);
                }

                PermutateDragons();
                BASIC_LOG("GameEventMgr: [Dragons of Nightmare] %u of 4 alive.", aliveCount);
                sGameEventMgr.StartEvent(m_eventId, true);
            }
        }

        return;
    }

    // allow some time before event stop so the players could loot the body
    if (reqUpdate > 1)
    {
        --reqUpdate;
        sObjectMgr.SetSavedVariable(VAR_REQ_UPDATE, reqUpdate, true);
        return;
    }

    // stop the event, update all timers
    if (sGameEventMgr.IsActiveEvent(m_eventId))
    {
        UpdateRespawnTimeForDeadDragons(guids, respawnTimer);
        sObjectMgr.SetSavedVariable(VAR_REQ_UPDATE, 0, true);
        sObjectMgr.SetSavedVariable(VAR_REQ_PERM, 1, true);
        BASIC_LOG("GameEventMgr: [Dragons of Nightmare] last dragon just died.", aliveCount);
        sGameEventMgr.StopEvent(m_eventId, true);
    }
}

// todo, remove this function entirely
uint32_t EmeraldDragons::CheckSingleVariable(uint32_t index, uint32_t default)
{
    bool variableExists = false;
    uint32_t value = sObjectMgr.GetSavedVariable(index, default, variableExists);
    
    if (!variableExists)
    {
        sObjectMgr.SetSavedVariable(index, value, true);
    }

    return value;
}

void EmeraldDragons::UpdateRespawnTimeForDeadDragons(std::vector<ObjectGuid> &dragons, time_t respawnTime)
{
    for (auto& guid : dragons)
    {
        auto cData = sObjectMgr.GetCreatureData(guid.GetCounter());

        if (!cData)
        {
            sLog.outError("GameEventMgr: [Dragons of Nightmare] creature data %u not found!", guid.GetCounter());
            continue;
        }

        auto instanceId = sMapMgr.GetContinentInstanceId(cData->mapid, cData->posX, cData->posY);

        // get the map that currently creature belongs to
        auto map = sMapMgr.FindMap(cData->mapid, instanceId);

        if (!map)
        {
            sLog.outError("GameEventMgr: [Dragons of Nightmare] instance %u of map %u not found!", instanceId, cData->mapid);
            continue;
        }

        auto pCreature = map->GetCreature(guid);

        if (!pCreature)
        {
            sLog.outError("GameEventMgr: [Dragons of Nightmare] creature %u not found!", guid.GetCounter());
            continue;
        }

        if (pCreature->isDead())
            map->GetPersistentState()->SaveCreatureRespawnTime(guid.GetCounter(), respawnTime);
    }
}

std::vector<ObjectGuid> EmeraldDragons::LoadDragons()
{
    std::vector<ObjectGuid> guids;

    for (auto& dragonID : NightmareDragons)
    {
        auto dCreatureGuid = sObjectMgr.GetOneCreatureByEntry(dragonID);

        if (dCreatureGuid.IsEmpty())
        {
            sLog.outError("GameEventMgr: [Dragons of Nightmare] creature %u not found in world!", dragonID);
            break;
        }

        guids.push_back(dCreatureGuid);
    }

    return guids;
}

void EmeraldDragons::PermutateDragons()
{
    auto reqPerm = sObjectMgr.GetSavedVariable(VAR_REQ_PERM, 0);

    if (!reqPerm) return;

    std::vector<uint32> permutation = { NPC_LETHON, NPC_EMERISS, NPC_YSONDRE, NPC_TAERAR };
    std::random_shuffle(permutation.begin(), permutation.end());

    sObjectMgr.SetSavedVariable(VAR_PERM_1, permutation[0]);
    sObjectMgr.SetSavedVariable(VAR_PERM_2, permutation[1]);
    sObjectMgr.SetSavedVariable(VAR_PERM_3, permutation[2]);
    sObjectMgr.SetSavedVariable(VAR_PERM_4, permutation[3]);

    sObjectMgr.SetSavedVariable(VAR_REQ_PERM, 0);
}

/* 
 * This could be a lot cleaner if the core offered a better solution for serialising
 * arbitrary structures to the database. Consider this an indefinite todo.
 */
bool EmeraldDragons::LoadState()
{
    spawnStates.lethon  = (uint32_t)CheckSingleVariable(LETHON_ALIVE, 0); // change casts later
    spawnStates.ysondre = (uint32_t)CheckSingleVariable(YSONDRE_ALIVE, 0);
    spawnStates.emeriss = (uint32_t)CheckSingleVariable(EMERISS_ALIVE, 0);
    spawnStates.taerar  = (uint32_t)CheckSingleVariable(TAERAR_ALIVE, 0);
}