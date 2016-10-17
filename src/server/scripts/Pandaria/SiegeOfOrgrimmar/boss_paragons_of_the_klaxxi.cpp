#include "ScriptMgr.h"
#include "ScriptedCreature.h"


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
	SPELL_EARTHQUAKE = 117526,
	SPELL_SUNDER_ARMOR = 143701,
	SPELL_CHAIN_LIGHTNING = 143702,
	SPELL_OVERRUN = 143735,
	SPELL_ENRAGE = 143482,
	SPELL_MARK_DEATH = 143759,
	SPELL_AURA_DEATH = 40631
};

enum Events
{
	EVENT_ENRAGE = 1,
	EVENT_ARMOR = 2,
	EVENT_CHAIN = 3,
	EVENT_QUAKE = 4,
	EVENT_OVERRUN = 5
};

class boss_skeer_the_bloodseeker : public CreatureScript
{
public:
	boss_skeer_the_bloodseeker() : CreatureScript("boss_skeer_the_bloodseeker") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};


class boss_kilruk_the_wind_reaver : public CreatureScript
{
public:
	boss_kilruk_the_wind_reaver() : CreatureScript("boss_kilruk_the_wind_reaver") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 142232;
			int SPELL_SUNDER_ARMOR = 142922;
			int SPELL_CHAIN_LIGHTNING = 142931;
			int SPELL_OVERRUN = 142930;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 143939;
			int SPELL_AURA_DEATH = 143941;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};


class boss_iyyokuk_the_lucid : public CreatureScript
{
public:
	boss_iyyokuk_the_lucid() : CreatureScript("boss_iyyokuk_the_lucid") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 142668;
			int SPELL_SUNDER_ARMOR = 143765;
			int SPELL_CHAIN_LIGHTNING = 143768;
			int SPELL_OVERRUN = 143768;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 71928;
			int SPELL_AURA_DEATH = 40827;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};


class boss_rikkal_the_dissector : public CreatureScript
{
public:
	boss_rikkal_the_dissector() : CreatureScript("boss_rikkal_the_dissector") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 143278;
			int SPELL_SUNDER_ARMOR = 143336;
			int SPELL_CHAIN_LIGHTNING = 143337;
			int SPELL_OVERRUN = 143339;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 40936;
			int SPELL_AURA_DEATH = 40599;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};


class boss_xaril_the_poisoned_mind : public CreatureScript
{
public:
	boss_xaril_the_poisoned_mind() : CreatureScript("boss_xaril_the_poisoned_mind") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 143605;
			int SPELL_SUNDER_ARMOR = 143606;
			int SPELL_CHAIN_LIGHTNING = 143610;
			int SPELL_OVERRUN = 143616;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 143621;
			int SPELL_AURA_DEATH = 143627;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};

class boss_kaztik_the_manipulator : public CreatureScript
{
public:
	boss_kaztik_the_manipulator() : CreatureScript("boss_kaztik_the_manipulator") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 142668;
			int SPELL_SUNDER_ARMOR = 143765;
			int SPELL_CHAIN_LIGHTNING = 143768;
			int SPELL_OVERRUN = 71929;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 71928;
			int SPELL_AURA_DEATH = 40599;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};


class boss_korven_the_prime : public CreatureScript
{
public:
	boss_korven_the_prime() : CreatureScript("boss_korven_the_prime") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 142564;
			int SPELL_SUNDER_ARMOR = 143974;
			int SPELL_CHAIN_LIGHTNING = 143977;
			int SPELL_OVERRUN = 143979;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 72169;
			int SPELL_AURA_DEATH = 40599;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};


class boss_karoz_the_locust : public CreatureScript
{
public:
	boss_karoz_the_locust() : CreatureScript("boss_karoz_the_locust") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 143278;
			int SPELL_SUNDER_ARMOR = 143336;
			int SPELL_CHAIN_LIGHTNING = 143337;
			int SPELL_OVERRUN = 143339;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 40631;
			int SPELL_AURA_DEATH = 40599;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};

class boss_hisek_the_swarmkeeper : public CreatureScript
{
public:
	boss_hisek_the_swarmkeeper() : CreatureScript("boss_hisek_the_swarmkeeper") { }

	struct boss_skeer_the_bloodseekerAI : public ScriptedAI
	{
		boss_skeer_the_bloodseekerAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8481)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
			int SPELL_EARTHQUAKE = 142948;
			int SPELL_SUNDER_ARMOR = 143240;
			int SPELL_CHAIN_LIGHTNING = 143243;
			int SPELL_OVERRUN = 144839;
			int SPELL_ENRAGE = 143482;
			int SPELL_MARK_DEATH = 60073;
			int SPELL_AURA_DEATH = 40599;
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
		return new boss_skeer_the_bloodseekerAI(creature);
	}
};

void AddSC_paragons_of_the_klaxxi()
{
	//71152
	new boss_skeer_the_bloodseeker();
	//71161
	new boss_kilruk_the_wind_reaver();
	//71160
	new boss_iyyokuk_the_lucid();
	//71158
	new boss_rikkal_the_dissector();
	//71157
	new boss_xaril_the_poisoned_mind();
	//71156
	new boss_kaztik_the_manipulator();
	//71155
	new boss_korven_the_prime();
	//71154
	new boss_karoz_the_locust();
	//71153
	new boss_hisek_the_swarmkeeper();
}
