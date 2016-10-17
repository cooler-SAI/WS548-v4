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
    SPELL_LIGHTNING_BOLT = 20805,
    SPELL_COBRA_FORM     =  7965,
    SPELL_POISON         =   744
};

enum Events
{
    EVENT_DRUIDS_SLUMBER = 0,
    EVENT_HEALING_TOUCH  = 1,
    EVENT_LIGHTNING_BOLT = 2,
    EVENT_COBRA_FORM     = 3,
    EVENT_POISON         = 4
};

enum Texts
{
    SAY_AGGRO    = 0
};

class boss_lord_cobrahn : public CreatureScript
{
    public:
        boss_lord_cobrahn() : CreatureScript("boss_lord_cobrahn") { }

        struct boss_lord_cobrahnAI : public BossAI
        {
            boss_lord_cobrahnAI(Creature* creature) : BossAI(creature, TYPE_LORD_COBRAHN) { }

            void Reset()
            {
                _Reset();
                heal      = false;
                transform = false;
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
                if(!heal && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(50))
                {
                    heal = true;
                    events.ScheduleEvent(EVENT_HEALING_TOUCH, 500);
                }
                if(!transform && !me->IsNonMeleeSpellCasted(false) && HealthBelowPct(45))
                {
                    transform = true;
                    DoCast(me, SPELL_COBRA_FORM);
                    events.CancelEvent(EVENT_LIGHTNING_BOLT);
                    events.CancelEvent(EVENT_DRUIDS_SLUMBER);
                    events.CancelEvent(EVENT_HEALING_TOUCH);
                    events.ScheduleEvent(EVENT_POISON, 1000);
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
                        case EVENT_POISON:
                            DoCast(me, SPELL_POISON);
                            events.ScheduleEvent(EVENT_POISON, 15000);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
    private:
        bool heal;
        bool transform;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lord_cobrahnAI(creature);
        }
};

void AddSC_boss_lord_cobrahn()
{
    new boss_lord_cobrahn();
}
