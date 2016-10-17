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
	SPELL_EARTHQUAKE = 41276,
	SPELL_SUNDER_ARMOR = 147042,
	SPELL_CHAIN_LIGHTNING = 147043,
	SPELL_OVERRUN = 23862,
	SPELL_ENRAGE = 27650,
	SPELL_MARK_DEATH = 23461,
	SPELL_AURA_DEATH = 40508
};

enum Events
{
	EVENT_ENRAGE = 1,
	EVENT_ARMOR = 2,
	EVENT_CHAIN = 3,
	EVENT_QUAKE = 4,
	EVENT_OVERRUN = 5
};

class boss_galakras : public CreatureScript
{
public:
	boss_galakras() : CreatureScript("boss_galakras") { }

	struct  boss_galakrasAI : public ScriptedAI
	{
		boss_galakrasAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8468)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
		return new boss_galakrasAI(creature);
	}
};

class mob_master_cannoneer_dagryn : public CreatureScript
{
    public:
        mob_master_cannoneer_dagryn() : CreatureScript("mob_master_cannoneer_dagryn") { }

        struct mob_master_cannoneer_dagrynAI : public ScriptedAI
        {
            mob_master_cannoneer_dagrynAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_master_cannoneer_dagrynAI(creature);
        }
};

class mob_high_enforcer_thranok : public CreatureScript
{
    public:
        mob_high_enforcer_thranok() : CreatureScript("mob_high_enforcer_thranok") { }

        struct mob_high_enforcer_thranokAI : public ScriptedAI
        {
            mob_high_enforcer_thranokAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_high_enforcer_thranokAI(creature);
        }
};

class mob_lieutenant_krugruk : public CreatureScript
{
    public:
        mob_lieutenant_krugruk() : CreatureScript("mob_lieutenant_krugruk") { }

        struct mob_lieutenant_krugrukAI : public ScriptedAI
        {
            mob_lieutenant_krugrukAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_lieutenant_krugrukAI(creature);
        }
};

class mob_korgra_the_snake: public CreatureScript
{
    public:
        mob_korgra_the_snake() : CreatureScript("mob_korgra_the_snake") { }

        struct mob_korgra_the_snakeAI : public ScriptedAI
        {
            mob_korgra_the_snakeAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_korgra_the_snakeAI(creature);
        }
};

class mob_dragonmaw_bonecrusher : public CreatureScript
{
    public:
        mob_dragonmaw_bonecrusher() : CreatureScript("mob_dragonmaw_bonecrusher") { }

        struct mob_dragonmaw_bonecrusherAI : public ScriptedAI
        {
            mob_dragonmaw_bonecrusherAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_dragonmaw_bonecrusherAI(creature);
        }
};

void AddSC_galakras()
{
    new boss_galakras();
    new mob_master_cannoneer_dagryn();
    new mob_high_enforcer_thranok();
    new mob_lieutenant_krugruk();
    new mob_korgra_the_snake();
    new mob_dragonmaw_bonecrusher();
}
