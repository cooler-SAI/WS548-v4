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
#include "GridNotifiers.h"

static const Position aLeiWps[] =
{
    { -1017.7f, -2864.0f, 30.4f, 4.71f },
    { -1017.8f, -2771.9f, 38.6f, 4.71f },
};

enum eLeiShiSpells
{
    SPELL_AFRAID            = 123181,
    SPELL_SPRAY             = 123121,
    SPELL_HIDE              = 123244,
    SPELL_HIDE_SUMMON       = 123213,
    SPELL_HIDE_STACKS       = 123233,
    SPELL_GET_AWAY          = 123461,
    SPELL_GET_AWAY_TELEPORT = 123441,
    SPELL_PROTECT           = 123250,
    SPELL_PROTECT_RESPAWN   = 123493,
    SPELL_PROTECT_VISUAL    = 123505,
    SPELL_CLOUDED_REFLECTION= 123620,
    SPELL_CLOUDED_IMPACT    = 123625,

    // This is for Heroic Mode
    SPELL_SCARY_FOG_CIRCLE  = 123797,
    SPELL_SCARY_FOG_DOT     = 123705,
    SPELL_SCARY_FOG_STACKS  = 123712,

    SPELL_LEI_SHI_TRANSFORM = 127535
};

enum eLeiShiEvents
{
    // Lei Shi
    EVENT_SPECIAL       = 1,
    EVENT_SPRAY         = 2,
    EVENT_GET_AWAY      = 3,
    EVENT_GET_AWAY_CAST = 4,
    EVENT_PROTECT       = 5,
    EVENT_HIDE          = 6,

    // Lei Shi (hidden)
    EVENT_HIDDEN_SPRAY  = 7,
    EVENT_BERSERK       = 8,

    // OOC
    EVENT_WP_1          = 9,
    EVENT_WP_2          = 10,
    EVENT_WP_3          = 11
};

enum eLeiShiActions
{
    ACTION_ANIMATED_PROTECTOR_DIED,
    ACTION_ACTIVATE_ANIMATED_PROTECTORS,
    ACTION_TERMINATE_GET_AWAY_PHASE,
    ACTION_TERMINATE_HIDE_PHASE
};

enum eLeiShiSays
{
    TALK_AGGRO  = 1,
    TALK_HIDE,
    TALK_GET_AWAY,
    TALK_SLAY,
    TALK_INTRO,
    TALK_DEFEATED,
    TALK_OUTRO_1,
    TALK_OUTRO_2
};

enum eReflectionSays
{
    EMOTE_RIPPLE,
    EMOTE_RIPPLE_2,
    EMOTE_APPEAR
};

Position leiShiPos = { -1017.93f, -2911.3f, 19.902f, 4.74f };

Position hidePositions[4] =
{
    { -990.678f,  -2890.043f, 19.172f, 3.781f },
    { -994.835f,  -2933.835f, 19.172f, 2.377f },
    { -1041.316f, -2932.84f,  19.172f, 0.733f },
    { -1045.101f, -2890.742f, 19.172f, 5.646f }
};

#define HEROIC_DIST_TO_VORTEX 21.0f
#define DIST_TO_SCARY_FOG_DOT 4.5f

class boss_lei_shi : public CreatureScript
{
    public:
        boss_lei_shi() : CreatureScript("boss_lei_shi") { }

        struct boss_lei_shiAI : public BossAI
        {
            boss_lei_shiAI(Creature* creature) : BossAI(creature, DATA_LEI_SHI)
            {
                pInstance = creature->GetInstanceScript();
                leiShiFreed = false;
                me->SetReactState(REACT_PASSIVE);
            }

            InstanceScript* pInstance;
            EventMap events;
            EventMap m_mEvents;

            uint8 nextAfraidPct;
            uint8 nextProtectPct;

            uint8 endCombatPct;

            std::list<uint64> animatedProtectors;

            bool hidden;
            bool shielded;
            bool getAwayPhase;
            bool protectScheduled;
            bool leiShiFreed;
            bool nextSpecial;

            float getAwayHealthPct;

            void Reset()
            {
                _Reset();

                summons.DespawnAll();

                hidden              = false;
                getAwayPhase        = false;
                shielded            = false;
                protectScheduled    = false;
                nextSpecial         = (bool)urand(0, 1);

                nextAfraidPct       = 90;
                nextProtectPct      = 80;
                endCombatPct        = 2;

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->RestoreDisplayId();
                me->RemoveAura(SPELL_AFRAID);
                me->RemoveAura(SPELL_HIDE);
                me->RemoveAura(SPELL_HIDE_STACKS);
                me->RemoveAura(SPELL_SCARY_FOG_CIRCLE);
                me->RemoveAura(SPELL_SCARY_FOG_DOT);
                me->RemoveAura(SPELL_SCARY_FOG_STACKS);
                me->RemoveAllAreaTriggers();

                events.Reset();

                events.ScheduleEvent(EVENT_SPRAY, 400);

                ScheduleSpecialSpell();

                if (pInstance)
                {
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SCARY_FOG_CIRCLE);
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SCARY_FOG_DOT);
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SCARY_FOG_STACKS);
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPRAY);

                   /* if (pInstance->GetData(SPELL_RITUAL_OF_PURIFICATION))
                        me->AddAura(SPELL_RITUAL_OF_PURIFICATION, me);*/

                    std::list<Creature*> protectors;
                    me->GetCreatureListWithEntryInGrid(protectors, NPC_ANIMATED_PROTECTOR, 100.0f);

                    for (auto itr : protectors)
                    {
                        animatedProtectors.push_back(itr->GetGUID());
                        itr->Respawn();
                        itr->AddAura(SPELL_PROTECT_RESPAWN, itr);
                        itr->RemoveAura(SPELL_PROTECT_VISUAL);
                        itr->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC);
                    }
                }

                if (leiShiFreed)
                    me->CastSpell(me, SPELL_LEI_SHI_TRANSFORM, true);
            }

            void ScheduleSpecialSpell()
            {
                events.ScheduleEvent(nextSpecial ? EVENT_GET_AWAY : EVENT_HIDE, 32000);
                nextSpecial = !nextSpecial;
            }

            void JustReachedHome()
            {
                _JustReachedHome();

                if (pInstance)
                    pInstance->SetBossState(DATA_LEI_SHI, FAIL);
            }

            void MovementInform(uint32 uiType, uint32 uiPointId)
            {
                if (uiType != POINT_MOTION_TYPE)
                    return;

                if (uiPointId == 45)
                {
                    me->SetFacingTo(4.71f);
                    m_mEvents.ScheduleEvent(EVENT_WP_2, 500);
                }

                if (uiPointId == 46)
                {
                    if (Creature* pSha = ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_SHA_OF_FEAR)))
                    {
                        if (pSha->AI())
                            pSha->AI()->DoAction(ACTION_SHA_INTRO);
                    }

                    me->SetPhaseMask(128, true);
                    events.Reset();
                    me->GetMotionMaster()->MoveIdle();
                }
            }

            void EnterCombat(Unit* /*attacker*/)
            {
                if (pInstance)
                {
                    me->CastSpell(me, SPELL_AFRAID, true);

                    if (auto const afraid = me->GetAuraEffect(SPELL_AFRAID, EFFECT_0))
                        afraid->ChangeAmount(0);

                    pInstance->SetBossState(DATA_LEI_SHI, IN_PROGRESS);
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    DoZoneInCombat();

                    if (me->GetMap()->IsHeroic())
                        me->CastSpell(me, SPELL_SCARY_FOG_CIRCLE, true);

                    Talk(TALK_AGGRO);

                    events.ScheduleEvent(EVENT_BERSERK, 7 * MINUTE*IN_MILLISECONDS);
                }
            }

            void KilledUnit(Unit* who)
            {
                if (who->GetTypeId() == TYPEID_PLAYER)
                    Talk(TALK_SLAY);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (!pInstance)
                    return;

                if (nextAfraidPct)
                {
                    if (me->HealthBelowPctDamaged(nextAfraidPct, damage))
                    {
                        if (auto const afraid = me->GetAuraEffect(SPELL_AFRAID, EFFECT_0))
                            afraid->ChangeAmount(afraid->GetAmount() + 8);

                        nextAfraidPct -= 10;
                    }

                    if (me->HealthBelowPctDamaged(int32(getAwayHealthPct - 4.0f), damage))
                        me->RemoveAura(SPELL_GET_AWAY);

                    if (nextProtectPct >= 20)
                    {
                        if (me->HealthBelowPctDamaged(nextProtectPct, damage) && !protectScheduled)
                        {
                            protectScheduled = true;
                            events.ScheduleEvent(EVENT_PROTECT, 100);
                        }
                    }
                }

                if (me->HealthBelowPctDamaged(endCombatPct, damage))
                {
                    damage = 0;

                    if (leiShiFreed)
                        return;

                    summons.DespawnAll();
                    pInstance->SetBossState(DATA_LEI_SHI, DONE);
                    events.Reset();

                    EnterEvadeMode();

                    me->CombatStop();
                    me->DeleteThreatList();
                    me->setFaction(35);
                    me->CastSpell(me, SPELL_LEI_SHI_TRANSFORM, true);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->RestoreDisplayId();
                    me->RemoveAura(SPELL_AFRAID);
                    me->RemoveAura(SPELL_HIDE);
                    me->RemoveAura(SPELL_HIDE_STACKS);
                    me->RemoveAura(SPELL_SCARY_FOG_CIRCLE);
                    me->RemoveAura(SPELL_SCARY_FOG_DOT);
                    me->RemoveAura(SPELL_SCARY_FOG_STACKS);
                    me->RemoveAura(SPELL_BERSERK);
                    me->RemoveAllAreaTriggers();
                    me->setRegeneratingHealth(false);

                    if (attacker && attacker->GetTypeId() == TYPEID_PLAYER)
                    {
                        attacker->ToPlayer()->SetLastKilledCreature(me);
                        me->GetMap()->ToInstanceMap()->PermBindAllPlayers(attacker->ToPlayer());
                    }
                    else if (attacker && attacker->GetTypeId() == TYPEID_UNIT && attacker->GetOwner() && attacker->GetOwner()->ToPlayer())
                    {
                        attacker->GetOwner()->ToPlayer()->SetLastKilledCreature(me);
                        me->GetMap()->ToInstanceMap()->PermBindAllPlayers(attacker->GetOwner()->ToPlayer());
                    }

                    leiShiFreed = true;
                    Talk(TALK_DEFEATED);

                    m_mEvents.ScheduleEvent(EVENT_WP_1, 15000);

                    if (pInstance)
                    {
                        pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SCARY_FOG_CIRCLE);
                        pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SCARY_FOG_DOT);
                        pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SCARY_FOG_STACKS);
                        pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPRAY);
                    }
                }
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (!hidden)
                    return;

                if (!caster)
                    return;

                if (caster->GetTypeId() == TYPEID_UNIT && (caster->GetEntry() == NPC_LEI_SHI || caster->GetEntry() == NPC_LEI_SHI_HIDDEN))
                    return;

                if (!spell)
                    return;

                if (!spell->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) &&
                    !spell->HasEffect(SPELL_EFFECT_NORMALIZED_WEAPON_DMG) &&
                    !spell->HasEffect(SPELL_EFFECT_WEAPON_DAMAGE) &&
                    !spell->HasEffect(SPELL_EFFECT_WEAPON_PERCENT_DAMAGE) &&
                    !spell->HasEffect(SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL))
                    return;

                if (auto const hide = me->GetAura(SPELL_HIDE_STACKS))
                    hide->ModCharges(-1);
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                    case ACTION_ANIMATED_PROTECTOR_DIED:
                    {
                        me->RemoveAura(SPELL_PROTECT);
                        shielded = false;
                        protectScheduled = false;
                        nextProtectPct -= 20;

                        Creature* protector = NULL;

                        for (auto itr : animatedProtectors)
                        {
                            if (Creature* elemental = me->GetMap()->GetCreature(itr))
                            {
                                if (!elemental->isAlive())
                                    protector = elemental;

                                elemental->RemoveAura(SPELL_PROTECT_VISUAL);
                                elemental->AddAura(SPELL_PROTECT_RESPAWN, elemental);
                                elemental->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC);
                            }
                        }

                        if (protector)
                            animatedProtectors.remove(protector->GetGUID());

                        break;
                    }
                    case ACTION_ACTIVATE_ANIMATED_PROTECTORS:
                    {
                        if (animatedProtectors.empty())
                            break;

                        me->CastSpell(me, SPELL_PROTECT, false);
                        shielded = true;

                        uint8 protectorsActivated = 0;
                        for (auto itr : animatedProtectors)
                        {
                            // 3 Animated Protectors at 80% and 60%
                            if (((me->GetMap()->IsHeroic() && protectorsActivated >= 4) ||
                                (!me->GetMap()->IsHeroic() && protectorsActivated >= 3)) &&
                                nextProtectPct > 40)
                                break;

                            // 4 Animated Protectors at 40% and 20%
                            if (((me->GetMap()->IsHeroic() && protectorsActivated >= 5) ||
                                (!me->GetMap()->IsHeroic() && protectorsActivated >= 4)) &&
                                nextProtectPct <= 40)
                                break;

                            if (Creature* protector = me->GetMap()->GetCreature(itr))
                            {
                                protector->SetHealth(protector->GetMaxHealth());
                                protector->RemoveAura(SPELL_PROTECT_RESPAWN);
                                protector->CastSpell(protector, SPELL_PROTECT_VISUAL, true);
                                protector->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC);

                                if (protector->AI())
                                    protector->AI()->DoZoneInCombat();

                                ++protectorsActivated;
                            }
                        }

                        break;
                    }
                    case ACTION_TERMINATE_GET_AWAY_PHASE:
                    {
                        getAwayPhase = false;
                        me->RemoveAllAreaTriggers();

                        ScheduleSpecialSpell();
                        break;
                    }
                    case ACTION_TERMINATE_HIDE_PHASE:
                    {
                        me->RemoveAura(SPELL_HIDE);
                        me->RemoveAura(SPELL_HIDE_STACKS);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->RestoreDisplayId();
                        hidden = false;

                        if (me->GetMap()->IsHeroic())
                            me->CastSpell(me, SPELL_SCARY_FOG_CIRCLE, true);

                        // Only have Lei Shi (hidden) in summons
                        summons.DespawnAll();

                        ScheduleSpecialSpell();
                        break;
                    }
                    case ACTION_LEISHI_INTRO:
                        me->SetPhaseMask(1, true);
                        me->SetReactState(REACT_AGGRESSIVE);
                        Talk(TALK_INTRO);
                        
                        if (Creature* pReflection = ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_REFLECTION_OF_LEI_SHI)))
                        {
                            pReflection->SetPhaseMask(128, true);
                        }

                        if (GameObject* pVortex = ObjectAccessor::GetGameObject(*me, instance->GetData64(GOB_LEI_SHIS_VORTEX)))
                            pVortex->SetGoState(GO_STATE_ACTIVE);

                        if (GameObject* pVortexWall = ObjectAccessor::GetGameObject(*me, instance->GetData64(GOB_WALL_OF_LEI_SHI)))
                            pVortexWall->SetGoState(GO_STATE_ACTIVE);

                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                m_mEvents.Update(diff);

                switch (m_mEvents.ExecuteEvent())
                {
                case EVENT_WP_1:
                    Talk(TALK_OUTRO_1);
                    
                    me->SetWalk(true);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MovePoint(45, aLeiWps[0]);
                    break;
                case EVENT_WP_2:
                    Talk(TALK_OUTRO_2);
                    m_mEvents.ScheduleEvent(EVENT_WP_3, 8000);
                    break;
                case EVENT_WP_3:
                    me->SetWalk(false);
                    me->GetMotionMaster()->MovePoint(46, aLeiWps[1]);
                    break;
                }

                if (!UpdateVictim())
                {
                    /*
                    if (pInstance && pInstance->GetData(SPELL_RITUAL_OF_PURIFICATION) == false)
                        me->RemoveAura(SPELL_RITUAL_OF_PURIFICATION);*/

                    std::list<Creature*> protectors;
                    me->GetCreatureListWithEntryInGrid(protectors, NPC_ANIMATED_PROTECTOR, 100.0f);

                    protectors.remove_if(WoWSource::UnitAuraCheck(true, SPELL_PROTECT_RESPAWN));

                    for (auto itr : protectors)
                    {
                        animatedProtectors.push_back(itr->GetGUID());
                        itr->Respawn();
                        itr->AddAura(SPELL_PROTECT_RESPAWN, itr);
                        itr->RemoveAura(SPELL_PROTECT_VISUAL);
                        itr->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC);
                    }

                    return;
                }

                if (hidden)
                    return;

                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK, true);
                        break;
                    case EVENT_SPRAY:
                    {
                        if (getAwayPhase || me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            events.ScheduleEvent(EVENT_SPRAY, 400);
                            break;
                        }

                        if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                            me->CastSpell(target, SPELL_SPRAY, false);
                        events.ScheduleEvent(EVENT_SPRAY, 400);
                        break;
                    }
                    case EVENT_GET_AWAY:
                    {
                        if ((me->HasUnitState(UNIT_STATE_CASTING) && !getAwayPhase) || shielded)
                        {
                            events.ScheduleEvent(EVENT_GET_AWAY, 0);
                            break;
                        }

                        // Teleport Lei Shi to the center of the room
                        Talk(TALK_GET_AWAY);
                        me->CastSpell(me, SPELL_GET_AWAY_TELEPORT, true);
                        getAwayPhase = true;
                        getAwayHealthPct = me->GetHealthPct();
                        events.ScheduleEvent(EVENT_GET_AWAY_CAST, 300);
                        break;
                    }
                    case EVENT_GET_AWAY_CAST:
                    {
                        me->CastSpell(me, SPELL_GET_AWAY, false);
                        break;
                    }
                    case EVENT_PROTECT:
                    {
                        if (getAwayPhase || me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            events.RescheduleEvent(EVENT_PROTECT, 0);
                            break;
                        }

                        DoAction(ACTION_ACTIVATE_ANIMATED_PROTECTORS);
                        break;
                    }
                    case EVENT_HIDE:
                    {
                        if (me->HasUnitState(UNIT_STATE_CASTING) || shielded)
                        {
                            events.RescheduleEvent(EVENT_HIDE, 0);
                            break;
                        }

                        Talk(TALK_HIDE);
                        me->CastSpell(me, SPELL_HIDE, false);
                        hidden = true;
                        me->RemoveAura(SPELL_SCARY_FOG_CIRCLE);
                        break;
                    }
                    default:
                        break;
                }
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lei_shiAI(creature);
        }
};

// Animated Protector - 62995
class mob_animated_protector : public CreatureScript
{
    public:
        mob_animated_protector() : CreatureScript("mob_animated_protector") { }

        struct mob_animated_protectorAI : public ScriptedAI
        {
            mob_animated_protectorAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;

            void JustDied(Unit* /*killer*/)
            {
                if (Creature* leiShi = me->GetMap()->GetCreature(pInstance->GetData64(NPC_LEI_SHI)))
                    leiShi->AI()->DoAction(ACTION_ANIMATED_PROTECTOR_DIED);
            }

            void UpdateAI(const uint32 /*diff*/)
            {
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_animated_protectorAI(creature);
        }
};

// Lei Shi (hidden) - 63099
class mob_lei_shi_hidden : public CreatureScript
{
    enum
    {
        SPELL_HIDE_VISUAL       = 132363,
        SPELL_HIDE_PROC         = 123233,
        SPELL_HIDE_TRIGGER      = 132362,
    };
    public:
        mob_lei_shi_hidden() : CreatureScript("mob_lei_shi_hidden") { }

        struct mob_lei_shi_hiddenAI : public ScriptedAI
        {
            mob_lei_shi_hiddenAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void SpellHitTarget(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_HIDE_TRIGGER)
                    caster->CastSpell(caster, SPELL_HIDE_VISUAL, true);
            }

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);

                events.Reset();
                events.ScheduleEvent(EVENT_HIDDEN_SPRAY, 400);

                if (pInstance)
                {
                    if (Creature* leiShi = me->GetMap()->GetCreature(pInstance->GetData64(NPC_LEI_SHI)))
                    {
                        auto const afraid = leiShi->GetAuraEffect(SPELL_AFRAID, EFFECT_0);
                        if (!afraid)
                            return;

                        if (!me->HasAura(SPELL_AFRAID))
                        {
                            if (auto const newAfraid = me->AddAura(SPELL_AFRAID, me))
                                if (newAfraid->GetEffect(0))
                                    newAfraid->GetEffect(0)->ChangeAmount(afraid->GetAmount());
                        }
                        else
                        {
                            if (auto const newAfraid = me->GetAuraEffect(SPELL_AFRAID, EFFECT_0))
                                newAfraid->ChangeAmount(afraid->GetAmount());
                        }
                    }
                }

                DoCast(me, SPELL_HIDE_PROC, true);
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_HIDDEN_SPRAY:
                    {
                        if (me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            events.ScheduleEvent(EVENT_HIDDEN_SPRAY, 0);
                            break;
                        }

                        uint64 leiShiGuid = 0;
                        if (pInstance)
                            leiShiGuid = pInstance->GetData64(NPC_LEI_SHI);

                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f))
                            me->CastSpell(target, SPELL_SPRAY, false, NULL, NULL, leiShiGuid);
                        events.ScheduleEvent(EVENT_HIDDEN_SPRAY, 400);
                        break;
                    }
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_lei_shi_hiddenAI(creature);
        }
};

class npc_lei_shi_reflection : public CreatureScript
{
    enum eEvents : uint32
    {
        EVENT_NONE,
        EVENT_RIPPLE_1,
        EVENT_RIPPLE_2,
        EVENT_APPEAR
    };
public:
    npc_lei_shi_reflection() : CreatureScript("npc_lei_shi_reflection") {}

    struct npc_lei_shi_reflectionAI : public ScriptedAI
    {
        npc_lei_shi_reflectionAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void DoAction(const int32 iAction)
        {
            if (iAction == ACTION_LEISHI_INTRO)
            {
                me->ClearUnitState(UNIT_STATE_STUNNED);
                events.ScheduleEvent(EVENT_RIPPLE_1, 2000);
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            events.Update(uiDiff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_RIPPLE_1:
                    DoCast(SPELL_CLOUDED_IMPACT);
                    Talk(EMOTE_RIPPLE);
                    events.ScheduleEvent(EVENT_RIPPLE_2, 14000);
                    break;
                case EVENT_RIPPLE_2:
                    DoCast(SPELL_CLOUDED_IMPACT);
                    Talk(EMOTE_RIPPLE_2);
                    events.ScheduleEvent(EVENT_APPEAR, 10000);
                    break;
                case EVENT_APPEAR:
                    if (me->GetInstanceScript())
                    {
                        if (Creature* pLeiShi = ObjectAccessor::GetCreature(*me, me->GetInstanceScript()->GetData64(NPC_LEI_SHI)))
                        {
                            Talk(EMOTE_APPEAR);

                            if (pLeiShi->AI())
                                pLeiShi->AI()->DoAction(ACTION_LEISHI_INTRO);
                        }
                    }
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_lei_shi_reflectionAI(pCreature);
    }
};

// Get Away ! - 123461
class spell_get_away : public SpellScriptLoader
{
    public:
        spell_get_away() : SpellScriptLoader("spell_get_away") { }

        class spell_get_away_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_get_away_AuraScript);

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->ToCreature())
                        if (caster->ToCreature()->AI())
                            caster->ToCreature()->AI()->DoAction(ACTION_TERMINATE_GET_AWAY_PHASE);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_get_away_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_get_away_AuraScript();
        }
};

// Hide - 123244
class spell_hide : public SpellScriptLoader
{
    public:
        spell_hide() : SpellScriptLoader("spell_hide") { }

        class spell_hide_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hide_AuraScript);

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {

                    uint8 pos = urand(0, 3);

                    caster->NearTeleportTo(hidePositions[pos].GetPositionX(),
                                           hidePositions[pos].GetPositionY(),
                                           hidePositions[pos].GetPositionZ(),
                                           hidePositions[pos].GetOrientation());

                    caster->CastSpell(caster, SPELL_HIDE_STACKS, true);
                    caster->CastSpell(caster, SPELL_HIDE_SUMMON, true);

                    caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    caster->SetDisplayId(11686);
                }
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    caster->RemoveAura(SPELL_HIDE_STACKS);
                    caster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    caster->RestoreDisplayId();
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_hide_AuraScript::OnApply, EFFECT_1, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_hide_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hide_AuraScript();
        }
};

// Get Away damage - 123467
class spell_get_away_dmg : public SpellScriptLoader
{
public:
    spell_get_away_dmg() : SpellScriptLoader("spell_get_away_dmg") {}

    class spell_impl : public SpellScript
    {
        PrepareSpellScript(spell_impl);

        void ModifyDamage(SpellEffIndex effIndex)
        {
            Unit * caster = GetCaster();
            Player * victim = GetHitPlayer();

            if (!caster || !victim)
                return;

            if (victim->IsMoving() && !victim->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_BACKWARD) && victim->isInFront(caster))
                SetHitDamage(GetHitDamage() / 2);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_impl::ModifyDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_impl();
    }
};


// Hide (stacks) - 123233
class spell_hide_stacks : public SpellScriptLoader
{
    public:
        spell_hide_stacks() : SpellScriptLoader("spell_hide_stacks") { }

        class spell_hide_stacks_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hide_stacks_AuraScript);

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->ToCreature())
                        if (caster->ToCreature()->AI())
                            caster->ToCreature()->AI()->DoAction(ACTION_TERMINATE_HIDE_PHASE);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_hide_stacks_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hide_stacks_AuraScript();
        }
};

// Scary Fog (DoT) - 123705
class spell_scary_fog_dot : public SpellScriptLoader
{
    public:
        spell_scary_fog_dot() : SpellScriptLoader("spell_scary_fog_dot") { }

        class spell_scary_fog_dot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_scary_fog_dot_SpellScript);

            void CorrectRange(std::list<WorldObject*>& targets)
            {
                targets.clear();

                if (Unit* pCaster = GetCaster())
                {
                    Map::PlayerList const& players = pCaster->GetMap()->GetPlayers();

                    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if (Player* player = itr->getSource())
                        {
                            if (player->GetExactDist2d(GetCaster()->GetPositionX(), GetCaster()->GetPositionY()) >= HEROIC_DIST_TO_VORTEX)
                                targets.push_back(player);
                        }
                    }

                    for (auto itr : targets)
                    {
                        if (itr->ToUnit())
                        {
                            if (auto const scaryFog = pCaster->AddAura(SPELL_SCARY_FOG_STACKS, itr->ToUnit()))
                            {
                                scaryFog->SetDuration(35000);
                                scaryFog->SetMaxDuration(35000);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_scary_fog_dot_SpellScript::CorrectRange, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_scary_fog_dot_SpellScript();
        }

        class spell_scary_fog_dot_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_scary_fog_dot_AuraScript);

            void OnTick(constAuraEffectPtr /*aurEff*/)
            {
                if (Unit* target = GetTarget())
                    target->CastSpell(target, SPELL_SCARY_FOG_STACKS, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_scary_fog_dot_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_scary_fog_dot_AuraScript();
        }
};

// Scary Fog (stacks) - 123712
class spell_scary_fog_stacks : public SpellScriptLoader
{
    public:
        spell_scary_fog_stacks() : SpellScriptLoader("spell_scary_fog_stacks") { }

        class spell_scary_fog_stacks_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_scary_fog_stacks_SpellScript);

            void CorrectTargets(std::list<WorldObject*>& targets)
            {
                for (auto itr : targets)
                {
                    if (itr->ToUnit() && itr->ToUnit()->GetEntry() != NPC_LEI_SHI_HIDDEN)
                    {
                        if (Unit* pCaster = GetCaster())
                        {
                            if (auto const scary = pCaster->GetAura(SPELL_SCARY_FOG_STACKS))
                            {
                                if (auto const scaryTarget = itr->ToUnit()->GetAura(SPELL_SCARY_FOG_STACKS))
                                    scaryTarget->SetStackAmount(scary->GetStackAmount());
                                else if (auto const scaryTarget = pCaster->AddAura(SPELL_SCARY_FOG_STACKS, itr->ToUnit()))
                                    scaryTarget->SetStackAmount(scary->GetStackAmount());
                            }
                        }
                    }
                }

                targets.clear();
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_scary_fog_stacks_SpellScript::CorrectTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_scary_fog_stacks_SpellScript();
        }
};

void AddSC_boss_lei_shi()
{
    new boss_lei_shi();
    new mob_animated_protector();
    new mob_lei_shi_hidden();
    new npc_lei_shi_reflection();
    new spell_get_away();
    new spell_get_away_dmg();
    new spell_hide();
    new spell_hide_stacks();
    new spell_scary_fog_dot();
    new spell_scary_fog_stacks();
}
