#pragma once

extern "C" {
    #include "d_trap.h"

    #include "p_local.h"
    #include "s_sound.h"
    #include "r_state.h"
    #include "z_zone.h"
}

#include <map>
#include <vector>
#include <set>
#include <utility>
#include <algorithm>
#include <cstdlib>
#include <ctime>

typedef enum
{
    // Is there at least 1 enemy in the sector.
    TSF_ONE_ENEMY = 0x1,
    // Is there at least 2 enemies in the sector.
    TSF_TWO_ENEMY = 0x2,
    // Is there at least 4 enemies in the sector.
    TSF_FOUR_ENEMY = 0x4,
    // Is there at least 8 enemies in the sector.
    TSF_EIGHT_ENEMY = 0x8,
    // Is there at least 1 living enemy in the sector.
    TSF_ONE_LIVING_ENEMY = 0x10,
    // Is there at least 2 living enemies in the sector.
    TSF_TWO_LIVING_ENEMY = 0x20,
    // Is there at least 4 living enemies in the sector.
    TSF_FOUR_LIVING_ENEMY = 0x40,
    // Is there at least 8 living enemies in the sector.
    TSF_EIGHT_LIVING_ENEMY = 0x80,
    // Is there at least 1 dead enemy in the sector.
    TSF_ONE_DEAD_ENEMY = 0x100,
    // Is there at least 2 dead enemies in the sector.
    TSF_TWO_DEAD_ENEMY = 0x200,
    // Is there at least 4 dead enemies in the sector.
    TSF_FOUR_DEAD_ENEMY = 0x400,
    // Is there at least 8 dead enemies in the sector.
    TSF_EIGHT_DEAD_ENEMY = 0x800,
    // Is there at least two spawn points in this sector
    TSF_TWO_SPAWN_POINT = 0x1000,
    // Is there at least three spawn points in this sector
    TSF_THREE_SPAWN_POINT = 0x2000,
    // Is there at least four spawn points in this sector
    TSF_FOUR_SPAWN_POINT = 0x4000,
    // Is there at least five spawn points in this sector
    TSF_FIVE_SPAWN_POINT = 0x8000,
    // Is there at least six spawn points in this sector
    TSF_SIX_SPAWN_POINT = 0x10000,
    // Is there at least seven spawn points in this sector
    TSF_SEVEN_SPAWN_POINT = 0x20000,
    // Is there at least eight spawn points in this sector
    TSF_EIGHT_SPAWN_POINT = 0x40000,
    // Is there at least sixteen spawn points in this sector
    TSF_SIXTEEN_SPAWN_POINT = 0x80000,
    TSF_UNUSED1 = 0x100000,
    TSF_UNUSED2 = 0x200000,
    TSF_UNUSED3 = 0x400000,
    TSF_UNUSED4 = 0x800000,
    TSF_UNUSED5 = 0x1000000,
    TSF_UNUSED6 = 0x2000000,
    TSF_UNUSED7 = 0x4000000,
    TSF_UNUSED8 = 0x8000000,
    TSF_UNUSED9 = 0x10000000,
    TSF_UNUSED10 = 0x20000000,
    TSF_UNUSED11 = 0x40000000,
    TSF_UNUSED12 = 0x80000000
} trapspawnflag_t;

struct TrapData
{
    int flags;
    std::vector<std::pair<fixed_t, fixed_t>> midpoints;
};

std::map<int, TrapData> sectorTrapData;
mobj_t mannequin;

bool AttemptToSpawnTrapMobs(int sectorIndex, int type);
bool AttemptToSpawn(int type, fixed_t x, fixed_t y);

bool IsSpawnPointValid(fixed_t x, fixed_t y)
{
    return P_CheckPosition(&mannequin, x, y);
}

bool static IsResurrectableEnemy(int type)
{
    if (type == 68 ||   // THING_TYPE_ARACHNOTRON
        type == 3003 || // THING_TYPE_BARON_OF_HELL
        type == 3005 || // THING_TYPE_CACODEMON
        type == 65 ||   // THING_TYPE_HEAVY_WEAPON_DUDE
        type == 3002 || // THING_TYPE_DEMON
        type == 3004 || // THING_TYPE_ZOMBIEMAN
        type == 9 ||    // THING_TYPE_SHOTGUN_GUY
        type == 69 ||   // THING_TYPE_HELL_KNIGTH
        type == 3001 || // THING_TYPE_IMP
        type == 67 ||   // THING_TYPE_MANCUBUS
        type == 66 ||   // THING_TYPE_REVENANT
        type == 58 ||   // THING_TYPE_SPECTRE
        type == 84)     // THING_TYPE_WOLFENSTEIN_SS
    {
        return true;
    }

    return false;
}

bool static IsEnemy(int type)
{
    if (IsResurrectableEnemy(type) ||
        type == 64 ||   // THING_TYPE_ARCH_VILLE
        type == 16 ||   // THING_TYPE_CYBERDEMON
        type == 3006 || // THING_TYPE_LOST_SOUL
        type == 71 ||   // THING_TYPE_PAIN_ELEMENTAL
        type == 7)      // THING_TYPE_SPIDER_MASTERMIND
    {
        return true;
    }

    return false;
}

bool static AttemptToReduceSectors(std::vector<int>& validSectors, int targetFlags)
{
    std::vector<int> tempValidSectors;
    for (int i = 0; i < numsectors; ++i)
    {
        auto search = sectorTrapData.find(i);
        if (search == sectorTrapData.end())
        {
            continue;
        }

        auto trapData = &sectorTrapData[i];

        // check that this sector has the target flags
        if ((trapData->flags & targetFlags) != targetFlags)
        {
            continue;
        }

        tempValidSectors.push_back(i);
    }

    if (tempValidSectors.empty())
    {
        return false;
    }

    fprintf(stdout, "Reduced valid sectors from %d to %d using flag 0x%X\n", (int)validSectors.size(), (int)tempValidSectors.size(), targetFlags);
    validSectors = tempValidSectors;
    return true;
}

void SetupTrapData()
{
    sectorTrapData.clear();

    // used for collision checking
    mannequin.radius = 24 * FRACUNIT;

    int numDamagingSectors = 0;
    int numSecretSectors = 0;
    int numBadSubSectors = 0;
    int numSmallSubSectors = 0;
    int numFinalSubSectors = 0;

    std::set<vertex_t*> allSubSectorVerts;
    for (int i = 0; i < numsubsectors; ++i)
    {
        subsector_t* subsector = &(subsectors[i]);

        // ignore damaging sectors
        if (subsector->sector->special == 4 ||  // SECTOR_TYPE_BOTH
            subsector->sector->special == 5 ||  // SECTOR_TYPE_DAMAGE_10
            subsector->sector->special == 7 ||  // SECTOR_TYPE_DAMAGE_5
            subsector->sector->special == 11 || // SECTOR_TYPE_END
            subsector->sector->special == 16)   // SECTOR_TYPE_DAMAGE_20
        {
            numDamagingSectors++;
            continue;
        }

        // ignore secret sectors
        if (subsector->sector->special == 9) // SECTOR_TYPE_SECRET
        {
            numSecretSectors++;
            continue;
        }

        // TODO: figure out a better way of trimming bad subsectors
        if (subsector->numlines < 2)
        {
            numBadSubSectors++;
            continue;
        }

        allSubSectorVerts.clear();

        int maxX = INT_MIN;
        int minX = INT_MAX;
        int maxY = INT_MIN;
        int minY = INT_MAX;
        for (int j = 0; j < subsector->numlines; ++j)
        {
            seg_t* seg = &(segs[subsector->firstline + j]);
            allSubSectorVerts.insert(seg->v1);
            allSubSectorVerts.insert(seg->v2);

            int x1 = seg->v1->x / FRACUNIT;
            int y1 = seg->v1->y / FRACUNIT;
            int x2 = seg->v2->x / FRACUNIT;
            int y2 = seg->v2->y / FRACUNIT;

            if (x1 > maxX)
            {
                maxX = x1;
            }
            if (x1 < minX)
            {
                minX = x1;
            }
            if (y1 > maxY)
            {
                maxY = y1;
            }
            if (y1 < minY)
            {
                minY = y1;
            }

            if (x2 > maxX)
            {
                maxX = x2;
            }
            if (x2 < minX)
            {
                minX = x2;
            }
            if (y2 > maxY)
            {
                maxY = y2;
            }
            if (y2 < minY)
            {
                minY = y2;
            }
        }

        // TODO: trim area too low?
        if (((maxX - minX) < 64) || ((maxY - minY) < 64))
        {
            numSmallSubSectors++;
            //fprintf(stdout, "Subsector %d: (%d, %d) -> (%d, %d) | (%d, %d)\n", i, minX, minY, maxX, maxY, (maxX - minX), (maxY - minY));
            continue;
        }

        //fprintf(stdout, "Set has %d items\n", allSubSectorVerts.size());

        fixed_t x = 0;
        fixed_t y = 0;
        std::for_each(allSubSectorVerts.cbegin(), allSubSectorVerts.cend(), [&x, &y](vertex_t* v)
        { 
            //fprintf(stdout, "Vert: %d, %d\n", (v->x / FRACUNIT), (v->y / FRACUNIT));
            x += (v->x / FRACUNIT);
            y += (v->y / FRACUNIT);
        });
        //fprintf(stdout, "Sum Vert: %d, %d\n", x, y);
        x = x / (int)allSubSectorVerts.size();
        y = y / (int)allSubSectorVerts.size();
        //fprintf(stdout, "Average Vert: %d, %d\n", x, y);
        x = x * FRACUNIT;
        y = y * FRACUNIT;

        for (int j = 0; j < numsectors; ++j)
        {
            if (&sectors[j] != subsector->sector)
            {
                continue;
            }

            if (sectorTrapData.find(j) != sectorTrapData.end())
            {
                auto value = std::pair<fixed_t, fixed_t>(x, y);
                sectorTrapData[j].midpoints.push_back(value);
            }
            else
            {
                auto trapData = TrapData();
                auto value = std::pair<fixed_t, fixed_t>(x, y);
                trapData.midpoints.push_back(value);
                sectorTrapData.insert({j, trapData});
            }

            break;
        }

        numFinalSubSectors++;
    }

    fprintf(stdout, "%d Sectors, %d subsectors. Trimmed %d damaging, %d secret, %d bad, %d small\n", (int)sectorTrapData.size(), numFinalSubSectors, numDamagingSectors, numSecretSectors, numBadSubSectors, numSmallSubSectors);
}

// TODO: remove player sector?
void ActivateTrap(int type)
{
    // TODO: Remove
    //{
    //    int count = 0;
    //    for (int i = 0; i < sectorTrapData.size(); ++i)
    //    {
    //        for (int j = 0; j < sectorTrapData[i].size(); ++j)
    //        {
    //            mobj_t* newMob = P_SpawnMobj(sectorTrapData[i][j].first, sectorTrapData[i][j].second, ONFLOORZ, MT_BARREL);
    //            //fprintf(stdout, "Spawned at %d, %d, %d\n", (newMob->x / FRACUNIT), (newMob->y / FRACUNIT), (newMob->z / FRACUNIT));
    //            count++;
    //        }
    //    }
    //    fprintf(stdout, "Spawned %d barrels\n", count);
    //}

    std::vector<int> validSectors;
    sector_t* sector;
    for (int i = 0; i < numsectors; ++i)
    {
        auto search = sectorTrapData.find(i);
        if (search == sectorTrapData.end())
        {
            continue;
        }

        sector = &sectors[i];

        // remove sectors that are too short for this enemy type
        switch (type)
        {
            case 65900: // Archvile Trap
                if (sector->ceilingheight - sector->floorheight < (72 * FRACUNIT))
                {
                    fprintf(stdout, "Sector %d is too short for Archvile (%d < 72)\n", i, (sector->ceilingheight - sector->floorheight) / FRACUNIT);
                    continue;
                }
                break;
            case 65901: // Revenant Trap
                if (sector->ceilingheight - sector->floorheight < (80 * FRACUNIT))
                {
                    fprintf(stdout, "Sector %d is too short for Revenant (%d < 80)\n", i, (sector->ceilingheight - sector->floorheight) / FRACUNIT);
                    continue;
                }
                break;
        }

        // update the flags for this sector
        auto trapData = &sectorTrapData[i];
        trapData->flags = 0;

        int validSpawnPoints = 0;
        for (int j = 0; j < trapData->midpoints.size(); ++j)
        {
            if (!IsSpawnPointValid(trapData->midpoints[j].first, trapData->midpoints[j].second))
            {
                continue;
            }

            validSpawnPoints++;
            if (validSpawnPoints > 15)
            {
                break;
            }
        }

        if (validSpawnPoints < 1)
        {
            continue;
        }

        // this sector meets minimum requirements to be considered valid
        validSectors.push_back(i);

        if (validSpawnPoints > 1)
        {
            trapData->flags |= TSF_TWO_SPAWN_POINT;
            if (validSpawnPoints > 2)
            {
                trapData->flags |= TSF_THREE_SPAWN_POINT;
                if (validSpawnPoints > 3)
                {
                    trapData->flags |= TSF_FOUR_SPAWN_POINT;
                    if (validSpawnPoints > 4)
                    {
                        trapData->flags |= TSF_FIVE_SPAWN_POINT;
                        if (validSpawnPoints > 5)
                        {
                            trapData->flags |= TSF_SIX_SPAWN_POINT;
                            if (validSpawnPoints > 6)
                            {
                                trapData->flags |= TSF_SEVEN_SPAWN_POINT;
                                if (validSpawnPoints > 7)
                                {
                                    trapData->flags |= TSF_EIGHT_SPAWN_POINT;
                                    if (validSpawnPoints > 15)
                                    {
                                        trapData->flags |= TSF_SIXTEEN_SPAWN_POINT;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        int livingEnemies = 0;
        int deadEnemies = 0;
        auto thing = sector->thinglist;
        while (thing != nullptr)
        {
            if (IsEnemy(thing->info->doomednum))
            {
                if (thing->health > 0)
                {
                    livingEnemies++;
                }
                else
                {
                    deadEnemies++;
                }

                if (livingEnemies > 7 && deadEnemies > 7)
                {
                    break;
                }
            }

            thing = thing->snext;
        }

        if (livingEnemies + deadEnemies > 0)
        {
            trapData->flags |= TSF_ONE_ENEMY;
            if (livingEnemies > 0)
            {
                trapData->flags |= TSF_ONE_LIVING_ENEMY;
            }
            if (deadEnemies > 0)
            {
                trapData->flags |= TSF_ONE_DEAD_ENEMY;
            }

            if (livingEnemies + deadEnemies > 1)
            {
                trapData->flags |= TSF_TWO_ENEMY;
                if (livingEnemies > 1)
                {
                    trapData->flags |= TSF_TWO_LIVING_ENEMY;
                }
                if (deadEnemies > 1)
                {
                    trapData->flags |= TSF_TWO_DEAD_ENEMY;
                }

                if (livingEnemies + deadEnemies > 3)
                {
                    trapData->flags |= TSF_FOUR_ENEMY;
                    if (livingEnemies > 3)
                    {
                        trapData->flags |= TSF_FOUR_LIVING_ENEMY;
                    }
                    if (deadEnemies > 3)
                    {
                        trapData->flags |= TSF_FOUR_DEAD_ENEMY;
                    }

                    if (livingEnemies + deadEnemies > 7)
                    {
                        trapData->flags |= TSF_EIGHT_ENEMY;
                        if (livingEnemies > 7)
                        {
                            trapData->flags |= TSF_EIGHT_LIVING_ENEMY;
                        }
                        if (deadEnemies > 7)
                        {
                            trapData->flags |= TSF_EIGHT_DEAD_ENEMY;
                        }
                    }
                }
            }
        }
    }

    // always attempt to reduce validSectors to only include sectors with enemies
    AttemptToReduceSectors(validSectors, TSF_ONE_ENEMY);

    switch (type)
    {
        case 65900: // Archvile Trap
            if (AttemptToReduceSectors(validSectors, TSF_EIGHT_DEAD_ENEMY)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_FOUR_DEAD_ENEMY)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_TWO_DEAD_ENEMY)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_ONE_DEAD_ENEMY)) { break; }
            break;
        case 65901: // Revenant Trap
            if (AttemptToReduceSectors(validSectors, TSF_ONE_LIVING_ENEMY | TSF_FOUR_SPAWN_POINT)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_ONE_LIVING_ENEMY | TSF_THREE_SPAWN_POINT)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_ONE_LIVING_ENEMY | TSF_TWO_SPAWN_POINT)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_ONE_LIVING_ENEMY)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_FOUR_SPAWN_POINT)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_THREE_SPAWN_POINT)) { break; }
            if (AttemptToReduceSectors(validSectors, TSF_TWO_SPAWN_POINT)) { break; }
            break;
    }

    int remainingTries = 100;
    while (remainingTries > 0)
    {
        int rnd = rand() % validSectors.size();
        if (AttemptToSpawnTrapMobs(validSectors[rnd], type))
        {
            break;
        }

        remainingTries--;
    }
}

bool AttemptToSpawnTrapMobs(int sectorIndex, int type)
{
    bool spawnedSomething = false;
    auto midpoints = sectorTrapData[sectorIndex].midpoints;

    int numToSpawn;
    switch (type)
    {
        case 65901:
            numToSpawn = 4;
            break;
        case 65900:
        default:
            numToSpawn = 1;
            break;
    }

    int remainingTries = 100;
    while (!midpoints.empty())
    {
        int rnd = rand() % midpoints.size();
        if (AttemptToSpawn(type, midpoints[rnd].first, midpoints[rnd].second))
        {
            spawnedSomething = true;
            numToSpawn--;

            if (numToSpawn == 0)
            {
                return true;
            }
        }
        
        midpoints.erase(midpoints.begin() + rnd);
        remainingTries--;
    }

    return spawnedSomething;
}

bool AttemptToSpawn(int type, fixed_t x, fixed_t y)
{
    if (!IsSpawnPointValid(x, y))
    {
        return false;
    }

    mobj_t* fog = P_SpawnMobj(x, y, ONFLOORZ, MT_SPAWNFIRE);
    S_StartSound(fog, sfx_telept);

    mobj_t* newMob;
    switch (type)
    {
        case 65900: // Archvile Trap
            newMob = P_SpawnMobj(x, y, ONFLOORZ, MT_VILE);
            break;
        case 65901: // Revenant Trap
            newMob = P_SpawnMobj(x, y, ONFLOORZ, MT_UNDEAD);
            break;
        default:
            newMob = P_SpawnMobj(x, y, ONFLOORZ, MT_BARREL);
            break;
    }
    fprintf(stdout, "Spawned at %d, %d, %d\n", (newMob->x / FRACUNIT), (newMob->y / FRACUNIT), (newMob->z / FRACUNIT));

    return true;
}