/*
 * Copyright (C) 2015 WoWSource [WoWSC] <http://www.wowsource.info/>
 *
 * In Cooperation with
 *
 * Copyright (C) 2015 StealthCoders [SC] <http://www.stealthcoders.net/>
 *
 * This SourceCode is NOT free a software. Please hold everything Private
 * and read our Terms.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "wailing_caverns.h"

enum Spells
{
    SPELL_DRUIDS_SLUMBER =  8040,
    SPELL_HEALING_TOUCH  = 23381,
    SPELL_LIGHTNING_BOLT = 20295,
    SPELL_THORNS_AURA    =  8148
};

enum Events
{
    EVENT_DRUIDS_SLUMBER = 0,
    EVENT_HEALING_TOUCH  = 1,
    EVENT_LIGHTNING_BOLT = 2,
    EVENT_THORNS_AURA    = 3
};

enum Texts
{
    SAY_AGGRO    = 0
};

class boss_lady_anacondra : public CreatureScript
{
    public:
        boss_lady_anacondra() : CreatureScript("boss_lady_anacondra") { }

        struct boss_lady_anacondraAI : public BossAI
        {
            boss_lady_anacondraAI(Creature* creature) : BossAI(creature, TYPE_LADY_ANACONDRA) { }

            void Reset()
            {
                _Reset();
                thorns = false;
                heal   = false;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 500);
                events.ScheduleEvent(EVENT_DRUIDS_SLUMBER, 10000);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
            {
                if(!thorns && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(85))
                {
                    thorns = true;
                    events.RescheduleEvent(EVENT_THORNS_AURA, 500);
                }
                if(!heal && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(50))
                {
                    heal = true;
                    events.ScheduleEvent(EVENT_HEALING_TOUCH, 500);
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void UpdateAI(uint32 const diff)
            {
                if(!UpdateVictim())
                    return;

                events.Update(diff);

                if(me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_LIGHTNING_BOLT:
                            DoCastVictim(SPELL_LIGHTNING_BOLT);
                            events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 4000);
                            break;
                        case EVENT_DRUIDS_SLUMBER:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                                DoCast(target, SPELL_DRUIDS_SLUMBER);
                            events.ScheduleEvent(EVENT_DRUIDS_SLUMBER, 20000);
                            break;
                        case EVENT_HEALING_TOUCH:
                            DoCast(me, SPELL_HEALING_TOUCH);
                            events.ScheduleEvent(EVENT_HEALING_TOUCH, urand(18000, 20000));
                            break;
                        case EVENT_THORNS_AURA:
                            DoCast(me, SPELL_THORNS_AURA);
                            events.ScheduleEvent(EVENT_THORNS_AURA, 60000);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
    private:
        bool thorns;
        bool heal;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lady_anacondraAI(creature);
        }
};

void AddSC_boss_lady_anacondra()
{
    new boss_lady_anacondra();
}
