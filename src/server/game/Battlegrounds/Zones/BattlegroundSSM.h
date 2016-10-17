/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BATTLEGROUNDSSM_H
#define __BATTLEGROUNDSSM_H

#include "Language.h"
#include "Battleground.h"

enum BG_SSM_WorldStates
{
    SSM_MINE_CARTS_DISPLAY            = 6436,
    SSM_ALLIANCE_RESOURCES            = 6437,
    SSM_HORDE_RESOURCES               = 6438,
    SSM_MINE_CART_1                   = 6439,
    SSM_MINE_CART_2                   = 6440,
    SSM_DISPLAY_ALLIANCE_RESSOURCES   = 6441,
    SSM_MINE_CART_3                   = 6442,
    SSM_DISPLAY_HORDE_RESSOURCES      = 6443,
    SSM_DISPLAY_PROGRESS_BAR          = 6875, // 0 = false, 1 = true
    SSM_PROGRESS_BAR_STATUS           = 6876, // 0 = Horde max, 50 = Neutral, 100 = Alliance max
    SSM_UNK                           = 6877
};

enum BG_SSM_MineCarts
{
    BG_SSM_MINE_CART_1 = 1,
    BG_SSM_MINE_CART_2 = 2,
    BG_SSM_MINE_CART_3 = 3
};

enum BG_SSM_ProgressBarConsts
{
    BG_SSM_PROGRESS_BAR_DONT_SHOW  = 0,
    BG_SSM_PROGRESS_BAR_SHOW       = 1,
    BG_SSM_PROGRESS_BAR_NEUTRAL    = 50,
};

enum BG_SSM_Sounds
{
    BG_SSM_SOUND_MINE_CART_CAPTURED_HORDE     = 8213,
    BG_SSM_SOUND_MINE_CART_CAPTURED_ALLIANCE  = 8173,
    BG_SSM_SOUND_NEAR_VICTORY                 = 8456,
    BG_SSM_SOUND_MINE_CART_SPAWNED            = 8174,
};

enum BG_SSM_Spells
{
    BG_SSM_CONTROL_VISUAL_ALLIANCE   = 116086,
    BG_SSM_CONTROL_VISUAL_HORDE      = 116085,
    BG_SSM_CONTROL_VISUAL_NEUTRAL    = 118001,
    BG_SSM_TRACK_SWITCH_OPENED       = 120228,
    BG_SSM_TRACK_SWITCH_CLOSED       = 120229,
    BG_SSM_FEIGN_DEATH_STUN          = 135781, // Prevent from turn moves
    BG_SSM_PREVENTION_AURA           = 135846, // Prevent from using multiple times track switches
};

enum SSMBattlegroundObjectEntry
{
    BG_SSM_MINE_DEPOT              = 400433,
    BG_SSM_DOOR                    = 400434
};

enum SSMBattlegroundGaveyards
{
    SSM_GRAVEYARD_MAIN_ALLIANCE     = 4062,
    SSM_GRAVEYARD_MAIN_HORDE        = 4061,
};

enum SSMBattlegroundCreaturesTypes
{
    SSM_SPIRIT_ALLIANCE        = 0,
    SSM_SPIRIT_HORDE           = 1,
    SSM_MINE_CART_TRIGGER      = 2,
    SSM_TRACK_SWITCH_EAST      = 3,
    SSM_TRACK_SWITCH_NORTH     = 4,

    BG_SSM_CREATURES_MAX       = 5
};

enum SSMDepots
{
    SSM_WATERFALL_DEPOT,
    SSM_LAVA_DEPOT,
    SSM_DIAMOND_DEPOT,
    SSM_TROLL_DEPOT,
};

enum SSMTracks
{
    SSM_EAST_TRACK_SWITCH,
    SSM_NORTH_TRACK_SWITCH,
    SSM_MAX_TRACK_SWITCH
};

enum SSMBattlegroundObjectTypes
{
    BG_SSM_OBJECT_DOOR_A_1                   = 0,
    BG_SSM_OBJECT_DOOR_H_1                   = 1,
    BG_SSM_OBJECT_DOOR_A_2                   = 2,
    BG_SSM_OBJECT_DOOR_H_2                   = 3,
    BG_SSM_OBJECT_WATERFALL_DEPOT            = 4,
    BG_SSM_OBJECT_LAVA_DEPOT                 = 5,
    BG_SSM_OBJECT_DIAMOND_DEPOT              = 6,
    BG_SSM_OBJECT_TROLL_DEPOT                = 7,
    BG_SSM_OBJECT_BERSERKING_BUFF_EAST       = 8,
    BG_SSM_OBJECT_BERSERKING_BUFF_WEST       = 9,
    BG_SSM_OBJECT_RESTORATION_BUFF_WATERFALL = 10,
    BG_SSM_OBJECT_RESTORATION_BUFF_LAVA      = 11,

    BG_SSM_OBJECT_MAX                        = 12
};

enum BG_SSM_Score
{
    BG_SSM_WARNING_NEAR_VICTORY_SCORE    = 1400,
    BG_SSM_MAX_TEAM_SCORE                = 1600
};

enum SSMBattlegroundMineCartState
{
    SSM_MINE_CART_CONTROL_NEUTRAL    = 0,
    SSM_MINE_CART_CONTROL_ALLIANCE   = 1,
    SSM_MINE_CART_CONTROL_HORDE       = 2,
};

enum BG_SSM_CreatureIds
{
    NPC_TRACK_SWITCH_EAST    = 60283,
    NPC_TRACK_SWITCH_NORTH   = 60309,
    NPC_MINE_CART_1          = 60378,
    NPC_MINE_CART_2          = 60379,
    NPC_MINE_CART_3          = 60380,
    NPC_MINE_CART_TRIGGER    = 400464,
};

enum BG_SSM_Paths
{
    SSM_EAST_PATH,
    SSM_NORTH_PATH
};

const float BG_SSM_BuffPos[4][4] =
{
    {749.444153f, 64.338188f, 369.535797f, 6.058259f},   // Berserking buff East
    {789.979431f, 281.883575f, 355.389984f, 0.652173f},  // Berserking buff West
    {539.873596f, 396.386749f, 345.722412f, 3.994188f},  // Restoration buff Waterfall
    {614.202698f, 120.924660f, 294.430908f, 4.241807f}   // Restoration buff Lava
};

const float BG_SSM_DepotPos[4][4] =
{
    {566.950989f, 337.05801f, 347.295013f, 1.559089f},   // Waterfall
    {619.469971f, 79.719597f, 299.067993f, 1.625564f},   // Lava
    {895.974426f, 27.210802f, 364.390991f, 3.445790f},   // Diamond
    {778.444946f, 500.949707f, 359.738983f, 0.737040f}   // Troll
};

const float BG_SSM_DoorPos[4][4] =
{
    {852.0289952f, 158.216003f, 326.761011f, 0.150758f}, // Alliance 1
    {830.092102f, 143.925507f, 326.5f, 3.130245f},       // Alliance 2
    {652.177612f, 228.493423f, 326.917480f, 0.163844f},  // Horde 1
    {635.622925f, 208.220886f, 326.648315f, 3.717332f}   // Horde 2
};

const float BG_SSM_TrackPos[2][4] =
{
    {715.585388f, 101.272034f, 319.994690f, 4.647377f}, // East
    {847.481689f, 308.032562f, 346.573242f, 0.587086f}  // North
};

#define MINE_CART_AT_DEPOT_POINTS       200
#define POINTS_PER_MINE_CART            150
#define DISPLAY_WORLDSTATE              1
#define BG_SSM_NotSSMWeekendHonorTicks    200
#define BG_SSM_SSMWeekendHonorTicks       130
#define SSM_MINE_CART_MAX                3
#define SSM_MAX_PATHS                    2

struct BattlegroundSSMScore : public BattlegroundScore
{
    BattlegroundSSMScore() : BattlegroundScore(), MineCartCaptures(0) { }
    virtual ~BattlegroundSSMScore() { }

    uint32 MineCartCaptures;
};

class BattlegroundSSM : public Battleground
{
    public:
        BattlegroundSSM();
        ~BattlegroundSSM();

        /* inherited from BattlegroundClass */
        void AddPlayer(Player* player);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();
        virtual void StartingEventDespawnDoors();
        void HandleKillPlayer(Player* player, Player* killer);
        bool SetupBattleground();
        void Reset();
        WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);
        void PostUpdateImpl(uint32 diff);
        void UpdateTeamScore(uint32 Team);
        void EndBattleground(uint32 winner);
        void UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor = true);
        void FillInitialWorldStates(WorldPacket& data);

        /* Battleground Events */
        void EventReopenDepot(uint32 diff);

        uint32 GetPrematureWinner();
    private:
        void EventTeamCapturedMineCart(uint32 team, uint8 mineCart);
        void UpdatePointsCount(uint32 Team);
        void SummonMineCart(uint32 diff);
        void FirstMineCartSummon(uint32 diff);
        void MineCartsMoves(uint32 diff);
        void CheckPlayerNearMineCart(uint32 diff);
        void CheckMineCartNearDepot(uint32 diff);
        void MineCartAddPoints(uint32 diff);
        void ResetDepotsAndMineCarts(uint8 depot, uint8 mineCart);
        void CheckTrackSwitch(uint32 diff);

        /* Scorekeeping */
        uint32 GetTeamScore(Team team) const         { return m_TeamScores[GetTeamIndexByTeamId(team)]; }
        void AddPoints(uint32 Team, uint32 Points);

        void RemovePoint(uint32 TeamID, uint32 Points = 1) { m_TeamScores[GetTeamIndexByTeamId(TeamID)] -= Points; }
        void SetTeamPoint(uint32 TeamID, uint32 Points = 0) { m_TeamScores[GetTeamIndexByTeamId(TeamID)] = Points; }
        
        uint32 GetMineCartTeamKeeper(uint8 mineCart);
        uint32 m_HonorScoreTics[2];

        uint32 m_MineCartsTrigger[SSM_MINE_CART_MAX];
        int32 m_MineCartsProgressBar[SSM_MINE_CART_MAX];
        uint32 m_MineCartTeamKeeper[SSM_MINE_CART_MAX]; // keepers team
        int32 m_MineCartSpawnTimer;
        int32 m_FirstMineCartSummonTimer;
        int32 m_MineCartCheckTimer;
        int32 m_DepotCloseTimer[4];
        int32 m_MineCartAddPointsTimer;
        int32 m_TrackSwitchClickTimer[SSM_MAX_TRACK_SWITCH];
        bool m_Depot[4]; // 0 = Waterfall, 1 = Lava, 2 = Diamond, 3 = Troll
        bool m_MineCartReachedDepot[SSM_MINE_CART_MAX];
        bool m_MineCartNearDepot[SSM_MINE_CART_MAX];
        bool m_MineCartSpawned[SSM_MINE_CART_MAX];
        bool m_FirstMineCartSpawned;
        bool m_PathDone[SSM_MINE_CART_MAX - 1][SSM_MAX_PATHS]; // Only for first and third mine cart
        bool m_WaterfallPathDone; // Waterfall path
        bool m_TrackSwitch[SSM_MAX_TRACK_SWITCH]; // East : true = open, false = close | North : true = close, false = open
        bool m_TrackSwitchCanInterract[SSM_MAX_TRACK_SWITCH];

        uint32 m_HonorTics;
        bool m_IsInformedNearVictory;
};
#endif
