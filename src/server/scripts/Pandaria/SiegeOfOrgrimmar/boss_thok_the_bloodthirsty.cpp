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
	SPELL_EARTHQUAKE = 143780,
	SPELL_SUNDER_ARMOR = 143773,
	SPELL_CHAIN_LIGHTNING = 143767,
	SPELL_OVERRUN = 143428,
	SPELL_ENRAGE = 149569,
	SPELL_MARK_DEATH = 143426,
	SPELL_AURA_DEATH = 143343
};

enum Events
{
	EVENT_ENRAGE = 1,
	EVENT_ARMOR = 2,
	EVENT_CHAIN = 3,
	EVENT_QUAKE = 4,
	EVENT_OVERRUN = 5
};

class boss_thok_the_bloodthirsty : public CreatureScript
{
public:
	boss_thok_the_bloodthirsty() : CreatureScript("boss_thok_the_bloodthirsty") { }

	struct boss_thok_the_bloodthirstyAI : public ScriptedAI
	{
		boss_thok_the_bloodthirstyAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8479)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
		return new boss_thok_the_bloodthirstyAI(creature);
	}
};

class mob_korkron_jailer : public CreatureScript
{
    public:
        mob_korkron_jailer() : CreatureScript("mob_korkron_jailer") { }

        struct mob_korkron_jailerAI : public ScriptedAI
        {
            mob_korkron_jailerAI(Creature* creature) : ScriptedAI(creature)
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
        return new mob_korkron_jailerAI(creature);
        }
};

class mob_akolik : public CreatureScript
{
    public:
        mob_akolik() : CreatureScript("mob_akolik") { }

        struct mob_akolikAI : public ScriptedAI
        {
            mob_akolikAI(Creature* creature) : ScriptedAI(creature)
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
        return new mob_akolikAI(creature);
        }
};

class mob_waterspeaker_gorai : public CreatureScript
{
    public:
        mob_waterspeaker_gorai() : CreatureScript("mob_waterspeaker_gorai") { }

        struct mob_waterspeaker_goraiAI : public ScriptedAI
        {
            mob_waterspeaker_goraiAI(Creature* creature) : ScriptedAI(creature)
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
        return new mob_waterspeaker_goraiAI(creature);
        }
};

class mob_montak : public CreatureScript
{
    public:
        mob_montak() : CreatureScript("mob_montak") { }

        struct mob_montakAI : public ScriptedAI
        {
            mob_montakAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_montakAI(creature);
        }
};

void AddSC_thok_the_bloodthirsty()
{
    new boss_thok_the_bloodthirsty();
    new mob_korkron_jailer();
    new mob_akolik();
    new mob_waterspeaker_gorai();
    new mob_montak();
};