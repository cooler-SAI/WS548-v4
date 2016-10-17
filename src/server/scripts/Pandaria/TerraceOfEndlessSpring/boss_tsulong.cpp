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
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Spell.h"

enum eTsulongEvents
{
    EVENT_NONE,
    EVENT_FLY,
    EVENT_WAYPOINT_FIRST,
    EVENT_WAYPOINT_SECOND,
    EVENT_SWITCH_TO_NIGHT_PHASE,
    EVENT_BERSERK,
    // night phase
    EVENT_SPAWN_SUNBEAM,
    EVENT_SHADOW_BREATH,
    EVENT_DARK_OF_NIGHT,
    EVENT_NIGHTMARES,
    // day phase
    EVENT_SUMMON_TERROR,
    EVENT_SUMMON_SHA,
    EVENT_SUN_BREATH,
    EVENT_EVADE_CHECK,
};

enum eTsulongSpells
{
    SPELL_DREAD_SHADOWS                 = 122767,
    SPELL_DREAD_SHADOWS_DEBUFF          = 122768,
    SPELL_SUNBEAM_DUMMY                 = 122782,
    SPELL_SUNBEAM_PROTECTION            = 122789,
    SPELL_NIGHT_PHASE_EFFECT            = 122841,
    SPELL_SHADOW_BREATH                 = 122752,
    SPELL_NIGHTMARES                    = 122775,
    SPELL_NIGHTMARES_EFF                = 122770,
    SPELL_SHA_ACTIVE                    = 122438, // night phase regen

    // day phase
    SPELL_GOLD_ACTIVE                   = 122453, // day phase regen and model
    SPELL_SUN_BREATH                    = 122855,
    SPELL_SUMMON_EMBODIED_TERROR        = 122995, // 62969
    SPELL_SUMMON_SHA_PERIODIC           = 122946,
    SPELL_SUMMON_SHA_PERIODIC_EFF       = 122952,
    SPELL_SUMMON_SHA_MISSILE            = 122953, // 62919
    SPELL_UNLEASHED_SHA_EXPLOSION       = 130008,

    // Heroic mode
    SPELL_THE_DARK_OF_NIGHT             = 123739,
    SPELL_DARK_FIXATE_AURA              = 123740,
    SPELL_DARK_EXPLOSION                = 123794,

    SPELL_LIGHT_OF_DAY_BUFF             = 123716,
    SPELL_LIGHT_OF_DAY_AURA             = 123816

};

enum eTsulongTimers
{
    TIMER_FIRST_WAYPOINT = 5000, // 5 secs for test, live : 120 000
};

enum eTsulongPhase
{
    PHASE_NONE,
    PHASE_FLY,
    PHASE_DAY,
    PHASE_NIGHT
};

enum eTsulongWaypoints
{
    WAYPOINT_FIRST = 10001,
    WAYPOINT_SECOND = 10002
};

enum eTsulongDisplay
{
    DISPLAY_TSULON_NIGHT = 42532,
    DISPLAY_TSULON_DAY   = 42533,

    FACTION_DAY          = 2104,
    FACTION_NIGHT        = 1965,
};

enum eTsulongActions
{
    ACTION_SPAWN_SUNBEAM = 10,
    ACTION_REGEN_COMPLETE,
};

enum eTsulongCreatures
{
    SUNBEAM_DUMMY_ENTRY    = 62849,
};

enum
{
    SPELL_TERRORIZE                 = 123011,
    SPELL_TERRORIZE_TSULONG         = 123012,
    SPELL_TINY_TERROR               = 123038,
    SPELL_TINY_TERROR_EFF           = 123026,
    SPELL_SUMMON_TINY_TERROR        = 123508
};

class boss_tsulong : public CreatureScript
{
    enum Quotes
    {
        EMOTE_SUMMON_TERROR         = 1,
        EMOTE_TERRORIZE,
        SAY_INTRO,
        SAY_AGGRO,
        SAY_NIGHTMARES,
        SAY_NIGHT_TO_DAY,
        SAY_DAY_TO_NIGHT,
        SAY_SLAY_DAY,
        SAY_SLAY_NIGHT,
        SAY_DEATH,
        SAY_WIPE,
        EMOTE_SUNBEAM,
    };
    public:
        boss_tsulong() : CreatureScript("boss_tsulong") { }

        struct boss_tsulongAI : public BossAI
        {
            boss_tsulongAI(Creature* creature) : BossAI(creature, DATA_TSULONG) { hasBeenDefeated = false; }

            EventMap m_oocEvents;

            uint8 phase;
            bool firstSpecialEnabled;
            bool secondSpecialEnabled;
            bool inFly;
            bool hasBeenDefeated;

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                switch (spell->Id)
                {
                    case SPELL_NIGHTMARES:
                        DoCast(target, SPELL_NIGHTMARES_EFF, true);
                        break;
                    case SPELL_SUMMON_SHA_PERIODIC_EFF:
                        DoCast(target, SPELL_SUMMON_SHA_MISSILE, true);
                        break;
                }
            }

            void SpellHit(Unit* , SpellInfo const* spell)
            {
                if (spell->Id == SPELL_TERRORIZE_TSULONG)
                    Talk(EMOTE_TERRORIZE);
            }

            void JustSummoned(Creature* pSummoned)
            {
                summons.Summon(pSummoned);

                if (pSummoned->GetEntry() == NPC_EMBODIED_TERROR)
                {
                    if (pSummoned->AI())
                        pSummoned->AI()->DoZoneInCombat();
                }
            }

            void Reset()
            {
                m_oocEvents.RescheduleEvent(1, 2000);

                dayPhaseOri = 0.0f;
                _Reset();
                events.Reset();
                summons.DespawnAll();

                inFly = false;

                me->SetDisableGravity(true);
                me->SetCanFly(true);
                me->RemoveAurasDueToSpell(SPELL_DREAD_SHADOWS);
                me->SetPower(POWER_ENERGY, 0);
                
                if (hasBeenDefeated)
                    return;

                
                if (instance)
                {
                    if (instance->GetData(TYPE_PROTECTORS) == DONE)
                    {
                        phase = PHASE_NIGHT;
                        me->SetDisplayId(DISPLAY_TSULON_NIGHT);
                        me->setFaction(14);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                        me->SetHomePosition(-1017.841f, -3049.621f, 12.823f, 4.72f);
                        me->GetMotionMaster()->MoveTargetedHome();
                    }
                    else
                    {
                        phase = PHASE_DAY;
                        me->SetDisplayId(DISPLAY_TSULON_DAY);
                        me->setFaction(35);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 35);
                    }
                }
            }

            void JustReachedHome()
            {
                _JustReachedHome();

                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->SetBossState(DATA_TSULONG, FAIL);

                summons.DespawnAll();
            }

            void MoveInLineOfSight(Unit* pWho)
            {
                CreatureAI::MoveInLineOfSight(pWho);
            }

            void EnterCombat(Unit* pWho)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                instance->SetBossState(DATA_TSULONG, IN_PROGRESS);
                DoZoneInCombat();

                Talk(SAY_AGGRO);
                me->SetPower(POWER_ENERGY, 0);
                me->SetHealth(me->GetMaxHealth());
                SetPhase(PHASE_NIGHT);

                events.ScheduleEvent(EVENT_BERSERK, 8 * MINUTE*IN_MILLISECONDS + 10000);
            }

            void KilledUnit(Unit* who)
            {
                if (who->GetTypeId() == TYPEID_PLAYER)
                    Talk((phase == PHASE_DAY) ? SAY_SLAY_DAY : SAY_SLAY_NIGHT);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (me->IsInEvadeMode())
                    damage = 0;
                else
                if (phase == PHASE_DAY)
                {
                    if (me->GetHealth() <= damage)
                    {
                        DoCast(me, SPELL_UNLEASHED_SHA_EXPLOSION, true);
                        damage = 0;
                        summons.DespawnAll();
                        EnterEvadeMode();
                    }
                }
                else if (phase == PHASE_NIGHT)
                {
                    if (me->GetHealthPct() < 1.1f || me->GetHealth() <= damage)
                    {
                        damage = 0;
                        HandleDefeat(attacker);
                    }
                }
            }

            void HealReceived(Unit* healer, uint32& heal)
            {
                // victory if healed during the day phase
                if (me->HealthAbovePctHealed(99, heal))
                    if (phase == PHASE_DAY)
                    {
                        HandleDefeat(healer);
                    }
            }

            void JustDied(Unit* /*killer*/)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                _JustDied();
            }

            void DoAction(const int32 action)
            {
                if (action == ACTION_START_TSULONG_WAYPOINT)
                {
                    SetPhase(PHASE_FLY);

                    Talk(SAY_INTRO);
                }

                if (action == ACTION_SPAWN_SUNBEAM)
                    events.ScheduleEvent(EVENT_SPAWN_SUNBEAM, 0, 0, PHASE_NIGHT);

                if (action == ACTION_REGEN_COMPLETE)
                {
                    me->SetPower(POWER_ENERGY, 0);
                    SetPhase((phase == PHASE_DAY) ? PHASE_NIGHT : PHASE_DAY);
                }
            }

            void SetPhase(uint8 newPhase)
            {
                uint8 oldPhase = phase;
                phase = newPhase;
                events.SetPhase(newPhase);
                switch (newPhase)
                {
                    case PHASE_FLY:
                        me->RemoveAurasDueToSpell(SPELL_GOLD_ACTIVE);
                        me->RemoveAurasDueToSpell(SPELL_SHA_ACTIVE);
                        events.ScheduleEvent(EVENT_FLY, 5000, 0, PHASE_FLY);
                        break;
                    case PHASE_DAY:
                    {
                        Unit * victim = me->getVictim();
                        me->SetReactState(REACT_PASSIVE);
                        me->setFaction(FACTION_DAY);
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveIdle();
                        me->AttackStop();
                        me->StopMoving();
                        me->SetTarget(0);
                        
                        dayPhaseOri = victim ? me->GetAngle(victim) : 0.0f;
                        me->SetFacingTo(dayPhaseOri);
                        me->RemoveAurasDueToSpell(SPELL_SHA_ACTIVE);
                        me->RemoveAurasDueToSpell(SPELL_DREAD_SHADOWS);

                        DoCast(me, SPELL_GOLD_ACTIVE, true);
                        DoCast(me, SPELL_SUMMON_SHA_PERIODIC, true);
                        
                        if (IsHeroic())
                            DoCast(me, SPELL_LIGHT_OF_DAY_AURA, true);

                        events.Reset();
                        events.ScheduleEvent(EVENT_SUN_BREATH, 6000, 0, PHASE_DAY);
                        events.ScheduleEvent(EVENT_SUMMON_TERROR, 12000, 0, PHASE_DAY);
                        events.ScheduleEvent(EVENT_EVADE_CHECK, 2000, 0, PHASE_DAY);

                        if (oldPhase == PHASE_NIGHT)
                        {
                            Talk(SAY_NIGHT_TO_DAY);
                            me->SetHealth(me->GetMaxHealth() - me->GetHealth());
                        }
                    }
                        break;
                    case PHASE_NIGHT:
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveAurasDueToSpell(SPELL_GOLD_ACTIVE);
                        me->RemoveAurasDueToSpell(SPELL_SUMMON_SHA_PERIODIC);
                        DoCast(me, SPELL_SHA_ACTIVE, true);

                        me->setFaction(FACTION_NIGHT);
                        events.Reset();
                        events.ScheduleEvent(EVENT_SWITCH_TO_NIGHT_PHASE, 0, 0, PHASE_NIGHT);
                        events.ScheduleEvent(EVENT_SHADOW_BREATH, 10000, 0, PHASE_NIGHT);
                        events.ScheduleEvent(EVENT_NIGHTMARES, urand(15000, 16000), 0, PHASE_NIGHT);
                        events.ScheduleEvent(EVENT_SPAWN_SUNBEAM, 2000, 0, PHASE_NIGHT);

                        if (IsHeroic())
                        {
                            events.ScheduleEvent(EVENT_DARK_OF_NIGHT, 10000, 0, PHASE_NIGHT);
                            me->RemoveAurasDueToSpell(SPELL_LIGHT_OF_DAY_AURA);
                        }

                        if (oldPhase == PHASE_DAY)
                        {
                            Talk(SAY_DAY_TO_NIGHT);
                            me->SetHealth(me->GetMaxHealth() - me->GetHealth());
                        }
                        break;
                }

            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (id)
                {
                    case WAYPOINT_FIRST:
                        events.ScheduleEvent(EVENT_WAYPOINT_FIRST, 0, 0, PHASE_FLY);
                        break;
                    case WAYPOINT_SECOND:
                        events.ScheduleEvent(EVENT_WAYPOINT_SECOND, 0, 0, PHASE_FLY);
                        break;
                    default:
                        break;
                }
            }

            void HandleDefeat(Unit* attacker)
            {
                if (hasBeenDefeated)
                    return;

                hasBeenDefeated = true;

                EnterEvadeMode();

                Talk(SAY_DEATH);
                
                me->SetReactState(REACT_PASSIVE);
                me->setFaction(FACTION_DAY);
                me->RemoveAllAuras();

                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MoveIdle();

                events.Reset();

                me->setRegeneratingHealth(false);
                me->setRegeneratingMana(false);

                DoCast(me, SPELL_GOLD_ACTIVE, true);
                me->SetDisplayId(DISPLAY_TSULON_DAY);

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_PACIFIED);

                if (attacker && attacker->GetTypeId() == TYPEID_PLAYER)
                {
                    //attacker->ToPlayer()->SetLastKilledCreature(me);
                    me->GetMap()->ToInstanceMap()->PermBindAllPlayers(attacker->ToPlayer());
                }
                else if (attacker && attacker->GetTypeId() == TYPEID_UNIT && attacker->GetOwner() && attacker->GetOwner()->ToPlayer())
                {
                    //attacker->GetOwner()->ToPlayer()->SetLastKilledCreature(me);
                    me->GetMap()->ToInstanceMap()->PermBindAllPlayers(attacker->GetOwner()->ToPlayer());
                }

                instance->SetBossState(DATA_TSULONG, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                events.Reset();

                summons.DespawnAll();

                Map::PlayerList const& lPlayers = me->GetMap()->GetPlayers();

                for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                {
                    if (auto const pPlayer = itr->getSource())
                    {
                        pPlayer->CombatStop();
                    }
                }
            }

            void HandleEvadeDuringDay()
            {
                if (me->IsInEvadeMode())
                    return;

                Map::PlayerList const& lPlayers = me->GetMap()->GetPlayers();

                for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                {
                    if (Player* pPlayer = itr->getSource())
                    {
                        if (pPlayer->isAlive() && pPlayer->IsWithinDist(me, 40.f))
                            return;
                    }
                }

                summons.DespawnAll();

                EnterEvadeMode();
            }

            void EnterEvadeMode()
            {
                CreatureAI::EnterEvadeMode();

                me->GetMotionMaster()->Clear();

                if (!hasBeenDefeated)
                {
                    instance->SetData(TYPE_TSULONG, FAIL);
                    me->GetMotionMaster()->MoveTargetedHome();
                    Talk(SAY_WIPE);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!me->IsInEvadeMode() && !me->isInCombat() && !inFly)
                {
                    m_oocEvents.Update(diff);

                    while (uint32 eventId = m_oocEvents.ExecuteEvent())
                    {
                        if (eventId == 1)
                        {
                            summons.DespawnAll();
                            m_oocEvents.ScheduleEvent(1, 1500);

                        }
                    }
                }

                events.Update(diff);

                {
                    if (phase == PHASE_FLY)
                    {
                        switch (events.ExecuteEvent())
                        {
                        case EVENT_FLY:
                            me->setActive(true);
                            me->UpdateObjectVisibility(true);
                            me->setFaction(FACTION_NIGHT);
                            me->SetReactState(REACT_PASSIVE);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                            me->SetDisplayId(DISPLAY_TSULON_NIGHT);
                            me->GetMotionMaster()->MovePoint(WAYPOINT_FIRST, -1018.10f, -2947.431f, 50.12f);
                            inFly = true;
                            break;
                        case EVENT_WAYPOINT_FIRST:
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MovePoint(WAYPOINT_SECOND, -1017.841f, -3049.621f, 12.823f);
                            break;
                        case EVENT_WAYPOINT_SECOND:
                            me->SetHomePosition(-1017.841f, -3049.621f, 12.823f, 4.72f);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                            inFly = false;
                            events.SetPhase(PHASE_NONE);
                            phase = PHASE_NONE;
                            break;
                        default:
                            break;
                        }
                    }
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (phase == PHASE_NIGHT)
                {
                    if (!UpdateVictim())
                        return;

                    switch (events.ExecuteEvent())
                    {
                        case EVENT_BERSERK:
                            DoCast(me, SPELL_BERSERK, true);
                            break;
                        case EVENT_SWITCH_TO_NIGHT_PHASE:
                            //me->SetDisplayId(DISPLAY_TSULON_NIGHT);
                            me->setFaction(FACTION_NIGHT);
                            me->CastSpell(me, SPELL_DREAD_SHADOWS, true);
                            break;
                        case EVENT_SPAWN_SUNBEAM:
                            if (Creature* pSunbeam = GetClosestCreatureWithEntry(me, SUNBEAM_DUMMY_ENTRY, 200.0f))
                                pSunbeam->DisappearAndDie();

                            Talk(EMOTE_SUNBEAM, me->GetGUID());
                            Position pos;
                            me->GetRandomNearPosition(pos, 30.0f);
                            me->SummonCreature(SUNBEAM_DUMMY_ENTRY, pos);
                            events.ScheduleEvent(EVENT_SPAWN_SUNBEAM, 40000, 0, PHASE_NIGHT);
                            break;
                        case EVENT_NIGHTMARES:
                            Talk(SAY_NIGHTMARES);
                            DoCast(me, SPELL_NIGHTMARES, true);
                            events.ScheduleEvent(EVENT_NIGHTMARES, 15000, 0, PHASE_NIGHT);
                            break;
                        case EVENT_SHADOW_BREATH:
                            DoCastVictim(SPELL_SHADOW_BREATH, false);
                            events.ScheduleEvent(EVENT_SHADOW_BREATH, urand(25000, 30000), 0, PHASE_NIGHT);
                            break;
                        default:
                            break;
                    }

                    DoMeleeAttackIfReady();
                }
                else if (phase == PHASE_DAY)
                {
                    switch (events.ExecuteEvent())
                    {
                        case EVENT_SUMMON_TERROR:
                            Talk(EMOTE_SUMMON_TERROR, me->GetGUID());
                            DoCast(me, SPELL_SUMMON_EMBODIED_TERROR, true);
                            events.ScheduleEvent(EVENT_SUMMON_TERROR, 30000, 0, PHASE_DAY);
                            break;
                        case EVENT_SUN_BREATH:
                            me->SetFacingTo(dayPhaseOri);
                            me->SetOrientation(dayPhaseOri);
                            DoCast(me, SPELL_SUN_BREATH, false);
                            events.ScheduleEvent(EVENT_SUN_BREATH, 29000, 0, PHASE_DAY);
                            break;
                        case EVENT_SUMMON_SHA:
                            //events.ScheduleEvent(EVENT_SUN_BREATH, 30000, 0, PHASE_DAY);
                            break;
                        case EVENT_EVADE_CHECK:
                            HandleEvadeDuringDay();
                            events.ScheduleEvent(EVENT_EVADE_CHECK, 2000, 0, PHASE_DAY);
                            break;
                    }
                }

            }
            private:
                float dayPhaseOri;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_tsulongAI(creature);
        }
};

class npc_sunbeam : public CreatureScript
{
    public:
        npc_sunbeam() : CreatureScript("npc_sunbeam") { }

        struct npc_sunbeamAI : public CreatureAI
        {
            InstanceScript* pInstance;

            npc_sunbeamAI(Creature* creature) : CreatureAI(creature)
            {
                pInstance = creature->GetInstanceScript();
                creature->SetObjectScale(5.0f);
                creature->SetReactState(REACT_PASSIVE);
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
                creature->CastSpell(creature, SPELL_SUNBEAM_DUMMY, true);
            }

            void UpdateAI(uint32 const /*diff*/)
            {
                float scale = me->GetFloatValue(OBJECT_FIELD_SCALE_X);
                if (scale <= 1.0f)
                {
                    /*
                    if (pInstance)
                    {
                        if (Creature* tsulong = pInstance->instance->GetCreature(pInstance->GetData64(NPC_TSULONG)))
                            tsulong->AI()->DoAction(ACTION_SPAWN_SUNBEAM);
                    }*/

                    Talk(0);
                    me->DespawnOrUnsummon();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sunbeamAI(creature);
        }
};

// Embodied Terror - 62969
class npc_embodied_terror : public CreatureScript
{
    struct npc_embodied_terrorAI : public CreatureAI
    {
        npc_embodied_terrorAI(Creature* creature) : CreatureAI(creature) {}

        void Reset()
        {
            pInstance = me->GetInstanceScript();

            if (pInstance)
            {
                if (pInstance->GetData(TYPE_TSULONG) != DONE)
                    terrorizeTimer = urand(3000, 6000);
            }

            died = false;
        }

        InstanceScript* pInstance;

        void DamageTaken(Unit* , uint32& damage)
        {
            if (died)
            {
                damage = 0;
                return;
            }

            if (me->GetHealth() <= damage)
            {
                died = true;
                me->DespawnOrUnsummon(5000);
                DoCast(me, SPELL_TINY_TERROR, true);
            }
        }

        void SpellHit(Unit* , SpellInfo const* spell)
        {
            if (spell->Id == SPELL_TINY_TERROR_EFF)
                for (int i = 0; i < 5; ++i)
                    DoCast(me, SPELL_SUMMON_TINY_TERROR, true);
        }

        void UpdateAI(uint32 const diff)
        {
            if (!UpdateVictim() || !pInstance)
                return;

            if (terrorizeTimer)
            {
                if (terrorizeTimer <= diff)
                {
                    DoCast(me, SPELL_TERRORIZE, true);
                    DoCast(me, SPELL_TERRORIZE_TSULONG, true);
                    terrorizeTimer = 0;
                } 
                else
                if (pInstance->GetData(TYPE_TSULONG) < FAIL)
                    terrorizeTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }

    private:
        uint32 terrorizeTimer;
        bool died;
    };

public:
    npc_embodied_terror() : CreatureScript("npc_embodied_terror") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_embodied_terrorAI(creature);
    }
};

// Terrorize - 123018
class spell_terrorize_periodic_player : public SpellScriptLoader
{
    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit * target = GetHitUnit())
                SetHitDamage(target->CountPctFromCurHealth(GetHitDamage()));
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_impl::HandleDummy, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

public:
    spell_terrorize_periodic_player() : SpellScriptLoader("spell_terrorize_periodic_player") {}

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};

 // Fright Spawn - 62977
class npc_fright_spawn : public CreatureScript
{
    enum
    {
        SPELL_FRIGHT        = 123036
    };

    struct npc_fright_spawnAI : public CreatureAI
    {
        npc_fright_spawnAI(Creature* creature) : CreatureAI(creature) {}

        void Reset()
        {
            frightTimer = urand(3000, 5000);
        }

        void UpdateAI(uint32 const diff)
        {
            if (!UpdateVictim())
                return;

            if (frightTimer <= diff)
            {
                DoCast(me, SPELL_FRIGHT, false);
                frightTimer = urand(8000, 10000);
            } else frightTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        uint32 frightTimer;
    };

public:
    npc_fright_spawn() : CreatureScript("npc_fright_spawn") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_fright_spawnAI(creature);
    }
};

// Unstable Sha - 62962
class npc_unstable_sha : public CreatureScript
{
    enum
    {
        SPELL_INSTABILITY_RIDE          = 122928,
        SPELL_INSTABILITY_TRANSFORM     = 122930,
        SPELL_INSTABILITY_DAMAGE        = 123697,
        SPELL_INSTABILITY_HEAL          = 130078,
        SPELL_UNSTABLE_BOLT             = 122881
    };

    struct npc_unstable_shaAI : public ScriptedAI
    {
        npc_unstable_shaAI(Creature* creature) : ScriptedAI(creature)
        {
            summonerGUID = 0;
            riding = false;
            cast = false;
        }

        void Reset()
        {
            events.ScheduleEvent(1, urand(2000, 6000));
        }

        void IsSummonedBy(Unit* summoner)
        {
            //me->GetMotionMaster()->MoveChase(summoner);
            //me->SetReactState(REACT_PASSIVE);
            me->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
            DoZoneInCombat();
            SetGazeOn(summoner);
            summonerGUID = summoner->GetGUID();
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (!cast && target == me && spell->Id == SPELL_INSTABILITY_TRANSFORM)
                if (Unit * summoner = Unit::GetUnit(*me, summonerGUID))
                {
                    DoCast(summoner, (summoner->getFaction() == FACTION_DAY) ? SPELL_INSTABILITY_DAMAGE : SPELL_INSTABILITY_HEAL);
                    cast = true;
                    me->DespawnOrUnsummon(500);
                }
        }

        void UpdateAI(uint32 const uiDiff)
        {
            if (!UpdateVictimWithGaze() || riding)
                return;

            if (Unit * summoner = Unit::GetUnit(*me, summonerGUID))
            {
                if (me->IsWithinMeleeRange(summoner))
                {
                    riding = true;
                    DoCast(summoner, SPELL_INSTABILITY_RIDE);
                }
            }

            events.Update(uiDiff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case 1:
                    if (Unit* pUnit = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, 0))
                    {
                        DoCast(pUnit, SPELL_UNSTABLE_BOLT);
                        events.ScheduleEvent(1, 4000);
                    }
                    break;
                }
            }
        }

        bool riding;
        bool cast;
        uint64 summonerGUID;
    };

public:
    npc_unstable_sha() : CreatureScript("npc_unstable_sha") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_unstable_shaAI(creature);
    }
};

// 125843, jam spell ?
class spell_dread_shadows_damage : public SpellScriptLoader
{
    public:
        spell_dread_shadows_damage() : SpellScriptLoader("spell_dread_shadows_damage") { }

        class spell_dread_shadows_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dread_shadows_damage_SpellScript);

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                targets.clear();
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dread_shadows_damage_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dread_shadows_damage_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_dread_shadows_damage_SpellScript();
        }
};

class DreadShadowsTargetCheck
{
    public:
        bool operator()(WorldObject* object) const
        {
            // check Sunbeam protection
            if (object->ToUnit() && object->ToUnit()->HasAura(122789))
                return true;

            return false;
        }
};

// 122768
class spell_dread_shadows_malus : public SpellScriptLoader
{
    public:
        spell_dread_shadows_malus() : SpellScriptLoader("spell_dread_shadows_malus") { }

        class spell_dread_shadows_malus_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dread_shadows_malus_SpellScript);

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                targets.remove(GetCaster());
                targets.remove_if(DreadShadowsTargetCheck());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dread_shadows_malus_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dread_shadows_malus_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_dread_shadows_malus_SpellScript();
        }
};

// 122789
class spell_sunbeam : public SpellScriptLoader
{
    public:
        spell_sunbeam() : SpellScriptLoader("spell_sunbeam") { }

        class spell_sunbeam_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sunbeam_SpellScript);

            void CheckTargets(std::list<WorldObject*>& targets)
            {
                targets.clear();
                Map::PlayerList const& players = GetCaster()->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* plr = itr->getSource();
                    if (!plr)
                        continue;

                    float scale = GetCaster()->GetFloatValue(OBJECT_FIELD_SCALE_X);
                    if (plr->GetExactDist2d(GetCaster()) <= scale)
                        targets.push_back(plr);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sunbeam_SpellScript::CheckTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sunbeam_SpellScript();
        }


        class spell_sunbeam_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sunbeam_aura_AuraScript);

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
                {
                    if (Pet* pet = GetTarget()->ToPlayer()->GetPet())
                        pet->AddAura(SPELL_SUNBEAM_PROTECTION, pet);

                    float scale = GetCaster()->GetFloatValue(OBJECT_FIELD_SCALE_X);
                    if (scale > 0.2f)
                        GetCaster()->SetObjectScale(scale - 0.2f);
                }

                GetTarget()->RemoveAurasDueToSpell(SPELL_DREAD_SHADOWS_DEBUFF);
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
                {
                    if (Pet* pet = GetTarget()->ToPlayer()->GetPet())
                        pet->RemoveAurasDueToSpell(SPELL_SUNBEAM_PROTECTION);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_sunbeam_aura_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_sunbeam_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sunbeam_aura_AuraScript();
        }
};

// Day and Night regen auras - 122438 and 122453
class spell_tsulong_sha_regen : public SpellScriptLoader
{
public:
    spell_tsulong_sha_regen() : SpellScriptLoader("spell_tsulong_sha_regen") {}

    class script_impl : public AuraScript
    {
        PrepareAuraScript(script_impl);

        void HandleEffectPeriodic(constAuraEffectPtr aurEff)
        {
            Unit * target = GetUnitOwner();
            if (!target)
                return;

            if (target->GetPower(POWER_ENERGY) == 99)
            {
                target->ToCreature()->AI()->DoAction(ACTION_REGEN_COMPLETE);
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(script_impl::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_ENERGIZE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new script_impl();
    }
};

class spell_tsulong_nightmares : public SpellScriptLoader
{
public:
    spell_tsulong_nightmares() : SpellScriptLoader("spell_tsulong_nightmares") {}

    class script_impl : public SpellScript
    {
        PrepareSpellScript(script_impl);

        bool Validate()
        {
            return true;
        }

        void SelectTargets(std::list<WorldObject*>&targets)
        {
            if (Unit* caster = GetCaster())
            {
                // Melee targets should be low priority
                targets.sort(WoWSource::ObjectDistanceOrderPred(caster, false));

                uint32 m_maxTargets = (caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC || caster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC) ? 3 : 1;

                if (targets.size() > m_maxTargets)
                {
                    targets.resize(m_maxTargets);
                }
            }
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(script_impl::SelectTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new script_impl();
    }
};

class spell_dark_of_night_fixate : public SpellScriptLoader
{
public:
    spell_dark_of_night_fixate() : SpellScriptLoader("spell_dark_of_night_fixate") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void SelectTargets(std::list<WorldObject*>&targets)
        {
            if (targets.size() > 1)
            {
                if (Unit* caster = GetCaster())
                    targets.sort(WoWSource::ObjectDistanceOrderPred(caster));

                targets.resize(1);
            }
        }

        void HandleOnHit()
        {
            Unit* pBeam = GetHitUnit();
            Unit* pCaster = GetCaster();

            if (!pBeam || !pCaster)
                return;

            pCaster->GetMotionMaster()->MoveFollow(pBeam, 0.f, 0.f);

            if (pCaster->GetExactDist2d(pBeam) < pBeam->GetFloatValue(OBJECT_FIELD_SCALE_X))
            {
                pCaster->CastSpell(pBeam, SPELL_DARK_EXPLOSION, true);
                pCaster->Kill(pCaster);
            }
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::SelectTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnHit += SpellHitFn(spell_impl::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};

class spell_tsulong_terrorize : public SpellScriptLoader
{
public:
    spell_tsulong_terrorize() : SpellScriptLoader("spell_tsulong_terrorize") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void SelectTargets(std::list<WorldObject*>&targets)
        {
            if (Unit* pCaster = GetCaster())
            {
                uint32 m_maxTargets = (pCaster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC || pCaster->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC) ? 5 : 2;
  
                if (targets.size() > m_maxTargets)
                {
                    targets.resize(m_maxTargets);
                }
            }
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_impl::SelectTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_impl();
    }
};

class npc_dark_of_night : public CreatureScript
{
public:
    npc_dark_of_night() : CreatureScript("npc_dark_of_night") {}

    struct npc_dark_of_nightAI : public ScriptedAI
    {
        npc_dark_of_nightAI(Creature* pCreature) : ScriptedAI(pCreature) 
        { 
            me->SetReactState(REACT_PASSIVE);
            me->AddAura(SPELL_DARK_FIXATE_AURA, me);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_dark_of_nightAI(pCreature);
    }
};

void AddSC_boss_tsulong()
{
    new boss_tsulong();
    new npc_sunbeam();
    new spell_dread_shadows_damage();
    new spell_dread_shadows_malus();
    new spell_sunbeam();
    new spell_tsulong_sha_regen();
    new spell_terrorize_periodic_player();
    new spell_tsulong_terrorize();
    new spell_tsulong_nightmares();
    new spell_dark_of_night_fixate();
    new npc_embodied_terror();
    new npc_fright_spawn();
    new npc_unstable_sha();
    new npc_dark_of_night();
}
