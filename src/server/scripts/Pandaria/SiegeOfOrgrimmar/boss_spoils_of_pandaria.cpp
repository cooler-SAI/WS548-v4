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
	SPELL_EARTHQUAKE = 8050,
	SPELL_SUNDER_ARMOR = 42650,
	SPELL_CHAIN_LIGHTNING = 114548,
	SPELL_OVERRUN = 51462,
	SPELL_ENRAGE = 27650,
	SPELL_MARK_DEATH = 51514,
	SPELL_AURA_DEATH = 60009
};

enum Events
{
	EVENT_ENRAGE = 1,
	EVENT_ARMOR = 2,
	EVENT_CHAIN = 3,
	EVENT_QUAKE = 4,
	EVENT_OVERRUN = 5
};

class boss_spoils_of_pandaria : public CreatureScript
{
public:
	boss_spoils_of_pandaria() : CreatureScript("boss_spoils_of_pandaria") { }

	struct boss_spoils_of_pandariaAI : public ScriptedAI
	{
		boss_spoils_of_pandariaAI(Creature* creature) : ScriptedAI(creature)
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
			if (killer) { if (killer->GetTypeId() == TYPEID_PLAYER)         if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(8478)) killer->ToPlayer()->CompletedAchievement(achievementEntry); }Talk(SAY_DEATH);
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
		return new boss_spoils_of_pandariaAI(creature);
	}
};

class mob_secured_stockpile_of_spoils : public CreatureScript
{
    public:
        mob_secured_stockpile_of_spoils() : CreatureScript("mob_secured_stockpile_of_spoils") { }

        struct mob_secured_stockpile_of_spoilsAI : public ScriptedAI
        {
            mob_secured_stockpile_of_spoilsAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_secured_stockpile_of_spoilsAI(creature);
        }
};

class mob_modified_anima_golem : public CreatureScript
{
    public:
        mob_modified_anima_golem() : CreatureScript("mob_modified_anima_golem") { }

        struct mob_modified_anima_golemAI : public ScriptedAI
        {
            mob_modified_anima_golemAI(Creature* creature) : ScriptedAI(creature)
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
        return new mob_modified_anima_golemAI(creature);
    }
};

class mob_mogu_shadow_ritualist : public CreatureScript
{
    public:
        mob_mogu_shadow_ritualist() : CreatureScript("mob_mogu_shadow_ritualist") { }

        struct mob_mogu_shadow_ritualistAI: public ScriptedAI
        {
            mob_mogu_shadow_ritualistAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_mogu_shadow_ritualistAI(creature);
        }
};

class mob_zarthik_amber_priest : public CreatureScript
{ 
    public:
        mob_zarthik_amber_priest() : CreatureScript("mob_zarthik_amber_priest") { }

        struct mob_zarthik_amber_priestAI : public ScriptedAI
        {
            mob_zarthik_amber_priestAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_zarthik_amber_priestAI(creature);
        }
};

class mob_setthik_wielder : public CreatureScript
{
    public:
        mob_setthik_wielder() : CreatureScript("mob_setthik_wielder") { }

        struct mob_setthik_wielderAI : public ScriptedAI
        {
            mob_setthik_wielderAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_setthik_wielderAI(creature);
        }
};

class mob_animated_stone_mogu : public CreatureScript
{
    public:
        mob_animated_stone_mogu() : CreatureScript("mob_animated_stone_mogu") { }

        struct mob_animated_stone_moguAI : public ScriptedAI
        {
            mob_animated_stone_moguAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_animated_stone_moguAI(creature);
        }
};

class mob_burial_urn : public CreatureScript
{
    public:
        mob_burial_urn() : CreatureScript("mob_burial_urn") { }

        struct mob_burial_urnAI : public ScriptedAI
        {
            mob_burial_urnAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_burial_urnAI(creature);
        }
};

class mob_quilen_gardians : public CreatureScript
{
    public:
        mob_quilen_gardians() : CreatureScript("mob_quilen_gardians") { }

        struct mob_quilen_gardiansAI : public ScriptedAI
        {
            mob_quilen_gardiansAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_quilen_gardiansAI(creature);
        }
};

class mob_srithik_bombardier : public CreatureScript
{
    public:
        mob_srithik_bombardier() : CreatureScript("mob_srithik_bombardier") { }

        struct mob_srithik_bombardierAI : public ScriptedAI
        {
            mob_srithik_bombardierAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_srithik_bombardierAI(creature);
        }
};

class mob_korthik_warcaller : public CreatureScript
{
    public:
        mob_korthik_warcaller() : CreatureScript("mob_korthik_warcaller") { }

        struct mob_korthik_warcallerAI : public ScriptedAI
        {
            mob_korthik_warcallerAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_korthik_warcallerAI(creature);
        }
};

class mob_ancient_breawmaster_spirit: public CreatureScript
{
    public:
        mob_ancient_breawmaster_spirit() : CreatureScript("mob_ancient_breawmaster_spirit") { }

        struct mob_ancient_breawmaster_spiritAI: public ScriptedAI
        {
            mob_ancient_breawmaster_spiritAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_ancient_breawmaster_spiritAI(creature);
        }
};

class mob_wise_misteweaver_spirit : public CreatureScript
{
    public:
        mob_wise_misteweaver_spirit() : CreatureScript("mob_wise_misteweaver_spirit") { }

        struct mob_wise_misteweaver_spiritAI : public ScriptedAI
        {
            mob_wise_misteweaver_spiritAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_wise_misteweaver_spiritAI(creature);
        }
};

class mob_namelesse_windwallker_spirit : public CreatureScript
{
    public:
        mob_namelesse_windwallker_spirit() : CreatureScript("mob_namelesse_windwallker_spirit") { } 

        struct mob_namelesse_windwallker_spiritAI : public ScriptedAI
        {
            mob_namelesse_windwallker_spiritAI(Creature* creature) : ScriptedAI(creature)
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
            return new mob_namelesse_windwallker_spiritAI(creature);
        }
};

void AddSC_spoils_of_pandaria()
{
	new boss_spoils_of_pandaria();
    new mob_secured_stockpile_of_spoils();
    new mob_modified_anima_golem();
    new mob_mogu_shadow_ritualist();
    new mob_zarthik_amber_priest();
    new mob_setthik_wielder();
    new mob_animated_stone_mogu();
    new mob_burial_urn();
    new mob_quilen_gardians();
    new mob_srithik_bombardier();
    new mob_korthik_warcaller();
    new mob_ancient_breawmaster_spirit();
    new mob_wise_misteweaver_spirit();
    new mob_namelesse_windwallker_spirit();
};