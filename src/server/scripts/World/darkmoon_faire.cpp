#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "SpellScript.h"
#include "GameObjectAI.h"

enum DarkmoonFaireRingToss
{
    SPELL_RING_TOSS_MISSILE         = 101697,
    SPELL_RING_TOSS_UI              = 102058,
    SPELL_RING_TOSS_VISUAL_RING_0   = 101734,
    SPELL_RING_TOSS_VISUAL_RING_1   = 101736,
    SPELL_RING_TOSS_VISUAL_RING_2   = 101738,
    SPELL_RING_TOSS_CREDIT          = 101699,

    ENTRY_RING_TOSS_DUBENKO         = 54490,

    NPC_ENTRY_RING_TOSS_CREDIT      = 54495
};

class spell_darkmoon_faire_ring_toss : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_ring_toss() : SpellScriptLoader("spell_darkmoon_faire_ring_toss") { }

        class spell_darkmoon_faire_ring_toss_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_faire_ring_toss_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_RING_TOSS_MISSILE) || !sSpellMgr->GetSpellInfo(SPELL_RING_TOSS_UI))
                    return false;
                return true;
            }

            void HandleAreaTargetSelect(std::list<WorldObject*>& /* targets */)
            {
                if (WorldLocation const* dest = GetExplTargetDest())
                    GetCaster()->CastSpell(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(), SPELL_RING_TOSS_MISSILE, false);
            }

            //! Slightly hacky but not sure what else we could do...
            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                if (GetCaster()->GetPower(POWER_ALTERNATE_POWER) == 0)
                    GetCaster()->RemoveAurasDueToSpell(SPELL_RING_TOSS_UI);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_darkmoon_faire_ring_toss_SpellScript::HandleAreaTargetSelect, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
                OnEffectLaunch += SpellEffectFn(spell_darkmoon_faire_ring_toss_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_darkmoon_faire_ring_toss_SpellScript();
        }
};

class spell_darkmoon_faire_ring_toss_missile : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_ring_toss_missile() : SpellScriptLoader("spell_darkmoon_faire_ring_toss_missile") { }

        class spell_darkmoon_faire_ring_toss_missile_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_faire_ring_toss_missile_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_RING_TOSS_VISUAL_RING_0) || !sSpellMgr->GetSpellInfo(SPELL_RING_TOSS_VISUAL_RING_1) || !sSpellMgr->GetSpellInfo(SPELL_RING_TOSS_VISUAL_RING_2))
                    return false;
                return true;
            }

            void HandleTriggerMissile(SpellEffIndex effIndex)
            {
                Player* caster = GetCaster()->ToPlayer();

                if (!caster)
                    return;

                if (Position const* pos = GetExplTargetDest())
                {
                    Unit* dubenko = caster->FindNearestCreature(ENTRY_RING_TOSS_DUBENKO, 100.0f);

                    if (dubenko && dubenko->GetDistance(*pos) <= 1.5f)
                    {
                        //! Don't spawn the ring on the impact ground
                        PreventHitDefaultEffect(effIndex);

                        uint32 ringTossVisuals[3] = { SPELL_RING_TOSS_VISUAL_RING_0, SPELL_RING_TOSS_VISUAL_RING_1, SPELL_RING_TOSS_VISUAL_RING_2 };

                        for (uint8 i = 0; i < 3; ++i)
                        {
                            if (!dubenko->HasAura(ringTossVisuals[i]))
                            {
                                dubenko->CastSpell(dubenko, ringTossVisuals[i]);
                                break;
                            }
                        }

                        //! The kill credit spell has a dummy effect that makes the target perform a visual as if he throws
                        //! another ring. And if it's not a triggered cast, it also interrupts the target. None of these happen
                        //! on video's of the quest, so we'll give credit without the spell.
                        caster->KilledMonsterCredit(NPC_ENTRY_RING_TOSS_CREDIT);
                    }
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_darkmoon_faire_ring_toss_missile_SpellScript::HandleTriggerMissile, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_darkmoon_faire_ring_toss_missile_SpellScript();
        }
};

enum DarkmoonFaireWhackaGnoll
{
    SPELL_WRONG_TARGET_WHACKED          = 101679,
    SPELL_WHACK_A_GNOLL_AURA_STATE      = 101612,
    SPELL_WHACK_A_GNOLL_GNOLL_SPAWNED   = 102136,
    SPELL_HAS_GNOLL_SPAWNED             = 102136,
    SPELL_GNOLL_VISUAL_SUMMON_EFFECT    = 102136,
    SPELL_GNOLL_AURA_OKAY_TO_HIT        = 101996,
    SPELL_WHACK_A_GNOLL_KILL_CREDIT     = 101835,
    SPELL_STAY_OUT_WHACK_A_GNOLL_RING   = 109977,
    SPELL_WHACK_A_GNOLL_ROOT            = 101829,

    NPC_WHACK_A_GNOLL_BARREL            = 54546,
    NPC_WHACK_A_GNOLL_SUMMON_GNOLL      = 54549,
    NPC_WHACK_A_GNOLL_SUMMON_NORMAL     = 54444,
    NPC_WHACK_A_GNOLL_SUMMON_BABY       = 54466,
    NPC_WHACK_A_GNOLL_KILL_CREDIT       = 54505,
    NPC_WHACK_A_GNOLL_MONA              = 54601,

    QUEST_ITS_HAMMER_TIME               = 29463,

    MONA_BOSS_WHISPER_PLAYER_STAY_OUT   = 1,
};

class TargetCheckForProperGnollEntries
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->GetTypeId() != TYPEID_UNIT || !object->ToUnit()->HasAura(SPELL_GNOLL_AURA_OKAY_TO_HIT))
                return true;

            if (object->GetEntry() != NPC_WHACK_A_GNOLL_SUMMON_GNOLL && object->GetEntry() != NPC_WHACK_A_GNOLL_SUMMON_NORMAL && object->GetEntry() != NPC_WHACK_A_GNOLL_SUMMON_BABY)
                return true;

            return false;
        }
};

class WhackAGnollBarrelCheck
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->GetTypeId() != TYPEID_UNIT || object->ToUnit()->HasAura(SPELL_WHACK_A_GNOLL_GNOLL_SPAWNED))
                return true;

            return false;
        }
};

class spell_darkmoon_faire_whack : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_whack() : SpellScriptLoader("spell_darkmoon_faire_whack") { }

        class spell_darkmoon_faire_whack_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_faire_whack_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_WHACK_A_GNOLL_ROOT) || !sSpellMgr->GetSpellInfo(SPELL_WRONG_TARGET_WHACKED) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAS_GNOLL_SPAWNED) || !sSpellMgr->GetSpellInfo(SPELL_WHACK_A_GNOLL_KILL_CREDIT))
                    return false;
                return true;
            }

            bool Load()
            {
                gnollTarget = NULL;
                return true;
            }

            void HandleAreaTargetSelect(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                targets.remove_if(TargetCheckForProperGnollEntries());

                if (!targets.empty())
                    gnollTarget = targets.front()->ToUnit();
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();

                if (!gnollTarget || gnollTarget->GetTypeId() != TYPEID_UNIT || !caster || caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                uint8 creditCount = 0;

                //! Cant do this with spell_linked_spell because they should only be rooted when they hit a target. Not
                //! if no target was hit at all (hence it's not in the spell itself).
                caster->CastSpell(caster, SPELL_WHACK_A_GNOLL_ROOT);

                switch (gnollTarget->GetEntry())
                {
                    case NPC_WHACK_A_GNOLL_SUMMON_GNOLL:
                        creditCount = 3;
                        break;
                    case NPC_WHACK_A_GNOLL_SUMMON_NORMAL:
                        creditCount = 1;
                        break;
                    case NPC_WHACK_A_GNOLL_SUMMON_BABY:
                        caster->CastSpell(caster, SPELL_WRONG_TARGET_WHACKED);
                        gnollTarget->SetFacingToObject(caster); //! Baby gnolls face their attacker on video's when they get hit
                        return; //! Baby spawns don't despawn or die or whatever after being hit. They just stun you and stay for the full length until despawn.
                    default:
                        return; //! Shouldn't be possible
                }

                if (Unit* barrel = gnollTarget->FindNearestCreature(NPC_WHACK_A_GNOLL_BARREL, 3.0f))
                    barrel->RemoveAurasDueToSpell(SPELL_HAS_GNOLL_SPAWNED);

                for (uint8 i = 0; i < creditCount; ++i)
                    caster->CastSpell(caster, SPELL_WHACK_A_GNOLL_KILL_CREDIT); //! Also updates the UI of the player by energizing their altpower

                gnollTarget->ToCreature()->DespawnOrUnsummon(300);
                gnollTarget->Kill(gnollTarget); //! Their die visual effect is the one seen on video's
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_darkmoon_faire_whack_SpellScript::HandleAreaTargetSelect, EFFECT_0, TARGET_UNIT_CONE_ENTRY);
                OnEffectLaunch += SpellEffectFn(spell_darkmoon_faire_whack_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }

        private:
            Unit* gnollTarget; //! I do realize it's bad practice to store a pointer, but it's not possible to store the guid for some reason.
                               //! We check for NULL anyway so it should be okay.
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_darkmoon_faire_whack_SpellScript();
        }
};

class spell_darkmoon_faire_whack_a_gnoll_summon_aura : public SpellScriptLoader
{
public:
    spell_darkmoon_faire_whack_a_gnoll_summon_aura() : SpellScriptLoader("spell_darkmoon_faire_whack_a_gnoll_summon_aura") {}

    class spell_darkmoon_faire_whack_a_gnoll_summon_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_darkmoon_faire_whack_a_gnoll_summon_aura_AuraScript);

        bool Validate(SpellInfo const* /*spell*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_WHACK_A_GNOLL_AURA_STATE) || !sSpellMgr->GetSpellInfo(SPELL_HAS_GNOLL_SPAWNED))
                return false;
            return true;
        }

        void HandleEffectPeriodic(constAuraEffectPtr /* aurEff */)
        {
            Unit* target = GetTarget();

            if (!target || target->GetTypeId() != TYPEID_PLAYER || !target->HasAura(SPELL_WHACK_A_GNOLL_AURA_STATE))
                return;

            std::list<Creature*> barrelList;
            target->GetCreatureListWithEntryInGrid(barrelList, NPC_WHACK_A_GNOLL_BARREL, 100.0f);
            barrelList.remove_if(WhackAGnollBarrelCheck());

            //! We leave 1 barrel empty at all times to add a more blizzlike feeling to it.
            if (barrelList.size() <= 1)
                return;

            WoWSource::Containers::RandomResizeList(barrelList, 1);

            if (Creature* barrel = barrelList.front())
            {
                barrel->AddAura(SPELL_HAS_GNOLL_SPAWNED, barrel);

                //! Normal gnolls and babies spawn more often than the bonus gnolls according to video's. This also feels a lot more blizzlike in-game.
                uint32 possibleSummons[6]=
                {
                        NPC_WHACK_A_GNOLL_SUMMON_GNOLL,
                        NPC_WHACK_A_GNOLL_SUMMON_NORMAL,
                        NPC_WHACK_A_GNOLL_SUMMON_NORMAL,
                        NPC_WHACK_A_GNOLL_SUMMON_NORMAL,
                        NPC_WHACK_A_GNOLL_SUMMON_BABY,
                };

                //! The facing can't be done with SAI. They seem to face the nearest player on spawn.
                if (Creature* gnollOrBaby = barrel->SummonCreature(possibleSummons[urand(0, 5)], *barrel, TEMPSUMMON_MANUAL_DESPAWN))
                    gnollOrBaby->SetFacingToObject(gnollOrBaby->SelectNearestPlayer(25.0f));
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_darkmoon_faire_whack_a_gnoll_summon_aura_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_darkmoon_faire_whack_a_gnoll_summon_aura_AuraScript();
    }
};

class spell_darkmoon_faire_whack_a_gnoll_aura : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_whack_a_gnoll_aura() : SpellScriptLoader("spell_darkmoon_faire_whack_a_gnoll_aura") { }

        class spell_darkmoon_faire_whack_a_gnoll_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_darkmoon_faire_whack_a_gnoll_aura_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_STAY_OUT_WHACK_A_GNOLL_RING))
                    return false;
                return true;
            }

            void HandleEffectApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                //! Set the player's quest counter visibility to the state it was in. This is required because when
                //! the aura (SPELL_AURA_ENABLE_ALT_POWER) effect is unapplied, it calls Unit::SetMaxPower with 0 for
                //! altpower, which then calls Unit::SetPower for the altpower and sets it to 0. That means the visible
                //! counter for the player also resets to 0.
                if (Player* target = GetTarget()->ToPlayer())
                    if (target->GetQuestStatus(QUEST_ITS_HAMMER_TIME) == QUEST_STATUS_INCOMPLETE || target->GetQuestStatus(QUEST_ITS_HAMMER_TIME) == QUEST_STATUS_COMPLETE)
                        if (uint32 killCreditCount = target->GetReqKillOrCastCurrentCount(QUEST_ITS_HAMMER_TIME, NPC_WHACK_A_GNOLL_KILL_CREDIT))
                            target->ModifyPower(POWER_ALTERNATE_POWER, killCreditCount);
            }

            void HandleEffectRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                //! Teleport player out (let Mona do this) and make her warn the player.
                if (Player* target = GetTarget()->ToPlayer())
                {
                    //! Don't do any teleporting if the player is not near a barrel at the time. It means he/she is not in the playfield
                    //! and in order to get back to the playfield, they WILL trigger the areatrigger again which teleports (SAI).
                    if (!target->FindNearestCreature(NPC_WHACK_A_GNOLL_BARREL, 50.0f))
                        return;

                    if (Creature* mona = target->FindNearestCreature(NPC_WHACK_A_GNOLL_MONA, 60.0f))
                    {
                        mona->CastSpell(target, SPELL_STAY_OUT_WHACK_A_GNOLL_RING);

                        if (mona->IsAIEnabled)
                            mona->AI()->Talk(MONA_BOSS_WHISPER_PLAYER_STAY_OUT, target->GetGUID());
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_darkmoon_faire_whack_a_gnoll_aura_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_darkmoon_faire_whack_a_gnoll_aura_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_darkmoon_faire_whack_a_gnoll_aura_AuraScript();
        }
};

enum sDarkmoonFaireTonkChallenge
{
    SPELL_STAY_OUT_TONK_COMMANDER_RING              = 109976,

    QUEST_TONK_COMMANDER                            = 29434,

    NPC_TONK_COMMANDER_FINLAY_COOLSHOT              = 54605,
    NPC_TONK_COMMANDER_CREDIT                       = 33081,

    FINLAY_COOLSHOT_BOSS_WHISPER_PLAYER_STAY_OUT    = 1,
};

class spell_darkmoon_faire_using_darkmoon_tonk_controller : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_using_darkmoon_tonk_controller() : SpellScriptLoader("spell_darkmoon_faire_using_darkmoon_tonk_controller") { }

        class spell_darkmoon_faire_using_darkmoon_tonk_controller_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_darkmoon_faire_using_darkmoon_tonk_controller_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_STAY_OUT_TONK_COMMANDER_RING))
                    return false;
                return true;
            }

            void HandleEffectApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                //! Teleporting into the battle is handled in npc_darkmoon_faire_tonk::PassengerBoarded.

                //! Set the player's quest counter visibility to the state it was in. This is required because when
                //! the aura (SPELL_AURA_ENABLE_ALT_POWER) effect is unapplied, it calls Unit::SetMaxPower with 0 for
                //! altpower, which then calls Unit::SetPower for the altpower and sets it to 0. That means the visible
                //! counter for the player also resets to 0.
                if (Player* target = GetTarget()->ToPlayer())
                    if (target->GetQuestStatus(QUEST_TONK_COMMANDER) == QUEST_STATUS_INCOMPLETE || target->GetQuestStatus(QUEST_TONK_COMMANDER) == QUEST_STATUS_COMPLETE)
                        if (uint32 killCreditCount = target->GetReqKillOrCastCurrentCount(QUEST_TONK_COMMANDER, NPC_TONK_COMMANDER_CREDIT))
                            target->ModifyPower(POWER_ALTERNATE_POWER, killCreditCount);
            }

            void HandleEffectRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                static const Position exitPos = {-4127.838f, 6325.841f, 13.115f, 4.308f};

                if (Player* target = GetTarget()->ToPlayer())
                {
                    if (!target->GetVehicleKit())
                    {
                        //? This needs to be delayed somehow
                        if (Unit* vehicle = target->GetVehicleBase())
                            vehicle->NearTeleportTo(exitPos.GetPositionX(), exitPos.GetPositionY(), exitPos.GetPositionZ(), exitPos.GetOrientation());

                        target->NearTeleportTo(exitPos.GetPositionX(), exitPos.GetPositionY(), exitPos.GetPositionZ(), exitPos.GetOrientation());
                    }
                    //! Exit the vehicle and make the player move to a safe area
                    else
                        target->_ExitVehicle(&exitPos);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_darkmoon_faire_using_darkmoon_tonk_controller_AuraScript::HandleEffectApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_darkmoon_faire_using_darkmoon_tonk_controller_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_darkmoon_faire_using_darkmoon_tonk_controller_AuraScript();
        }
};

enum DarkmoonDeathMatch
{
    SPELL_ENTER_DEATHMATCH_1                = 108919,
    SPELL_ENTER_DEATHMATCH_2                = 113212,
    SPELL_ENTER_DEATHMATCH_3                = 113213,
    SPELL_ENTER_DEATHMATCH_4                = 113216,
    SPELL_ENTER_DEATHMATCH_5                = 113219,
    SPELL_ENTER_DEATHMATCH_6                = 113224,
    SPELL_ENTER_DEATHMATCH_7                = 113227,
    SPELL_ENTER_DEATHMATCH_8                = 113228,
    SPELL_EXIT_DEATHMATCH                   = 108923,
    SPELL_DEMONIC_CIRCLE_SUMMON             = 48018,

    GAME_EVENT_DARKMOON_FAIRE_DEATHMATCH    = 77,
};

class spell_darkmoon_faire_darkmoon_deathmatch : public SpellScriptLoader
{
public:
    spell_darkmoon_faire_darkmoon_deathmatch() : SpellScriptLoader("spell_darkmoon_faire_darkmoon_deathmatch") {}

    class spell_darkmoon_faire_darkmoon_deathmatch_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_darkmoon_faire_darkmoon_deathmatch_SpellScript);

        bool Validate(SpellInfo const* /*spell*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_1) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_2) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_3) ||
                !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_4) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_5) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_6) ||
                !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_7) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_8) || !sSpellMgr->GetSpellInfo(SPELL_EXIT_DEATHMATCH) ||
                !sSpellMgr->GetSpellInfo(SPELL_DEMONIC_CIRCLE_SUMMON))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Player* target = GetHitPlayer())
            {
                //! Don't make them re-cast if they're already casting
                if (target->IsNonMeleeSpellCasted(false))
                    return;

                target->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);

                //! If player is in deathmatch area
                if (target->GetAreaId() != 5877)
                {
                    uint32 allEnterDeathmatchSpells[9] = { SPELL_ENTER_DEATHMATCH_1, SPELL_ENTER_DEATHMATCH_2, SPELL_ENTER_DEATHMATCH_3, SPELL_ENTER_DEATHMATCH_4, SPELL_ENTER_DEATHMATCH_5, SPELL_ENTER_DEATHMATCH_6, SPELL_ENTER_DEATHMATCH_7, SPELL_ENTER_DEATHMATCH_8 };
                    target->CastSpell(target, allEnterDeathmatchSpells[urand(0, 8)]);
                } else
                {
                    //! We remove the Demonic Circle effect because one of the Exit Deathmatch spells does this
                    //! as it was, most likely, abusable and was taken notice of by Blizzard at the PTR. It does
                    //! still work if you want to teleport OUT with Demonic Circle (video proof).
                    target->RemoveAurasDueToSpell(SPELL_DEMONIC_CIRCLE_SUMMON);
                    target->CastSpell(target, SPELL_EXIT_DEATHMATCH);
                }
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_darkmoon_faire_darkmoon_deathmatch_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_darkmoon_faire_darkmoon_deathmatch_SpellScript();
    }
};

enum DarkmoonFaireInjuredCarnie
{
    NPC_INJURED_CARNIE          = 54518,
    NPC_DAMAGED_TONK            = 54504,

    SPELL_REPAIR_DAMAGED_TONK   = 101838,
    SPELL_HEAL_INJURED_CARNIE   = 101883,
};

class spell_darkmoon_faire_enter_deathmatch : public SpellScriptLoader
{
public:
    spell_darkmoon_faire_enter_deathmatch() : SpellScriptLoader("spell_darkmoon_faire_enter_deathmatch") {}

    class spell_darkmoon_faire_enter_deathmatch_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_darkmoon_faire_enter_deathmatch_SpellScript);

        bool Validate(SpellInfo const* /*spell*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_1) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_2) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_3) ||
                !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_4) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_5) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_6) ||
                !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_7) || !sSpellMgr->GetSpellInfo(SPELL_ENTER_DEATHMATCH_8) || !sSpellMgr->GetSpellInfo(SPELL_EXIT_DEATHMATCH) ||
                !sSpellMgr->GetSpellInfo(SPELL_DEMONIC_CIRCLE_SUMMON))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Player* target = GetHitPlayer())
                target->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_darkmoon_faire_enter_deathmatch_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_darkmoon_faire_enter_deathmatch_SpellScript();
    }
};

class npc_darkmoon_faire_injured_carnie : public CreatureScript
{
public:
    npc_darkmoon_faire_injured_carnie() : CreatureScript("npc_darkmoon_faire_injured_carnie") {}

    struct npc_darkmoon_faire_injured_carnieAI : public ScriptedAI
    {
        npc_darkmoon_faire_injured_carnieAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 standUpDelay;
        bool alreadyHealed;

        void Reset()
        {
            standUpDelay = 0;
            alreadyHealed = false;

            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->SetHealth(me->CountPctFromMaxHealth(25));
        }

        void EnterCombat(Unit* /*who*/) {}

        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (!spell || spell->Id != SPELL_HEAL_INJURED_CARNIE || !caster || caster->GetTypeId() != TYPEID_PLAYER || alreadyHealed)
                return;

            alreadyHealed = true;
            me->SetUInt32Value(UNIT_FIELD_BYTES_1, UNIT_STAND_STATE_STAND);
            me->SetFacingToObject(caster);
            caster->ToPlayer()->KilledMonsterCredit(me->GetEntry());
            Talk(0, caster->GetGUID());
            me->DespawnOrUnsummon(3000);
        }

        void UpdateAI(uint32 const diff) {}
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_darkmoon_faire_injured_carnieAI(creature);
    }
};

class spell_darkmoon_faire_target_conditions : public SpellScriptLoader
{
public:
    spell_darkmoon_faire_target_conditions() : SpellScriptLoader("spell_darkmoon_faire_target_conditions") {}

    class spell_darkmoon_faire_target_conditions_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_darkmoon_faire_target_conditions_SpellScript);

        bool Validate(SpellInfo const* /*spell*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_REPAIR_DAMAGED_TONK) || !sSpellMgr->GetSpellInfo(SPELL_HEAL_INJURED_CARNIE))
                return false;
            return true;
        }

        //! This seems to be needed because conditions won't work on the target type and I can't figure out which one to use.
        //! Nearby target is an even worse hack as it wouldn't be blizzlike.
        void CheckTarget(WorldObject*& target)
        {
            if (!target)
                return;

            if (!target->ToCreature())
                target = NULL;
            else if (SpellInfo const* spellInfo = GetSpellInfo())
            {
                if ((spellInfo->Id == SPELL_REPAIR_DAMAGED_TONK && target->GetEntry() != NPC_DAMAGED_TONK) ||
                    (spellInfo->Id == SPELL_HEAL_INJURED_CARNIE && target->GetEntry() != NPC_INJURED_CARNIE))
                    target = NULL;
                else if (target->GetEntry() != NPC_DAMAGED_TONK && !target->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH))
                    target = NULL;
            }
        }

        void Register()
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_darkmoon_faire_target_conditions_SpellScript::CheckTarget, EFFECT_0, TARGET_UNIT_TARGET_ANY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_darkmoon_faire_target_conditions_SpellScript();
    }
};

enum DarkmoonFaireHeShootsHeScores
{
    NPC_SHOOT_TARGET_BUNNY          = 24171,
    NPC_HE_SHOOTS_HE_SCORES_CREDIT  = 54231,

    QUEST_HE_SHOOTS_HE_SCORES       = 29438,
};

class TargetCheckForProperShootTarget
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->GetTypeId() != TYPEID_UNIT || object->GetEntry() != NPC_SHOOT_TARGET_BUNNY)
                return true;

            return false;
        }
};

class spell_darkmoon_faire_shoot_rifle : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_shoot_rifle() : SpellScriptLoader("spell_darkmoon_faire_shoot_rifle") { }

        class spell_darkmoon_faire_shoot_rifle_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_faire_shoot_rifle_SpellScript);

            void HandleAreaTargetSelect(std::list<WorldObject*>& targets)
            {
                if (!targets.empty())
                    targets.remove_if(TargetCheckForProperShootTarget());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_darkmoon_faire_shoot_rifle_SpellScript::HandleAreaTargetSelect, EFFECT_0, TARGET_UNIT_CONE_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_darkmoon_faire_shoot_rifle_SpellScript();
        }
};

class spell_darkmoon_faire_crack_shot : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_crack_shot() : SpellScriptLoader("spell_darkmoon_faire_crack_shot") { }

        class spell_darkmoon_faire_crack_shot_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_darkmoon_faire_crack_shot_AuraScript);

            void HandleEffectApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                //! Set the player's quest counter visibility to the state it was in. This is required because when
                //! the aura (SPELL_AURA_ENABLE_ALT_POWER) effect is unapplied, it calls Unit::SetMaxPower with 0 for
                //! altpower, which then calls Unit::SetPower for the altpower and sets it to 0. That means the visible
                //! counter for the player also resets to 0.
                if (Player* target = GetTarget()->ToPlayer())
                    if (target->GetQuestStatus(QUEST_HE_SHOOTS_HE_SCORES) == QUEST_STATUS_INCOMPLETE || target->GetQuestStatus(QUEST_HE_SHOOTS_HE_SCORES) == QUEST_STATUS_COMPLETE)
                        if (uint32 killCreditCount = target->GetReqKillOrCastCurrentCount(QUEST_HE_SHOOTS_HE_SCORES, NPC_HE_SHOOTS_HE_SCORES_CREDIT))
                            target->ModifyPower(POWER_ALTERNATE_POWER, killCreditCount);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_darkmoon_faire_crack_shot_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_darkmoon_faire_crack_shot_AuraScript();
        }
};

enum DarkmoonFaireTheHumanoidCannonBall
{
    SPELL_CANNON_BLAST_UI_STATE     = 102121,
    SPELL_CANNON_PREP_VISUAL        = 102112,
    SPELL_MAGIC_WINGS               = 102116,
    SPELL_JUST_SHOT_FROM_CANNON     = 62172,
    SPELL_BULLSEYE                  = 62173,
    SPELL_GREAT_SHOT                = 62175,
    SPELL_POOR_SHOT                 = 62179,
    SPELL_CANNON_TARGET_AREA_AURA_A = 62171,
    SPELL_CANNON_TARGET_AREA_AURA_B = 62174,
    SPELL_CANNON_TARGET_AREA_AURA_C = 62178,
    SPELL_LANDING_RESULT_CREDIT     = 100962,

    NPC_CANNON_TARGET_BUNNY         = 33068,
};

class spell_darkmoon_faire_cannon_prep : public SpellScriptLoader
{
public:
    spell_darkmoon_faire_cannon_prep() : SpellScriptLoader("spell_darkmoon_faire_cannon_prep") {}

    class spell_darkmoon_faire_cannon_prep_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_darkmoon_faire_cannon_prep_SpellScript);

        bool Validate(SpellInfo const* /*spell*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_CANNON_BLAST_UI_STATE) || !sSpellMgr->GetSpellInfo(SPELL_CANNON_PREP_VISUAL))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Player* target = GetHitPlayer())
            {
                target->Dismount();
                target->RemoveAurasByType(SPELL_AURA_MOUNTED);
                target->RemoveAurasByType(SPELL_AURA_OVERRIDE_SPELLS); //! Remove all other quest-related alternate power states
                target->CastSpell(target, SPELL_CANNON_BLAST_UI_STATE);
                target->CastSpell(target, SPELL_CANNON_PREP_VISUAL);
                target->SetControlled(true, UNIT_STATE_STUNNED);
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_darkmoon_faire_cannon_prep_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_darkmoon_faire_cannon_prep_SpellScript();
    }
};


class spell_darkmoon_faire_fire_cannon : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_fire_cannon() : SpellScriptLoader("spell_darkmoon_faire_fire_cannon") { }

        class spell_darkmoon_faire_fire_cannon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_faire_fire_cannon_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MAGIC_WINGS))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Player* target = GetHitPlayer())
                {
                    target->SetControlled(false, UNIT_STATE_STUNNED);
                    target->RemoveAurasDueToSpell(SPELL_BULLSEYE);
                    target->RemoveAurasDueToSpell(SPELL_GREAT_SHOT);
                    target->RemoveAurasDueToSpell(SPELL_POOR_SHOT);
                    target->CastSpell(target, SPELL_MAGIC_WINGS);
                    target->CastSpell(target, SPELL_JUST_SHOT_FROM_CANNON);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_darkmoon_faire_fire_cannon_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_darkmoon_faire_fire_cannon_SpellScript();
        }
};

class spell_darkmoon_faire_just_shot_from_cannon : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_just_shot_from_cannon() : SpellScriptLoader("spell_darkmoon_faire_just_shot_from_cannon") { }

        class spell_darkmoon_faire_just_shot_from_cannon_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_darkmoon_faire_just_shot_from_cannon_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_CANNON_TARGET_AREA_AURA_A) || !sSpellMgr->GetSpellInfo(SPELL_BULLSEYE) || !sSpellMgr->GetSpellInfo(SPELL_GREAT_SHOT) ||
                    !sSpellMgr->GetSpellInfo(SPELL_CANNON_TARGET_AREA_AURA_B) || !sSpellMgr->GetSpellInfo(SPELL_POOR_SHOT) || !sSpellMgr->GetSpellInfo(SPELL_LANDING_RESULT_CREDIT) ||
                    !sSpellMgr->GetSpellInfo(SPELL_CANNON_TARGET_AREA_AURA_C))
                    return false;
                return true;
            }

            void HandleEffectRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* target = GetTarget()->ToPlayer())
                {
                    if (Creature* bunny = target->FindNearestCreature(NPC_CANNON_TARGET_BUNNY, 20.0f))
                    {
                        float distToBunny = target->GetDistance(bunny->GetPositionX(), bunny->GetPositionY(), bunny->GetPositionZ());
                        uint8 possibleCredits[3] = { 1, 3, 5 };
                        uint32 possibleSpells[3] = { SPELL_POOR_SHOT, SPELL_GREAT_SHOT, SPELL_BULLSEYE };
                        int8 index = -1;

                        //! Not blizzlike values according to the spells but they still feel a whole lot more retail-like.
                        if (distToBunny <= 1.0f)
                            index = 2;
                        else if (distToBunny <= 5.0f)
                            index = 1;
                        else if (distToBunny <= 15.0f)
                            index = 0;

                        if (index >= 0)
                        {
                            target->CastSpell(target, possibleSpells[index]);

                            for (int i = 0; i < possibleCredits[index]; ++i)
                                target->CastSpell(target, SPELL_LANDING_RESULT_CREDIT);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_darkmoon_faire_just_shot_from_cannon_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_darkmoon_faire_just_shot_from_cannon_AuraScript();
        }
};

enum DarkmoonFaireJublingCooldown
{
    SPELL_JUBLING_COOLDOWN = 23852,
};

class spell_darkmoon_faire_jubling_cooldown : public SpellScriptLoader
{
    public:
        spell_darkmoon_faire_jubling_cooldown() : SpellScriptLoader("spell_darkmoon_faire_jubling_cooldown") { }

        class spell_darkmoon_faire_jubling_cooldown_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_faire_jubling_cooldown_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_JUBLING_COOLDOWN))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Player* target = GetHitPlayer())
                    target->CastSpell(target, SPELL_JUBLING_COOLDOWN);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_darkmoon_faire_jubling_cooldown_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_darkmoon_faire_jubling_cooldown_SpellScript();
        }
};

enum DarkmoonFaireDarkmoonCannonBalls
{
    NPC_DARKMOON_STEAM_TONK             = 54588,

    SPELL_BOMBS_AWAY_VISUAL_EXPLOSION   = 102211,
};

class go_darkmoon_faire_darkmoon_cannon_balls : public GameObjectScript
{
    public:
        go_darkmoon_faire_darkmoon_cannon_balls() : GameObjectScript("go_darkmoon_faire_darkmoon_cannon_balls") { }

        struct go_darkmoon_faire_darkmoon_cannon_ballsAI : public GameObjectAI
        {
            go_darkmoon_faire_darkmoon_cannon_ballsAI(GameObject* go) : GameObjectAI(go) { }

            //! For some reason not possible with SmartGameObjectAI. Doesn't return a list of creatures
            //! within given distance.
            void InitializeAI()
            {
                std::list<Creature*> nearbyDarkmoonTonks;
                go->GetCreatureListWithEntryInGrid(nearbyDarkmoonTonks, NPC_DARKMOON_STEAM_TONK, 1.5f);

                for (std::list<Creature*>::iterator itr = nearbyDarkmoonTonks.begin(); itr != nearbyDarkmoonTonks.end(); ++itr)
                    if ((*itr)->IsAIEnabled)
                        (*itr)->AI()->SetData(1, 1);

                if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_BOMBS_AWAY_VISUAL_EXPLOSION))
                    if (Creature* trigger = go->SummonTrigger(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), 0, 2000))
                        trigger->CastSpell(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), SPELL_BOMBS_AWAY_VISUAL_EXPLOSION, true);
            }
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return new go_darkmoon_faire_darkmoon_cannon_ballsAI(go);
        }
};

enum DarkmoonFaireDeathMatch
{
    EVENT_DARKMOON_FAIRE_DEATHMATCH_PREPARE         = 78,
    EVENT_DARKMOON_FAIRE_DEATHMATCH_CHEST_SPAWN     = 79,

    AREA_ID_DARKMOON_DEATHMATCH                     = 5877,

    SPELL_EXIT_DEATHMATCH_INSTANT                   = 113246,

    NPC_KORGOL_CRUSHSKULL                           = 55402,

    ITEM_PIT_FIGHTER                                = 74034,
};

class go_darkmoon_faire_darkmoon_treasure_chest : public GameObjectScript
{
public:
    go_darkmoon_faire_darkmoon_treasure_chest() : GameObjectScript("go_darkmoon_faire_darkmoon_treasure_chest") {}

    struct go_darkmoon_faire_darkmoon_treasure_chestAI : public GameObjectAI
    {
        go_darkmoon_faire_darkmoon_treasure_chestAI(GameObject* go) : GameObjectAI(go) {}

        uint64 looterGuid;

        void InitializeAI()
        {
            looterGuid = 0;
        }

        void OnStateChanged(uint32 state, Unit* unit)
        {
            switch (state)
            {
                //! Store last looter's guid so we can yell which player won when GO_JUST_DEACTIVATED is called
                case GO_ACTIVATED:
                    if (unit && unit->GetTypeId() == TYPEID_PLAYER)
                    {
                        //! Prevent exploiting
                        if (!unit->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP))
                        {
                            if (uint64 lguid = unit->ToPlayer()->GetLootGUID())
                                unit->ToPlayer()->GetSession()->DoLootRelease(lguid);

                            break;
                        }

                        looterGuid = unit->GetGUID();
                    }

                    break;
                    //! Stop the game events when the player finished looting. Also yell that they won.
                case GO_JUST_DEACTIVATED:
                    if (Player* looter = Unit::GetPlayer(*go, looterGuid))
                    {
                        bool lootedPitFighter = false;

                        //! Nobody won if there's still a Pit Fighter in the chest. It looks like it works like this
                        //! on retail according to several video's of people looting it.
                        for (uint8 i = 0; i < go->loot.items.size(); ++i)
                        {
                            if (go->loot.items[i].itemid == ITEM_PIT_FIGHTER && go->loot.items[i].is_looted)
                            {
                                lootedPitFighter = true;
                                break;
                            }
                        }

                        if (!lootedPitFighter)
                            break;

                        //! Stop both events so the chest won't spawn again
                        sGameEventMgr->StopEvent(EVENT_DARKMOON_FAIRE_DEATHMATCH_PREPARE);
                        sGameEventMgr->StopEvent(EVENT_DARKMOON_FAIRE_DEATHMATCH_CHEST_SPAWN);

                        if (Player* looter = Unit::GetPlayer(*go, looterGuid))
                            if (Creature* korgol = looter->FindNearestCreature(NPC_KORGOL_CRUSHSKULL, 120.0f))
                                if (korgol->IsAIEnabled)
                                    korgol->AI()->Talk(2, looter->GetGUID());
                    }
                    break;
            }
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_darkmoon_faire_darkmoon_treasure_chestAI(go);
    }
};


Position TonkTargetSpawnPositions[] =
{
    { -4133.45f, 6294.77f, 13.1171f, 0.0f },
    { -4126.34f, 6295.69f, 13.1171f, 0.0f },
    { -4130.49f, 6296.65f, 13.1171f, 0.0f },
    { -4134.18f, 6298.85f, 13.1171f, 0.0f },
    { -4137.41f, 6295.36f, 13.1171f, 0.0f },
    { -4141.71f, 6292.57f, 13.1171f, 0.0f },
    { -4131.61f, 6287.74f, 13.1171f, 0.0f },
    { -4145.38f, 6292.29f, 13.1171f, 0.0f },
    { -4146.07f, 6297.25f, 13.1171f, 0.0f },
    { -4149.71f, 6300.93f, 13.1171f, 0.0f },
    { -4147.46f, 6305.5f, 13.1171f, 0.0f  },
    { -4142.5f, 6301.93f, 13.1171f, 0.0f  },
    { -4137.12f, 6304.24f, 13.1171f, 0.0f },
    { -4125.22f, 6290.79f, 13.1171f, 0.0f },
    { -4131.22f, 6302.09f, 13.1171f, 0.0f },
    { -4129.64f, 6292.6f, 13.1171f, 0.0f  },
    { -4126.73f, 6300.91f, 13.1171f, 0.0f },
    { -4121.15f, 6298.68f, 13.1171f, 0.0f },
    { -4122.81f, 6304.52f, 13.1169f, 0.0f },
    { -4126.98f, 6307.38f, 13.1169f, 0.0f },
    { -4141.62f, 6306.57f, 13.1169f, 0.0f },
    { -4131.88f, 6307.74f, 13.1169f, 0.0f },
    { -4143.52f, 6310.8f, 13.1169f, 0.0f  },
    { -4145.31f, 6314.28f, 13.1169f, 0.0f },
    { -4138.19f, 6310.31f, 13.1169f, 0.0f },
    { -4135.99f, 6317.89f, 13.1177f, 0.0f },
    { -4128.91f, 6313.64f, 13.1177f, 0.0f },
    { -4130.87f, 6288.45f, 13.1171f, 0.0f },
    { -4138.44f, 6313.64f, 13.1177f, 0.0f },
    { -4133.31f, 6287.59f, 13.1171f, 0.0f },
    { -4136.56f, 6287.86f, 13.1171f, 0.0f },
    { -4135.54f, 6284.48f, 13.1171f, 0.0f },
    { -4139.15f, 6290.02f, 13.1171f, 0.0f },
    { -4140.22f, 6298.38f, 13.1171f, 0.0f },
    { -4133.2f, 6311.75f, 13.1171f, 0.0f  },
    { -4141.31f, 6317.32f, 13.1171f, 0.0f },
    { -4138.00f, 6300.72f, 13.1174f, 0.0f },
    { -4126.2f, 6309.96f, 13.117f, 0.0f   },
    { -4133.48f, 6304.84f, 13.117f, 0.0f  },
    { -4145.88f, 6301.11f, 13.117f, 0.0f  },
    { -4138.22f, 6306.69f, 13.117f, 0.0f  },
    { -4132.02f, 6316.23f, 13.1167f, 0.0f },
    { -4123.13f, 6294.02f, 13.1165f, 0.0f },
    { -4128.00f, 6303.8f, 13.1172f, 0.0f  },
    { -4129.02f, 6310.43f, 13.1172f, 0.0f },
};

enum nDarkmoonFaireTonkChallenge
{
    SPELL_USING_STEAM_TONK_CONTROLLER   = 100752,
    SPELL_TONK_COMMANDER_CREDIT         = 110162,
    SPELL_DARKMOON_STEAM_TONK_SHOOT     = 102292,
    SPELL_DUST_EXPLOSION                = 84277,
    SPELL_ENEMY_TONK_MARKED             = 102341,
    SPELL_STAY_OUT_OF_TONK_RING         = 109976,
    SPELL_PLAYER_EXITS_TONK             = 102270,
    SPELL_MARKED_BY_ENEMY_TONK          = 102341,
    SPELL_ENEMY_TONK_CANNON_BLAST       = 102227,

    NPC_TONK_TARGET                     = 33081,
    NPC_TONK_COMMANDER_CONTROLLER       = 71234,
    NPC_FINLAY_COOLSHOT                 = 54605,
    NPC_DARKMOON_TONK                   = 54588,

    MAX_POSITIONS_COUNT                 = 45,

    DATASET_SLOT_ID                     = 1234,
};

class npc_darkmoon_faire_tonk_commander_controller : public CreatureScript
{
public:
    npc_darkmoon_faire_tonk_commander_controller() : CreatureScript("npc_darkmoon_faire_tonk_commander_controller") {}

    struct npc_darkmoon_faire_tonk_commander_controllerAI : public ScriptedAI
    {
        npc_darkmoon_faire_tonk_commander_controllerAI(Creature* creature) : ScriptedAI(creature) {}

        std::unordered_map<uint32 /* slotId */, Position*> freeSpawnPositions;
        std::list<std::pair<uint8 /* amountToSpawn */, uint32 /* spawnDelay */>> pendingSpawns;
        uint32 checkAmountOfSpawnsTimer;
        uint32 checkForPlayersTimer;
        uint8 totalSpawnedTargets;

        void Reset()
        {
            checkAmountOfSpawnsTimer = 120000;
            checkForPlayersTimer = 10000;
            totalSpawnedTargets = 0;

            for (int32 i = 0; i < MAX_POSITIONS_COUNT; ++i)
                freeSpawnPositions[i] = &TonkTargetSpawnPositions[i];

            //! Spawn 12 to 16 targets when starting
            uint8 startWithSpawns = urand(12, 16);

            for (uint8 i = 0; i < startWithSpawns; ++i)
                SpawnTonkTargetOnRandomFreePosition();
        }

        void EnterCombat(Unit* /*who*/) {}

        void UpdateAI(uint32 const diff)
        {
            if (checkAmountOfSpawnsTimer <= diff)
            {
                checkAmountOfSpawnsTimer = 120000;

                uint32 totalSpawnedAndPendingTargets = GetTotalSpawnedAndPendingTonkTargets();

                if (totalSpawnedAndPendingTargets <= 10)
                {
                    uint8 amountOfNewSpawns = urand(12, 16) - totalSpawnedAndPendingTargets;

                    for (uint32 i = 0; i < amountOfNewSpawns; ++i)
                        SpawnTonkTargetOnRandomFreePosition();
                }
            } else
                checkAmountOfSpawnsTimer -= diff;

            if (checkForPlayersTimer <= diff)
            {
                checkForPlayersTimer = 10000;

                if (Creature* finlayCoolshot = me->FindNearestCreature(NPC_FINLAY_COOLSHOT, 40.0f))
                {
                    std::list<Player*> players;
                    WoWSource::AnyPlayerInObjectRangeCheck checker(me, 18.0f);
                    WoWSource::PlayerListSearcher<WoWSource::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
                    me->VisitNearbyWorldObject(18.0f, searcher);

                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        //! Don't teleport out players that are playing at the moment
                        if ((*itr)->HasAura(SPELL_USING_STEAM_TONK_CONTROLLER))
                            continue;

                        finlayCoolshot->CastSpell(*itr, SPELL_STAY_OUT_OF_TONK_RING);

                        if (finlayCoolshot->IsAIEnabled)
                            finlayCoolshot->AI()->Talk(1, (*itr)->GetGUID());
                    }
                }
            } else
                checkForPlayersTimer -= diff;

            if (!pendingSpawns.empty())
            {
                for (std::list<std::pair<uint8, uint32>>::iterator itr = pendingSpawns.begin(); itr != pendingSpawns.end();)
                {
                    if (itr->second <= diff)
                    {
                        //! Sometimes no new target is spawned, sometimes it spawns two. In this case the count (itr->first) is
                        //! set to 0 so nothing is spawned. Max amount of spawns is 3.
                        //! The max amount of spawns at a time is 25 to make it look optimal.
                        if (GetTotalSpawnedAndPendingTonkTargets() <= 25)
                            for (uint8 i = 0; i < itr->first; ++i)
                                SpawnTonkTargetOnRandomFreePosition();

                        itr = pendingSpawns.erase(itr);
                    } else
                    {
                        itr->second -= diff;
                        itr++;
                    }
                }
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            //! The Tonk Target has died
            if (type == DATASET_SLOT_ID)
            {
                --totalSpawnedTargets;
                freeSpawnPositions[data] = &TonkTargetSpawnPositions[data];
                pendingSpawns.push_back(std::make_pair(urand(0, 2), urand(3000, 4500)));
            }
        }

        void SpawnTonkTargetOnRandomFreePosition()
        {
            if (freeSpawnPositions.empty())
                return;

            std::unordered_map<uint32 /* slotId */, Position*>::iterator itr = freeSpawnPositions.begin();

            //! We avoid WoWSource::Containers::SelectRandomContainerElement because of double iteration (one for advance, one for erase)
            std::advance(itr, urand(0, freeSpawnPositions.size() - 1));

            //! Make the target know about its slot id so we can free the spot later on. MANUAL_DESPAWN as we despawn
            //! it in its own script.
            if (Creature* creature = me->SummonCreature(NPC_TONK_TARGET, *itr->second))
            {
                ++totalSpawnedTargets;

                //! The target has to face a random orientation so the spawns don't look static in-game.
                creature->SetFacingTo(frand(0.0f, 2.0f * M_PI));

                if (creature->IsAIEnabled)
                    creature->AI()->SetData(DATASET_SLOT_ID, itr->first);
            }

            freeSpawnPositions.erase(itr);
        }

        uint32 GetTotalSpawnedAndPendingTonkTargets()
        {
            return totalSpawnedTargets + pendingSpawns.size();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_darkmoon_faire_tonk_commander_controllerAI(creature);
    }
};

//! Sadly can't be done in SAI because we need to store the slot id
class npc_darkmoon_faire_tonk_target : public CreatureScript
{
    public:
        npc_darkmoon_faire_tonk_target() : CreatureScript("npc_darkmoon_faire_tonk_target") { }

        struct npc_darkmoon_faire_tonk_targetAI : public ScriptedAI
        {
            npc_darkmoon_faire_tonk_targetAI(Creature* creature) : ScriptedAI(creature) { }

            uint8 targetSlotId;

            void Reset()
            {
                targetSlotId = 0;
            }

            void EnterCombat(Unit* /*who*/) { }

            //! Block the taken damage because the player shouldn't be actually killing the Tonk Target. If they did,
            //! they would get credit for it but since the credits were not given through the spell, the player's UI
            //! counter wouldn't update (alternate power stuff).
            void DamageTaken(Unit* /*doneBy*/, uint32& damage)
            {
                damage = 0;
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (!spell || spell->Id != SPELL_DARKMOON_STEAM_TONK_SHOOT || !caster || caster->GetTypeId() != TYPEID_UNIT)
                    return;

                Unit* casterOwner = caster->GetCharmerOrOwner();

                if (!casterOwner || casterOwner->GetTypeId() != TYPEID_PLAYER)
                    return;

                casterOwner->CastSpell(casterOwner, SPELL_TONK_COMMANDER_CREDIT);
                me->Kill(me);
                me->DespawnOrUnsummon(2000);

                if (Unit* controller = me->ToTempSummon()->GetSummoner())
                    if (controller->GetTypeId() == TYPEID_UNIT && controller->GetEntry() == NPC_TONK_COMMANDER_CONTROLLER && controller->IsAIEnabled)
                        controller->ToCreature()->AI()->SetData(DATASET_SLOT_ID, targetSlotId);
            }

            void SetData(uint32 type, uint32 data)
            {
                if (type == DATASET_SLOT_ID)
                    targetSlotId = data;
            }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_darkmoon_faire_tonk_targetAI(creature);
        }
};

class npc_darkmoon_faire_tonk : public CreatureScript
{
    public:
        npc_darkmoon_faire_tonk() : CreatureScript("npc_darkmoon_faire_tonk") { }

        struct npc_darkmoon_faire_tonkAI : public VehicleAI
        {
            npc_darkmoon_faire_tonkAI(Creature* creature) : VehicleAI(creature) { }

            uint32 stopBattlingAfterDeathDelay;
            uint32 checkIfTonkIsOutGamingAreaTimer;

            void Reset()
            {
                stopBattlingAfterDeathDelay = 0;
                checkIfTonkIsOutGamingAreaTimer = 5000;
            }

            void EnterCombat(Unit* /*who*/) { }

            void JustDied(Unit* /*killer*/)
            {
                MakeOwnerCallExitsTonk();
            }

            void PassengerBoarded(Unit* /*passenger*/, int8 /*seat*/, bool apply)
            {
                //! We have to do this with VehicleAI script because jumping on a vehicle is
                //! delayed serverside, meaning we can't teleport the player's vehicle as soon
                //! as he/she casts the spell to jump on it.
                if (apply)
                    me->NearTeleportTo(-4129.012f, 6318.809f, 13.116f, 3.463f);
                else
                    MakeOwnerCallExitsTonk();
            }

            void SetData(uint32 /*type*/, uint32 /*data*/)
            {
                //! Don't reset the timer if our corpse was hit a second time.
                if (stopBattlingAfterDeathDelay)
                    return;

                me->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
                me->SetControlled(true, UNIT_STATE_STUNNED);
                stopBattlingAfterDeathDelay = 3000;
            }

            void UpdateAI(uint32 const diff)
            {
                if (stopBattlingAfterDeathDelay)
                {
                    if (stopBattlingAfterDeathDelay <= diff)
                        MakeOwnerCallExitsTonk();
                    else
                        stopBattlingAfterDeathDelay -= diff;
                }

                if (checkIfTonkIsOutGamingAreaTimer <= diff)
                {
                    checkIfTonkIsOutGamingAreaTimer = 5000;

                    if (!me->FindNearestCreature(NPC_TONK_COMMANDER_CONTROLLER, 23.0f))
                        MakeOwnerCallExitsTonk();
                }
                else
                    checkIfTonkIsOutGamingAreaTimer -= diff;
            }

            void MakeOwnerCallExitsTonk()
            {
                if (Unit* owner = me->GetCharmerOrOwner())
                    owner->CastSpell(owner, SPELL_PLAYER_EXITS_TONK);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_darkmoon_faire_tonkAI(creature);
        }
};

class npc_darkmoon_faire_enemy_tonk : public CreatureScript
{
    public:
        npc_darkmoon_faire_enemy_tonk() : CreatureScript("npc_darkmoon_faire_enemy_tonk") { }

        struct npc_darkmoon_faire_enemy_tonkAI : public ScriptedAI
        {
            npc_darkmoon_faire_enemy_tonkAI(Creature* creature) : ScriptedAI(creature) { }

            uint32 recentlyShotSomeoneTimer;

            void Reset()
            {
                recentlyShotSomeoneTimer = 0;
            }

            void EnterCombat(Unit* /*who*/) { }

            void UpdateAI(uint32 const diff)
            {
                if (recentlyShotSomeoneTimer)
                {
                    if (recentlyShotSomeoneTimer > diff)
                        recentlyShotSomeoneTimer -= diff;
                    else
                        recentlyShotSomeoneTimer = 0;
                }
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (recentlyShotSomeoneTimer || !who || who->GetEntry() != NPC_DARKMOON_TONK || me->GetDistance(who) >= 0.03f ||
                    me->IsNonMeleeSpellCasted(true) || !me->isInFront(who) || who->HasFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD))
                    return;

                me->SetTarget(who->GetGUID());
                me->StopMoving();
                me->SetFacingToObject(who);
                me->CastSpell(who, SPELL_MARKED_BY_ENEMY_TONK);
                me->CastSpell(who, SPELL_ENEMY_TONK_CANNON_BLAST);
                recentlyShotSomeoneTimer = 2000; //! There's a 2 second delay inbetween which the enemy tonk can attack player vehicles
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_darkmoon_faire_enemy_tonkAI(creature);
        }
};

enum DarkmoonFaireMysticMage
{
    TELEPORT_COST_LVL_10                    = 30,
    TELEPORT_COST_LVL_40                    = 100,
    TELEPORT_COST_LVL_55                    = 500,
    TELEPORT_COST_LVL_70                    = 2000,
    TELEPORT_COST_LVL_85                    = 3000,

    GOSSIP_TEXT_MYSTIC_MAGE                 = 12992,
    GOSSIP_TEXT_TELEPORTOLOGIST_FUZLEBOB    = 13352,

    NPC_DARKMOON_MYSTIC_MAGE_A              = 54334,
    NPC_DARKMOON_MYSTIC_MAGE_H              = 55382,
    NPC_TELEPORTOLOGIST_FUZLEBOB            = 57850,

    SPELL_TO_THE_STAGING_AREA_A             = 101260,
    SPELL_TO_THE_STAGING_AREA_H             = 103582,
    SPELL_BACK_TO_THE_CANNON                = 109244,
};

#define GOSSIP_TAKE_ME_TO_THE_STAGING_AREA      "Take me to the faire staging area."
#define GOSSIP_TELEPORT_ME_TO_THE_CANNON        "Teleport me to the cannon."
#define GOSSIP_DONT_NEED_TELEPORT               "I don't need a teleport."

class npc_darkmoon_faire_teleporters : public CreatureScript
{
public:
    npc_darkmoon_faire_teleporters() : CreatureScript("npc_darkmoon_faire_teleporters") {}

    uint32 GetMoneyForTeleportation(uint32 level)
    {
        uint32 moneyForTeleport = 0;

        if (level <= 10)
            return TELEPORT_COST_LVL_10;
        else if (level <= 40)
            return TELEPORT_COST_LVL_40;
        else if (level <= 55)
            return TELEPORT_COST_LVL_55;
        else if (level <= 70)
            return TELEPORT_COST_LVL_70;
        else
            return TELEPORT_COST_LVL_85;

        return 0;
    }

    uint32 GetGossipTextForEntry(uint32 entry)
    {
        switch (entry)
        {
            case NPC_DARKMOON_MYSTIC_MAGE_A:
            case NPC_DARKMOON_MYSTIC_MAGE_H:
                return GOSSIP_TEXT_MYSTIC_MAGE;
            case NPC_TELEPORTOLOGIST_FUZLEBOB:
                return GOSSIP_TEXT_TELEPORTOLOGIST_FUZLEBOB;
        }

        return 0;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->GetEntry() == NPC_TELEPORTOLOGIST_FUZLEBOB)
            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, GOSSIP_TELEPORT_ME_TO_THE_CANNON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1, "Teleportation to the cannon will cost:", GetMoneyForTeleportation(player->getLevel()), false);
        else
            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, GOSSIP_TAKE_ME_TO_THE_STAGING_AREA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1, "Travel to the faire staging area will cost:", GetMoneyForTeleportation(player->getLevel()), false);

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_DONT_NEED_TELEPORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        player->PlayerTalkClass->SendGossipMenu(GetGossipTextForEntry(creature->GetEntry()), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
            {
                int64 money = int64(GetMoneyForTeleportation(player->getLevel()));

                if (!player->HasEnoughMoney(money))
                {
                    player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                    player->PlayerTalkClass->SendCloseGossip();
                    break;
                }

                player->ModifyMoney(-money);

                switch (creature->GetEntry())
                {
                    case NPC_DARKMOON_MYSTIC_MAGE_A:
                        creature->CastSpell(player, SPELL_TO_THE_STAGING_AREA_A);
                        break;
                    case NPC_DARKMOON_MYSTIC_MAGE_H:
                        creature->CastSpell(player, SPELL_TO_THE_STAGING_AREA_H);
                        break;
                    case NPC_TELEPORTOLOGIST_FUZLEBOB:
                        creature->CastSpell(player, SPELL_BACK_TO_THE_CANNON);
                        break;

                }
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 2:
                break;
        }

        player->PlayerTalkClass->SendCloseGossip();
        return true;
    }
};

enum
{
    NPC_HE_SHOOTS_HE_SCORES_TARGET_BUNNY    = 24171,

    NPC_GUID_TARGET_BUNNY_0                 = 699457,
    NPC_GUID_TARGET_BUNNY_1                 = 699458,
    NPC_GUID_TARGET_BUNNY_2                 = 699514,

    SPELL_TARGET_INDICATOR_ARROW            = 43313,
    SPELL_QUICK_SHOT_INDICATOR              = 101010,
    SPELL_HIT_TARGET                        = 43300,
    SPELL_QUICK_SHOT_BONUS                  = 101012,
    SPELL_SHOOT                             = 101872,
};

class npc_darkmoon_faire_target_controller : public CreatureScript
{
    public:
        npc_darkmoon_faire_target_controller() : CreatureScript("npc_darkmoon_faire_target_controller") { }

        struct npc_darkmoon_faire_target_controllerAI : public ScriptedAI
        {
            npc_darkmoon_faire_target_controllerAI(Creature* creature) : ScriptedAI(creature) { }

            uint32 spawnNewTargetTimer;
            uint64 lastTargetedBunnyGuid;
            uint32 allBunnyGuids[3];
            std::list<Creature*> bunnyList;
            Creature* lastBunny; //! I realize it's a bad idea to store pointers but there's problems otherwise. NULL-checks are present.

            void Reset()
            {
                spawnNewTargetTimer = 3500;
                lastTargetedBunnyGuid = 0;
                lastBunny = NULL;

                me->GetCreatureListWithEntryInGrid(bunnyList, NPC_HE_SHOOTS_HE_SCORES_TARGET_BUNNY, 30.0f);
            }

            void EnterCombat(Unit* /*who*/) { }

            void UpdateAI(uint32 const diff)
            {
                if (spawnNewTargetTimer <= diff)
                {
                    if (!lastBunny)
                    {
                        spawnNewTargetTimer = 3500;

                        if ((lastBunny = WoWSource::Containers::SelectRandomContainerElement(bunnyList)))
                        {
                            lastBunny->AddAura(SPELL_TARGET_INDICATOR_ARROW, lastBunny);
                            lastBunny->AddAura(SPELL_QUICK_SHOT_INDICATOR, lastBunny);
                        }
                    }
                    else
                    {
                        spawnNewTargetTimer = 1500;

                        //! Arrow is permanent so we manually remove it
                        if (lastBunny)
                            lastBunny->RemoveAurasDueToSpell(SPELL_TARGET_INDICATOR_ARROW);

                        lastBunny = NULL;
                    }
                }
                else
                    spawnNewTargetTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_darkmoon_faire_target_controllerAI(creature);
        }
};

class npc_darkmoon_faire_target_bunny : public CreatureScript
{
    public:
        npc_darkmoon_faire_target_bunny() : CreatureScript("npc_darkmoon_faire_target_bunny") { }

        struct npc_darkmoon_faire_target_bunnyAI : public ScriptedAI
        {
            npc_darkmoon_faire_target_bunnyAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() { }

            void EnterCombat(Unit* /*who*/) { }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (!spell || spell->Id != SPELL_SHOOT || !caster || caster->GetTypeId() != TYPEID_PLAYER || !me->HasAura(SPELL_TARGET_INDICATOR_ARROW))
                    return;

                caster->CastSpell(caster, SPELL_HIT_TARGET);

                //! Quick shot deserves double credit
                if (me->HasAura(SPELL_QUICK_SHOT_INDICATOR))
                {
                    //! We must cast it to the player because the spell has a limitation in the DBCs (requires the quick shot
                    //! indicator aura on caster). The spell rewards with extra credit + achievement
                    me->CastSpell(caster, SPELL_QUICK_SHOT_BONUS);
                    me->AI()->Talk(0, caster->GetGUID()); //! Send 'Quick Shot!' boss whisper to player
                }
            }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_darkmoon_faire_target_bunnyAI(creature);
        }
};

class npc_darkmoon_faire_korgol_crushskull : public CreatureScript
{
public:
    npc_darkmoon_faire_korgol_crushskull() : CreatureScript("npc_darkmoon_faire_korgol_crushskull") {}

    struct npc_darkmoon_faire_korgol_crushskullAI : public ScriptedAI
    {
        npc_darkmoon_faire_korgol_crushskullAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 delayStartDarkmoonDeathmatch;
        uint32 checkForGameEventTimer;
        uint32 checkForPlayersWithoutFfaTimer;

        void Reset()
        {
            delayStartDarkmoonDeathmatch = 0;
            checkForGameEventTimer = 60000; //! 1 minute
            checkForPlayersWithoutFfaTimer = 5000;
        }

        void EnterCombat(Unit* /*who*/) {}

        void UpdateAI(uint32 const diff)
        {
            if (checkForGameEventTimer <= diff)
            {
                if (sGameEventMgr->IsActiveEvent(EVENT_DARKMOON_FAIRE_DEATHMATCH_PREPARE) && !sGameEventMgr->IsActiveEvent(EVENT_DARKMOON_FAIRE_DEATHMATCH_CHEST_SPAWN))
                {
                    checkForGameEventTimer = 600000; //! 10 minutes
                    Talk(0); //! Yell the deathmatch is about to begin
                    delayStartDarkmoonDeathmatch = 7000;
                } else
                    checkForGameEventTimer = 60000; //! 1 minute
            } else
                checkForGameEventTimer -= diff;

            if (delayStartDarkmoonDeathmatch)
            {
                if (delayStartDarkmoonDeathmatch <= diff)
                {
                    //! For some reason none of the grid searchers seem to work for gameobjects. Therefore
                    //! I have to do it from the creaturescript..
                    std::list<Player*> players;
                    WoWSource::AnyPlayerInObjectRangeCheck checker(me, 95.0f);
                    WoWSource::PlayerListSearcher<WoWSource::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
                    me->VisitNearbyWorldObject(95.0f, searcher);

                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if ((*itr)->GetAreaId() != AREA_ID_DARKMOON_DEATHMATCH)
                            continue;

                        (*itr)->CastSpell((*itr), SPELL_EXIT_DEATHMATCH_INSTANT, true);
                    }

                    delayStartDarkmoonDeathmatch = 0;
                    Talk(1); //! Yell that the deathmatch has begun
                    sGameEventMgr->StartEvent(EVENT_DARKMOON_FAIRE_DEATHMATCH_CHEST_SPAWN);
                } else
                    delayStartDarkmoonDeathmatch -= diff;
            }

            if (checkForPlayersWithoutFfaTimer <= diff)
            {
                //! Mark all players inside the deathmatch for FFA. HORRIBLE hack, I know. No time to find the original cause of
                //! this issue. Please just act like you've never seen this, I really don't like it myself either. :)
                std::list<Player*> playerList;
                WoWSource::AnyPlayerInObjectRangeCheck checker(me, 95.0f);
                WoWSource::PlayerListSearcher<WoWSource::AnyPlayerInObjectRangeCheck> searcher(me, playerList, checker);
                //WoWSource::VisitNearbyWorldObject(me, 95.0f, searcher);

                for (std::list<Player*>::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                {
                    if ((*itr)->GetAreaId() != AREA_ID_DARKMOON_DEATHMATCH)
                        continue;

                    if (!(*itr)->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP))
                        (*itr)->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);
                }

                checkForPlayersWithoutFfaTimer = 7500;
            } else
                checkForPlayersWithoutFfaTimer -= diff;
        }

        //void sOnGameEvent(bool started, uint16 eventId)
        //{
        //    if (!started || eventId != EVENT_DARKMOON_FAIRE_DEATHMATCH_PREPARE)
        //        return;

        //    Talk(0); //! Yell the deathmatch is about to begin
        //    delayStartDarkmoonDeathmatch = 7000;
        //}
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_darkmoon_faire_korgol_crushskullAI(creature);
    }
};

enum DarkmoonFaireSpawnOfJubJub
{
    ENTRY_MORJA = 14871,

    GO_ENTRY_DARKMMON_IRON_ALE_MUG = 165578,

    WAYPOINT_ID_JUBJUB_END = 7,
};

class npc_darkmoon_faire_jubjub : public CreatureScript
{
public:
    npc_darkmoon_faire_jubjub() : CreatureScript("npc_darkmoon_faire_jubjub") { }

    struct npc_darkmoon_faire_jubjubAI : public npc_escortAI
    {
        npc_darkmoon_faire_jubjubAI(Creature* creature) : npc_escortAI(creature)
        {
            SetDespawnAtEnd(false);
            SetDespawnAtFar(false);
        }

        uint32 finishedGuzzlingTheKegTimer;
        uint32 checkIfKegSpawnedTimer;
        uint32 despawnOrUnsummonDelay;
        bool eventIsRunning;

        void Reset()
        {
            finishedGuzzlingTheKegTimer = 0;
            checkIfKegSpawnedTimer = 10000;
            despawnOrUnsummonDelay = 0;
            eventIsRunning = false;
        }

        void EnterCombat(Unit* /*who*/) { }

        void WaypointReached(uint32 pointId)
        {
            if (pointId != WAYPOINT_ID_JUBJUB_END)
                return;

            if (GameObject* darkIronAleMug = me->FindNearestGameObject(GO_ENTRY_DARKMMON_IRON_ALE_MUG, 5.0f))
                me->SetFacingToObject(darkIronAleMug);

            Talk(0);
            finishedGuzzlingTheKegTimer = 3000;
        }

        void UpdateAI(uint32 const diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (!eventIsRunning)
            {
                if (checkIfKegSpawnedTimer <= diff)
                {
                    //! Find the keg within 10 yards of Morja
                    if (Creature* morja = me->FindNearestCreature(ENTRY_MORJA, 150.0f))
                    {
                        if (morja->FindNearestGameObject(GO_ENTRY_DARKMMON_IRON_ALE_MUG, 10.0f))
                        {
                            eventIsRunning = true;

                            if (!IsEscorted())
                                Start();
                        }
                    }

                    checkIfKegSpawnedTimer = 10000;
                }
                else
                    checkIfKegSpawnedTimer -= diff;
            }

            if (despawnOrUnsummonDelay)
            {
                if (despawnOrUnsummonDelay <= diff)
                {
                    despawnOrUnsummonDelay = 0;

                    if (Creature* morja = me->FindNearestCreature(ENTRY_MORJA, 20.0f))
                        morja->SetFacingTo(morja->GetHomePosition().GetOrientation());

                    me->DespawnOrUnsummon();
                }
                else
                    despawnOrUnsummonDelay -= diff;
            }

            if (finishedGuzzlingTheKegTimer)
            {
                if (finishedGuzzlingTheKegTimer <= diff)
                {
                    finishedGuzzlingTheKegTimer = 0;

                    if (Creature* morja = me->FindNearestCreature(ENTRY_MORJA, 20.0f))
                    {
                        if (morja->IsAIEnabled)
                        {
                            morja->AI()->Talk(0);
                            morja->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                            morja->SetFacingToObject(me);
                            despawnOrUnsummonDelay = 5000;
                        }
                    }
                }
                else
                    finishedGuzzlingTheKegTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_darkmoon_faire_jubjubAI(creature);
    }
};

enum DarkmoonFairEnemyMiniZep
{
    SPELL_BOMBS_AWAY = 102209,
};

//! Hacky as hell...
class npc_darkmoon_faire_enemy_minizep : public CreatureScript
{
public:
    npc_darkmoon_faire_enemy_minizep() : CreatureScript("npc_darkmoon_faire_enemy_minizep") {}

    struct npc_darkmoon_faire_enemy_minizepAI : public ScriptedAI
    {
        npc_darkmoon_faire_enemy_minizepAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 checkForBombsAwayAuraTimer;

        void Reset()
        {
            checkForBombsAwayAuraTimer = 5000;
            me->AddAura(SPELL_BOMBS_AWAY, me);
        }

        void EnterCombat(Unit* /*who*/) {}

        void UpdateAI(uint32 const diff)
        {
            if (checkForBombsAwayAuraTimer <= diff)
            {
                checkForBombsAwayAuraTimer = 5000;

                if (!me->HasAura(SPELL_BOMBS_AWAY))
                    me->AddAura(SPELL_BOMBS_AWAY, me);
            } else
                checkForBombsAwayAuraTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_darkmoon_faire_enemy_minizepAI(creature);
    }
};

void AddSC_darkmoon_faire()
{
    new spell_darkmoon_faire_ring_toss();
    new spell_darkmoon_faire_ring_toss_missile();
    new spell_darkmoon_faire_whack();
    new spell_darkmoon_faire_whack_a_gnoll_summon_aura();
    new spell_darkmoon_faire_whack_a_gnoll_aura();
    new spell_darkmoon_faire_using_darkmoon_tonk_controller();
    new spell_darkmoon_faire_darkmoon_deathmatch();
    new spell_darkmoon_faire_target_conditions();
    new spell_darkmoon_faire_shoot_rifle();
    new spell_darkmoon_faire_crack_shot();
    new spell_darkmoon_faire_cannon_prep();
    new spell_darkmoon_faire_fire_cannon();
    new spell_darkmoon_faire_just_shot_from_cannon();
    new spell_darkmoon_faire_jubling_cooldown();
    new spell_darkmoon_faire_enter_deathmatch();

    new go_darkmoon_faire_darkmoon_cannon_balls();
    new go_darkmoon_faire_darkmoon_treasure_chest();

    new npc_darkmoon_faire_tonk_commander_controller();
    new npc_darkmoon_faire_tonk();
    new npc_darkmoon_faire_enemy_tonk();
    new npc_darkmoon_faire_tonk_target();
    new npc_darkmoon_faire_teleporters();
    new npc_darkmoon_faire_target_controller();
    new npc_darkmoon_faire_target_bunny();
    new npc_darkmoon_faire_korgol_crushskull();
    new npc_darkmoon_faire_jubjub();
    new npc_darkmoon_faire_injured_carnie();
    new npc_darkmoon_faire_enemy_minizep();
}
