#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "siege_of_orgrimmar.h"

enum Texts
{
	SAY_AGGRO = 0,
	SAY_EARTHQUAKE = 1,
	SAY_OVERRUN = 2,
	SAY_SLAY = 3,
	SAY_DEATH = 4
};

enum Spells
{
	SPELL_EARTHQUAKE = 144089,
	SPELL_SUNDER_ARMOR = 144090,
	SPELL_CHAIN_LIGHTNING = 144215,
	SPELL_OVERRUN = 144328,
	SPELL_ENRAGE = 144302,
	SPELL_MARK_DEATH = 60011,
	SPELL_AURA_DEATH = 60016
};

enum Events
{
	EVENT_ENRAGE = 1,
	EVENT_ARMOR = 2,
	EVENT_CHAIN = 3,
	EVENT_QUAKE = 4,
	EVENT_OVERRUN = 5
};

class boss_earthbreaker_haromm : public CreatureScript
{
public:
	boss_earthbreaker_haromm() : CreatureScript("boss_earthbreaker_haromm") { }

	struct boss_earthbreaker_harommAI : public ScriptedAI
	{
		boss_earthbreaker_harommAI(Creature* creature) : ScriptedAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			_inEnrage = false;
		}

		void Reset() override
		{
			_events.Reset();
			_events.ScheduleEvent(EVENT_ENRAGE, 0);
			_events.ScheduleEvent(EVENT_ARMOR, urand(5000, 13000));
			_events.ScheduleEvent(EVENT_CHAIN, urand(10000, 30000));
			_events.ScheduleEvent(EVENT_QUAKE, urand(25000, 35000));
			_events.ScheduleEvent(EVENT_OVERRUN, urand(30000, 45000));
			Initialize();
		}

		void KilledUnit(Unit* victim) override
		{


			if (urand(0, 4))
				return;

			Talk(SAY_SLAY);
		}

		void JustDied(Unit* killer) override
		{
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8470)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			Talk(SAY_AGGRO);
		}

		void MoveInLineOfSight(Unit* who) override

		{



		}

		void UpdateAI(uint32 const diff) override
		{
			if (!UpdateVictim())
				return;

			_events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = _events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_ENRAGE:
					if (!HealthAbovePct(30))
					{
						DoCast(me, SPELL_ENRAGE);
						_events.ScheduleEvent(EVENT_ENRAGE, 6000);
						_inEnrage = true;
					}
					break;
				case EVENT_OVERRUN:
					Talk(SAY_OVERRUN);
					DoCastVictim(SPELL_OVERRUN);
					_events.ScheduleEvent(EVENT_OVERRUN, urand(25000, 40000));
					break;
				case EVENT_QUAKE:
					if (urand(0, 1))
						return;

					Talk(SAY_EARTHQUAKE);

					//remove enrage before casting earthquake because enrage + earthquake = 16000dmg over 8sec and all dead
					if (_inEnrage)
						me->RemoveAurasDueToSpell(SPELL_ENRAGE);

					DoCastVictim(SPELL_EARTHQUAKE);
					_events.ScheduleEvent(EVENT_QUAKE, urand(30000, 55000));
					break;
				case EVENT_CHAIN:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
						DoCast(target, SPELL_CHAIN_LIGHTNING);
					_events.ScheduleEvent(EVENT_CHAIN, urand(7000, 27000));
					break;
				case EVENT_ARMOR:
					Talk(irand(5, 12)); if (irand(0, 5) == 0) DoCastVictim(SPELL_MARK_DEATH); else if (irand(0, 5) == 1) DoCastVictim(SPELL_AURA_DEATH); else DoCastVictim(SPELL_SUNDER_ARMOR);
					_events.ScheduleEvent(EVENT_ARMOR, urand(10000, 25000));
					break;
				default:
					break;
				}
			}
			DoMeleeAttackIfReady();
		}

	private:
		EventMap _events;
		bool _inEnrage;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new boss_earthbreaker_harommAI(creature);
	}
};

class boss_bloodclaw : public CreatureScript
{
    public:
        boss_bloodclaw() : CreatureScript("boss_bloodclaw") { }

        struct boss_bloodclawAI : public BossAI
        {
            boss_bloodclawAI(Creature* creature) : BossAI(creature, DATA_KORKRON_DARK_SHAMANS_EVENT)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_bloodclawAI(creature);
        }
};

class mob_darkfang : public CreatureScript
{
    public:
        mob_darkfang() : CreatureScript("mob_darkfang") { }

        struct mob_darkfangAI : public ScriptedAI
        {
            mob_darkfangAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

		    void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_darkfangAI(creature);
        }
};

class mob_foul_slime : public CreatureScript
{
    public:
        mob_foul_slime() : CreatureScript("mob_foul_slime") { }

        struct mob_foul_slimeAI : public ScriptedAI
        {
            mob_foul_slimeAI(Creature* pCreature) : ScriptedAI(pCreature)
            {
                pInstance = pCreature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
            }
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new mob_foul_slimeAI(pCreature);
        }
};

void AddSC_korkron_dark_shamans()
{
    new boss_earthbreaker_haromm();
    new boss_bloodclaw();
    new mob_darkfang();
    new mob_foul_slime();
}
