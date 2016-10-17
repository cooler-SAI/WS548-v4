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
    SPELL_HEALING_TOUCH  = 11431,
    SPELL_LIGHTNING_BOLT = 20805,
    SPELL_THUNDERCLAP    =  8147
};

enum Events
{
    EVENT_DRUIDS_SLUMBER = 0,
    EVENT_HEALING_TOUCH  = 1,
    EVENT_LIGHTNING_BOLT = 2,
    EVENT_THUNDERCLAP    = 3
};

enum Texts
{
    SAY_AGGRO    = 0
};

class boss_lord_pythas : public CreatureScript
{
    public:
        boss_lord_pythas() : CreatureScript("boss_lord_pythas") { }

        struct boss_lord_pythasAI : public BossAI
        {
            boss_lord_pythasAI(Creature* creature) : BossAI(creature, TYPE_LORD_PYTHAS) { }

            void Reset()
            {
                _Reset();
                thunder = false;
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
                if(!thunder && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(85))
                {
                    thunder = true;
                    events.RescheduleEvent(EVENT_THUNDERCLAP, 500);
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
                        case EVENT_THUNDERCLAP:
                            DoCast(me, SPELL_THUNDERCLAP);
                            events.ScheduleEvent(EVENT_THUNDERCLAP, 60000);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
    private:
        bool thunder;
        bool heal;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lord_pythasAI(creature);
        }
};

void AddSC_boss_lord_pythas()
{
    new boss_lord_pythas();
}
