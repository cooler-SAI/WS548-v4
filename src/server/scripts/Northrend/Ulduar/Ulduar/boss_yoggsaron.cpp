/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PassiveAI.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "MoveSplineInit.h"
#include "CreatureTextMgr.h"
#include "ulduar.h"

enum Yells
{
    // Sara
    SAY_SARA_ULDUAR_SCREAM_1                = 0,  // screams randomly in a whole instance, unused on retail
    SAY_SARA_ULDUAR_SCREAM_2                = 1,  // screams randomly in a whole instance, unused on retail
    SAY_SARA_AGGRO                          = 2,
    SAY_SARA_FERVOR_HIT                     = 3,
    SAY_SARA_BLESSING_HIT                   = 4,
    SAY_SARA_KILL                           = 5,
    SAY_SARA_TRANSFORM_1                    = 6,
    SAY_SARA_TRANSFORM_2                    = 7,
    SAY_SARA_TRANSFORM_3                    = 8,
    SAY_SARA_TRANSFORM_4                    = 9,
    SAY_SARA_DEATH_RAY                      = 10,
    SAY_SARA_PSYCHOSIS_HIT                  = 11,

    // Yogg-Saron
    SAY_YOGG_SARON_SPAWN                    = 0,
    SAY_YOGG_SARON_MADNESS                  = 1,
    EMOTE_YOGG_SARON_MADNESS                = 2,
    SAY_YOGG_SARON_PHASE_3                  = 3,
    SAY_YOGG_SARON_DEAFENING_ROAR           = 4,
    EMOTE_YOGG_SARON_DEAFENING_ROAR         = 5,
    SAY_YOGG_SARON_DEATH                    = 6,
    EMOTE_YOGG_SARON_EMPOWERING_SHADOWS     = 7,
    EMOTE_YOGG_SARON_EXTINGUISH_ALL_LIFE    = 8,

    // Voice of Yogg-Saron
    WHISPER_VOICE_PHASE_1_WIPE              = 0,
    WHISPER_VOICE_INSANE                    = 1,

    // Brain of Yogg-Saron
    EMOTE_BRAIN_ILLUSION_SHATTERED          = 0,

    // Ominous Cloud
    EMOTE_OMINOUS_CLOUD_PLAYER_TOUCH        = 0,

    // Keepers
    SAY_KEEPER_CHOSEN_1                     = 0,
    SAY_KEEPER_CHOSEN_2                     = 1,

    // Yogg-Saron illusions
    SAY_STORMWIND_ROLEPLAY_4                = 0,
    SAY_STORMWIND_ROLEPLAY_7                = 1,
    SAY_ICECROWN_ROLEPLAY_5                 = 2,
    SAY_ICECROWN_ROLEPLAY_6                 = 3,
    SAY_CHAMBER_ROLEPLAY_5                  = 4,

    // Neltharion
    SAY_CHAMBER_ROLEPLAY_1                  = 0,
    SAY_CHAMBER_ROLEPLAY_3                  = 1,

    // Ysera
    SAY_CHAMBER_ROLEPLAY_2                  = 0,

    // Malygos
    SAY_CHAMBER_ROLEPLAY_4                  = 0,

    // Immolated Champion
    SAY_ICECROWN_ROLEPLAY_1                 = 0,
    SAY_ICECROWN_ROLEPLAY_3                 = 1,

    // The Lich King
    SAY_ICECROWN_ROLEPLAY_2                 = 0,
    SAY_ICECROWN_ROLEPLAY_4                 = 1,

    // Garona
    SAY_STORMWIND_ROLEPLAY_1                = 0,
    SAY_STORMWIND_ROLEPLAY_2                = 1,
    SAY_STORMWIND_ROLEPLAY_3                = 2,
    SAY_STORMWIND_ROLEPLAY_6                = 3,

    // King Llane
    SAY_STORMWIND_ROLEPLAY_5                = 0,
};

enum Spells
{
    // Voice of Yogg-Saron
    SPELL_SUMMON_GUARDIAN_2                 = 62978,
    SPELL_SANITY_PERIODIC                   = 63786,
    SPELL_SANITY                            = 63050,
    SPELL_INSANE_PERIODIC                   = 64554,
    SPELL_INSANE                            = 63120,
    //SPELL_CLEAR_INSANE                      = 63122,  // when should it be cast?
    SPELL_CONSTRICTOR_TENTACLE              = 64132,
    SPELL_CRUSHER_TENTACLE_SUMMON           = 64139,
    SPELL_CORRUPTOR_TENTACLE_SUMMON         = 64143,
    SPELL_IMMORTAL_GUARDIAN                 = 64158,

    // Sara
    SPELL_SARAS_FERVOR                      = 63138,
    SPELL_SARAS_FERVOR_TARGET_SELECTOR      = 63747,
    SPELL_SARAS_BLESSING                    = 63134,
    SPELL_SARAS_BLESSING_TARGET_SELECTOR    = 63745,
    SPELL_SARAS_ANGER                       = 63147,
    SPELL_SARAS_ANGER_TARGET_SELECTOR       = 63744,
    SPELL_FULL_HEAL                         = 43978,
    SPELL_PHASE_2_TRANSFORM                 = 65157,
    SPELL_SHADOWY_BARRIER_SARA              = 64775,
    SPELL_RIDE_YOGG_SARON_VEHICLE           = 61791,
    SPELL_PSYCHOSIS                         = 63795,
    SPELL_MALADY_OF_THE_MIND                = 63830,
    SPELL_BRAIN_LINK                        = 63802,
    SPELL_BRAIN_LINK_DAMAGE                 = 63803,  // red beam
    SPELL_BRAIN_LINK_NO_DAMAGE              = 63804,  // yellow beam
    SPELL_DEATH_RAY                         = 63891,

    // Ominous Cloud
    SPELL_OMINOUS_CLOUD_VISUAL              = 63084,
    SPELL_SUMMON_GUARDIAN_1                 = 63031,

    // Guardian of Yogg-Saron
    SPELL_DARK_VOLLEY                       = 63038,
    SPELL_SHADOW_NOVA                       = 62714,
    SPELL_SHADOW_NOVA_2                     = 65719,

    // Yogg-Saron
    SPELL_EXTINGUISH_ALL_LIFE               = 64166,
    SPELL_SHADOWY_BARRIER_YOGG              = 63894,
    SPELL_KNOCK_AWAY                        = 64022,
    SPELL_PHASE_3_TRANSFORM                 = 63895,
    SPELL_DEAFENING_ROAR                    = 64189,
    SPELL_LUNATIC_GAZE                      = 64163,
    SPELL_LUNATIC_GAZE_DAMAGE               = 64164,
    SPELL_SHADOW_BEACON                     = 64465,

    // Brain of Yogg-Saron
    SPELL_MATCH_HEALTH                      = 64066,
    SPELL_MATCH_HEALTH_2                    = 64069,
    SPELL_INDUCE_MADNESS                    = 64059,
    SPELL_BRAIN_HURT_VISUAL                 = 64361,
    SPELL_SHATTERED_ILLUSION                = 64173,
    SPELL_SHATTERED_ILLUSION_REMOVE         = 65238,

    // Tentacles
    SPELL_ERUPT                             = 64144,
    SPELL_TENTACLE_VOID_ZONE                = 64017,  // used by Corruptor Tentacle and Crusher Tentacle only

    // Crusher Tentacle
    SPELL_DIMINISH_POWER                    = 64145,
    SPELL_DIMINSH_POWER                     = 64148,
    SPELL_FOCUSED_ANGER                     = 57688,
    SPELL_CRUSH                             = 64146,
    //SPELL_CRUSH_2                           = 65201,  // triggered by SPELL_CRUSH, basepoints of SPELL_MALADY_OF_THE_MIND

    // Constrictor Tentacle
    SPELL_TENTACLE_VOID_ZONE_2              = 64384,
    SPELL_LUNGE                             = 64131,

    // Corruptor Tentacle
    SPELL_APATHY                            = 64156,
    SPELL_BLACK_PLAGUE                      = 64153,
    SPELL_CURSE_OF_DOOM                     = 64157,
    SPELL_DRAINING_POISON                   = 64152,

    // Immortal Guardian
    SPELL_EMPOWERING_SHADOWS                = 64468,
    SPELL_EMPOWERED                         = 64161,
    SPELL_EMPOWERED_BUFF                    = 65294,
    SPELL_WEAKENED                          = 64162,
    SPELL_DRAIN_LIFE                        = 64159,
    SPELL_RECENTLY_SPAWNED                  = 64497,
    SPELL_SIMPLE_TELEPORT                   = 64195,

    // Keepers at Observation Ring
    SPELL_TELEPORT                          = 62940,

    // Keepers
    SPELL_SIMPLE_TELEPORT_KEEPERS           = 12980,
    SPELL_KEEPER_ACTIVE                     = 62647,

    // Mimiron
    SPELL_SPEED_OF_INVENTION                = 62671,
    SPELL_DESTABILIZATION_MATRIX            = 65206,

    // Freya
    SPELL_RESILIENCE_OF_NATURE              = 62670,
    SPELL_SANITY_WELL_SUMMON                = 64170,

    // Sanity Well
    SPELL_SANITY_WELL_VISUAL                = 63288,
    SPELL_SANITY_WELL                       = 64169,

    // Thorim
    SPELL_FURY_OF_THE_STORM                 = 62702,
    SPELL_TITANIC_STORM                     = 64171,

    // Hodir
    SPELL_FORTITUDE_OF_FROST                = 62650,
    SPELL_HODIRS_PROTECTIVE_GAZE            = 64174,
    SPELL_FLASH_FREEZE_VISUAL               = 64176,

    // Death Orb
    SPELL_DEATH_RAY_ORIGIN_VISUAL           = 63893,

    // Death Ray
    SPELL_DEATH_RAY_WARNING_VISUAL          = 63882,
    SPELL_DEATH_RAY_PERIODIC                = 63883,
    SPELL_DEATH_RAY_DAMAGE_VISUAL           = 63886,

    // Laughing Skull
    SPELL_LUNATIC_GAZE_SKULL                = 64167,

    // Descend Into Madness
    SPELL_TELEPORT_PORTAL_VISUAL            = 64416,

    // Illusions
    SPELL_GRIM_REPRISAL                     = 63305,
    SPELL_GRIM_REPRISAL_DAMAGE              = 64039,

    // Suit of Armor
    SPELL_NONDESCRIPT_1                     = 64013,

    // Dragon Consorts & Deathsworn Zealot
    SPELL_NONDESCRIPT_2                     = 64010,

    // Garona
    SPELL_ASSASSINATE                       = 64063,

    // King Llane
    SPELL_PERMANENT_FEIGN_DEATH             = 29266,

    // The Lich King
    SPELL_DEATHGRASP                        = 63037,

    // Turned Champion
    SPELL_VERTEX_COLOR_BLACK                = 39662,

    // Player self cast spells
    SPELL_MALADY_OF_THE_MIND_JUMP           = 63881,
    SPELL_ILLUSION_ROOM                     = 63988,
    SPELL_HATE_TO_ZERO                      = 63984,
    SPELL_TELEPORT_BACK_TO_MAIN_ROOM        = 63992,
    SPELL_INSANE_VISUAL                     = 64464,
    SPELL_CONSTRICTOR_TENTACLE_SUMMON       = 64133,
    SPELL_SQUEEZE                           = 64125,
    SPELL_FLASH_FREEZE                      = 64175,
    SPELL_LOW_SANITY_SCREEN_EFFECT          = 63752,

    SPELL_IN_THE_MAWS_OF_THE_OLD_GOD        = 64184,
};

enum Phases
{
    PHASE_ONE               = 1,
    PHASE_TRANSFORM         = 2,
    PHASE_TWO               = 3,
    PHASE_THREE             = 4,
};

enum Events
{
    // Voice of Yogg-Saron
    EVENT_LOCK_DOOR                         = 1,
    EVENT_SUMMON_GUARDIAN_OF_YOGG_SARON     = 2,
    EVENT_SUMMON_CORRUPTOR_TENTACLE         = 3,
    EVENT_SUMMON_CONSTRICTOR_TENTACLE       = 4,
    EVENT_SUMMON_CRUSHER_TENTACLE           = 5,
    EVENT_ILLUSION                          = 6,
    EVENT_SUMMON_IMMORTAL_GUARDIAN          = 7,
    EVENT_EXTINGUISH_ALL_LIFE               = 8,    // handled by Voice, timer starts at the beginning of the fight (Yogg-Saron is not spawned at this moment)

    // Sara
    EVENT_SARAS_FERVOR                      = 9,
    EVENT_SARAS_BLESSING                    = 10,
    EVENT_SARAS_ANGER                       = 11,
    EVENT_TRANSFORM_1                       = 12,
    EVENT_TRANSFORM_2                       = 13,
    EVENT_TRANSFORM_3                       = 14,
    EVENT_TRANSFORM_4                       = 15,
    EVENT_PSYCHOSIS                         = 16,
    EVENT_MALADY_OF_THE_MIND                = 17,
    EVENT_BRAIN_LINK                        = 18,
    EVENT_DEATH_RAY                         = 19,

    // Tentacles
    EVENT_DIMINISH_POWER                    = 20,
    EVENT_CAST_RANDOM_SPELL                 = 21,

    // Yogg-Saron
    EVENT_YELL_BOW_DOWN                     = 22,
    EVENT_SHADOW_BEACON                     = 23,
    EVENT_LUNATIC_GAZE                      = 24,
    EVENT_DEAFENING_ROAR                    = 25,  // only on 25-man with 0-3 keepers active (Hard Mode)

    // Guardian of Yogg-Saron
    EVENT_DARK_VOLLEY                       = 26,

    // Immortal Guardian
    EVENT_DRAIN_LIFE                        = 27,

    // Keepers
    EVENT_DESTABILIZATION_MATRIX            = 28,
    EVENT_HODIRS_PROTECTIVE_GAZE            = 29,

    // Chamber Illusion
    EVENT_CHAMBER_ROLEPLAY_1                = 30,
    EVENT_CHAMBER_ROLEPLAY_2                = 31,
    EVENT_CHAMBER_ROLEPLAY_3                = 32,
    EVENT_CHAMBER_ROLEPLAY_4                = 33,
    EVENT_CHAMBER_ROLEPLAY_5                = 34,

    // Icecrown Illusion
    EVENT_ICECROWN_ROLEPLAY_1               = 35,
    EVENT_ICECROWN_ROLEPLAY_2               = 36,
    EVENT_ICECROWN_ROLEPLAY_3               = 37,
    EVENT_ICECROWN_ROLEPLAY_4               = 38,
    EVENT_ICECROWN_ROLEPLAY_5               = 39,
    EVENT_ICECROWN_ROLEPLAY_6               = 40,

    // Stormwind Illusion
    EVENT_STORMWIND_ROLEPLAY_1              = 41,
    EVENT_STORMWIND_ROLEPLAY_2              = 42,
    EVENT_STORMWIND_ROLEPLAY_3              = 43,
    EVENT_STORMWIND_ROLEPLAY_4              = 44,
    EVENT_STORMWIND_ROLEPLAY_5              = 45,
    EVENT_STORMWIND_ROLEPLAY_6              = 46,
    EVENT_STORMWIND_ROLEPLAY_7              = 47,
};

enum EventGroups
{
    EVENT_GROUP_SUMMON_TENTACLES            = 1,
};

enum Actions
{
    ACTION_PHASE_TRANSFORM              = 0,
    ACTION_PHASE_TWO                    = 1,
    ACTION_PHASE_THREE                  = 2,
    ACTION_INDUCE_MADNESS               = 3,
    ACTION_SANITY_WELLS                 = 4,
    ACTION_FLASH_FREEZE                 = 5,
    ACTION_TENTACLE_KILLED              = 6,
    ACTION_START_ROLEPLAY               = 8,
    ACTION_TOGGLE_SHATTERED_ILLUSION    = 9,
};

enum CreatureGroups
{
    CREATURE_GROUP_CLOUDS       = 0,
    CREATURE_GROUP_PORTALS_10   = 1,
    CREATURE_GROUP_PORTALS_25   = 2,
};

Position const YoggSaronSpawnPos            = {1980.43f, -25.7708f, 324.9724f, 3.141593f};
Position const ObservationRingKeepersPos[4] =
{
    {1945.682f,  33.34201f, 411.4408f, 5.270895f},  // Freya
    {1945.761f, -81.52171f, 411.4407f, 1.029744f},  // Hodir
    {2028.822f, -65.73573f, 411.4426f, 2.460914f},  // Thorim
    {2028.766f,  17.42014f, 411.4446f, 3.857178f},  // Mimiron
};
Position const YSKeepersPos[4] =
{
    {2036.873f,  25.42513f, 338.4984f, 3.909538f},  // Freya
    {1939.045f, -90.87457f, 338.5426f, 0.994837f},  // Hodir
    {1939.148f,  42.49035f, 338.5427f, 5.235988f},  // Thorim
    {2036.658f, -73.58822f, 338.4985f, 2.460914f},  // Mimiron
};
Position const IllusionsMiscPos[2] =
{
    {1928.793f,  65.03109f, 242.3763f, 0.0f}, // Garona end position
    {1912.324f, -155.7967f, 239.9896f, 0.0f}, // Saurfang end position
};

enum MiscData
{
    ACHIEV_TIMED_START_EVENT                = 21001,
    SOUND_LUNATIC_GAZE                      = 15757,
};

class StartAttackEvent : public BasicEvent
{
    public:
        StartAttackEvent(Creature* summoner, Creature* owner)
            : _summoner(summoner), _owner(owner)
        {
        }

        bool Execute(uint64 /*time*/, uint32 /*diff*/)
        {
            _owner->SetReactState(REACT_AGGRESSIVE);
            return true;
        }

    private:
        Creature* _summoner;
        Creature* _owner;
};

class boss_voice_of_yogg_saron : public CreatureScript
{
    public:
        boss_voice_of_yogg_saron() : CreatureScript("boss_voice_of_yogg_saron") { }

        struct boss_voice_of_yogg_saronAI : public BossAI
        {

            boss_voice_of_yogg_saronAI(Creature* creature) : BossAI(creature, BOSS_YOGG_SARON)
            {
                Initialize();
                SetCombatMovement(false);
            }

            void Initialize()
            {
                _guardiansCount = 0;
                _guardianTimer = 20000;
                _illusionShattered = false;
            }

            void MoveInLineOfSight(Unit* who) 

            {
                // TODO: MoveInLineOfSight doesn't work for such a big distance
                //if (who->GetTypeId() == TYPEID_PLAYER && me->GetDistance2d(who) < 99.0f && !me->IsInCombat())
                    me->SetInCombatWithZone();
            }

          

            void Reset() 
            {
                _Reset();
                events.SetPhase(PHASE_ONE);

                instance->SetData(DATA_DRIVE_ME_CRAZY, uint32(true));
                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);

                Initialize();

            }

            void EnterCombat(Unit* /*who*/)  
            {
            	Talk(SAY_YOGG_SARON_SPAWN);
            	

                for (uint8 i = DATA_FREYA_YS; i <= DATA_MIMIRON_YS; ++i)


                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);

                me->CastCustomSpell(SPELL_SUMMON_GUARDIAN_2, SPELLVALUE_MAX_TARGETS, 1);
                DoCast(me, SPELL_SANITY_PERIODIC);

                events.ScheduleEvent(EVENT_LOCK_DOOR, 15000);
                events.ScheduleEvent(EVENT_SUMMON_GUARDIAN_OF_YOGG_SARON, _guardianTimer, 0, PHASE_ONE);
                events.ScheduleEvent(EVENT_EXTINGUISH_ALL_LIFE, 900000);    // 15 minutes
            }

            void JustDied(Unit* killer) 
            {
                


             Talk(SAY_YOGG_SARON_DEATH);

			 // don't despawn Yogg-Saron's corpse, remove him from SummonList!



                BossAI::JustDied(killer);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);


                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
if (irand(0,10)==0)
{
            	if (irand(0,20)==5 )
{

                	DoCastVictim(40904);
 return;
}
 else                	if (irand(0,20)==6 )
{

                	DoCastVictim(41238);
 return;
}
else                	if (irand(0,20)==4 )
                	{
                	Talk(EMOTE_YOGG_SARON_EMPOWERING_SHADOWS);
                	DoCastVictim(64456);
 return;
}
else                	if (irand(0,20)==0 )
                	               	{
                	DoCastVictim(64022);
 return;
}
else                	if (irand(0,20)==1 )
                	               	               	{
                	DoCastVictim(64163);
 return;
}
else if (irand(0,20)==2 )
 {
                	DoCastVictim(64164);
 return;
}
 else                	if (irand(0,20)==3 )
{
                            Talk(SAY_YOGG_SARON_DEAFENING_ROAR);
                            Talk(EMOTE_YOGG_SARON_DEAFENING_ROAR);
                	DoCastVictim(64189);
 return;
}

}

              // don't summon tentacles when illusion is shattered, delay them
                if (_illusionShattered)
                   events.DelayEvents(diff, EVENT_GROUP_SUMMON_TENTACLES);


                while (uint32 eventId = events.ExecuteEvent())
                {

 
                    switch (eventId)
                    {
                        case EVENT_LOCK_DOOR:
                            DoCast(me, SPELL_INSANE_PERIODIC);
                            instance->SetBossState(BOSS_YOGG_SARON, IN_PROGRESS);
                            break;
                        case EVENT_EXTINGUISH_ALL_LIFE:

                            events.ScheduleEvent(EVENT_EXTINGUISH_ALL_LIFE, 10000);    // cast it again after a short while, players can survive
                            break;
                        case EVENT_SUMMON_GUARDIAN_OF_YOGG_SARON:
                            me->CastCustomSpell(SPELL_SUMMON_GUARDIAN_2, SPELLVALUE_MAX_TARGETS, 1);
                            ++_guardiansCount;
                            if (_guardiansCount <= 6 && _guardiansCount % 3 == 0)
                                _guardianTimer -= 5000;
                            events.ScheduleEvent(EVENT_SUMMON_GUARDIAN_OF_YOGG_SARON, _guardianTimer, 0, PHASE_ONE);
                            break;
                        case EVENT_SUMMON_CORRUPTOR_TENTACLE:
                            DoCastAOE(SPELL_CORRUPTOR_TENTACLE_SUMMON);
                            events.ScheduleEvent(EVENT_SUMMON_CORRUPTOR_TENTACLE, 30000, EVENT_GROUP_SUMMON_TENTACLES, PHASE_TWO);
                            break;
                        case EVENT_SUMMON_CONSTRICTOR_TENTACLE:
                            me->CastCustomSpell(SPELL_CONSTRICTOR_TENTACLE, SPELLVALUE_MAX_TARGETS, 1);
                            events.ScheduleEvent(EVENT_SUMMON_CONSTRICTOR_TENTACLE, 25000, EVENT_GROUP_SUMMON_TENTACLES, PHASE_TWO);
                            break;
                        case EVENT_SUMMON_CRUSHER_TENTACLE:
                            DoCastAOE(SPELL_CRUSHER_TENTACLE_SUMMON);
                            events.ScheduleEvent(EVENT_SUMMON_CRUSHER_TENTACLE, 60000, EVENT_GROUP_SUMMON_TENTACLES, PHASE_TWO);
                            break;
                        case EVENT_ILLUSION:
                        {

                            events.ScheduleEvent(EVENT_ILLUSION, 80000, 0, PHASE_TWO);  // wowwiki says 80 secs, wowhead says something about 90 secs
                            break;
                        }
                        case EVENT_SUMMON_IMMORTAL_GUARDIAN:
                            DoCastAOE(SPELL_IMMORTAL_GUARDIAN);
                            events.ScheduleEvent(EVENT_SUMMON_IMMORTAL_GUARDIAN, 15000, 0, PHASE_THREE);
                            break;
                        default:
                            break;
                    }
                DoMeleeAttackIfReady();
                }
               DoMeleeAttackIfReady(); 
            }

            void DoAction(int32 const action) 
            {
                switch (action)
                {
                    case ACTION_PHASE_TRANSFORM:
                        events.SetPhase(PHASE_TRANSFORM);

                        break;
                    case ACTION_PHASE_TWO:
                        events.SetPhase(PHASE_TWO);

                        events.ScheduleEvent(EVENT_SUMMON_CORRUPTOR_TENTACLE, 1, EVENT_GROUP_SUMMON_TENTACLES, PHASE_TWO);
                        events.ScheduleEvent(EVENT_SUMMON_CONSTRICTOR_TENTACLE, 1, EVENT_GROUP_SUMMON_TENTACLES, PHASE_TWO);
                        events.ScheduleEvent(EVENT_SUMMON_CRUSHER_TENTACLE, 1, EVENT_GROUP_SUMMON_TENTACLES, PHASE_TWO);
                        events.ScheduleEvent(EVENT_ILLUSION, 60000, 0, PHASE_TWO);
                        break;
                    case ACTION_TOGGLE_SHATTERED_ILLUSION:
                        _illusionShattered = !_illusionShattered;
                        break;
                    case ACTION_PHASE_THREE:
                        events.SetPhase(PHASE_THREE);
                        events.ScheduleEvent(EVENT_SUMMON_IMMORTAL_GUARDIAN, 1000, 0, PHASE_THREE);
                        break;
                    default:
                        break;
                }
           // DoMeleeAttackIfReady(); 
            }

            void JustSummoned(Creature* summon) 
            {
                

                BossAI::JustSummoned(summon);
            }

        private:
            uint8 _guardiansCount;
            uint32 _guardianTimer;
            bool _illusionShattered;
        };

        CreatureAI* GetAI(Creature* creature) const 
        {
            return GetUlduarAI<boss_voice_of_yogg_saronAI>(creature);
        }
};

class boss_sara : public CreatureScript
{
    public:
        boss_sara() : CreatureScript("boss_sara") { }

        struct boss_saraAI : public ScriptedAI
        {
            boss_saraAI(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()) { }




            // called once for each target on aura remove


            void SpellHitTarget(Unit* /*target*/, SpellInfo const* spell) 
            {
                if (!roll_chance_i(30) || _events.IsInPhase(PHASE_TRANSFORM))
                    return;

                switch (spell->Id)
                {
                    case SPELL_SARAS_FERVOR:
                        Talk(SAY_SARA_FERVOR_HIT);
                        break;
                    case SPELL_SARAS_BLESSING:
                        Talk(SAY_SARA_BLESSING_HIT);
                        break;
                    case SPELL_PSYCHOSIS:
                        Talk(SAY_SARA_PSYCHOSIS_HIT);
                        break;
                    default:
                        break;
                }
            }

            void KilledUnit(Unit* victim) 
            {
                if (victim->GetTypeId() == TYPEID_PLAYER && !me->IsInEvadeMode())
                    Talk(SAY_SARA_KILL);
            }

            void EnterCombat(Unit* /*who*/) 
            {
                Talk(SAY_SARA_AGGRO);
                _events.ScheduleEvent(EVENT_SARAS_FERVOR, 5000, 0, PHASE_ONE);
                _events.ScheduleEvent(EVENT_SARAS_BLESSING, urand(10000, 30000), 0, PHASE_ONE);
                _events.ScheduleEvent(EVENT_SARAS_ANGER, urand(15000, 25000), 0, PHASE_ONE);
            }

            void Reset() 
            {
                me->RemoveAllAuras();
                me->SetReactState(REACT_PASSIVE);
                me->setFaction(35);
                _events.Reset();
                _events.SetPhase(PHASE_ONE);
            }

            void UpdateAI(uint32 const diff)
            {


                if (me->HasAura(SPELL_SHATTERED_ILLUSION))
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SARAS_FERVOR:
                            me->CastCustomSpell(SPELL_SARAS_FERVOR_TARGET_SELECTOR, SPELLVALUE_MAX_TARGETS, 1);
                            _events.ScheduleEvent(EVENT_SARAS_FERVOR, 6000, 0, PHASE_ONE);
                            break;
                        case EVENT_SARAS_ANGER:
                            me->CastCustomSpell(SPELL_SARAS_ANGER_TARGET_SELECTOR, SPELLVALUE_MAX_TARGETS, 1);
                            _events.ScheduleEvent(EVENT_SARAS_ANGER, urand(6000, 8000), 0, PHASE_ONE);
                            break;
                        case EVENT_SARAS_BLESSING:
                            me->CastCustomSpell(SPELL_SARAS_BLESSING_TARGET_SELECTOR, SPELLVALUE_MAX_TARGETS, 1);
                            _events.ScheduleEvent(EVENT_SARAS_BLESSING, urand(6000, 30000), 0, PHASE_ONE);
                            break;
                        case EVENT_TRANSFORM_1:
                            Talk(SAY_SARA_TRANSFORM_2);
                            break;
                        case EVENT_TRANSFORM_2:
                            Talk(SAY_SARA_TRANSFORM_3);
                            break;
                        case EVENT_TRANSFORM_3:
                            Talk(SAY_SARA_TRANSFORM_4);
                            DoCast(me, SPELL_FULL_HEAL);
                            me->setFaction(16);
                            break;
                        case EVENT_TRANSFORM_4:
                            DoCast(me, SPELL_PHASE_2_TRANSFORM);
                            DoCast(me, SPELL_SHADOWY_BARRIER_SARA);
                            _events.SetPhase(PHASE_TWO);
                            _events.ScheduleEvent(EVENT_DEATH_RAY, 20000, 0, PHASE_TWO);    // almost never cast at scheduled time, why?
                            _events.ScheduleEvent(EVENT_MALADY_OF_THE_MIND, 18000, 0, PHASE_TWO);
                            _events.ScheduleEvent(EVENT_PSYCHOSIS, 1, 0, PHASE_TWO);
                            _events.ScheduleEvent(EVENT_BRAIN_LINK, 23000, 0, PHASE_TWO);
                            break;
                        case EVENT_DEATH_RAY:
                            DoCast(me, SPELL_DEATH_RAY);
                            _events.ScheduleEvent(EVENT_DEATH_RAY, 21000, 0, PHASE_TWO);
                            break;
                        case EVENT_MALADY_OF_THE_MIND:
                            me->CastCustomSpell(SPELL_MALADY_OF_THE_MIND, SPELLVALUE_MAX_TARGETS, 1);
                            _events.ScheduleEvent(EVENT_MALADY_OF_THE_MIND, urand(18000, 25000), 0, PHASE_TWO);
                            break;
                        case EVENT_PSYCHOSIS:
                            me->CastCustomSpell(SPELL_PSYCHOSIS, SPELLVALUE_MAX_TARGETS, 1);
                            _events.ScheduleEvent(EVENT_PSYCHOSIS, 4000, 0, PHASE_TWO);
                            break;
                        case EVENT_BRAIN_LINK:
                            me->CastCustomSpell(SPELL_BRAIN_LINK, SPELLVALUE_MAX_TARGETS, 2);
                            _events.ScheduleEvent(EVENT_BRAIN_LINK, urand(23000, 26000), 0, PHASE_TWO);
                            break;
                        default:
                            break;
                    }
                }
            }

            void JustSummoned(Creature* summon) 
            {
               
            }

            void DoAction(int32 const action) 
            {
                switch (action)
                {
                    case ACTION_PHASE_THREE:    // Sara does nothing in phase 3
                        _events.SetPhase(PHASE_THREE);
                        break;
                    default:
                        break;
                }
            }

            private:
                EventMap _events;
                InstanceScript* _instance;

        };

        CreatureAI* GetAI(Creature* creature) const 
        {
            return GetUlduarAI<boss_saraAI>(creature);
        }
};

class boss_yogg_saron : public CreatureScript
{
    public:
        boss_yogg_saron() : CreatureScript("boss_yogg_saron") { }

        struct boss_yogg_saronAI : public BossAI
        {
            //boss_yogg_saronAI(Creature* creature) : PassiveAI(creature), _instance(creature->GetInstanceScript()) { }
            boss_yogg_saronAI(Creature* creature) : BossAI(creature, BOSS_YOGG_SARON)
            {
            }

            void Reset() 
            {
            	                _Reset();
            	
                _events.Reset();
                _events.SetPhase(PHASE_TWO);
                _events.ScheduleEvent(EVENT_YELL_BOW_DOWN, 30, 0, PHASE_TWO);
                //DoCast(me, SPELL_SHADOWY_BARRIER_YOGG);
                //DoCast(me, SPELL_KNOCK_AWAY);

                me->ResetLootMode();
                switch (_instance->GetData(DATA_KEEPERS_COUNT))
                {
                    case 0:
                        me->AddLootMode(LOOT_MODE_HARD_MODE_4);
                    case 1:
                        me->AddLootMode(LOOT_MODE_HARD_MODE_3);
                    case 2:
                        me->AddLootMode(LOOT_MODE_HARD_MODE_2);
                    case 3:
                        me->AddLootMode(LOOT_MODE_HARD_MODE_1);
                }
            }
            void EnterCombat(Unit* /*who*/) 
            {
                Talk(SAY_SARA_AGGRO);

            }
            void SpellHit(Unit* /*caster*/, SpellInfo const* spell) 
            {
                if (spell->Id == SPELL_IN_THE_MAWS_OF_THE_OLD_GOD)
                    me->AddLootMode(32);
            }

            void JustDied(Unit* /*killer*/) 
            {
                Talk(SAY_YOGG_SARON_DEATH);








            }

            void UpdateAI(uint32 const diff)
            {
                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_YELL_BOW_DOWN:
                            Talk(SAY_YOGG_SARON_SPAWN);
                            break;
                        case EVENT_SHADOW_BEACON:
                            DoCastAOE(SPELL_SHADOW_BEACON);
                            Talk(EMOTE_YOGG_SARON_EMPOWERING_SHADOWS);
                            _events.ScheduleEvent(EVENT_SHADOW_BEACON, 45000, 0, PHASE_THREE);
                            break;
                        case EVENT_LUNATIC_GAZE:
                            DoCast(me, SPELL_LUNATIC_GAZE);
                            sCreatureTextMgr->SendSound(me, SOUND_LUNATIC_GAZE, CHAT_MSG_MONSTER_YELL, 0, TEXT_RANGE_NORMAL, TEAM_OTHER, false);
                            _events.ScheduleEvent(EVENT_LUNATIC_GAZE, 12000, 0, PHASE_THREE);
                            break;
                        case EVENT_DEAFENING_ROAR:
                            DoCastAOE(SPELL_DEAFENING_ROAR);
                            Talk(SAY_YOGG_SARON_DEAFENING_ROAR);
                            Talk(EMOTE_YOGG_SARON_DEAFENING_ROAR);
                            _events.ScheduleEvent(EVENT_DEAFENING_ROAR, urand(20000, 25000), 0, PHASE_THREE);    // timer guessed
                            break;
                        default:
                            break;
                    }
                }
            DoMeleeAttackIfReady();
            }

            void DoAction(int32 const action) 
            {
                switch (action)
                {
                    case ACTION_PHASE_THREE:
                        _events.SetPhase(PHASE_THREE);
                        _events.ScheduleEvent(EVENT_SHADOW_BEACON, 45000, 0, PHASE_THREE);
                        _events.ScheduleEvent(EVENT_LUNATIC_GAZE, 12000, 0, PHASE_THREE);
                        if (me->GetMap()->Is25ManRaid() && _instance->GetData(DATA_KEEPERS_COUNT) < 4)
                            _events.ScheduleEvent(EVENT_DEAFENING_ROAR, urand(20000, 25000), 0, PHASE_THREE);    // timer guessed
                        Talk(SAY_YOGG_SARON_PHASE_3);
                        DoCast(me, SPELL_PHASE_3_TRANSFORM);
                        me->RemoveAurasDueToSpell(SPELL_SHADOWY_BARRIER_YOGG);
                        me->ResetPlayerDamageReq();
                        break;
                    default:
                        break;
                }
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
        };

};

class boss_brain_of_yogg_saron : public CreatureScript
{
    public:
        boss_brain_of_yogg_saron() : CreatureScript("boss_brain_of_yogg_saron") { }

        struct boss_brain_of_yogg_saronAI : public PassiveAI
        {
            boss_brain_of_yogg_saronAI(Creature* creature) : PassiveAI(creature), _instance(creature->GetInstanceScript()), _summons(creature)
            {
                _tentaclesKilled = 0;
            }

            void Reset() 
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
                DoCast(me, SPELL_MATCH_HEALTH);
                _summons.DespawnAll();
            }


            

            void DoAction(int32 const action) 
            {
                switch (action)
                {
                    case ACTION_INDUCE_MADNESS:
                    {
                        _tentaclesKilled = 0;


                        DoCastAOE(SPELL_INDUCE_MADNESS);
                        break;
                    }
                    case ACTION_TENTACLE_KILLED:
                    {

                        break;
                    }
                    default:
                        break;
                }
            }

            void JustSummoned(Creature* summon) 
            {
                _summons.Summon(summon);
            }

        private:
            InstanceScript* _instance;
            SummonList _summons;
            uint8 _tentaclesKilled;
        };

		CreatureAI* GetAI(Creature* creature) const
        {
            return GetUlduarAI<boss_brain_of_yogg_saronAI>(creature);
        }
};


 

void AddSC_boss_yogg_saron()
{
    new boss_voice_of_yogg_saron();
    new boss_sara();
    new boss_yogg_saron();
    new boss_brain_of_yogg_saron();

}
