/*
 *    Dungeon : Mogu'shan palace 87-89
 *    Gekkan
 *    Jade servers
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "mogu_shan_palace.h"

#define TYPE_GET_ENTOURAGE 14

const uint32 auiGekkanAdds[4] =
{
    CREATURE_GLINTROK_SKULKER,
    CREATURE_GLINTROK_HEXXER,
    CREATURE_GLINTROK_ORACLE,
    CREATURE_GLINTROK_IRONHIDE
};


class boss_gekkan : public CreatureScript
{
public:
    boss_gekkan() : CreatureScript("boss_gekkan") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_gekkan_AI(creature);
    }

    enum eSpells
    {
        SPELL_RECKLESS_INSPIRATION     = 118988,
        SPELL_RECKLESS_INSPIRATION_2   = 129262
    };

    enum eActions
    {
        ACTION_ADD_DEATH,
        ACTION_COMBAT
    };

    enum eEvents
    {
        EVENT_RECKLESS_INSPIRATION = 1,
    };

    enum eTalks
    {
        TALK_INTRO,
        TALK_KILLING,
        TALK_SPELL,
        TALK_AGGRO,
        TALK_DEATH,
    };

    struct boss_gekkan_AI : public BossAI
    {
        boss_gekkan_AI(Creature* creature) : BossAI(creature, DATA_GEKKAN)
        {
            InitializeGekkan();
        }

        std::vector<uint64> m_uilGekkanAdds;

        bool m_bYelled;

        void InitializeAI() final
        {
            Reset();
            SetImmuneToPullPushEffects(true);
        }

        void InitializeGekkan()
        {
            m_bYelled = false;

            InsertAdds(3);
        }

        bool InsertAdds(int n)
        {
            if (n < 0)
                return false;

            if (Creature* pAdd = GetClosestCreatureWithEntry(me, auiGekkanAdds[n], 40.f))
                m_uilGekkanAdds.push_back(pAdd->GetGUID());

            return InsertAdds(n - 1);
        }

        void HandleAddDeath()
        {
            Talk(TALK_SPELL);
            DoCast(SPELL_RECKLESS_INSPIRATION_2);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat();

            if (me->getVictim())
            {
                // Get the four adds.
                for (auto const &guid : m_uilGekkanAdds)
                {
                    auto const pAdd = ObjectAccessor::GetCreature(*me, guid);
                    if (pAdd && pAdd->AI())
                        pAdd->SetInCombatWithZone();
                }
            }

            Talk(TALK_AGGRO);

            events.ScheduleEvent(EVENT_RECKLESS_INSPIRATION, urand(10000, 16000));
        }

        void MoveInLineOfSight(Unit* pWho)
        {
            if (pWho && pWho->GetTypeId() == TYPEID_PLAYER && !m_bYelled)
            {
                Talk(TALK_INTRO);
                m_bYelled = true;
            }

            CreatureAI::MoveInLineOfSight(pWho);
        }

        void JustReachedHome()
        {
            // Respawn all adds on evade
            for (auto const &guid : m_uilGekkanAdds)
            {
                if (Creature* pAdd = ObjectAccessor::GetCreature(*me, guid))
                {
                    if (!pAdd->isAlive())
                        pAdd->Respawn();

                    pAdd->RemoveAllAuras();
                }
            }
        }

        Creature* GetRandomAliveAdd()
        {
            decltype(m_uilGekkanAdds) uiTempGuidList;

            for (auto const &guid : m_uilGekkanAdds)
            {
                auto const pAdd = ObjectAccessor::GetCreature(*me, guid);
                if (pAdd && pAdd->isAlive())
                    uiTempGuidList.push_back(guid);
            }

            if (uiTempGuidList.empty())
                return nullptr;

            auto itr = uiTempGuidList.cbegin();
            std::advance(itr, urand(0, uiTempGuidList.size() - 1));

            return ObjectAccessor::GetCreature(*me, *itr);
        }

        void JustDied(Unit* /*who*/)
        {
            // If we die, all remaining adds should get buff
            for (auto const &guid : m_uilGekkanAdds)
            {
                auto const pAdd = ObjectAccessor::GetCreature(*me, guid);
                if (pAdd && pAdd->isAlive())
                    pAdd->CastSpell(pAdd, SPELL_RECKLESS_INSPIRATION_2, false);
            }

            Talk(TALK_DEATH);

            if (auto const script = me->GetInstanceScript())
                script->SetData(TYPE_GEKKAN, DONE);
        }

        void KilledUnit(Unit* /*u*/)
        {
            Talk(TALK_KILLING);
        }

        void DoAction(const int32 action)
        {
            switch (action)
            {
                case ACTION_ADD_DEATH:
                    HandleAddDeath();
                    break;
                case ACTION_COMBAT:
                    DoZoneInCombat();
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_RECKLESS_INSPIRATION:
                        if (Creature* pAdd = GetRandomAliveAdd())
                            DoCast(pAdd, SPELL_RECKLESS_INSPIRATION, false);
                        events.ScheduleEvent(EVENT_RECKLESS_INSPIRATION, urand(15000, 21000));
                        Talk(TALK_SPELL);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

class mob_glintrok_skulker : public CreatureScript
{
public:
    mob_glintrok_skulker() : CreatureScript("mob_glintrok_skulker") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_glintrok_skulker_AI(creature);
    }

    enum eSpells
    {
        SPELL_SHANK        = 118963,
        SPELL_STEALTH      = 118969
    };

    struct mob_glintrok_skulker_AI : public ScriptedAI
    {
        mob_glintrok_skulker_AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        EventMap events;

        void EnterCombat(Unit* /*unit*/)
        {
            events.ScheduleEvent(1, 2000);

            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(1);
            }
        }

        void Reset()
        {
            DoCast(SPELL_STEALTH);
        }

        void JustDied(Unit* /*pKiller*/)
        {
            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(0);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case 1:
                        me->CastSpell(me->getVictim(), SPELL_SHANK, false);
                        events.ScheduleEvent(1, 7000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

class mob_glintrok_ironhide : public CreatureScript
{
public:
    mob_glintrok_ironhide() : CreatureScript("mob_glintrok_ironhide") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_glintrok_ironhide_AI(creature);
    }

    enum eSpells
    {
        SPELL_IRON_PROTECTOR        = 118958,
    };

    struct mob_glintrok_ironhide_AI : public ScriptedAI
    {
        mob_glintrok_ironhide_AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        EventMap events;

        void EnterCombat(Unit* /*unit*/)
        {
            events.ScheduleEvent(1, 2000);

            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(1);
            }
        }

        void JustDied(Unit* /*pKiller*/)
        {
            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(0);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case 1:
                        me->CastSpell(me, SPELL_IRON_PROTECTOR, true);
                        events.ScheduleEvent(1, 15000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

class mob_glintrok_oracle : public CreatureScript
{
public:
    mob_glintrok_oracle() : CreatureScript("mob_glintrok_oracle") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_glintrok_oracle_AI(creature);
    }

    enum eSpells
    {
        SPELL_CLEANSING_FLAME        = 118940,
        SPELL_FIRE_BOLT              = 118936,
    };

    struct mob_glintrok_oracle_AI : public ScriptedAI
    {
        mob_glintrok_oracle_AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        EventMap events;

        void EnterCombat(Unit* /*unit*/)
        {
            events.ScheduleEvent(1, 16000);
            events.ScheduleEvent(2, 4000);

            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(1);
            }
        }

        void JustDied(Unit* /*pKiller*/)
        {
            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(0);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case 1:
                        me->CastSpell(me, SPELL_CLEANSING_FLAME, false);
                        events.ScheduleEvent(1, 25000);
                        break;
                    case 2:
                        me->CastSpell(me->getVictim(), SPELL_FIRE_BOLT, false);
                        events.ScheduleEvent(2, 7000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

class mob_glintrok_hexxer : public CreatureScript
{
public:
    mob_glintrok_hexxer() : CreatureScript("mob_glintrok_hexxer") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_glintrok_hexxer_AI(creature);
    }

    enum eSpells
    {
        SPELL_HEX_OF_LETHARGY        = 118903,
        SPELL_DARK_BOLT              = 118917,
    };

    struct mob_glintrok_hexxer_AI : public ScriptedAI
    {
        mob_glintrok_hexxer_AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        EventMap events;

        void EnterCombat(Unit* /*unit*/)
        {
            events.ScheduleEvent(1, 10000);
            events.ScheduleEvent(2, 4000);

            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(1);
            }
        }

        void JustDied(Unit* /*pKiller*/)
        {
            if (Creature* pGekkan = GetClosestCreatureWithEntry(me, CREATURE_GEKKAN, 100.0f))
            {
                if (pGekkan->AI())
                    pGekkan->AI()->DoAction(0);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case 1:
                        me->CastSpell(me->getVictim(), SPELL_HEX_OF_LETHARGY, false);
                        events.ScheduleEvent(1, 20000);
                        break;
                    case 2:
                        me->CastSpell(me->getVictim(), SPELL_DARK_BOLT, false);
                        events.ScheduleEvent(2, 5000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_gekkan()
{
    new boss_gekkan();
    new mob_glintrok_hexxer();
    new mob_glintrok_skulker();
    new mob_glintrok_oracle();
    new mob_glintrok_ironhide();
}
