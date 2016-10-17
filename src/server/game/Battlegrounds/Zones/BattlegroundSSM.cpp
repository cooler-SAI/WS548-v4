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

#include "BattlegroundSSM.h"
#include "Player.h"
#include "Language.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "ScriptedCreature.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "GossipDef.h"

BattlegroundSSM::BattlegroundSSM()
{
    m_BuffChange = true;
    BgObjects.resize(BG_SSM_OBJECT_MAX);
    BgCreatures.resize(BG_SSM_CREATURES_MAX);
}

BattlegroundSSM::~BattlegroundSSM() { }

void BattlegroundSSM::Reset()
{
    //call parent's class reset
    Battleground::Reset();

    m_TeamScores[TEAM_ALLIANCE] = 0;
    m_TeamScores[TEAM_HORDE] = 0;
    m_HonorScoreTics[TEAM_ALLIANCE] = 0;
    m_HonorScoreTics[TEAM_HORDE] = 0;
    m_MineCartCheckTimer = 1000;
    m_FirstMineCartSummonTimer = 110*IN_MILLISECONDS; // 10 sec before opening doors
    bool isBGWeekend = sBattlegroundMgr->IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? BG_SSM_NotSSMWeekendHonorTicks : BG_SSM_SSMWeekendHonorTicks;
    m_IsInformedNearVictory = false;
    m_MineCartSpawnTimer = 30*IN_MILLISECONDS;
    m_MineCartAddPointsTimer = 2000;
    m_FirstMineCartSpawned = false;
    m_WaterfallPathDone = false;
    m_TrackSwitch[SSM_EAST_TRACK_SWITCH] = true;
    m_TrackSwitch[SSM_NORTH_TRACK_SWITCH] = false;
    m_TrackSwitchClickTimer[SSM_EAST_TRACK_SWITCH] = 3000;
    m_TrackSwitchClickTimer[SSM_NORTH_TRACK_SWITCH] = 3000;
    m_TrackSwitchCanInterract[SSM_EAST_TRACK_SWITCH] = true;
    m_TrackSwitchCanInterract[SSM_NORTH_TRACK_SWITCH] = true;

    for (uint8 i = 0; i < SSM_MINE_CART_MAX; ++i)
    {
        m_MineCartsProgressBar[i] = BG_SSM_PROGRESS_BAR_NEUTRAL;
        m_MineCartReachedDepot[i] = false;
        m_MineCartNearDepot[i] = false;
        m_MineCartSpawned[i] = false;
    }

    for (uint8 i = 0; i < 4; ++i)
    {
        m_Depot[i] = false;
        m_DepotCloseTimer[i] = 3000;
    }

    for (uint8 i = 0; i < SSM_MAX_PATHS; ++i)
        m_PathDone[i][i] = false;
}

void BattlegroundSSM::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        BattlegroundSSM::SummonMineCart(diff);
        BattlegroundSSM::CheckPlayerNearMineCart(diff);
        BattlegroundSSM::CheckMineCartNearDepot(diff);
        BattlegroundSSM::EventReopenDepot(diff);
        BattlegroundSSM::MineCartAddPoints(diff);
        BattlegroundSSM::CheckTrackSwitch(diff);
    }

    if (!m_FirstMineCartSpawned)
        BattlegroundSSM::FirstMineCartSummon(diff);

    if (Creature* trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
    {
        if (trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
            m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] = true;
        else
        {
            m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] = false;
            m_PathDone[SSM_EAST_PATH][0] = false;
            m_PathDone[SSM_EAST_PATH][1] = false;
        }

        if (trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f, true))
            m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] = true;
        else
        {
            m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] = false;
            m_WaterfallPathDone = false;
        }

        if (trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
            m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1] = true;
        else
        {
            m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1] = false;
            m_PathDone[SSM_NORTH_PATH][0] = false;
            m_PathDone[SSM_NORTH_PATH][1] = false;
        }
    }

    if (!m_TrackSwitchCanInterract[SSM_EAST_TRACK_SWITCH])
    {
        if (m_TrackSwitchClickTimer[SSM_EAST_TRACK_SWITCH] <= 0)
        {
            if (Creature* track = GetBgMap()->GetCreature(BgCreatures[SSM_TRACK_SWITCH_EAST]))
            {
                for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
                    if (Player* player = ObjectAccessor::FindPlayer(itr->first))
                        if (player->GetExactDist2d(track->GetPositionX(), track->GetPositionY()) <= 10.0f)
                            player->PlayerTalkClass->SendCloseGossip(); // Prevent from using multiple times track switches

                track->RemoveAurasDueToSpell(BG_SSM_PREVENTION_AURA);
                m_TrackSwitchCanInterract[SSM_EAST_TRACK_SWITCH] = true;
            }
        }
        else m_TrackSwitchClickTimer[SSM_EAST_TRACK_SWITCH] -= diff;
    }

    if (!m_TrackSwitchCanInterract[SSM_NORTH_TRACK_SWITCH])
    {
        if (m_TrackSwitchClickTimer[SSM_NORTH_TRACK_SWITCH] <= 0)
        {
            if (Creature* track = GetBgMap()->GetCreature(BgCreatures[SSM_TRACK_SWITCH_NORTH]))
            {
                for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
                    if (Player* player = ObjectAccessor::FindPlayer(itr->first))
                        if (player->GetExactDist2d(track->GetPositionX(), track->GetPositionY()) <= 10.0f)
                            player->PlayerTalkClass->SendCloseGossip(); // Prevent from using multiple times track switches

                track->RemoveAurasDueToSpell(BG_SSM_PREVENTION_AURA);
                m_TrackSwitchCanInterract[SSM_NORTH_TRACK_SWITCH] = true;
            }
        }
        else m_TrackSwitchClickTimer[SSM_NORTH_TRACK_SWITCH] -= diff;
    }

    BattlegroundSSM::MineCartsMoves(diff);
}

void BattlegroundSSM::StartingEventCloseDoors()
{
    // Starting doors
    for (int doorType = BG_SSM_OBJECT_DOOR_A_1; doorType <= BG_SSM_OBJECT_DOOR_H_2; ++doorType)
    {
        DoorClose(doorType);
        SpawnBGObject(doorType, RESPAWN_IMMEDIATELY);
    }

    for (uint8 i = BG_SSM_OBJECT_WATERFALL_DEPOT; i < BG_SSM_OBJECT_MAX; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);
}

void BattlegroundSSM::CheckTrackSwitch(uint32 diff)
{
    Creature* trigger = NULL;

    if (m_TrackSwitchCanInterract[SSM_EAST_TRACK_SWITCH])
    {
        if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
        {
            if (Creature* track = GetBgMap()->GetCreature(BgCreatures[SSM_TRACK_SWITCH_EAST]))
            {
                if (track->HasAura(BG_SSM_TRACK_SWITCH_OPENED) && !m_TrackSwitch[SSM_EAST_TRACK_SWITCH])
                {
                    SendMessageToAll(LANG_BG_SSM_EAST_DIRECTION_CHANGED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    m_TrackSwitchClickTimer[SSM_EAST_TRACK_SWITCH] = 3000;
                    m_TrackSwitch[SSM_EAST_TRACK_SWITCH] = true;
                    m_TrackSwitchCanInterract[SSM_EAST_TRACK_SWITCH] = false;
                }

                if (track->HasAura(BG_SSM_TRACK_SWITCH_CLOSED) && m_TrackSwitch[SSM_EAST_TRACK_SWITCH])
                {
                    SendMessageToAll(LANG_BG_SSM_EAST_DIRECTION_CHANGED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    m_TrackSwitchClickTimer[SSM_EAST_TRACK_SWITCH] = 3000;
                    m_TrackSwitch[SSM_EAST_TRACK_SWITCH] = false;
                    m_TrackSwitchCanInterract[SSM_EAST_TRACK_SWITCH] = false;
                }
            }
        }
    }

    if (m_TrackSwitchCanInterract[SSM_NORTH_TRACK_SWITCH])
    {
        if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
        {
            if (Creature* track = GetBgMap()->GetCreature(BgCreatures[SSM_TRACK_SWITCH_NORTH]))
            {
                if (track->HasAura(BG_SSM_TRACK_SWITCH_CLOSED) && m_TrackSwitch[SSM_NORTH_TRACK_SWITCH])
                {
                    SendMessageToAll(LANG_BG_SSM_NORTH_DIRECTION_CHANGED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    m_TrackSwitchClickTimer[SSM_NORTH_TRACK_SWITCH] = 3000;
                    m_TrackSwitch[SSM_NORTH_TRACK_SWITCH] = false;
                    m_TrackSwitchCanInterract[SSM_NORTH_TRACK_SWITCH] = false;
                }

                if (track->HasAura(BG_SSM_TRACK_SWITCH_OPENED) && !m_TrackSwitch[SSM_NORTH_TRACK_SWITCH])
                {
                    SendMessageToAll(LANG_BG_SSM_NORTH_DIRECTION_CHANGED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    m_TrackSwitchClickTimer[SSM_NORTH_TRACK_SWITCH] = 3000;
                    m_TrackSwitch[SSM_NORTH_TRACK_SWITCH] = true;
                    m_TrackSwitchCanInterract[SSM_NORTH_TRACK_SWITCH] = false;
                }
            }
        }
    }
}

void BattlegroundSSM::FirstMineCartSummon(uint32 diff)
{
    if (m_FirstMineCartSummonTimer <= 0)
    {
        Creature* trigger = NULL;
        m_FirstMineCartSpawned = true;

        if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
        {
            if (uint8 mineCart = urand(BG_SSM_MINE_CART_1, BG_SSM_MINE_CART_3))
            {
                switch (mineCart)
                {
                    case BG_SSM_MINE_CART_1:
                    {
                        if (trigger)
                        {
                            trigger->SummonCreature(NPC_MINE_CART_1, 744.542053f, 183.545883f, 319.658203f, 4.356342f);
                            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                                cart->SetUnitMovementFlags(MOVEMENTFLAG_BACKWARD);
                                cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                cart->SetSpeed(MOVE_WALK, 0.8f);
                                m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] = BG_SSM_PROGRESS_BAR_NEUTRAL;
                            }
                        }
                        break;
                    }
                    case BG_SSM_MINE_CART_2:
                    {
                        if (trigger)
                        {
                            trigger->SummonCreature(NPC_MINE_CART_2, 739.400330f, 203.598511f, 319.603333f, 2.308198f);
                            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                                cart->SetUnitMovementFlags(MOVEMENTFLAG_BACKWARD);
                                cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                cart->SetSpeed(MOVE_WALK, 0.8f);
                                m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] = BG_SSM_PROGRESS_BAR_NEUTRAL;
                            }
                        }
                        break;
                    }
                    case BG_SSM_MINE_CART_3:
                    {
                        if (trigger)
                        {
                            trigger->SummonCreature(NPC_MINE_CART_3, 760.184509f, 198.844742f, 319.446655f, 0.351249f);
                            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                                cart->SetUnitMovementFlags(MOVEMENTFLAG_BACKWARD);
                                cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                cart->SetSpeed(MOVE_WALK, 0.8f);
                                m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] = BG_SSM_PROGRESS_BAR_NEUTRAL;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    } else m_FirstMineCartSummonTimer -= diff;
}

void BattlegroundSSM::StartingEventOpenDoors()
{
    //Open doors
    for (int doorType = BG_SSM_OBJECT_DOOR_A_1; doorType <= BG_SSM_OBJECT_DOOR_H_2; ++doorType)
        DoorOpen(doorType);
    
    for (uint8 i = BG_SSM_OBJECT_WATERFALL_DEPOT; i < BG_SSM_OBJECT_MAX; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
}

void BattlegroundSSM::StartingEventDespawnDoors()
{
    for (int doorType = BG_SSM_OBJECT_DOOR_A_1; doorType <= BG_SSM_OBJECT_DOOR_H_2; ++doorType)
        DoorDespawn(doorType);
}

void BattlegroundSSM::SummonMineCart(uint32 diff)
{
    if (m_MineCartSpawnTimer <= 0)
    {
        Creature* trigger = NULL;
        uint8 mineCart = 0;

        if (m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
            mineCart = 0;

        else if (!m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
        {
            mineCart = RAND(BG_SSM_MINE_CART_1, BG_SSM_MINE_CART_2, BG_SSM_MINE_CART_3);
            SendWarningToAll(LANG_BG_SSM_MINE_CART_SPAWNED);
            PlaySoundToAll(BG_SSM_SOUND_MINE_CART_SPAWNED);
        }

        else if (m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
        {
            mineCart = RAND(BG_SSM_MINE_CART_2, BG_SSM_MINE_CART_3);
            SendWarningToAll(LANG_BG_SSM_MINE_CART_SPAWNED);
            PlaySoundToAll(BG_SSM_SOUND_MINE_CART_SPAWNED);
        }

        else if (!m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
        {
            mineCart = RAND(BG_SSM_MINE_CART_1, BG_SSM_MINE_CART_3);
            SendWarningToAll(LANG_BG_SSM_MINE_CART_SPAWNED);
            PlaySoundToAll(BG_SSM_SOUND_MINE_CART_SPAWNED);
        }

        else if (!m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
        {
            mineCart = RAND(BG_SSM_MINE_CART_1, BG_SSM_MINE_CART_2);
            SendWarningToAll(LANG_BG_SSM_MINE_CART_SPAWNED);
            PlaySoundToAll(BG_SSM_SOUND_MINE_CART_SPAWNED);
        }

        else if (m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
        {
            mineCart = BG_SSM_MINE_CART_3;
            SendWarningToAll(LANG_BG_SSM_MINE_CART_SPAWNED);
            PlaySoundToAll(BG_SSM_SOUND_MINE_CART_SPAWNED);
        }

        else if (m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            !m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
        {
            mineCart = BG_SSM_MINE_CART_2;
            SendWarningToAll(LANG_BG_SSM_MINE_CART_SPAWNED);
            PlaySoundToAll(BG_SSM_SOUND_MINE_CART_SPAWNED);
        }

        else if (!m_MineCartSpawned[BG_SSM_MINE_CART_1 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_2 - 1] &&
            m_MineCartSpawned[BG_SSM_MINE_CART_3 - 1])
        {
            mineCart = BG_SSM_MINE_CART_1;
            SendWarningToAll(LANG_BG_SSM_MINE_CART_SPAWNED);
            PlaySoundToAll(BG_SSM_SOUND_MINE_CART_SPAWNED);
        }

        if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
        {
            switch (mineCart)
            {
                case BG_SSM_MINE_CART_1:
                {
                    if (trigger)
                    {
                        trigger->SummonCreature(NPC_MINE_CART_1, 744.542053f, 183.545883f, 319.658203f, 4.356342f);
                        if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f))
                        {
                            cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                            cart->SetUnitMovementFlags(MOVEMENTFLAG_BACKWARD);
                            cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            cart->SetSpeed(MOVE_WALK, 0.8f);
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] = BG_SSM_PROGRESS_BAR_NEUTRAL;
                        }
                    }
                    break;
                }
                case BG_SSM_MINE_CART_2:
                {
                    if (trigger)
                    {
                        trigger->SummonCreature(NPC_MINE_CART_2, 739.400330f, 203.598511f, 319.603333f, 2.308198f);
                        if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f))
                        {
                            cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                            cart->SetUnitMovementFlags(MOVEMENTFLAG_BACKWARD);
                            cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            cart->SetSpeed(MOVE_WALK, 0.8f);
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] = BG_SSM_PROGRESS_BAR_NEUTRAL;
                        }
                    }
                    break;
                }
                case BG_SSM_MINE_CART_3:
                {
                    if (trigger)
                    {
                        trigger->SummonCreature(NPC_MINE_CART_3, 760.184509f, 198.844742f, 319.446655f, 0.351249f);
                        if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f))
                        {
                            cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                            cart->SetUnitMovementFlags(MOVEMENTFLAG_BACKWARD);
                            cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            cart->SetSpeed(MOVE_WALK, 0.8f);
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] = BG_SSM_PROGRESS_BAR_NEUTRAL;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        m_MineCartSpawnTimer = 30000;
    }
    else 
        m_MineCartSpawnTimer -= diff;
}

void BattlegroundSSM::CheckPlayerNearMineCart(uint32 diff)
{
    if (m_MineCartCheckTimer <= 0)
    {
        for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
        {
            if (Player* player = ObjectAccessor::FindPlayer(itr->first))
            {
                if (player->isDead()) 
                {
                    UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_DONT_SHOW, player);
                    continue;
                }

                if (player->GetTeam() == ALLIANCE)
                {
                    if (Creature* cart = player->FindNearestCreature(NPC_MINE_CART_1, 22.0f, true))
                    {
                        UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_SHOW, player);

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] >= 100)
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] = 100;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1], player);
                        }
                        else
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1]++;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1], player);
                        }
                        
                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] > BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_NEUTRAL, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_ALLIANCE, true);
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CONTROLLED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                            }
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] == BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                        }
                    }

                    else if (Creature* cart = player->FindNearestCreature(NPC_MINE_CART_2, 22.0f, true))
                    {
                        UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_SHOW, player);

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] >= 100)
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] = 100;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1], player);
                        }
                        else
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1]++;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1], player);
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] > BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_NEUTRAL, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_ALLIANCE, true);
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CONTROLLED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                            }
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] == BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                        }
                    }

                    else if (Creature* cart = player->FindNearestCreature(NPC_MINE_CART_3, 22.0f, true))
                    {
                        UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_SHOW, player);

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] >= 100)
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] = 100;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1], player);
                        }
                        else
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1]++;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1], player);
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] > BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_NEUTRAL, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_ALLIANCE, true);
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CONTROLLED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                            }
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] == BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                        }
                    }

                    else UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_DONT_SHOW, player);
                }
                else // for GetTeam() == HORDE
                {
                    if (Creature* cart = player->FindNearestCreature(NPC_MINE_CART_1, 22.0f, true))
                    {
                        UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_SHOW, player);

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] <= 0)
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] = 0;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1], player);
                        }
                        else
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1]--;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1], player);
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] < BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_NEUTRAL, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_HORDE, true);
                                SendMessageToAll(LANG_BG_SSM_HORDE_CONTROLLED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                            }
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] == BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                        }
                    }

                    else if (Creature* cart = player->FindNearestCreature(NPC_MINE_CART_2, 22.0f, true))
                    {
                        UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_SHOW, player);

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] <= 0)
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] = 0;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1], player);
                        }
                        else
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1]--;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1], player);
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] < BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_NEUTRAL, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_HORDE, true);
                                SendMessageToAll(LANG_BG_SSM_HORDE_CONTROLLED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                            }
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] == BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                        }
                    }

                    else if (Creature* cart = player->FindNearestCreature(NPC_MINE_CART_3, 22.0f, true))
                    {
                        UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_SHOW, player);

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] <= 0)
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] = 0;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1], player);
                        }
                        else
                        {
                            m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1]--;
                            UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1], player);
                        }
                        
                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] < BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_NEUTRAL, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                            {
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_HORDE, true);
                                SendMessageToAll(LANG_BG_SSM_HORDE_CONTROLLED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                            }
                        }

                        if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] == BG_SSM_PROGRESS_BAR_NEUTRAL)
                        {
                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_ALLIANCE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_ALLIANCE, cart->GetGUID());

                            if (cart->HasAura(BG_SSM_CONTROL_VISUAL_HORDE))
                                cart->RemoveAurasDueToSpell(BG_SSM_CONTROL_VISUAL_HORDE, cart->GetGUID());

                            if (!cart->HasAura(BG_SSM_CONTROL_VISUAL_NEUTRAL))
                                cart->CastSpell(cart, BG_SSM_CONTROL_VISUAL_NEUTRAL, true);
                        }
                    }

                    else UpdateWorldStateForPlayer(SSM_DISPLAY_PROGRESS_BAR, BG_SSM_PROGRESS_BAR_DONT_SHOW, player);
                }

                    m_MineCartCheckTimer = 1000;
            }
        }
    } else m_MineCartCheckTimer -= diff;
}

void BattlegroundSSM::CheckMineCartNearDepot(uint32 diff)
{
    Creature* trigger = NULL;
    if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
    {
        if (!m_MineCartNearDepot[BG_SSM_MINE_CART_1 - 1])
        {
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
            {
                if (cart->GetExactDist2d(BG_SSM_DepotPos[SSM_LAVA_DEPOT][0], BG_SSM_DepotPos[SSM_LAVA_DEPOT][1]) <= 6.0f)
                {
                    m_Depot[SSM_LAVA_DEPOT] = true;
                    BattlegroundSSM::EventTeamCapturedMineCart(GetMineCartTeamKeeper(BG_SSM_MINE_CART_1), BG_SSM_MINE_CART_1);
                    m_MineCartNearDepot[BG_SSM_MINE_CART_1 - 1] = true;
                }

                if (cart->GetExactDist2d(BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][0], BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][1]) <= 6.0f)
                {
                    m_Depot[SSM_DIAMOND_DEPOT] = true;
                    BattlegroundSSM::EventTeamCapturedMineCart(GetMineCartTeamKeeper(BG_SSM_MINE_CART_1), BG_SSM_MINE_CART_1);
                    m_MineCartNearDepot[BG_SSM_MINE_CART_1 - 1] = true;
                }
            }
        }

        if (!m_MineCartNearDepot[BG_SSM_MINE_CART_2 - 1])
        {
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f, true))
            {
                if (cart->GetExactDist2d(BG_SSM_DepotPos[SSM_WATERFALL_DEPOT][0], BG_SSM_DepotPos[SSM_WATERFALL_DEPOT][1]) <= 6.0f)
                {
                    m_Depot[SSM_WATERFALL_DEPOT] = true;
                    BattlegroundSSM::EventTeamCapturedMineCart(GetMineCartTeamKeeper(BG_SSM_MINE_CART_2), BG_SSM_MINE_CART_2);
                    m_MineCartNearDepot[BG_SSM_MINE_CART_2 - 1] = true;
                }
            }
        }

        if (!m_MineCartNearDepot[BG_SSM_MINE_CART_3 - 1])
        {
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
            {
                if (cart->GetExactDist2d(BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][0], BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][1]) <= 6.0f)
                {
                    m_Depot[SSM_DIAMOND_DEPOT] = true;
                    BattlegroundSSM::EventTeamCapturedMineCart(GetMineCartTeamKeeper(BG_SSM_MINE_CART_3), BG_SSM_MINE_CART_3);
                    m_MineCartNearDepot[BG_SSM_MINE_CART_3 - 1] = true;
                }

                if (cart->GetExactDist2d(BG_SSM_DepotPos[SSM_TROLL_DEPOT][0], BG_SSM_DepotPos[SSM_TROLL_DEPOT][1]) <= 6.0f)
                {
                    m_Depot[SSM_TROLL_DEPOT] = true;
                    BattlegroundSSM::EventTeamCapturedMineCart(GetMineCartTeamKeeper(BG_SSM_MINE_CART_3), BG_SSM_MINE_CART_3);
                    m_MineCartNearDepot[BG_SSM_MINE_CART_3 - 1] = true;
                }
            }
        }
    }
}

void BattlegroundSSM::EventTeamCapturedMineCart(uint32 team, uint8 mineCart)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
    {
        if (Player* player = ObjectAccessor::FindPlayer(itr->first))
        {
            if (player->GetTeam() == team)
                if (player->FindNearestCreature(NPC_MINE_CART_1, 22.0f, true) ||
                    player->FindNearestCreature(NPC_MINE_CART_2, 22.0f, true) ||
                    player->FindNearestCreature(NPC_MINE_CART_3, 22.0f, true))
                {
                    UpdatePlayerScore(player, SCORE_CARTS_HELPED, 1);
                    player->RewardHonor(player, 1, irand(10, 12));
                }
        }
    }

    Creature* trigger = NULL;
    if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
    {
        switch (mineCart)
        {
            case BG_SSM_MINE_CART_1:
            {
                if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
                {
                    if (m_Depot[SSM_LAVA_DEPOT])
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_LAVA_DEPOT]))
                        {
                            cart->StopMoving();
                            depot->UseDoorOrButton();
                            m_MineCartReachedDepot[BG_SSM_MINE_CART_1 - 1] = true;
                        }
                    }

                    if (m_Depot[SSM_DIAMOND_DEPOT])
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_DIAMOND_DEPOT]))
                        {
                            cart->StopMoving();
                            depot->UseDoorOrButton();
                            m_MineCartReachedDepot[BG_SSM_MINE_CART_1 - 1] = true;
                        }
                    }
                }
                break;
            }

            case BG_SSM_MINE_CART_2:
            {
                if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f, true))
                {
                    if (m_Depot[SSM_WATERFALL_DEPOT])
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_WATERFALL_DEPOT]))
                        {
                            cart->StopMoving();
                            depot->UseDoorOrButton();
                            m_MineCartReachedDepot[BG_SSM_MINE_CART_2 - 1] = true;
                        }
                    }
                }
                break;
            }

            case BG_SSM_MINE_CART_3:
            {
                if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
                {
                    if (m_Depot[SSM_DIAMOND_DEPOT])
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_DIAMOND_DEPOT]))
                        {
                            cart->StopMoving();
                            depot->UseDoorOrButton();
                            m_MineCartReachedDepot[BG_SSM_MINE_CART_3 - 1] = true;
                        }
                    }

                    if (m_Depot[SSM_TROLL_DEPOT])
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_TROLL_DEPOT]))
                        {
                            cart->StopMoving();
                            depot->UseDoorOrButton();
                            m_MineCartReachedDepot[BG_SSM_MINE_CART_3 - 1] = true;
                        }
                    }
                }
                break;
            }
        }
    }
}

void BattlegroundSSM::EventReopenDepot(uint32 diff)
{
    Creature* trigger = NULL;
    if (m_MineCartReachedDepot[BG_SSM_MINE_CART_1 - 1])
    {
        if (m_Depot[SSM_LAVA_DEPOT])
        {
            if (m_DepotCloseTimer[SSM_LAVA_DEPOT] <= 0)
            {
                if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
                {
                    if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_LAVA_DEPOT]))
                        {
                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == ALLIANCE)
                            {
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_ALLIANCE);
                            }

                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == HORDE)
                            {
                                SendMessageToAll(LANG_BG_SSM_HORDE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_HORDE);
                            }

                            if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] != BG_SSM_PROGRESS_BAR_NEUTRAL)
                                BattlegroundSSM::AddPoints(GetMineCartTeamKeeper(BG_SSM_MINE_CART_1), POINTS_PER_MINE_CART);

                            BattlegroundSSM::ResetDepotsAndMineCarts(SSM_LAVA_DEPOT, BG_SSM_MINE_CART_1);
                            depot->ResetDoorOrButton();
                            cart->DespawnOrUnsummon();
                        }
                    }
                }
            } else m_DepotCloseTimer[SSM_LAVA_DEPOT] -= diff;
        }

        if (m_Depot[SSM_DIAMOND_DEPOT])
        {
            if (m_DepotCloseTimer[SSM_DIAMOND_DEPOT] <= 0)
            {
                if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
                {
                    if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_DIAMOND_DEPOT]))
                        {
                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == ALLIANCE)
                            {
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_ALLIANCE);
                            }

                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == HORDE)
                            {
                                SendMessageToAll(LANG_BG_SSM_HORDE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_HORDE);
                            }

                            if (m_MineCartsProgressBar[BG_SSM_MINE_CART_1 - 1] != BG_SSM_PROGRESS_BAR_NEUTRAL)
                                BattlegroundSSM::AddPoints(GetMineCartTeamKeeper(BG_SSM_MINE_CART_1), POINTS_PER_MINE_CART);
                            
                            BattlegroundSSM::ResetDepotsAndMineCarts(SSM_DIAMOND_DEPOT, BG_SSM_MINE_CART_1);
                            depot->ResetDoorOrButton();
                            cart->DespawnOrUnsummon();
                        }
                    }
                }
            } else m_DepotCloseTimer[SSM_DIAMOND_DEPOT] -= diff;
        }
    }

    if (m_MineCartReachedDepot[BG_SSM_MINE_CART_2 - 1])
    {
        if (m_Depot[SSM_WATERFALL_DEPOT])
        {
            if (m_DepotCloseTimer[SSM_WATERFALL_DEPOT] <= 0)
            {
                if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
                {
                    if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f, true))
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_WATERFALL_DEPOT]))
                        {
                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == ALLIANCE)
                            {
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_ALLIANCE);
                            }

                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == HORDE)
                            {
                                SendMessageToAll(LANG_BG_SSM_HORDE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_HORDE);
                            }

                            if (m_MineCartsProgressBar[BG_SSM_MINE_CART_2 - 1] != BG_SSM_PROGRESS_BAR_NEUTRAL)
                                BattlegroundSSM::AddPoints(GetMineCartTeamKeeper(BG_SSM_MINE_CART_2), POINTS_PER_MINE_CART);
                            
                            BattlegroundSSM::ResetDepotsAndMineCarts(SSM_WATERFALL_DEPOT, BG_SSM_MINE_CART_2);
                            depot->ResetDoorOrButton();
                            cart->DespawnOrUnsummon();
                        }
                    }
                }
            } else m_DepotCloseTimer[SSM_WATERFALL_DEPOT] -= diff;
        }
    }

    if (m_MineCartReachedDepot[BG_SSM_MINE_CART_3 - 1])
    {
        if (m_Depot[SSM_DIAMOND_DEPOT])
        {
            if (m_DepotCloseTimer[SSM_DIAMOND_DEPOT] <= 0)
            {
                if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
                {
                    if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_DIAMOND_DEPOT]))
                        {
                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == ALLIANCE)
                            {
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_ALLIANCE);
                            }

                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == HORDE)
                            {
                                SendMessageToAll(LANG_BG_SSM_HORDE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_HORDE);
                            }

                            if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] != BG_SSM_PROGRESS_BAR_NEUTRAL)
                                BattlegroundSSM::AddPoints(GetMineCartTeamKeeper(BG_SSM_MINE_CART_3), POINTS_PER_MINE_CART);
                            
                            BattlegroundSSM::ResetDepotsAndMineCarts(SSM_DIAMOND_DEPOT, BG_SSM_MINE_CART_3);
                            depot->ResetDoorOrButton();
                            cart->DespawnOrUnsummon();
                        }
                    }
                }
            } else m_DepotCloseTimer[SSM_DIAMOND_DEPOT] -= diff;
        }

        if (m_Depot[SSM_TROLL_DEPOT])
        {
            if (m_DepotCloseTimer[SSM_TROLL_DEPOT] <= 0)
            {
                if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
                {
                    if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
                    {
                        if (GameObject* depot = GetBgMap()->GetGameObject(BgObjects[BG_SSM_OBJECT_TROLL_DEPOT]))
                        {
                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == ALLIANCE)
                            {
                                SendMessageToAll(LANG_BG_SSM_ALLIANCE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_ALLIANCE);
                            }

                            if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == HORDE)
                            {
                                SendMessageToAll(LANG_BG_SSM_HORDE_CAPTURED_MINE_CART, CHAT_MSG_BG_SYSTEM_HORDE);
                                PlaySoundToAll(BG_SSM_SOUND_MINE_CART_CAPTURED_HORDE);
                            }

                            if (m_MineCartsProgressBar[BG_SSM_MINE_CART_3 - 1] != BG_SSM_PROGRESS_BAR_NEUTRAL)
                                BattlegroundSSM::AddPoints(GetMineCartTeamKeeper(BG_SSM_MINE_CART_3), POINTS_PER_MINE_CART);
                            
                            BattlegroundSSM::ResetDepotsAndMineCarts(SSM_TROLL_DEPOT, BG_SSM_MINE_CART_3);
                            depot->ResetDoorOrButton();
                            cart->DespawnOrUnsummon();
                        }
                    }
                }
            } else m_DepotCloseTimer[SSM_TROLL_DEPOT] -= diff;
        }
    }
}

uint32 BattlegroundSSM::GetMineCartTeamKeeper(uint8 mineCart)
{
    if (m_MineCartsProgressBar[mineCart - 1] > BG_SSM_PROGRESS_BAR_NEUTRAL)
        return ALLIANCE;

    if (m_MineCartsProgressBar[mineCart - 1] < BG_SSM_PROGRESS_BAR_NEUTRAL)
        return HORDE;

    return 0;
}

void BattlegroundSSM::MineCartAddPoints(uint32 diff)
{
    if (m_MineCartAddPointsTimer <= 0)
    {
           /* ==================  ALLIANCE  ================== */
        if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == ALLIANCE)
            BattlegroundSSM::AddPoints(ALLIANCE, 5);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) != ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == ALLIANCE)
            BattlegroundSSM::AddPoints(ALLIANCE, 3);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) != ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == ALLIANCE)
            BattlegroundSSM::AddPoints(ALLIANCE, 3);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) != ALLIANCE)
            BattlegroundSSM::AddPoints(ALLIANCE, 3);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) != ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) != ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == ALLIANCE)
            BattlegroundSSM::AddPoints(ALLIANCE, 2);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) != ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) != ALLIANCE)
            BattlegroundSSM::AddPoints(ALLIANCE, 2);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) != ALLIANCE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) != ALLIANCE)
            BattlegroundSSM::AddPoints(ALLIANCE, 2);

            /* ==================  HORDE  ================== */
        if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == HORDE)
            BattlegroundSSM::AddPoints(HORDE, 5);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) != HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == HORDE)
            BattlegroundSSM::AddPoints(HORDE, 3);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) != HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == HORDE)
            BattlegroundSSM::AddPoints(HORDE, 3);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) != HORDE)
            BattlegroundSSM::AddPoints(HORDE, 3);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) != HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) != HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) == HORDE)
            BattlegroundSSM::AddPoints(HORDE, 2);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) != HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) != HORDE)
            BattlegroundSSM::AddPoints(HORDE, 2);

        else if (GetMineCartTeamKeeper(BG_SSM_MINE_CART_1) == HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_2) != HORDE &&
            GetMineCartTeamKeeper(BG_SSM_MINE_CART_3) != HORDE)
            BattlegroundSSM::AddPoints(HORDE, 2);

        m_MineCartAddPointsTimer = 2000;
    } else m_MineCartAddPointsTimer -= diff;
}

void BattlegroundSSM::ResetDepotsAndMineCarts(uint8 depot, uint8 mineCart)
{
    m_DepotCloseTimer[depot] = 3000;
    m_Depot[depot] = false;
    m_MineCartsProgressBar[mineCart - 1] = BG_SSM_PROGRESS_BAR_NEUTRAL;
    m_MineCartNearDepot[mineCart - 1] = false;
    m_MineCartReachedDepot[mineCart - 1] = false;
    m_MineCartSpawned[mineCart - 1] = false;
}

void BattlegroundSSM::MineCartsMoves(uint32 diff)
{
    Creature* trigger = NULL;
    if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
    {
        if (!m_PathDone[SSM_EAST_PATH][0])
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
            {
                cart->GetMotionMaster()->MovePath(NPC_MINE_CART_1, false);
                m_PathDone[SSM_EAST_PATH][0] = true;
            }

        if (m_PathDone[SSM_EAST_PATH][0] && !m_PathDone[SSM_EAST_PATH][1])
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
                if (cart->GetExactDist2d(717.169312f, 114.258339f) < 0.5f) // East pos
                    if (Creature* track = GetBgMap()->GetCreature(BgCreatures[SSM_TRACK_SWITCH_EAST]))
                        if (track->HasAura(BG_SSM_TRACK_SWITCH_OPENED))
                        {
                            cart->GetMotionMaster()->Clear(true);
                            cart->GetMotionMaster()->MovePath(NPC_MINE_CART_1 * 10, false);
                            m_PathDone[SSM_EAST_PATH][1] = true;
                        }
                        else
                        {
                            cart->GetMotionMaster()->Clear(true);
                            cart->GetMotionMaster()->MovePath(NPC_MINE_CART_1 * 100, false);
                            m_PathDone[SSM_EAST_PATH][1] = true;
                        }

        if (!m_WaterfallPathDone)
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f, true))
            {
                cart->GetMotionMaster()->MovePath(NPC_MINE_CART_2, false);
                m_WaterfallPathDone = true;
            }

        if (!m_PathDone[SSM_NORTH_PATH][0])
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
            {
                cart->GetMotionMaster()->MovePath(NPC_MINE_CART_3, false);
                m_PathDone[SSM_NORTH_PATH][0] = true;
            }

        if (m_PathDone[SSM_NORTH_PATH][0] && !m_PathDone[SSM_NORTH_PATH][1])
            if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
                if (cart->GetExactDist2d(834.727234f, 299.809753f) < 0.5f) // North pos
                    if (Creature* track = GetBgMap()->GetCreature(BgCreatures[SSM_TRACK_SWITCH_NORTH]))
                        if (track->HasAura(BG_SSM_TRACK_SWITCH_CLOSED))
                        {
                            cart->GetMotionMaster()->Clear(true);
                            cart->GetMotionMaster()->MovePath(NPC_MINE_CART_3 * 10, false);
                            m_PathDone[SSM_NORTH_PATH][1] = true;
                        }
                        else
                        {
                            cart->GetMotionMaster()->Clear(true);
                            cart->GetMotionMaster()->MovePath(NPC_MINE_CART_3 * 100, false);
                            m_PathDone[SSM_NORTH_PATH][1] = true;
                        }
    }
}

void BattlegroundSSM::AddPoints(uint32 team, uint32 points)
{
    BattlegroundTeamId team_index = GetTeamIndexByTeamId(team);
    m_TeamScores[team_index] += points;
    m_HonorScoreTics[team_index] += points;
    if (m_HonorScoreTics[team_index] >= m_HonorTics)
    {
        RewardHonorToTeam(uint32(GetBonusHonorFromKill(10) / 100), team);
        m_HonorScoreTics[team_index] -= m_HonorTics;
    }

    UpdateTeamScore(team);
}

void BattlegroundSSM::UpdateTeamScore(uint32 team)
{
    uint32 score = GetTeamScore(Team(team));
    team = GetTeamIndexByTeamId(team);
    if (!m_IsInformedNearVictory && score >= BG_SSM_WARNING_NEAR_VICTORY_SCORE)
    {
        if (team == TEAM_ALLIANCE)
            SendMessageToAll(LANG_BG_SSM_A_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        else
            SendMessageToAll(LANG_BG_SSM_H_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);

        PlaySoundToAll(BG_SSM_SOUND_NEAR_VICTORY);
        m_IsInformedNearVictory = true;
    }

    if (score >= BG_SSM_MAX_TEAM_SCORE)
    {
        score = BG_SSM_MAX_TEAM_SCORE;
        if (team == TEAM_ALLIANCE)
            EndBattleground(ALLIANCE);
        else
            EndBattleground(HORDE);
    }

    if (team == TEAM_ALLIANCE)
        UpdateWorldState(SSM_ALLIANCE_RESOURCES, score);
    else
        UpdateWorldState(SSM_HORDE_RESOURCES, score);
}

void BattlegroundSSM::EndBattleground(uint32 winner)
{
    Battleground::EndBattleground(winner);

    Creature* trigger = NULL;
    if (trigger = GetBgMap()->GetCreature(BgCreatures[SSM_MINE_CART_TRIGGER]))
    {
        if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_1, 99999.0f, true))
            cart->DespawnOrUnsummon();

        if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_2, 99999.0f, true))
            cart->DespawnOrUnsummon();

        if (Creature* cart = trigger->FindNearestCreature(NPC_MINE_CART_3, 99999.0f, true))
            cart->DespawnOrUnsummon();
    }
}

void BattlegroundSSM::UpdatePointsCount(uint32 Team)
{
    if (Team == ALLIANCE)
        UpdateWorldState(SSM_ALLIANCE_RESOURCES, m_TeamScores[TEAM_ALLIANCE]);
    else
        UpdateWorldState(SSM_HORDE_RESOURCES, m_TeamScores[TEAM_HORDE]);
}

void BattlegroundSSM::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    //create score and add it to map
    BattlegroundSSMScore* sc = new BattlegroundSSMScore();

    PlayerScores[player->GetGUID()] = sc;
}

bool BattlegroundSSM::SetupBattleground()
{
    // doors
    if (!AddObject(BG_SSM_OBJECT_WATERFALL_DEPOT, BG_SSM_MINE_DEPOT, BG_SSM_DepotPos[SSM_WATERFALL_DEPOT][0], BG_SSM_DepotPos[SSM_WATERFALL_DEPOT][1], BG_SSM_DepotPos[SSM_WATERFALL_DEPOT][2], BG_SSM_DepotPos[SSM_WATERFALL_DEPOT][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)    // Waterfall
        || !AddObject(BG_SSM_OBJECT_LAVA_DEPOT, BG_SSM_MINE_DEPOT, BG_SSM_DepotPos[SSM_LAVA_DEPOT][0], BG_SSM_DepotPos[SSM_LAVA_DEPOT][1], BG_SSM_DepotPos[SSM_LAVA_DEPOT][2], BG_SSM_DepotPos[SSM_LAVA_DEPOT][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)                            // Lava
        || !AddObject(BG_SSM_OBJECT_DIAMOND_DEPOT, BG_SSM_MINE_DEPOT, BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][0], BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][1], BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][2], BG_SSM_DepotPos[SSM_DIAMOND_DEPOT][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)            // Diamond
        || !AddObject(BG_SSM_OBJECT_TROLL_DEPOT, BG_SSM_MINE_DEPOT, BG_SSM_DepotPos[SSM_TROLL_DEPOT][0], BG_SSM_DepotPos[SSM_TROLL_DEPOT][1], BG_SSM_DepotPos[SSM_TROLL_DEPOT][2], BG_SSM_DepotPos[SSM_TROLL_DEPOT][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)                        // Troll
        || !AddObject(BG_SSM_OBJECT_DOOR_A_1, BG_SSM_DOOR, BG_SSM_DoorPos[0][0], BG_SSM_DoorPos[0][1], BG_SSM_DoorPos[0][2], BG_SSM_DoorPos[0][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_DOOR_A_2, BG_SSM_DOOR, BG_SSM_DoorPos[1][0], BG_SSM_DoorPos[1][1], BG_SSM_DoorPos[1][2], BG_SSM_DoorPos[1][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_DOOR_H_1, BG_SSM_DOOR, BG_SSM_DoorPos[2][0], BG_SSM_DoorPos[2][1], BG_SSM_DoorPos[2][2], BG_SSM_DoorPos[2][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_DOOR_H_2, BG_SSM_DOOR, BG_SSM_DoorPos[3][0], BG_SSM_DoorPos[3][1], BG_SSM_DoorPos[3][2], BG_SSM_DoorPos[3][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_BERSERKING_BUFF_EAST, BG_OBJECTID_BERSERKERBUFF_ENTRY, BG_SSM_BuffPos[0][0], BG_SSM_BuffPos[0][1], BG_SSM_BuffPos[0][2], BG_SSM_BuffPos[0][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_BERSERKING_BUFF_WEST, BG_OBJECTID_BERSERKERBUFF_ENTRY, BG_SSM_BuffPos[1][0], BG_SSM_BuffPos[1][1], BG_SSM_BuffPos[1][2], BG_SSM_BuffPos[1][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_RESTORATION_BUFF_WATERFALL, BG_OBJECTID_REGENBUFF_ENTRY, BG_SSM_BuffPos[2][0], BG_SSM_BuffPos[2][1], BG_SSM_BuffPos[2][2], BG_SSM_BuffPos[2][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_RESTORATION_BUFF_LAVA, BG_OBJECTID_REGENBUFF_ENTRY, BG_SSM_BuffPos[3][0], BG_SSM_BuffPos[3][1], BG_SSM_BuffPos[3][2], BG_SSM_BuffPos[3][3], 0, 0, 0.710569f, -0.703627f, RESPAWN_IMMEDIATELY))
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "BatteGroundSM: Failed to spawn some object Battleground not created!");
        return false;
    }

    // Npcs
    if (!AddCreature(NPC_MINE_CART_TRIGGER, SSM_MINE_CART_TRIGGER, TEAM_NEUTRAL, 748.360779f, 195.203018f, 331.861938f, 2.428625f))
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "BatteGroundSM: Failed to spawn some creatures Battleground not created!");
        return false;
    }

    if (Creature* track = AddCreature(NPC_TRACK_SWITCH_EAST, SSM_TRACK_SWITCH_EAST, TEAM_NEUTRAL, BG_SSM_TrackPos[SSM_EAST_PATH][0], BG_SSM_TrackPos[SSM_EAST_PATH][1], BG_SSM_TrackPos[SSM_EAST_PATH][2], BG_SSM_TrackPos[SSM_EAST_PATH][3]))
    {
        track->CastSpell(track, BG_SSM_FEIGN_DEATH_STUN, true);
        track->CastSpell(track, BG_SSM_TRACK_SWITCH_OPENED, true);
    }
    else 
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "BatteGroundSM: Failed to spawn some creatures Battleground not created!");
        return false;
    }

    if (Creature* track = AddCreature(NPC_TRACK_SWITCH_NORTH, SSM_TRACK_SWITCH_NORTH, TEAM_NEUTRAL, BG_SSM_TrackPos[SSM_NORTH_PATH][0], BG_SSM_TrackPos[SSM_NORTH_PATH][1], BG_SSM_TrackPos[SSM_NORTH_PATH][2], BG_SSM_TrackPos[SSM_NORTH_PATH][3]))
    {
        track->CastSpell(track, BG_SSM_FEIGN_DEATH_STUN, true);
        track->CastSpell(track, BG_SSM_TRACK_SWITCH_CLOSED, true);
    }
    else 
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "BatteGroundSM: Failed to spawn some creatures Battleground not created!");
        return false;
    }

    WorldSafeLocsEntry const* sg = NULL;
    sg = sWorldSafeLocsStore.LookupEntry(SSM_GRAVEYARD_MAIN_ALLIANCE);
    if (!sg || !AddSpiritGuide(SSM_SPIRIT_ALLIANCE, sg->x, sg->y, sg->z, 2.138462f, ALLIANCE))
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "BatteGroundSM: Failed to spawn spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(SSM_GRAVEYARD_MAIN_HORDE);
    if (!sg || !AddSpiritGuide(SSM_SPIRIT_HORDE, sg->x, sg->y, sg->z, 5.570653f, HORDE))
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "BatteGroundSM: Failed to spawn spirit guide! Battleground not created!");
        return false;
    }
    return true;
}

WorldSafeLocsEntry const * BattlegroundSSM::GetClosestGraveYard(Player* player)
{
    SSMBattlegroundGaveyards bggy = player->GetTeam() == ALLIANCE ? SSM_GRAVEYARD_MAIN_ALLIANCE : SSM_GRAVEYARD_MAIN_HORDE;
    WorldSafeLocsEntry const* sg = sWorldSafeLocsStore.LookupEntry(bggy);
    return sg;
}

void BattlegroundSSM::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    Battleground::HandleKillPlayer(player, killer);
    EventPlayerDroppedFlag(player);
}

void BattlegroundSSM::UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(player->GetGUID());
    if (itr == PlayerScores.end())                         // player not found
        return;

    switch (type)
    {
        case SCORE_CARTS_HELPED:                           // Mine Carts captures
            ((BattlegroundSSMScore*)itr->second)->MineCartCaptures += value;
            break;

        default:
            Battleground::UpdatePlayerScore(player, NULL, type, value, doAddHonor);
            break;
    }

    return;
}

void BattlegroundSSM::FillInitialWorldStates(WorldPacket& data)
{
    data << uint32(SSM_MINE_CARTS_DISPLAY) << uint32(DISPLAY_WORLDSTATE);
    data << uint32(SSM_ALLIANCE_RESOURCES) << uint32(m_TeamScores[TEAM_HORDE]);
    data << uint32(SSM_HORDE_RESOURCES) << uint32(m_TeamScores[TEAM_ALLIANCE]);
    data << uint32(SSM_MINE_CART_1) << uint32(DISPLAY_WORLDSTATE);
    data << uint32(SSM_MINE_CART_2) << uint32(DISPLAY_WORLDSTATE);
    data << uint32(SSM_DISPLAY_ALLIANCE_RESSOURCES) << uint32(DISPLAY_WORLDSTATE);
    data << uint32(SSM_MINE_CART_3) << uint32(DISPLAY_WORLDSTATE);
    data << uint32(SSM_DISPLAY_HORDE_RESSOURCES) << uint32(DISPLAY_WORLDSTATE);
    data << uint32(SSM_DISPLAY_PROGRESS_BAR) << uint32(BG_SSM_PROGRESS_BAR_DONT_SHOW); // This shows the mine cart control bar
    data << uint32(SSM_PROGRESS_BAR_STATUS) << uint32(BG_SSM_PROGRESS_BAR_NEUTRAL); // Neutral
    data << uint32(SSM_UNK) << uint32(0);
}

uint32 BattlegroundSSM::GetPrematureWinner()
{
    if (GetTeamScore(ALLIANCE) > GetTeamScore(HORDE))
        return ALLIANCE;

    else if (GetTeamScore(HORDE) > GetTeamScore(ALLIANCE))
        return HORDE;

    return Battleground::GetPrematureWinner();
}

class npc_track_switch_east : public CreatureScript
{
public:
    npc_track_switch_east() : CreatureScript("npc_track_switch_east") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->HasAura(BG_SSM_PREVENTION_AURA)) // Prevention aura
        {
            player->PlayerTalkClass->SendCloseGossip(); // Prevent from using multiple times track switches();
            return false;
        }

        if (creature->HasAura(BG_SSM_TRACK_SWITCH_OPENED)) // Opened
        {
            creature->CastSpell(creature, BG_SSM_TRACK_SWITCH_CLOSED, true); // Closed
            creature->RemoveAurasDueToSpell(BG_SSM_TRACK_SWITCH_OPENED);
            creature->CastSpell(creature, BG_SSM_PREVENTION_AURA, true); // Prevention aura
        }
        else // Closed
        {
            creature->CastSpell(creature, BG_SSM_TRACK_SWITCH_OPENED, true); // Opened
            creature->RemoveAurasDueToSpell(BG_SSM_TRACK_SWITCH_CLOSED);
            creature->CastSpell(creature, BG_SSM_PREVENTION_AURA, true); // Prevention aura
        }

        return true;
    }
};

class npc_track_switch_north : public CreatureScript
{
public:
    npc_track_switch_north() : CreatureScript("npc_track_switch_north") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->HasAura(BG_SSM_PREVENTION_AURA)) // Prevention aura
        {
            player->PlayerTalkClass->SendCloseGossip(); // Prevent from using multiple times track switches();
            return false;
        }

        if (creature->HasAura(BG_SSM_TRACK_SWITCH_OPENED)) // Opened
        {
            creature->CastSpell(creature, BG_SSM_TRACK_SWITCH_CLOSED, true); // Closed
            creature->RemoveAurasDueToSpell(BG_SSM_TRACK_SWITCH_OPENED);
            creature->CastSpell(creature, BG_SSM_PREVENTION_AURA, true); // Prevention aura
        }
        else // Closed
        {
            creature->CastSpell(creature, BG_SSM_TRACK_SWITCH_OPENED, true); // Opened
            creature->RemoveAurasDueToSpell(BG_SSM_TRACK_SWITCH_CLOSED);
            creature->CastSpell(creature, BG_SSM_PREVENTION_AURA, true); // Prevention aura
        }

        return true;
    }
};

void AddSC_BattlegroundSSMScripts()
{
    new npc_track_switch_east();
    new npc_track_switch_north();
}