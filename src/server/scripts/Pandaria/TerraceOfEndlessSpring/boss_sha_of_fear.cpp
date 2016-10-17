/*
 * Copyright (C) 2012-2016 WoWSource <http://wowsource.info/>
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

#include "GameObjectAI.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "terrace_of_endless_spring.h"
#include <numeric>

enum eShaOfFearSpells
{
    // Sha of Fear
    SPELL_REACHING_ATTACK           = 119775,
    SPELL_EERIE_SKULL               = 119519,
    SPELL_BREATH_OF_FEAR            = 119414,
    SPELL_THRASH_AURA               = 131996,
    SPELL_THRASH_EXTRA_ATTACKS      = 131994,
    SPELL_CONJURE_TERROR_SPAWN_TICK = 119108,
    // 4 spells for spawn, cauz' of different spawn coordinates
    SPELL_CONJURE_TERROR_SPAWN_01   = 119312,
    SPELL_CONJURE_TERROR_SPAWN_02   = 119370,
    SPELL_CONJURE_TERROR_SPAWN_03   = 119371,
    SPELL_CONJURE_TERROR_SPAWN_04   = 119372,
    SPELL_CUSTOM_ENERGY_REGEN       = 119417,

    //Heroic
    SPELL_NAKED_AND_AFRAID          = 120669,
    SPELL_FADING_LIGHT              = 129378,
    SPELL_IMPLACABLE_STRIKE         = 120672,
    SPELL_DREAD_THRASH              = 132007,
    SPELL_SUBMERGE                  = 120455,
    SPELL_EMERGE                    = 120458,
    SPELL_EMERGE_STUN               = 120475,
    SPELL_HUDDLE_IN_TERROR          = 120629,
    SPELL_WATERSPOUT                = 120519,
    SPELL_CHAMPION_OF_THE_LIGHT     = 120268,
    SPELL_TRANSFER_LIGHT            = 120285,
    SPELL_DREAD_EXPANSE             = 120289,
    SPELL_ETERNAL_DARKNESS          = 120394,
    SPELL_TELEPORT_SHA              = 120191,
    SPELL_TELEPORT_PLAYER           = 120192,
    SPELL_TELEPORT_TO_DREAD_EXPANSE = 120221,
    SPELL_PHASE2_ENERGY_REGEN       = 120284,

    // Other mobs
    SPELL_LIGHT_WALL                = 117865,
    SPELL_CHAMPION_OF_LIGHT         = 117866,
    SPELL_LIGHT_WALL_READY          = 117770,
    SPELL_LIGHT_WALL_VISUAL         = 107141,
    SPELL_FEARLESS                  = 118977,
    SPELL_WALL_OF_LIGHT_BUFF        = 117999,
    SPELL_PENETRATING_BOLT          = 129075,
    SPELL_PENETRATING_BOLT_MISSILE  = 129077,
    SPELL_DARK_BULWARK              = 119083
};

enum eShaOfFearEvents
{
    EVENT_CHECK_MELEE       = 1,
    EVENT_EERIE_SKULL       = 2,
    EVENT_CHECK_ENERGY      = 3,
    EVENT_FIRST_TERRORS     = 4,
    EVENT_PENETRATING_BOLT  = 5,
    EVENT_OMINOUS_CACKLE    = 6,
    EVENT_EVADE             = 7,
    EVENT_BERSERK           = 8,
    EVENT_SET_EVADE_TRUE    = 10,
    EVENT_HUDDLE_IN_TERROR  = 11,
    EVENT_WATERSPOUT        = 12,
    EVENT_NAKED_AND_AFRAID  = 13,
    EVENT_IMPLACABLE_STRIKE = 14
};

enum eShaOfFearActions
{
    ACTION_ACTIVATE_WALL_OF_LIGHT,
    ACTION_DESACTIVATE_WALL_OF_LIGHT,
    ACTION_SPAWN_TERROR,
    ACTION_BERSERK
};

enum eShaOfFearSays
{
    SAY_INTRO,
    SAY_AGGRO,
    SAY_BREATH_OF_FEAR,
    SAY_SLAY,
    SAY_PHASE_2,
    SAY_SUBMERGE,
    SAY_HUDDLE,
    EMOTE_PHASE_2,

};

enum Entities
{
    NPC_YANG_GUOSHI         = 61038,
    NPC_JINLUN_KUN          = 61046,
    NPC_CHENG_KANG          = 61042,
};

enum Misc
{
    ACTION_BOWMAN_AGGRESSIVE        = 10
};

struct BowmanData
{
    uint32 entry;
    uint32 spellId;
    float x;
    float y;
    float z;
    float o;
    bool active;
};

static BowmanData bowmenData[] = // DNC
{
    { NPC_YANG_GUOSHI, 119593, -1214.795f, -2824.823f, 41.24303f, 3.506719f },
    { NPC_CHENG_KANG, 119693, -1075.198f, -2577.711f, 15.828019f, 1.725f },
    { NPC_JINLUN_KUN, 119692, -832.0764f, -2745.405f, 31.67757f, 0.1583484f }
};

Player* GetChampionOfLight(Creature* me)
{
    if (!me)
        return NULL;

    Map::PlayerList const& playerList = me->GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
        if (Player* player = itr->getSource())
            if (player->HasAura(SPELL_CHAMPION_OF_LIGHT))
                return player;

    return NULL;
}

static const Position spawnTerrorPos[4] =
{
    { -1052.588f, -2788.164f, 38.268f, 0.0f },
    { -983.4550f, -2787.942f, 38.269f, 0.0f },
    { -989.6860f, -2772.245f, 38.303f, 0.0f },
    { -1046.274f, -2772.215f, 38.303f, 0.0f }
};

enum eShaPhases
{
    PHASE_TERRACE       = 1,
    PHASE_DREAD_EXPANSE
};

class DpsSelectPredicate
{
public:
    bool operator()(WorldObject* target) const
    {
        return target && target->ToPlayer() && target->ToPlayer()->GetRoleForGroup(target->ToPlayer()->GetSpecializationId(target->ToPlayer()->GetActiveSpec())) != ROLES_DPS;
    }
};

class notValidTargetPredicate
{
public:
    notValidTargetPredicate(Unit* _caster) : caster(_caster) {}

    bool operator()(WorldObject* target) const
    {
        return target && target->ToPlayer() && (target->ToPlayer()->HasAura(SPELL_CHAMPION_OF_LIGHT) || target->ToPlayer()->GetExactDist2d(caster) > 70.f);
    }
private:
    Unit* caster;
};

static const Position lightPos = { -1017.835f, -2771.984f, 38.65444f, 4.718282f };

class boss_sha_of_fear : public CreatureScript
{
    public:
        boss_sha_of_fear() : CreatureScript("boss_sha_of_fear") { }

        struct boss_sha_of_fearAI : public BossAI
        {
            boss_sha_of_fearAI(Creature* creature) : BossAI(creature, DATA_SHA_OF_FEAR)
            {
                pInstance = creature->GetInstanceScript();
                introDone = false;
                isDuringP2Transition = false;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);

                for (int i = 0; i < 3; ++i)
                {
                    BowmanData* data = new BowmanData();
                    data->entry = bowmenData[i].entry;
                    data->spellId = bowmenData[i].spellId;
                    data->x = bowmenData[i].x;
                    data->y = bowmenData[i].y;
                    data->z = bowmenData[i].z;
                    data->o = bowmenData[i].o;
                    data->active = false;

                    pandaMap.insert(std::make_pair(data, data->entry));
                }
            }

            InstanceScript* pInstance;
            EventMap m_mEvents;

            uint8 attacksCounter;
            uint8 terrorCounter;

            uint8 m_uiPhase;

            bool m_canEvade;
            bool isDuringP2Transition;
            bool isSecondEvent;

            std::unordered_map<BowmanData*, uint32> pandaMap;

            void Reset()
            {
                _Reset();


                if (isDuringP2Transition)
                    return;

                m_canEvade = true;
                isDuringP2Transition = false;

                me->AddAura(72242, me);
                me->SetReactState(REACT_DEFENSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                summons.DespawnAll();

                SetPhase(0);

                attacksCounter = 0;
                terrorCounter  = 0;

                for (std::unordered_map<BowmanData*, uint32>::iterator itr = pandaMap.begin(); itr != pandaMap.end(); ++itr)
                {
                    itr->first->active = false;
                }

                isSecondEvent = false;

                if (pInstance)
                {
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                }
            }

            void SetPhase(uint8 m_phase)
            {
                events.Reset();

                switch (m_phase)
                {
                case PHASE_TERRACE:
                    events.ScheduleEvent(EVENT_CHECK_MELEE, 1000);
                    events.ScheduleEvent(EVENT_EERIE_SKULL, 5000);
                    events.ScheduleEvent(EVENT_CHECK_ENERGY, 1000);
                    events.ScheduleEvent(EVENT_FIRST_TERRORS, 30000);
                    events.ScheduleEvent(EVENT_OMINOUS_CACKLE, 35000);
                    break;
                case PHASE_DREAD_EXPANSE:
                    summons.DespawnAll();
                    me->SetVisible(false);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    SetCombatMovement(true);
                    events.ScheduleEvent(EVENT_SET_EVADE_TRUE, 2000);
                    m_canEvade = false;
                    Talk(SAY_PHASE_2);
                    Talk(EMOTE_PHASE_2);

                    DoCast(SPELL_TELEPORT_TO_DREAD_EXPANSE);
                    DoCast(SPELL_TELEPORT_SHA);

                    if (Player* pPlayer = GetChampionOfLight(me))
                        pPlayer->CastSpell(pPlayer, SPELL_CHAMPION_OF_THE_LIGHT, true);

                    me->RemoveAurasDueToSpell(SPELL_CUSTOM_ENERGY_REGEN);

                    SetupPhaseTwoEvents();

                    events.ScheduleEvent(EVENT_CHECK_ENERGY, 1000);

                    me->SetHealth((me->GetMaxHealth() / 66.6f) * 100);
                    break;
                }

                m_uiPhase = m_phase;
                //events.SetPhase(m_phase);
            }

            void GetAvailablePandaAndSummon()
            {
                std::unordered_map<BowmanData*, uint32> newPandaMap;
                newPandaMap.reserve(sizeof(pandaMap));
                std::copy(pandaMap.begin(), pandaMap.end(), std::inserter(newPandaMap, newPandaMap.begin()));

                sLog->outError(LOG_FILTER_GENERAL, "Map size of PandaMap is %u, newPandaMap %u", pandaMap.size(), newPandaMap.size());

                std::unordered_map<BowmanData*, uint32>::iterator itr = newPandaMap.begin();

                for (;  itr != newPandaMap.end();)
                {
                    if (itr->first->active)
                    {
                        sLog->outError(LOG_FILTER_GENERAL, "erased itr because npc is already active (%u)", itr->first->entry);
                        itr = newPandaMap.erase(itr);
                    }
                    else
                        ++itr;
                }

                itr = newPandaMap.begin();

                sLog->outError(LOG_FILTER_GENERAL, "size of newPandaMap is now (%u)", newPandaMap.size());

                if (!newPandaMap.empty())
                {
                    if (newPandaMap.size() > 1)
                        std::advance(itr, std::rand() % (newPandaMap.size() - 1));
                    
                    if (itr != newPandaMap.cend())
                    {
                        for (std::unordered_map<BowmanData*, uint32>::const_iterator finder = pandaMap.begin(); finder != pandaMap.end(); ++finder)
                        {
                            if (finder->first->entry == itr->second)
                            {
                                sLog->outError(LOG_FILTER_GENERAL, "summon propagated for entry %u", finder->first->entry);
                                PropagateSummon(finder);
                                return;
                            }
                        }
                    }
                }

                itr = pandaMap.begin();

                std::advance(itr, std::rand() % (pandaMap.size() - 1));

                if (itr != pandaMap.cend())
                    DoCast(me, itr->first->spellId);

                sLog->outError(LOG_FILTER_GENERAL, "iterator set to pandaMap.begin(); because no inactive panda could be found. advanced to position %u", itr->first->entry);
            }

            void PropagateSummon(std::unordered_map<BowmanData*, uint32>::const_iterator &itr)
            {
                if (Creature* pBowman = me->SummonCreature(itr->first->entry, itr->first->x, itr->first->y, itr->first->z, itr->first->o))
                {
                    DoCast(me, itr->first->spellId);
                    itr->first->active = true;

                    if (pBowman->AI())
                        pBowman->AI()->DoAction(ACTION_BOWMAN_AGGRESSIVE);
                }
            }

            void JustReachedHome()
            {
                _JustReachedHome();

                if (pInstance)
                    pInstance->SetBossState(DATA_SHA_OF_FEAR, FAIL);
            }

            void SetupPhaseTwoEvents()
            {
                uint32 mEvents[3] = { EVENT_HUDDLE_IN_TERROR, EVENT_WATERSPOUT, EVENT_IMPLACABLE_STRIKE };

                std::iota(std::begin(mEvents), std::end(mEvents), 0);
                std::random_shuffle(std::begin(mEvents), std::end(mEvents));

                for (uint8 i = 0; i < 3; ++i)
                {
                    events.ScheduleEvent(mEvents[i], urand(12000, 17000) + i*(urand(15000, 19000)));
                }
            }

            void EnterCombat(Unit* /*attacker*/)
            {

                if (isDuringP2Transition)
                    return;

                me->SetPower(POWER_ENERGY, 0);

                DoCast(me, SPELL_CUSTOM_ENERGY_REGEN, true);

                if (pInstance)
                {
                    pInstance->SetBossState(DATA_SHA_OF_FEAR, IN_PROGRESS);
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    DoZoneInCombat();
                    Talk(SAY_AGGRO);
                }

                SetPhase(PHASE_TERRACE);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (pInstance)
                {
                    summons.DespawnAll();
                    pInstance->SetBossState(DATA_SHA_OF_FEAR, DONE);
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    _JustDied();
                }
            }

            void EnterEvadeMode()
            {
                if (m_canEvade)
                {
                    if (me->HasUnitState(UNIT_STATE_CANNOT_TURN))
                        me->ClearUnitState(UNIT_STATE_CANNOT_TURN);

                    Map::PlayerList const& lPlayers = me->GetMap()->GetPlayers();

                    for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                    {
                        if (Player* pPlayer = itr->getSource())
                        {
                            pPlayer->CastSpell(pPlayer, SPELL_FEARLESS, true);

                            if (pPlayer->isAlive())
                                pPlayer->Kill(pPlayer, false);
                            // No durability loss for a wipe in a case like this

                            if (pPlayer->HasAura(SPELL_CHAMPION_OF_THE_LIGHT))
                                pPlayer->RemoveAurasDueToSpell(SPELL_CHAMPION_OF_THE_LIGHT);

                            pPlayer->RemoveAurasDueToSpell(SPELL_DREAD_EXPANSE);
                            pPlayer->RemoveAurasDueToSpell(SPELL_FEARLESS);
                        }
                    }

                    if (m_uiPhase == PHASE_TERRACE)
                        BossAI::EnterEvadeMode();
                    else
                    {
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);

                        m_mEvents.ScheduleEvent(EVENT_EVADE, 10000);

                        float x, y, z, o;
                        me->GetHomePosition(x, y, z, o);
                        me->SetVisible(false);
                        me->NearTeleportTo(x, y, z, o);
                        me->UpdateObjectVisibility();

                        BossAI::EnterEvadeMode();
                    }
                }
            }

            void JustSummoned(Creature* summon)
            {
                if (summon->GetEntry() == NPC_TERROR_SPAWN)
                    ++terrorCounter;

                summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                if (summon->GetEntry() == NPC_TERROR_SPAWN)
                    --terrorCounter;

                summons.Despawn(summon);
            }

            void SummonedCreatureDies(Creature* summon, Unit* )
            {
                switch (summon->GetEntry())
                {
                    case NPC_YANG_GUOSHI:
                    case NPC_CHENG_KANG:
                    case NPC_JINLUN_KUN:               
                        for (std::unordered_map<BowmanData*, uint32>::iterator itr = pandaMap.begin(); itr != pandaMap.end(); ++itr)
                        {
                            if (itr->first->entry == summon->GetEntry())
                                itr->first->active = false;
                        }
                        if (events.GetNextEventTime(EVENT_OMINOUS_CACKLE) > 30000)
                            events.RescheduleEvent(EVENT_OMINOUS_CACKLE, 30000);
                        break;
                    default:
                        return;
                }
            }

            void KilledUnit(Unit* who)
            {
                if (who->GetTypeId() == TYPEID_PLAYER)
                {
                    Talk(SAY_SLAY);
                }
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                    case ACTION_SPAWN_TERROR:
                    {
                        if (!terrorCounter)
                        {
                            me->CastSpell(spawnTerrorPos[0].GetPositionX(), spawnTerrorPos[0].GetPositionY(),
                                          spawnTerrorPos[0].GetPositionZ(), SPELL_CONJURE_TERROR_SPAWN_01, true);
                            me->CastSpell(spawnTerrorPos[1].GetPositionX(), spawnTerrorPos[1].GetPositionY(),
                                          spawnTerrorPos[1].GetPositionZ(), SPELL_CONJURE_TERROR_SPAWN_02, true);
                        }
                        else
                        {
                            me->CastSpell(spawnTerrorPos[2].GetPositionX(), spawnTerrorPos[2].GetPositionY(),
                                          spawnTerrorPos[2].GetPositionZ(), SPELL_CONJURE_TERROR_SPAWN_03, true);
                            me->CastSpell(spawnTerrorPos[3].GetPositionX(), spawnTerrorPos[3].GetPositionY(),
                                          spawnTerrorPos[3].GetPositionZ(), SPELL_CONJURE_TERROR_SPAWN_04, true);
                        }

                        break;
                    }
                    case ACTION_SHA_INTRO:
                        Talk(SAY_INTRO);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);

                        if (GameObject* pVortex = ObjectAccessor::GetGameObject(*me, instance->GetData64(GO_SHA_VORTEX)))
                            pVortex->SetGoState(GO_STATE_ACTIVE);

                        if (GameObject* pVortexWall = ObjectAccessor::GetGameObject(*me, instance->GetData64(GO_SHA_VORTEX_WALL)))
                            pVortexWall->SetGoState(GO_STATE_ACTIVE);
                        break;
                    case ACTION_BERSERK:
                        DoCast(me, SPELL_BERSERK, true);
                        break;
                    default:
                        break;
                }
            }

            void DamageDealt(Unit* /*victim*/, uint32& /*damage*/, DamageEffectType damageType, const SpellInfo * /*spellInfo*/)
            {
                if (damageType == DIRECT_DAMAGE)
                {
                    if (m_uiPhase == PHASE_TERRACE)
                    {
                        if (attacksCounter >= 3 && !me->m_extraAttacks)
                        {
                            me->CastSpell(me, SPELL_THRASH_EXTRA_ATTACKS, true);
                            attacksCounter = 0;
                        }
                        else if (attacksCounter >= 2 && !me->m_extraAttacks)
                        {
                            me->CastSpell(me, SPELL_THRASH_AURA, true);
                            ++attacksCounter;
                        }
                        else if (!me->m_extraAttacks)
                        {
                            me->RemoveAura(SPELL_THRASH_AURA);
                            ++attacksCounter;
                        }
                    }
                    else
                    {
                        if (attacksCounter > 3 && !me->m_extraAttacks)
                        {
                            DoCast(me, SPELL_DREAD_THRASH, true);
                            attacksCounter = 0;
                        }
                        else
                            ++attacksCounter;
                    }
                }
            }

            void OnAddThreat(Unit* /*victim*/, float& fThreat, SpellSchoolMask /*schoolMask*/, SpellInfo const* /*threatSpell*/)
            {
                fThreat = 0;
                return;
            }

            void DamageTaken(Unit* /*dealer*/, uint32& uiDamage)
            {
                if (me->HealthBelowPctDamaged(51, uiDamage) && (m_uiPhase == PHASE_TERRACE))
                {
                    if (me->GetMap()->IsHeroic())
                    {
                        if (auto const pInstance = me->GetInstanceScript())
                        {
                            if (pInstance->GetData(TYPE_LEIS_HOPE) != DONE)
                                pInstance->SetData(TYPE_LEIS_HOPE, DONE);
                        }

                        SetPhase(PHASE_DREAD_EXPANSE);
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                m_mEvents.Update(diff);

                switch (m_mEvents.ExecuteEvent())
                {
                case EVENT_EVADE:
                    me->SetVisible(true);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    break;
                }

                if (!UpdateVictim())
                {
                    //if (pInstance && pInstance->GetData(SPELL_RITUAL_OF_PURIFICATION) == false)
                        //me->RemoveAura(SPELL_RITUAL_OF_PURIFICATION);

                    return;
                }

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_CHECK_ENERGY:
                        if (!pInstance)
                        {
                            events.ScheduleEvent(EVENT_CHECK_ENERGY, 1000);
                            break;
                        }

                        if (me->GetPower(POWER_ENERGY) >= me->GetMaxPower(POWER_ENERGY))
                        {
                            if (m_uiPhase == PHASE_TERRACE)
                            {
                                if (Creature* pureLight = Creature::GetCreature(*me, pInstance->GetData64(NPC_PURE_LIGHT_TERRACE)))
                                    me->CastSpell(pureLight, SPELL_BREATH_OF_FEAR, false);
                                Talk(SAY_BREATH_OF_FEAR);
                            }
                            else
                            {
                                Talk(SAY_SUBMERGE);
                                DoCast(SPELL_SUBMERGE);
                                SetupPhaseTwoEvents();
                            }
                            me->SetPower(POWER_ENERGY, 0);
                            me->SetInt32Value(UNIT_FIELD_POWER1, 0);
                        }

                        events.ScheduleEvent(EVENT_CHECK_ENERGY, 1000);
                        break;
                    case EVENT_CHECK_MELEE:
                        if (Player* target = GetChampionOfLight(me))
                        {
                            if (me->getVictim() && me->getVictim()->GetGUID() != target->GetGUID())
                            {
                                me->TauntFadeOut(me->getVictim());
                                DoResetThreat();
                                AttackStart(target);
                                me->TauntApply(target);
                                me->AddThreat(target, 5000000.0f);
                            }
                        }

                        if (!me->IsWithinMeleeRange(me->getVictim(), 2.0f))
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f))
                                me->CastSpell(target, SPELL_REACHING_ATTACK, false);
                        }
                        events.ScheduleEvent(EVENT_CHECK_MELEE, 1000);
                        break;
                    case EVENT_EERIE_SKULL:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f))
                            DoCast(target, SPELL_EERIE_SKULL, false);
                        events.ScheduleEvent(EVENT_EERIE_SKULL, 5000);
                        break;
                    case EVENT_FIRST_TERRORS:
                        me->CastSpell(me, SPELL_CONJURE_TERROR_SPAWN_TICK, true);
                        break;
                    case EVENT_OMINOUS_CACKLE:
                        GetAvailablePandaAndSummon();
                        events.ScheduleEvent(EVENT_OMINOUS_CACKLE, (me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC || me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC) ? 90000 : 45000);
                        sLog->outError(LOG_FILTER_GENERAL, "scheduled new ominous cackle event");
                        break;
                    case EVENT_WATERSPOUT:
                        break;
                    case EVENT_IMPLACABLE_STRIKE:
                        me->AddUnitState(UNIT_STATE_CANNOT_TURN);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        DoCastVictim(SPELL_IMPLACABLE_STRIKE);
                        break;
                    case EVENT_HUDDLE_IN_TERROR:
                        Talk(SAY_HUDDLE);
                        DoCast(SPELL_HUDDLE_IN_TERROR);
                        break;
                    case EVENT_SET_EVADE_TRUE:
                        isDuringP2Transition = false;
                        me->SetVisible(true);
                        me->UpdateObjectVisibility();
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        SetCombatMovement(true);
                        DoZoneInCombat();

                        if (me->getVictim())
                            AttackStart(me->getVictim());

                        DoCast(me, SPELL_PHASE2_ENERGY_REGEN, true);
                        m_canEvade = true;
                        break;
                    default:
                        break;
                    }
                }

                DoMeleeAttackIfReady();
            }
            private:
                bool bowmenStatus[3];
                bool introDone;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sha_of_fearAI(creature);
        }
};



// Pure Light Terrace - 60788
class mob_pure_light_terrace : public CreatureScript
{
    public:
        mob_pure_light_terrace() : CreatureScript("mob_pure_light_terrace") { }

        struct mob_pure_light_terraceAI : public ScriptedAI
        {
            mob_pure_light_terraceAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
                wallActivated = false;
            }

            InstanceScript* pInstance;

            bool wallActivated;
            //float ang1;
            //float ang2;

            void Reset()
            {
                lightTimer = 200;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
                me->CastSpell(me, SPELL_LIGHT_WALL, true);
                me->CastSpell(me, SPELL_LIGHT_WALL_READY, true);

                //Position dst1 = {-989.4236f, -2821.757f, 38.25466f, 0.0f};
                //Position dst2 = {-1045.602f, -2822.323f, 38.25466f, 0.0f};
                //
                //ang1 = src.GetAngle(&dst1) * 57.29578f;
                //ang2 = src.GetAngle(&dst2) * 57.29578f;
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                    case ACTION_ACTIVATE_WALL_OF_LIGHT:
                    {
                        if (wallActivated)
                            break;

                        wallActivated = true;
                        me->CastSpell(me, SPELL_LIGHT_WALL_VISUAL, true);
                        me->RemoveAura(SPELL_LIGHT_WALL_READY);
                        break;
                    }
                    case ACTION_DESACTIVATE_WALL_OF_LIGHT:
                    {
                        if (!wallActivated)
                            break;

                        wallActivated = false;
                        me->CastSpell(me, SPELL_LIGHT_WALL_READY, true);
                        me->RemoveAura(SPELL_LIGHT_WALL_VISUAL);
                        break;
                    }
                    default:
                        break;
                }
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                damage = 0;
                return;
            }

            void UpdateAI(const uint32 diff)
            {
                if (lightTimer >= diff)
                {
                    lightTimer -= diff;
                    return;
                }

                lightTimer = 100;

                Map::PlayerList const& playerList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                {
                    if (Player* player = itr->getSource())
                    {
                        // 60* frontal arc
                        if (lightPos.HasInArc(M_PI / 3, player) && wallActivated)
                        {
                            if (!player->HasAura(SPELL_WALL_OF_LIGHT_BUFF) && !player->HasAura(SPELL_CHAMPION_OF_LIGHT))
                                player->CastSpell(player, SPELL_WALL_OF_LIGHT_BUFF, true);
                        }else
                            player->RemoveAura(SPELL_WALL_OF_LIGHT_BUFF);
                    }
                }
            }

        private:
            uint32 lightTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_pure_light_terraceAI(creature);
        }
};

// Terror Spawn - 61034
class mob_terror_spawn : public CreatureScript
{
    public:
        mob_terror_spawn() : CreatureScript("mob_terror_spawn") { }

        struct mob_terror_spawnAI : public ScriptedAI
        {
            mob_terror_spawnAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                if (pInstance)
                    if (Creature* pureLight = Creature::GetCreature(*me, pInstance->GetData64(NPC_PURE_LIGHT_TERRACE)))
                        me->SetFacingToObject(pureLight);

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISABLE_TURN);

                me->CastSpell(me, SPELL_DARK_BULWARK, true);

                events.Reset();
                events.ScheduleEvent(EVENT_PENETRATING_BOLT, 5000);
            }

            void UpdateAI(const uint32 diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_PENETRATING_BOLT:
                    {
                        me->CastSpell(me, SPELL_PENETRATING_BOLT, false);
                        events.ScheduleEvent(EVENT_PENETRATING_BOLT, 5000);
                        break;
                    }
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_terror_spawnAI(creature);
        }
};

// Champion of Light - 117866
class spell_champion_of_light : public SpellScriptLoader
{
    public:
        spell_champion_of_light() : SpellScriptLoader("spell_champion_of_light") { }

        class spell_champion_of_light_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_champion_of_light_AuraScript);

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->ToCreature() && caster->ToCreature()->AI())
                        caster->ToCreature()->AI()->DoAction(ACTION_ACTIVATE_WALL_OF_LIGHT);
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->ToCreature() && caster->ToCreature()->AI())
                        caster->ToCreature()->AI()->DoAction(ACTION_DESACTIVATE_WALL_OF_LIGHT);
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_champion_of_light_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_SCALE, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_champion_of_light_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_SCALE, AURA_EFFECT_HANDLE_REAL);
            }
        };


        class spell_champion_of_light_Spellcript : public SpellScript
        {
            PrepareSpellScript(spell_champion_of_light_Spellcript);

            void SelectChampion(std::list<WorldObject*>& targets)
            {
                for (auto itr = targets.begin(); itr != targets.end();)
                {
                    Player * target = (*itr)->ToPlayer();
                    // we have a target outside the trigger range
                    if (!target->IsWithinDist2d(&lightPos, 3.0f))
                    {
                        if (target->HasAura(SPELL_CHAMPION_OF_LIGHT))
                            target->RemoveAurasDueToSpell(SPELL_CHAMPION_OF_LIGHT);

                        itr = targets.erase(itr);
                    } else if (target->HasAura(SPELL_CHAMPION_OF_LIGHT))
                    {
                        targets.clear();
                        return;
                    } else
                        ++itr;
                }

                if (!targets.empty())
                    WoWSource::Containers::RandomResizeList(targets, 1);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_champion_of_light_Spellcript::SelectChampion, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_champion_of_light_Spellcript();
        }

        AuraScript* GetAuraScript() const
        {
            return new spell_champion_of_light_AuraScript();
        }
};

// Breath of Fear - 119414 / 125786
class spell_breath_of_fear : public SpellScriptLoader
{
    public:
        spell_breath_of_fear() : SpellScriptLoader("spell_breath_of_fear") { }

        class aura_impl : public AuraScript
        {
            PrepareAuraScript(aura_impl);

            void OnTick(constAuraEffectPtr /*aurEff*/)
            {
                // Breath of Fear only affects targets not in the wall of light
                if (Unit* player = GetUnitOwner())
                    if (!player->IsWithinDist2d(GetCaster(), 60.0f) || player->HasAura(SPELL_WALL_OF_LIGHT_BUFF) || player->HasAura(SPELL_CHAMPION_OF_LIGHT))
                        PreventDefaultAction();
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(aura_impl::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new aura_impl();
        }
};

// Breath of Fear - 125786
class spell_breath_of_fear_fear : public SpellScriptLoader
{
public:
    spell_breath_of_fear_fear() : SpellScriptLoader("spell_breath_of_fear_fear") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void CorrectRange(std::list<WorldObject*>& targets)
        {
            targets.clear();

            // Breath of Fear only affects targets not in the wall of light
            Map::PlayerList const& players = GetCaster()->GetMap()->GetPlayers();
            if (!players.isEmpty())
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    if (Player* player = itr->getSource())
                        if (player->IsWithinDist2d(GetCaster(), 60.0f) && !player->HasAura(SPELL_WALL_OF_LIGHT_BUFF) && !player->HasAura(SPELL_CHAMPION_OF_LIGHT))
                            targets.push_back(player);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::CorrectRange, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::CorrectRange, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::CorrectRange, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };


    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};


// Conjure Terror Spawn - 119108
class spell_conjure_terror_spawn : public SpellScriptLoader
{
    public:
        spell_conjure_terror_spawn() : SpellScriptLoader("spell_conjure_terror_spawn") { }

        class spell_conjure_terror_spawn_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_conjure_terror_spawn_AuraScript);

            void OnTick(constAuraEffectPtr /*aurEff*/)
            {
                if (Creature* caster = GetCaster()->ToCreature())
                    caster->AI()->DoAction(ACTION_SPAWN_TERROR);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_conjure_terror_spawn_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_conjure_terror_spawn_AuraScript();
        }
};

// Penetrating Bolt - 129075
class spell_penetrating_bolt : public SpellScriptLoader
{
    public:
        spell_penetrating_bolt() : SpellScriptLoader("spell_penetrating_bolt") { }

        class spell_penetrating_bolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_penetrating_bolt_SpellScript);

            uint64 targetGuid;

            void CorrectRange(std::list<WorldObject*>& targets)
            {
                targetGuid = 0;

                if (!targets.empty())
                    WoWSource::Containers::RandomResizeList(targets, 1);

                for (auto itr : targets)
                    if (itr->GetGUID())
                        targetGuid = itr->GetGUID();
            }

            void HandleDummy(SpellEffIndex /*index*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (InstanceScript* instance = caster->GetInstanceScript())
                    {
                        if (Player* target = Player::GetPlayer(*caster, targetGuid))
                            caster->CastSpell(target, SPELL_PENETRATING_BOLT_MISSILE, true, NULL, NULL, instance->GetData64(NPC_SHA_OF_FEAR));
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_penetrating_bolt_SpellScript::CorrectRange, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectLaunch += SpellEffectFn(spell_penetrating_bolt_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_penetrating_bolt_SpellScript();
        }
};

// Ominous Cackle - 119593, 119692, 119693
class spell_ominous_caclke_target : public SpellScriptLoader
{
public:
    spell_ominous_caclke_target() : SpellScriptLoader("spell_ominous_caclke_target") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            std::list<WorldObject*> vTanks;
            std::list<WorldObject*> vHealers;

            if (Unit* caster = GetCaster())
            {
                targets.remove_if(notValidTargetPredicate(caster));

                for (auto const pTarget : targets)
                {
                    if (Player* pPlayer = pTarget->ToPlayer())
                    {
                        uint32 m_role = pPlayer->GetRoleForGroup(pPlayer->GetSpecializationId(pPlayer->GetActiveSpec()));

                        if (m_role == ROLES_TANK)
                            vTanks.push_back(pPlayer);
                        else if (m_role == ROLES_HEALER)
                            vHealers.push_back(pPlayer);
                    }
                }

                targets.remove_if(DpsSelectPredicate());

                if (targets.size() > 3)
                    targets.resize(3);

                std::list<WorldObject*>::iterator itr = vTanks.begin();
                std::list<WorldObject*>::iterator itr2 = vHealers.begin();

                if (vTanks.size() > 1)
                    std::advance(itr, urand(0, vTanks.size() - 1));

                if (itr != vTanks.end())
                    targets.insert(targets.begin(), *itr);
                
                if (vHealers.size() > 1)
                    std::advance(itr2, urand(0, vHealers.size() - 1));

                if (itr2 != vHealers.end())
                    targets.insert(targets.begin(), *itr2);

                if (targets.empty())
                {
                    if (caster->ToCreature()->AI())
                        caster->ToCreature()->AI()->DoAction(ACTION_BERSERK);
                }

            }

        }

        void ApplyMorph (SpellEffIndex effIndex)
        {
            Unit * caster = GetCaster();
            Player * victim = GetHitPlayer();

            if (!caster || !victim)
                return;


            victim->CastSpell(victim, 129147, true);
            uint32 path = 0;
            switch (GetSpellInfo()->Id)
            {
                case 119593: path = 1; break;
                case 119693: path = 2; break;
                case 119692: path = 3; break;
                default:
                    return;
            }
            victim->GetMotionMaster()->MovePath(path, false);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnEffectHitTarget += SpellEffectFn(spell_impl::ApplyMorph, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};


// Cheng Kang 61042, Yang Guoshi 61038, Jinlun Kun 61046
class npc_sha_of_fear_bowman : public CreatureScript
{
    enum
    {
        SPELL_DEATH_BLOSSOM             = 119888,
        SPELL_DREAD_SPRAY               = 119862,
        SPELL_SHOOT                     = 119862,
        SPELL_SHA_GLOBE                 = 129178,
        SPELL_DREAD_SPRAY_INIT          = 120047,
        SPELL_FEARLESS_ENABLER          = 131504,
        SPELL_SHA_CORRUPTION            = 120000
    };

    enum
    {
        EVENT_DEATH_BLOSSOM = 1,
        EVENT_DREAD_SPRAY,
        EVENT_DREAD_SPRAY_END,
        EVENT_SHOOT,
    };

    enum
    {
        SAY_AGGRO,
        SAY_DEATH,
        SAY_UNK,
    };

    struct npc_sha_of_fear_bowmanAI : public Scripted_NoMovementAI
    {
        npc_sha_of_fear_bowmanAI(Creature* creature) : Scripted_NoMovementAI(creature) {}

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            // shoot spell max distance
            me->m_ReactDistance = 40.0f;
            me->m_CombatDistance = 40.0f;
            lastHealthPct = 99;
            events.Reset();
        }

        EventMap m_mLowEvents;

        void EnterCombat(Unit* )
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_DEATH_BLOSSOM, 5000);
            events.ScheduleEvent(EVENT_SHOOT, 500);
            events.ScheduleEvent(EVENT_DREAD_SPRAY, 8000);
        }

        void EnterEvadeMode()
        {
        }

        void JustDied(Unit* )
        {
            Talk(SAY_DEATH);

            if (Creature* pTerrace = GetClosestCreatureWithEntry(me, NPC_RETURN_TO_TERRACE, 30.f))
            {
                pTerrace->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                pTerrace->UpdateObjectVisibility();
                pTerrace->AddAura(120216, pTerrace);
            }
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_BOWMAN_AGGRESSIVE)
            {
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->setActive(true);
                
                if (Creature* pTerrace = GetClosestCreatureWithEntry(me, NPC_RETURN_TO_TERRACE, 30.f))
                {
                    pTerrace->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    pTerrace->RemoveAura(120216);
                }
            }
        }

        void HealReceived(Unit* , uint32& heal)
        {
            lastHealthPct = me->GetHealthPct();
        }

        void DamageTaken(Unit* , uint32& damage)
        {
            if (me->HealthBelowPctDamaged(lastHealthPct, damage))
            {
                for (; lastHealthPct > me->GetHealthPct(); --lastHealthPct)
                    DoCast(me, SPELL_SHA_GLOBE, true);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_DREAD_SPRAY:
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    DoCast(me, SPELL_DREAD_SPRAY_INIT, false);
                    events.DelayEvents(9000);
                    events.ScheduleEvent(EVENT_DREAD_SPRAY, 20000);
                    events.ScheduleEvent(EVENT_DREAD_SPRAY_END,  8000);
                    break;
                case EVENT_DREAD_SPRAY_END:
                    me->SetReactState(REACT_AGGRESSIVE);
                    break;
                case EVENT_SHOOT:
                    DoCastVictim(SPELL_SHOOT, false);
                    events.ScheduleEvent(EVENT_SHOOT, 1000);
                    break;
                case EVENT_DEATH_BLOSSOM:
                    if (me->GetHealthPct() < 25.1f)
                    {
                        Talk(SAY_UNK);
                        DoCast(me, SPELL_DEATH_BLOSSOM, false);
                        events.ScheduleEvent(EVENT_DEATH_BLOSSOM, 13000);
                    }
                    else
                        events.ScheduleEvent(EVENT_DEATH_BLOSSOM, 5000);
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    private:
        uint32 lastHealthPct;
    };

public:
    npc_sha_of_fear_bowman() : CreatureScript("npc_sha_of_fear_bowman") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_fear_bowmanAI(creature);
    }
};

// Sha Globe - 65691
class npc_sha_globe : public CreatureScript
{
    enum
    {
        SPELL_SHA_GLOBE_VISUAL      = 129187,
        SPELL_SHA_GLOBE_PRE_VISUAL  = 132214,
        SPELL_SHA_GLOBE_HEAL        = 129190,
        SPELL_SHA_GLOBE_DMG         = 129189
    };

    struct npc_sha_globeAI : public Scripted_NoMovementAI
    {
        npc_sha_globeAI(Creature* creature) : Scripted_NoMovementAI(creature) {}

        void Reset()
        {
            DoCast(me, SPELL_SHA_GLOBE_VISUAL, true);
            visualTimer = 5000;
            healTimer = 8000;
            me->SetReactState(REACT_PASSIVE);
        }

        void EnterCombat(Unit*)
        {
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_SHA_GLOBE_DMG)
            {
                visualTimer = 0;
                healTimer = 0;
                me->RemoveAllAuras();
                me->DespawnOrUnsummon(500);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (visualTimer)
            {
                if (visualTimer <= diff)
                {
                    DoCast(me, SPELL_SHA_GLOBE_PRE_VISUAL);
                    visualTimer = 0;
                } else visualTimer -= diff;

            } else if (healTimer)
            {
                if (healTimer <= diff)
                {
                    DoCast(me, SPELL_SHA_GLOBE_HEAL, true);
                    me->RemoveAllAuras();
                    me->DespawnOrUnsummon(1000);
                    healTimer = 0;
                } else healTimer -= diff;
            }
        }

    private:
        uint32 visualTimer;
        uint32 healTimer;
    };

public:
    npc_sha_globe() : CreatureScript("npc_sha_globe") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_globeAI(creature);
    }
};

class spell_sha_globe_regen : public SpellScriptLoader
{
public:
    spell_sha_globe_regen() : SpellScriptLoader("spell_sha_globe_regen") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void RemoveOldAura()
        {
            if (Unit * victim = GetHitUnit())
                victim->RemoveAurasDueToSpell(129189);
        }

        void Register()
        {
            BeforeHit += SpellHitFn(spell_impl::RemoveOldAura);
        }

        bool prevented;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};

// Dread spray - 119983
class spell_dread_spray_stackable : public SpellScriptLoader
{
    enum
    {
        SPELL_DREAD_AURA        = 119983,
        SPELL_FEAR              = 119985,
    };

    class script_impl : public AuraScript
    {
        PrepareAuraScript(script_impl);

        void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetStackAmount() == 2)
                if (Unit * owner = GetUnitOwner())
                    owner->CastSpell(owner, SPELL_FEAR, true);
        }

        void Register()
        {
            AfterEffectApply += AuraEffectApplyFn(script_impl::OnApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAPPLY);
        }
    };

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (Unit * caster = GetCaster())
                targets.remove_if([&caster](WorldObject * u) { return u->ToUnit()->HasAura(SPELL_FEAR) || !u->IsWithinLOSInMap(caster) || !caster->HasInLine(u, 35.0f); });
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

public:
    spell_dread_spray_stackable() : SpellScriptLoader("spell_dread_spray_stackable") {}

    AuraScript * GetAuraScript() const
    {
        return new script_impl;
    }

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};

// Death Blossom - 119887
class spell_death_blossom : public SpellScriptLoader
{
    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (Unit * caster = GetCaster())
                targets.remove_if([&caster](WorldObject * u) { return !u->IsWithinLOSInMap(caster); });
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

public:
    spell_death_blossom() : SpellScriptLoader("spell_death_blossom") {}

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};

 // Dread Spray - 120047
class spell_dread_spray : public SpellScriptLoader
{
public:
    spell_dread_spray() : SpellScriptLoader("spell_dread_spray") {}

    class aura_impl : public AuraScript
    {
        PrepareAuraScript(aura_impl);

        void OnTick(constAuraEffectPtr /*aurEff*/)
        {
            if (Unit * owner = GetUnitOwner())
            {
                owner->SetTarget(0);
                float ori = float(rand_norm())*static_cast<float>(2 * M_PI);
                owner->SetOrientation(ori);
                owner->SetFacingTo(ori);
                //Position pos;
                //owner->GetRandomNearPosition(pos, 15.0f);
                owner->CastSpell(owner, 119958, true);
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(aura_impl::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new aura_impl();
    }
};

class notPlayerPredicate
{
public:
    bool operator()(WorldObject* target) const
    {
        return target && !target->ToPlayer();
    }
};

class spell_teleport_to_dread_expanse : public SpellScriptLoader
{
public:
    spell_teleport_to_dread_expanse() : SpellScriptLoader("spell_dread_expanse_tp") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void HandleOnHit()
        {
            if (Unit* pUnit = GetHitUnit())
            {
                pUnit->CastSpell(pUnit, SPELL_TELEPORT_PLAYER, true);

                if (pUnit->isAlive())
                    pUnit->CastSpell(pUnit, SPELL_FADING_LIGHT, true);

                pUnit->CastSpell(pUnit, SPELL_DREAD_EXPANSE, true);
            }
        }

        void SelectTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(notPlayerPredicate());
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_impl::HandleOnHit);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::SelectTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};

class spell_implacable_strike : public SpellScriptLoader
{
public:
    spell_implacable_strike() : SpellScriptLoader("spell_implacable_strike") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void HandleAfterCast()
        {
            if (Unit* caster = GetCaster())
            {
                caster->ClearUnitState(UNIT_STATE_CANNOT_TURN);
                caster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }
        }
    };
};

void AddSC_boss_sha_of_fear()
{
    new boss_sha_of_fear();
    new mob_pure_light_terrace();
    new mob_terror_spawn();
    new spell_champion_of_light();
    new spell_breath_of_fear();
    new spell_conjure_terror_spawn();
    new spell_penetrating_bolt();
    new spell_ominous_caclke_target();
    new npc_sha_of_fear_bowman();
    new spell_dread_spray_stackable();
    new spell_dread_spray();
    new npc_sha_globe();
    new spell_sha_globe_regen();
    new spell_breath_of_fear_fear();
    new spell_death_blossom();
    new spell_teleport_to_dread_expanse();
}
