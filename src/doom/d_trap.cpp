#pragma once

extern "C" {
    #include "d_trap.h"

    #include "p_local.h"
    #include "s_sound.h"
}

#include <vector>
#include <map>
#include <set>
#include <algorithm>

using Midpoints = std::vector<std::pair<fixed_t, fixed_t>>;

std::map<int, Midpoints> sectorMidpoints;
mobj_t mannequin;

bool static IsResurrectableEnemy(const int type)
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

bool static IsEnemy(const int type)
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

bool static IsSectorTooShort(const sector_t& sector, const int heightUnits)
{
    return (sector.ceilingheight - sector.floorheight) < (heightUnits * FRACUNIT);
}

bool static IsSpawnPointValid(const fixed_t x, const fixed_t y)
{
    return P_CheckPosition(&mannequin, x, y);
}

int CalculateNumberValidSpawns(const Midpoints& midpoints, const int radiusUnits)
{
    // used for collision checking
    mannequin.radius = radiusUnits * FRACUNIT;

    int validSpawnPoints = 0;
    for (int j = 0; j < midpoints.size(); ++j)
    {
        if (!IsSpawnPointValid(midpoints[j].first, midpoints[j].second))
        {
            continue;
        }

        validSpawnPoints++;
    }

    return validSpawnPoints;
}

class SpawnTrap
{
  public:
    virtual ~SpawnTrap() { }
    virtual int GetType() const = 0;
    virtual mobjtype_t GetTypeToSpawn() const = 0;
    virtual int GetNumberToSpawn() const = 0;
    virtual int GetSpawnedHeight() const = 0;
    virtual int GetSpawnedRadius() const = 0;
    virtual bool ShouldSpawnTogether() const = 0;
    virtual int CalculateSectorPriority(const sector_t& sector, const Midpoints& midpoints) const = 0;
};

// ============================================================================
// ============================================================================

class ArchvileTrap : public SpawnTrap
{
public:
    static constexpr int type{65900};
    int GetType() const override { return type; }
    mobjtype_t GetTypeToSpawn() const override { return MT_VILE; }
    int GetNumberToSpawn() const override { return 1; }
    int GetSpawnedHeight() const override { return 72; }
    int GetSpawnedRadius() const override { return 20; }
    bool ShouldSpawnTogether() const override { return true; }

    int CalculateSectorPriority(const sector_t& sector, const Midpoints& midpoints) const override
    {
        // exclude sectors that are too short
        if (IsSectorTooShort(sector, GetSpawnedHeight())) { return 0; }

        // exclude sectors that dont have any spawnpoints that fit this enemy
        if (CalculateNumberValidSpawns(midpoints, GetSpawnedRadius()) <= 0) { return 0; }

        bool sectorHasEnemy = false;
        int deadResurrectableEnemies = 0;
        mobj_t* thing = sector.thinglist;
        while (thing != nullptr)
        {
            if (IsResurrectableEnemy(thing->info->doomednum))
            {
                sectorHasEnemy = true;

                if (thing->health <= 0)
                {
                    deadResurrectableEnemies++;
                }

                if (deadResurrectableEnemies >= 8)
                {
                    break;
                }
            }

            if (!sectorHasEnemy && IsEnemy(thing->info->doomednum))
            {
                sectorHasEnemy = true;
            }

            thing = thing->snext;
        }

        // prioritize sectors based on number of dead resurrectable enemies
        if (deadResurrectableEnemies >= 8) { return 5; }
        if (deadResurrectableEnemies >= 4) { return 4; }
        if (deadResurrectableEnemies >= 2) { return 3; }
        if (deadResurrectableEnemies >= 1) { return 2; }

        // worst case, exclude sectors that have no enemies
        return sectorHasEnemy? 1 : 0;
    }
};

class RevenantTrap : public SpawnTrap
{
public:
    static constexpr int type{65901};
    int GetType() const override { return type; }
    mobjtype_t GetTypeToSpawn() const override { return MT_UNDEAD; }
    int GetNumberToSpawn() const override { return 3; }
    int GetSpawnedHeight() const override { return 80; }
    int GetSpawnedRadius() const override { return 20; }
    bool ShouldSpawnTogether() const override { return true; }

    int CalculateSectorPriority(const sector_t& sector, const Midpoints& midpoints) const override
    {
        // exclude sectors that are too short
        if (IsSectorTooShort(sector, GetSpawnedHeight())) { return 0; }

        // exclude sectors that dont have any spawnpoints that fit this enemy
        int numValidSpawns = CalculateNumberValidSpawns(midpoints, GetSpawnedRadius());
        if (numValidSpawns <= 0) { return 0; }

        bool sectorHasEnemy = false;
        bool sectorHasLivingEnemy = false;
        mobj_t* thing = sector.thinglist;
        while (thing != nullptr)
        {
            if (IsEnemy(thing->info->doomednum))
            {
                sectorHasEnemy = true;

                if (thing->health > 0)
                {
                    sectorHasLivingEnemy = true;
                    break;
                }
            }

            thing = thing->snext;
        }

        // we want a sector that can spawn up to 3 enemies
        // also we want a sector that has at least one living enemy over sectors that only have dead ones
        if (numValidSpawns >= 3 && sectorHasLivingEnemy) { return 16 + 2; }
        if (numValidSpawns >= 3 && sectorHasEnemy) { return 16 + 1; }
        if (numValidSpawns >= 2 && sectorHasLivingEnemy) { return 8 + 2; }
        if (numValidSpawns >= 2 && sectorHasEnemy) { return 8 + 1; }
        if (sectorHasLivingEnemy) { return 4 + 2; }
        
        // worst case, exclude sectors that have no enemies
        return sectorHasEnemy ? 4 + 1 : 0;
    }
};

class LostSoulTrap : public SpawnTrap
{
public:
    static constexpr int type{65902};
    int GetType() const override { return type; }
    mobjtype_t GetTypeToSpawn() const override { return MT_SKULL; }
    int GetNumberToSpawn() const override { return 8; }
    int GetSpawnedHeight() const override { return 56; }
    int GetSpawnedRadius() const override { return 16; }
    bool ShouldSpawnTogether() const override { return false; }

    int CalculateSectorPriority(const sector_t& sector, const Midpoints& midpoints) const override
    {
        // exclude sectors that are too short
        if (IsSectorTooShort(sector, GetSpawnedHeight())) { return 0; }

        // exclude sectors that dont have any spawnpoints that fit this enemy
        if (CalculateNumberValidSpawns(midpoints, GetSpawnedRadius()) <= 0) { return 0; }

        bool sectorHasEnemy = false;
        bool sectorHasLostSoul = false;
        mobj_t* thing = sector.thinglist;
        while (thing != nullptr)
        {
            if (IsEnemy(thing->info->doomednum))
            {
                sectorHasEnemy = true;

                if (thing->info->doomednum == 3006)
                {
                    sectorHasLostSoul = true;
                    break;
                }
            }

            thing = thing->snext;
        }

        if (sectorHasEnemy && !sectorHasLostSoul) { return 2; }

        // worst case, exclude sectors that have no enemies
        return sectorHasEnemy? 1 : 0;
    }
};

// -------------------- ADD NEW TRAPS HERE --------------------

SpawnTrap* trapTypes[3] =
{
    new ArchvileTrap(), 
    new RevenantTrap(),
    new LostSoulTrap()
};

// ============================================================================
// ============================================================================

void CalculateIdealSectors(const SpawnTrap& trap, const std::vector<int>& sectorsInConsideration, std::vector<int>& idealSectors, std::vector<int>& validSectors);
bool TrySpawnInSector(const SpawnTrap& trap, int sectorIndex, int numToSpawn);
bool TrySpawnTrapMobj(const SpawnTrap& trap, fixed_t x, fixed_t y);

// Uses static map data to predetermine the possible spawnpoints for spawn traps
void SetupSpawnTrapData()
{
    sectorMidpoints.clear();

    int numDamagingSectors = 0;
    int numSecretSectors = 0;
    int numBadSubSectors = 0;
    int numSmallSubSectors = 0;
    int numFinalSubSectors = 0;

    // go through all subsectors and pick out the ones that we can use for future trap spawning
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

        // ignore subsectors that dont have enough readily available data
        // TODO: reconstruct all the subsectors using the BSP to find the missing lines?
        if (subsector->numlines < 2)
        {
            numBadSubSectors++;
            continue;
        }

        allSubSectorVerts.clear();

        // find the extents of the subsector
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

        // ignore subsectors that are too small
        // TODO: possibly also ignore subsectors that have too small of a total area? for subsectors that are extremely long but still thin
        if (((maxX - minX) < 64) || ((maxY - minY) < 64))
        {
            numSmallSubSectors++;
            continue;
        }

        // calculate a point which is the average x and y of the subsector. this point is this subsectors midpoint
        fixed_t x = 0;
        fixed_t y = 0;
        std::for_each(allSubSectorVerts.cbegin(), allSubSectorVerts.cend(), [&x, &y](vertex_t* v)
        { 
            x += (v->x / FRACUNIT);
            y += (v->y / FRACUNIT);
        });
        x = x / (int)allSubSectorVerts.size();
        y = y / (int)allSubSectorVerts.size();
        x = x * FRACUNIT;
        y = y * FRACUNIT;

        // create a mapping between this subsectors midpoint and the sector it belongs to
        for (int j = 0; j < numsectors; ++j)
        {
            if (&sectors[j] != subsector->sector)
            {
                continue;
            }

            if (sectorMidpoints.find(j) != sectorMidpoints.end())
            {
                auto value = std::pair<fixed_t, fixed_t>(x, y);
                sectorMidpoints[j].push_back(value);
            }
            else
            {
                auto midpoints = Midpoints();
                auto value = std::pair<fixed_t, fixed_t>(x, y);
                midpoints.push_back(value);
                sectorMidpoints.insert({j, midpoints});
            }

            break;
        }

        numFinalSubSectors++;
    }

    //fprintf(stdout, "%d Sectors, %d subsectors. Trimmed %d damaging, %d secret, %d bad, %d small\n", (int)sectorMidpoints.size(), numFinalSubSectors, numDamagingSectors, numSecretSectors, numBadSubSectors, numSmallSubSectors);
}

// Spawns enemies of type somewhere in the map, according to trap specifications
void ActivateSpawnTrap(const int type)
{
    // Identify trap type
    SpawnTrap* trap = nullptr;
    for (int i = 0; i < std::size(trapTypes); ++i)
    {
        if (trapTypes[i]->GetType() == type)
        {
            trap = trapTypes[i];
            break;
        }
    }

    if (trap == nullptr)
    {
        fprintf(stderr, "No known spawn trap with type %d, skipping trap spawn\n", type);
        return;
    }

    // All sectors in this map are worth initial consideration
    std::vector<int> sectorsInConsideration;
    for (int i = 0; i < numsectors; ++i)
    {
        sectorsInConsideration.push_back(i);
    }

    std::vector<int> idealSectors;
    std::vector<int> validSectors;
    CalculateIdealSectors(*trap, sectorsInConsideration, idealSectors, validSectors);

    // Randomly pick an ideal sector and attempt to spawn enemies in it
    int numToSpawn = trap->GetNumberToSpawn();
    int remainingTries = 100;
    while (remainingTries > 0 && idealSectors.size() > 0)
    {
        int rnd = rand() % idealSectors.size();
        if (TrySpawnInSector(*trap, idealSectors[rnd], trap->ShouldSpawnTogether() ? numToSpawn : 1))
        {
            if (!trap->ShouldSpawnTogether())
            {
                numToSpawn--;
                if (numToSpawn <= 0)
                {
                    // we have spawned everything we wanted to across the sectors
                    break;
                }
            }
            else
            {
                // we have spawned everything we could have within one sector
                break;
            }
        }

        if (idealSectors.size() > 1)
        {
            // we have already attempted to spawn in this sector, remove it from the ideal collection
            if (rnd != idealSectors.size() - 1)
            {
                idealSectors[rnd] = idealSectors.back();
            }
            idealSectors.pop_back();
        }
        else
        {
            // no ideal sectors left, we must recalculate them
            sectorsInConsideration = validSectors;
            CalculateIdealSectors(*trap, sectorsInConsideration, idealSectors, validSectors);
        }

        remainingTries--;
    }

    if (remainingTries <= 0 || idealSectors.size() <= 0)
    {
        fprintf(stderr, "ActivateSpawnTrap failed to spawn some enemies\n");
    }
}

// Identifies the set of sectors that would be best for spawning enemies of this trap type
// Also identifies the set of sectors that would be valid for spawning enemies of this trap type
void CalculateIdealSectors(const SpawnTrap& trap, const std::vector<int>& sectorsInConsideration, std::vector<int>& idealSectors, std::vector<int>& validSectors)
{
    idealSectors.clear();
    validSectors.clear();

    sector_t sector;
    Midpoints midpoints;
    int maxPriority = 0;
    for (int sectorIndex : sectorsInConsideration)
    {
        auto search = sectorMidpoints.find(sectorIndex);
        if (search == sectorMidpoints.end())
        {
            continue;
        }

        sector = sectors[sectorIndex];
        midpoints = sectorMidpoints[sectorIndex];

        int priority = trap.CalculateSectorPriority(sector, midpoints);
        if (priority <= 0)
        {
            continue;
        }

        validSectors.push_back(sectorIndex);

        if (priority > maxPriority)
        {
            idealSectors.clear();
            idealSectors.push_back(sectorIndex);
            maxPriority = priority;

            //fprintf(stdout, "New highest priority 0x%X\n", priority);
        }
        else if (priority == maxPriority)
        {
            idealSectors.push_back(sectorIndex);
        }
    }

    //fprintf(stdout, "Selecting between %d idealSectors\n", (int)idealSectors.size());
}

// Randomly pick a midpoint mapped to this sector and attempt to spawn enemies in it
bool TrySpawnInSector(const SpawnTrap& trap, const int sectorIndex, int numToSpawn)
{
    bool spawnedSomething = false;
    auto midpoints = sectorMidpoints[sectorIndex];

    int remainingTries = 100;
    while (remainingTries > 0 && !midpoints.empty())
    {
        int rnd = rand() % midpoints.size();
        if (TrySpawnTrapMobj(trap, midpoints[rnd].first, midpoints[rnd].second))
        {
            spawnedSomething = true;
            numToSpawn--;

            if (numToSpawn <= 0)
            {
                // we have spawned everything we wanted to
                return true;
            }
        }

        if (rnd != midpoints.size() - 1)
        {
            midpoints[rnd] = midpoints.back();
        }
        midpoints.pop_back();

        remainingTries--;
    }

    // we have not spawned everything we wanted to
    return spawnedSomething;
}

// Spawn an enemy at a location as long as there is no collision, also spawns teleport effect and plays teleport sound
bool TrySpawnTrapMobj(const SpawnTrap& trap, const fixed_t x, const fixed_t y)
{
    // used for collision checking
    mannequin.radius = trap.GetSpawnedRadius() * FRACUNIT;

    if (!IsSpawnPointValid(x, y))
    {
        return false;
    }

    mobj_t* fog = P_SpawnMobj(x, y, ONFLOORZ, MT_TFOG);
    S_StartSound(fog, sfx_telept);

    mobj_t* newMob = P_SpawnMobj(x, y, ONFLOORZ, trap.GetTypeToSpawn());
    //fprintf(stdout, "Spawned at %d, %d, %d\n", (newMob->x / FRACUNIT), (newMob->y / FRACUNIT), (newMob->z / FRACUNIT));

    return true;
}