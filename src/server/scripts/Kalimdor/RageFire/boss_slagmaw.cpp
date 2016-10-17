/*
 * Copyright (C) 2015 WoWSource [WoWSC] <http://www.wowsource.info/>
 *
 * This SourceCode is NOT free a software. Please hold everything Private
 * and read our Terms
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Spells
{
    SPELL_LAVA_SPIT = 119434,
    SPELL_SUBMERGE  = 120384
};

enum Events
{
    EVENT_LAVA_SPIT,
    EVENT_SUBMERGE
};

class boss_slagmaw : public CreatureScript
{
    public:
        boss_slagmaw() : CreatureScript("boss_slagmaw") { }

        struct boss_slagmawAI : public ScriptedAI
        {
            boss_slagmawAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            }

            void Reset() { }

            void EnterCombat(Unit* /*who*/) { }

            void JustDied(Unit* /*who*/)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            }

            void AttackStart(Unit* target)
            {
                if (me->Attack(target, true))
                    DoStartNoMovement(target);
            }

            void UpdateAI(uint32 const diff)
            {

                if(me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_slagmawAI(creature);
        }
};

class boss_slagmaw_head : public CreatureScript
{
    public:
        boss_slagmaw_head() : CreatureScript("boss_slagmaw_head") { }

        struct boss_slagmaw_headAI : public ScriptedAI
        {
            boss_slagmaw_headAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() { }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_LAVA_SPIT, 8000);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (Creature* SlagmawBody = me->FindNearestCreature(61463, 50.0f))
                    me->Kill(SlagmawBody);

                me->DespawnOrUnsummon(500);
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
                        case EVENT_LAVA_SPIT:
                            DoCastVictim(SPELL_LAVA_SPIT);
                            events.ScheduleEvent(EVENT_LAVA_SPIT, 5*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_slagmaw_headAI(creature);
        }
};

void AddSC_boss_slagmaw()
{
    new boss_slagmaw();
    new boss_slagmaw_head();
}
