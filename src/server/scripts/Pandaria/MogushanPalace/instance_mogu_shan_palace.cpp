/*
 *    Dungeon : Template of Mogu'shan Palace 87-89
 *    Instance General Script
 *    Jade servers
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "VMapFactory.h"
#include "mogu_shan_palace.h"
#include "Log.h"
#include "Containers.h"

#include <numeric>

class instance_mogu_shan_palace : public InstanceMapScript
{
public:
    instance_mogu_shan_palace() : InstanceMapScript("instance_mogu_shan_palace", 994) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_mogu_shan_palace_InstanceMapScript(map);
    }

    struct instance_mogu_shan_palace_InstanceMapScript : public InstanceScript
    {
        instance_mogu_shan_palace_InstanceMapScript(Map* map) : InstanceScript(map) {}

        uint32 m_auiEncounter[MAX_TYPES];

        /*
         * * Trial of the king.
         */
        uint64 m_uiXinGuid;
        uint64 m_uiKuaiGuid;
        uint64 m_uiMingGuid;
        uint64 m_uiHaiyanGuid;
        uint64 m_uiBeaconGuid;
        std::list<uint64> m_lScrapperList;
        std::list<uint64> m_lAdeptList;
        std::list<uint64> m_lGruntList;

        uint32 m_uiBossCount;
        uint32 m_auiBossNumber[3];
        /*
         * * End of the trial of the king.
         */
        /*
         * * m_uiGekkanGuid.
         */
        uint64 m_uiGekkanGuid;
        uint64 m_uiAncientTreasureGuid;
        /*
         * * End of m_uiGekkanGuid.
         */
        /*
         * * Xin the weaponmaster.
         */
        std::list<uint64> m_lStaffList;
        std::list<uint64> m_lAxeList;
        /*
         * * End of Xin the weaponmaster.
         */

        uint64 m_uiScoutGuid;

        // Storage
        uint64 m_auiGuids64[MAX_GUIDS];
        std::unordered_map<uint32, uint64> m_mGoEntryGuidMap;

        void Initialize()
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

            // pretty dumb to even try to utilize data without this... lol
            SetBossNumber(MAX_ENCOUNTER);

            m_uiXinGuid = 0;
            m_uiKuaiGuid = 0;
            m_uiMingGuid = 0;
            m_uiHaiyanGuid = 0;
            m_uiBeaconGuid = 0;

            m_uiGekkanGuid = 0;
            m_uiAncientTreasureGuid = 0;

            InitializeTrialOfKing();

            m_uiScoutGuid = 0;
        }

        void InitializeTrialOfKing()
        {
            m_uiBossCount = 0;

            std::iota(std::begin(m_auiBossNumber), std::end(m_auiBossNumber), 0);
            std::random_shuffle(std::begin(m_auiBossNumber), std::end(m_auiBossNumber));
        }

        GameObject* GetGameObjectFromStorage(uint32 uiEntry)
        {
            GameObject* go = NULL;

            std::unordered_map<uint32, uint64>::iterator find = m_mGoEntryGuidMap.find(uiEntry);

            if (find != m_mGoEntryGuidMap.cend())
                go = instance->GetGameObject(find->second);

            return go;
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
                case DATA_GEKKAN:
                    HandleGameObject(0, state == DONE, GetGameObjectFromStorage(GO_DOOR_AFTER_TRIAL));
                    if (GameObject* pTreasure = instance->GetGameObject(m_uiAncientTreasureGuid))
                        pTreasure->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                    break;
            }

            return true;
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_TRIAL_CHEST:
                case GO_TRIAL_CHEST_HC:
                    m_mGoEntryGuidMap.insert(std::make_pair(go->GetEntry(), go->GetGUID()));
                    break;
                case GO_DOOR_BEFORE_TRIAL:
                case GO_DOOR_AFTER_TRIAL:
                case GO_DOOR_BEFORE_KING:
                case GO_DOOR_BEFORE_KING2:
                case GO_GEKKAN_TREASURE_DOOR:
                case GO_ANCIENT_MOGU_TREASURE:
                    m_mGoEntryGuidMap.insert(std::make_pair(go->GetEntry(), go->GetGUID()));
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                    break;
                case GO_USELESS_DOOR:
                    // dont need to use memory space for this
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                    break;
            }
        }

        void OnCreatureCreate(Creature* pCreature)
        {
            OnCreatureCreate_trial_of_the_king(pCreature);
            OnCreatureCreate_xin_the_weaponmaster(pCreature);

            switch (pCreature->GetEntry())
            {
                case CREATURE_GLINTROK_SCOUT:
                    m_uiScoutGuid = pCreature->GetGUID();
                    pCreature->setActive(true);
                    pCreature->SetPhaseMask(128, true);
                    break;
                case CREATURE_BEACON:
                    m_uiBeaconGuid = pCreature->GetGUID();
                    break;
            }
        }

        void OnUnitDeath(Unit* unit)
        {
            if (unit->ToCreature() && unit->ToCreature()->GetEntry() == CREATURE_GEKKAN)
            {
                SetBossState(DATA_GEKKAN, DONE);

                if (GameObject* pGo = GetGameObjectFromStorage(GO_ANCIENT_MOGU_TREASURE))
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);

                HandleGameObject(0, true, GetGameObjectFromStorage(GO_GEKKAN_TREASURE_DOOR));
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type >= MAX_TYPES)
                return;

            SetData_trial_of_the_king(type, data);
            SetData_xin_the_weaponmaster(type, data);

            if (type == TYPE_SHUFFLE_ORDER && m_uiBossCount >= 3)
                m_auiEncounter[type] = DONE;
            else
                m_auiEncounter[type] = data;

            if (data == DONE)
                SaveToDB();
        }

        std::string GetSaveData()
        {
            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0] << ' ' << m_auiEncounter[1] << ' ' << m_auiEncounter[2] << ' '
                       << m_auiEncounter[3] << ' ' << m_auiEncounter[4] << ' ' << m_auiEncounter[5] << ' '
                       << m_auiEncounter[6] << ' ' << m_auiEncounter[7] << ' ' << m_auiEncounter[8] << ' '
                       << m_auiEncounter[9] << ' ' << m_auiEncounter[10] << ' ' << m_auiEncounter[11] << ' '
                       << m_auiEncounter[12] << ' ' << m_auiEncounter[13];
            return saveStream.str();
        }

        uint32 GetData(uint32 type)
        {
            if (type >= MAX_TYPES)
                return 0;

            return m_auiEncounter[type];
        }

        uint64 GetData64(uint32 uiType)
        {
            if (uiType < MAX_GUIDS)
                return m_auiGuids64[uiType];

            auto const find = m_mGoEntryGuidMap.find(uiType);
            if (find != m_mGoEntryGuidMap.cend())
                return find->second;

            return 0;
        }

        bool isWipe()
        {
            Map::PlayerList const& PlayerList = instance->GetPlayers();

            if (!PlayerList.isEmpty())
            {
                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                {
                    if (Player* plr = i->getSource())
                        if (plr->isAlive() && !plr->isGameMaster())
                            return false;
                }
            }
            return true;
        }

        void SetData_xin_the_weaponmaster(uint32 type, uint32 data)
        {
            switch (type)
            {
                case TYPE_ACTIVATE_ANIMATED_STAFF:
                {
                    if (Creature* pCreature = instance->GetCreature(WoWSource::Containers::SelectRandomContainerElement(m_lStaffList)))
                        if (pCreature->GetAI())
                            pCreature->GetAI()->DoAction(0); //ACTION_ACTIVATE
                    break;
                }
                case TYPE_ACTIVATE_ANIMATED_AXE:
                {
                    for (auto guid : m_lAxeList)
                    {
                        if (Creature* pCreature = instance->GetCreature(guid))
                        {
                            if (data)
                            {
                                if (pCreature->AI())
                                    pCreature->AI()->DoAction(0);
                            }
                            else
                            {
                                if (pCreature->AI())
                                    pCreature->AI()->DoAction(1);
                            }
                        }
                    }
                    break;
                }
                case TYPE_XIN:
                    HandleGameObject(0, type != IN_PROGRESS, GetGameObjectFromStorage(GO_DOOR_BEFORE_KING));
                    HandleGameObject(0, type != IN_PROGRESS, GetGameObjectFromStorage(GO_DOOR_BEFORE_KING2));
                    break;
            }
        }

        void OnCreatureCreate_xin_the_weaponmaster(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
                case 59481:
                    pCreature->SetReactState(REACT_PASSIVE);
                    break;
                case CREATURE_ANIMATED_STAFF:
                    m_lStaffList.push_back(pCreature->GetGUID());
                    break;
                case CREATURE_ANIMATED_AXE:
                    m_lAxeList.push_back(pCreature->GetGUID());
                    break;
                case CREATURE_CROSSBOW:
                    pCreature->SetReactState(REACT_PASSIVE);
                    pCreature->CastSpell(pCreature, SPELL_PERMANENT_FEIGN_DEATH, true);
                    pCreature->SetDisplayId(42197);
                    break;
            }
        }

        void SetData_trial_of_the_king(uint32 type, uint32 data)
        {
            switch (type)
            {
                case TYPE_WIPE_FIRST_BOSS:
                {
                    HandleGameObject(0, true, GetGameObjectFromStorage(GO_DOOR_BEFORE_TRIAL));
                    switch (data)
                    {
                        case 0:
                            for (auto guid : m_lAdeptList)
                            {
                                Creature* pCreature = instance->GetCreature(guid);
                                if (!pCreature)
                                    continue;

                                if (pCreature && pCreature->GetAI())
                                    pCreature->GetAI()->DoAction(1); //EVENT_RETIRE
                                    pCreature->RemoveAllAuras();
                            }
                            break;
                        case 1:
                            for (auto guid : m_lScrapperList)
                            {
                                Creature* pCreature = instance->GetCreature(guid);
                                if (!pCreature)
                                    continue;

                                if (pCreature && pCreature->GetAI())
                                    pCreature->GetAI()->DoAction(1); //EVENT_RETIRE
                                    pCreature->RemoveAllAuras();
                            }
                            break;
                        case 2:
                            for (auto guid : m_lGruntList)
                            {
                                Creature* pCreature = instance->GetCreature(guid);
                                if (!pCreature)
                                    continue;

                                if (pCreature && pCreature->GetAI())
                                    pCreature->GetAI()->DoAction(1); //EVENT_RETIRE
                                    pCreature->RemoveAllAuras();
                            }
                            break;
                    }
                }
                break;
                case TYPE_MING:
                {
                    //Move the m_lAdeptList
                    for (auto guid : m_lAdeptList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(0); //EVENT_ENCOURAGE
                    }
                    Creature* pMing = instance->GetCreature(m_uiMingGuid);
                    if (!pMing)
                        return;
                    pMing->GetMotionMaster()->MovePoint(0, pTrialIntroPositions[type]);
                    pMing->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    pMing->SetReactState(REACT_AGGRESSIVE);

                    if (pMing->GetAI())
                        pMing->GetAI()->DoAction(0);
                }
                break;
                case TYPE_KUAI:
                {
                    //Move the m_lScrapperList
                    for (auto guid : m_lScrapperList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(0); //EVENT_ENCOURAGE
                    }
                    Creature* pKuai = instance->GetCreature(m_uiKuaiGuid);
                    if (!pKuai)
                        return;
                    pKuai->GetMotionMaster()->MovePoint(0, pTrialIntroPositions[type]);
                    pKuai->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    pKuai->SetReactState(REACT_AGGRESSIVE);

                    // special case action 2 for kuai
                    if (pKuai->GetAI())
                        pKuai->GetAI()->DoAction(2);
                }
                break;
                case TYPE_HAIYAN:
                {
                    //Move the m_lScrapperList
                    for (auto guid : m_lGruntList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(0); //EVENT_ENCOURAGE
                    }
                    Creature* pHaiyan = instance->GetCreature(m_uiHaiyanGuid);
                    if (!pHaiyan)
                        return;
                    pHaiyan->GetMotionMaster()->MovePoint(0, pTrialIntroPositions[type]);
                    pHaiyan->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    pHaiyan->SetReactState(REACT_AGGRESSIVE);

                    // Make sure he talks for the intro
                    if (pHaiyan->GetAI())
                        pHaiyan->GetAI()->DoAction(0);
                }
                break;
                case TYPE_TRIAL_ENDED:
                {
                    for (auto guid : m_lAdeptList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(2); //ACTION_ATTACK

                        std::list<uint64>::iterator itr = m_lGruntList.begin();
                        std::advance(itr, urand(0, m_lGruntList.size() - 1));

                        Creature* pGrunt = instance->GetCreature(*itr);
                        if (pCreature && pGrunt)
                        {
                            pCreature->Attack(pGrunt, true);
                            if (pCreature->AI())
                                pCreature->AI()->AttackStart(pGrunt);
                        }
                    }
                    for (auto guid : m_lGruntList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(2); //ACTION_ATTACK

                        std::list<uint64>::iterator itr = m_lScrapperList.begin();
                        std::advance(itr, urand(0, m_lScrapperList.size() - 1));

                        Creature* pScrapper = instance->GetCreature(*itr);
                        if (pCreature && pScrapper)
                        {
                            pCreature->Attack(pScrapper, true);
                            if (pCreature->AI())
                                pCreature->AI()->AttackStart(pScrapper);
                        }
                    }
                    for (auto guid : m_lScrapperList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(2); //ACTION_ATTACK

                        std::list<uint64>::iterator itr = m_lAdeptList.begin();
                        std::advance(itr, urand(0, m_lAdeptList.size() - 1));

                        Creature* pAdept = instance->GetCreature(*itr);
                        if (pCreature && pAdept)
                        {
                            pCreature->Attack(pAdept, true);
                            if (pCreature->AI())
                                pCreature->AI()->AttackStart(pAdept);
                        }
                    }
                }
                break;
                case TYPE_MING_RETIRED:
                    // This boss is down, move on to next
                    ++m_uiBossCount;

                    //Retire the m_lAdeptList
                    for (auto guid : m_lAdeptList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(1); //EVENT_RETIRE
                    }
                    break;
                case TYPE_KUAI_RETIRED:
                    // This boss is down, move on to next
                    ++m_uiBossCount;

                    //Retire the m_lAdeptList
                    for (auto guid : m_lScrapperList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(1); //EVENT_RETIRE
                    }
                    break;
                case TYPE_HAIYAN_RETIRED:
                    // This boss is down, move on to next
                    ++m_uiBossCount;

                    //Retire the m_lAdeptList
                    for (auto guid : m_lGruntList)
                    {
                        Creature* pCreature = instance->GetCreature(guid);

                        if (pCreature && pCreature->GetAI())
                            pCreature->GetAI()->DoAction(1); //EVENT_RETIRE
                    }
                    break;
                case TYPE_SHUFFLE_ORDER:
                    if (GetData(TYPE_SHUFFLE_ORDER) == DONE)
                        return;

                    if (m_uiBossCount < 3)
                    {
                        SetData(m_auiBossNumber[m_uiBossCount], 0);
                        HandleGameObject(0, false, GetGameObjectFromStorage(GO_DOOR_BEFORE_TRIAL));
                    }
                    else
                    {
                        SetBossState(DATA_TRIAL_OF_THE_KING, DONE);

                        if (GameObject* pGo = GetGameObjectFromStorage(GO_TRIAL_CHEST))
                            SetData64(TYPE_TRIAL_ENDED, pGo->GetGUID());

                        if (Creature* pScout = instance->GetCreature(m_uiScoutGuid))
                        {
                            if (pScout->AI())
                                pScout->AI()->DoAction(0);
                        }

                        HandleGameObject(0, true, GetGameObjectFromStorage(GO_DOOR_BEFORE_TRIAL));
                    }
                    break;
                case TYPE_TRIAL_CHEST:
                    if (GameObject* pGo = GetGameObjectFromStorage(instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC ? GO_TRIAL_CHEST_HC : GO_TRIAL_CHEST))
                        DoRespawnGameObject(pGo->GetGUID(), 7 * DAY);
                    if (Creature* pBeacon = instance->GetCreature(m_uiBeaconGuid))
                        pBeacon->SetPhaseMask(1, true);

                    if (instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
                    {
                        Map::PlayerList const &lPlayers = instance->GetPlayers();
                        for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                        {
                            if (Player * const player = itr->getSource())
                            {
//                                int32 const gain = player->ModifyCurrency(CURRENCY_TYPE_JUSTICE_POINTS, 10000);
//                                if (int32 const toMoney = (gain < 10000 ? 10000 - gain : 0))
//                                   player->ModifyMoney(toMoney * 475);
                            }
                        }
                    }
                    break;
            }
        }

        void SetData64(uint32 uiType, uint64 uiData)
        {
            if (uiType >= MAX_GUIDS)
                return;

            m_auiGuids64[uiType] = uiData;
        }

        void OnCreatureCreate_trial_of_the_king(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case CREATURE_GURTHAN_SCRAPPER:
                case CREATURE_GURTHAN_SCRAPPER_2:
                    m_lScrapperList.push_back(creature->GetGUID());
                    break;
                case CREATURE_HARTHAK_ADEPT:
                case CREATURE_HARTHAK_ADEPT_2:
                    m_lAdeptList.push_back(creature->GetGUID());
                    break;
                case CREATURE_KARGESH_GRUNT:
                case CREATURE_KARGESH_GRUNT_2:
                    m_lGruntList.push_back(creature->GetGUID());
                    break;
                case CREATURE_KUAI_THE_BRUTE:
                    m_uiKuaiGuid = creature->GetGUID();
                    SetData64(TYPE_KUAI, m_uiKuaiGuid);
                    creature->SetReactState(REACT_PASSIVE);
                    break;
                case CREATURE_MING_THE_CUNNING:
                    m_uiMingGuid = creature->GetGUID();
                    SetData64(TYPE_MING, m_uiMingGuid);
                    creature->SetReactState(REACT_PASSIVE);
                    break;
                case CREATURE_HAIYAN_THE_UNSTOPPABLE:
                    m_uiHaiyanGuid = creature->GetGUID();
                    SetData64(TYPE_HAIYAN, m_uiHaiyanGuid);
                    creature->SetReactState(REACT_PASSIVE);
                    break;
                case CREATURE_XIN_THE_WEAPONMASTER_TRIGGER:
                    m_uiXinGuid = creature->GetGUID();
                    creature->SetReactState(REACT_PASSIVE);
                    break;
                case CREATURE_WHIRLING_DERVISH:
                    break;
            }
        }

        void Load(const char* chrIn)
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);
            std::istringstream loadStream(chrIn);

            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3]
                >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6] >> m_auiEncounter[7]
                >> m_auiEncounter[8] >> m_auiEncounter[9] >> m_auiEncounter[10] >> m_auiEncounter[11]
                >> m_auiEncounter[12] >> m_auiEncounter[13];

            // Do not load an encounter as "In Progress" - reset it instead.
            for (uint8 i = 0; i < MAX_TYPES; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;

            OUT_LOAD_INST_DATA_COMPLETE;
        }
    };
};

class go_mogushan_palace_temp_portal : public GameObjectScript
{
public:
    go_mogushan_palace_temp_portal() : GameObjectScript("go_mogushan_palace_temp_portal") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (go->GetInstanceScript() && go->GetInstanceScript()->GetData(TYPE_GEKKAN) != DONE)
            return true;

        if (go->GetPositionZ() < 0.0f)
            player->NearTeleportTo(go->GetPositionX(), go->GetPositionY(), 22.31f, go->GetOrientation());
        else
            player->NearTeleportTo(go->GetPositionX(), go->GetPositionY(), -39.0f, go->GetOrientation());

        return false;
    }
};

void AddSC_instance_mogu_shan_palace()
{
    new instance_mogu_shan_palace();
    new go_mogushan_palace_temp_portal();
}
