/*
 * This file is part of mod-stellar-tarot.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "StellarTarotTutorial.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "StellarTarotBinder.h"
#include "StellarTarotLayout.h"
#include "StellarTarotMgr.h"
#include "StellarTarotStrings.h"
#include "WorldSession.h"
#include <map>
#include <set>

namespace
{
    enum Step : uint32
    {
        STEP_OPEN  = 87001,
        STEP_STUDY = 87002,
        STEP_EQUIP = 87003,
        STEP_PLACE = 87004,
        STEP_MATCH = 87005,
    };
    constexpr uint32 STEPS[] = { STEP_OPEN, STEP_STUDY, STEP_EQUIP, STEP_PLACE, STEP_MATCH };

    // Ce que l'etape 2 demande d'etudier : le Plateau du novice et The Seed, ceux que l'etape 1 donne.
    constexpr uint32 NOVICE_BOARD = 1;
    constexpr uint32 THE_SEED = 58;

    // La fenetre ouverte depuis la connexion : le seul geste qui ne laisse pas de trace.
    std::set<ObjectGuid>& Opened()
    {
        static std::set<ObjectGuid> opened;
        return opened;
    }

    bool Done(Player* player, uint32 step)
    {
        switch (step)
        {
            case STEP_OPEN:
                return Opened().count(player->GetGUID()) != 0;
            case STEP_STUDY:
            {
                uint32 const account = player->GetSession()->GetAccountId();
                return StellarTarotBinder::KnowsBoard(account, NOVICE_BOARD)
                    && StellarTarotBinder::KnowsCard(account, THE_SEED);
            }
            case STEP_EQUIP:
                return StellarTarotLayouts::Load(player).boardId != 0;
            case STEP_PLACE:
                return !StellarTarotLayouts::Load(player).cells.empty();
            case STEP_MATCH:
            {
                // DEUX CARTES QUI SE TOUCHENT par des chiffres egaux : les bords du plateau eveillent deja
                // une carte seule, la lecon est celle des cartes entre elles.
                StellarTarotLayout const layout = StellarTarotLayouts::Load(player);
                for (StellarTarotPlacement const& a : layout.cells)
                    for (StellarTarotPlacement const& b : layout.cells)
                    {
                        StellarTarotCard const* ca = sStellarTarotMgr->Card(a.cardId);
                        StellarTarotCard const* cb = sStellarTarotMgr->Card(b.cardId);
                        if (!ca || !cb)
                            continue;
                        if (b.row == a.row && b.col == a.col + 1
                            && ca->edges[STELLAR_TAROT_EDGE_RIGHT] == cb->edges[STELLAR_TAROT_EDGE_LEFT])
                            return true;
                        if (b.col == a.col && b.row == a.row + 1
                            && ca->edges[STELLAR_TAROT_EDGE_BOTTOM] == cb->edges[STELLAR_TAROT_EDGE_TOP])
                            return true;
                    }
                return false;
            }
        }
        return false;
    }

    // La premiere etape que le joueur n'a pas rendue ; 0 quand le tutoriel est fini.
    uint32 Current(Player* player)
    {
        for (uint32 step : STEPS)
            if (!player->GetQuestRewardStatus(step))
                return step;
        return 0;
    }

    // LE TAROT DU COMPTE (demande du 2026-10-18 : « si le joueur a debloque Stellar Tarot sur l'un de ses
    // personnages, il est debloque sur les autres personnages et les prochains crees des le niveau 1. La
    // quete ne doit pas etre redeclenchee ») : ouvert pour tout le compte des qu'un personnage a atteint
    // le niveau, et un seul tutoriel par compte -- celui du personnage qui l'a commence.
    struct Compte
    {
        bool unlocked = false;
        uint32 owner = 0;           // le personnage qui joue le tutoriel ; 0 : personne encore
        bool done = false;
    };

    std::map<uint32, Compte>& Comptes()
    {
        static std::map<uint32, Compte> comptes;
        return comptes;
    }

    void Ecrire(uint32 account, Compte const& c)
    {
        CharacterDatabase.Execute("REPLACE INTO mod_stellar_tarot_account (account_id, unlocked, tutorial_guid, "
                                  "tutorial_done) VALUES ({}, {}, {}, {})",
                                  account, c.unlocked ? 1 : 0, c.owner, c.done ? 1 : 0);
    }

    // Lu a la premiere connexion d'un personnage du compte. Un compte sans ligne -- d'avant cette regle --
    // se deduit de ses personnages : l'un d'eux au niveau, un tutoriel commence ou rendu.
    Compte& Lire(uint32 account)
    {
        auto it = Comptes().find(account);
        if (it != Comptes().end())
            return it->second;
        Compte c;
        if (QueryResult r = CharacterDatabase.Query("SELECT unlocked, tutorial_guid, tutorial_done FROM "
                                                    "mod_stellar_tarot_account WHERE account_id = {}", account))
        {
            Field* f = r->Fetch();
            c.unlocked = f[0].Get<uint8>() != 0;
            c.owner = f[1].Get<uint32>();
            c.done = f[2].Get<uint8>() != 0;
        }
        else
        {
            if (QueryResult r = CharacterDatabase.Query("SELECT MAX(level) FROM characters WHERE account = {}", account))
                c.unlocked = r->Fetch()[0].Get<uint32>() >= sStellarTarotMgr->UnlockLevel();
            if (QueryResult r = CharacterDatabase.Query("SELECT 1 FROM character_queststatus_rewarded q JOIN characters c "
                                                        "ON c.guid = q.guid WHERE c.account = {} AND q.quest = {}",
                                                        account, uint32(STEP_MATCH)))
                c.done = true;
            if (QueryResult r = CharacterDatabase.Query("SELECT q.guid FROM character_queststatus q JOIN characters c "
                                                        "ON c.guid = q.guid WHERE c.account = {} AND q.quest BETWEEN {} AND {} "
                                                        "LIMIT 1", account, uint32(STEP_OPEN), uint32(STEP_MATCH)))
                c.owner = r->Fetch()[0].Get<uint32>();
            Ecrire(account, c);
        }
        // le personnage qui jouait le tutoriel a ete supprime : le suivant le reprend
        if (c.owner && !c.done && !CharacterDatabase.Query("SELECT 1 FROM characters WHERE guid = {}", c.owner))
            c.owner = 0;
        return Comptes()[account] = c;
    }

    uint32 Compte_(Player const* player)
    {
        return player && player->GetSession() ? player->GetSession()->GetAccountId() : 0;
    }

    // le joueur deja prevenu que ses sacs etaient pleins, quete par quete
    std::set<std::pair<ObjectGuid, uint32>>& Prevenus()
    {
        static std::set<std::pair<ObjectGuid, uint32>> prevenus;
        return prevenus;
    }

    bool Ready(Player const* player)
    {
        return player && player->GetSession() && sStellarTarotMgr->Enabled() && sStellarTarotMgr->TutorialEnabled()
            && StellarTarotTutorial::Unlocked(player);
    }
}

bool StellarTarotTutorial::Unlocked(Player const* player)
{
    if (!player)
        return false;
    if (player->GetLevel() >= sStellarTarotMgr->UnlockLevel())
        return true;
    uint32 const account = Compte_(player);
    return account && Lire(account).unlocked;
}

void StellarTarotTutorial::Offer(Player* player)
{
    if (!player || !player->GetSession())
        return;
    uint32 const account = Compte_(player);
    Compte& compte = Lire(account);
    // un personnage au niveau ouvre le tarot a tout le compte
    if (!compte.unlocked && player->GetLevel() >= sStellarTarotMgr->UnlockLevel())
    {
        compte.unlocked = true;
        Ecrire(account, compte);
    }
    if (!Ready(player))
        return;
    // UN SEUL TUTORIEL PAR COMPTE : fini, ou joue par un autre personnage, il ne revient pas
    if (compte.done || (compte.owner && compte.owner != player->GetGUID().GetCounter()))
        return;
    if (!compte.owner)
    {
        compte.owner = player->GetGUID().GetCounter();
        Ecrire(account, compte);
    }
    uint32 const step = Current(player);
    Quest const* quest = step ? sObjectMgr->GetQuestTemplate(step) : nullptr;
    if (!quest)
        return;
    if (player->GetQuestStatus(step) == QUEST_STATUS_NONE)
    {
        if (!player->CanAddQuest(quest, true))
            return;                         // le journal est plein : on reessaiera au prochain geste
        player->AddQuestAndCheckCompletion(quest, nullptr);
        ChatHandler(player->GetSession()).PSendModuleSysMessage(STELLAR_TAROT_MODULE, STELLAR_TAROT_STR_TUTORIAL_STEP);
    }
    Advance(player);
}

void StellarTarotTutorial::OnOpen(Player* player)
{
    if (!player)
        return;
    Opened().insert(player->GetGUID());
    Advance(player);
}

void StellarTarotTutorial::Advance(Player* player)
{
    if (!Ready(player))
        return;
    Compte& compte = Lire(Compte_(player));
    if (compte.done || compte.owner != player->GetGUID().GetCounter())
        return;
    // Les etapes faites d'avance s'enchainent : une seule passe les rend toutes.
    for (uint8 garde = 0; garde < 5; ++garde)
    {
        uint32 const step = Current(player);
        Quest const* quest = step ? sObjectMgr->GetQuestTemplate(step) : nullptr;
        if (!quest)
            return;
        QuestStatus const status = player->GetQuestStatus(step);
        if (status == QUEST_STATUS_NONE)
        {
            Offer(player);
            return;
        }
        if (status == QUEST_STATUS_INCOMPLETE)
        {
            if (!Done(player, step))
                return;
            player->AreaExploredOrEventHappens(step);
        }
        if (player->GetQuestStatus(step) != QUEST_STATUS_COMPLETE)
            return;
        // LA RECOMPENSE NE SE DONNE QU'AVEC LA PLACE POUR ELLE (signalement du 2026-10-18 : « pas assez
        // d'espace dans l'inventaire » et l'objet perdu). Sacs pleins : la quete reste terminee au journal,
        // le joueur est prevenu une fois, et la mise a jour la rend des que la place est faite.
        if (!player->CanRewardQuest(quest, 0, false))
        {
            if (Prevenus().insert({ player->GetGUID(), step }).second)
            {
                std::string titre = quest->GetTitle();
                if (QuestLocale const* ql = sObjectMgr->GetQuestLocale(step))
                    ObjectMgr::GetLocaleString(ql->Title, player->GetSession()->GetSessionDbLocaleIndex(), titre);
                ChatHandler(player->GetSession()).PSendModuleSysMessage(STELLAR_TAROT_MODULE,
                    STELLAR_TAROT_STR_TUTORIAL_ROOM, titre);
            }
            return;
        }
        Prevenus().erase({ player->GetGUID(), step });
        player->RewardQuest(quest, 0, player);
        if (step == STEP_MATCH)
        {
            compte.done = true;
            Ecrire(Compte_(player), compte);
            return;
        }
        // l'etape suivante entre au journal ; elle est peut-etre deja faite
        uint32 const next = Current(player);
        Quest const* suite = next ? sObjectMgr->GetQuestTemplate(next) : nullptr;
        if (!suite || player->GetQuestStatus(next) != QUEST_STATUS_NONE || !player->CanAddQuest(suite, true))
            return;
        player->AddQuestAndCheckCompletion(suite, nullptr);
        ChatHandler(player->GetSession()).PSendModuleSysMessage(STELLAR_TAROT_MODULE, STELLAR_TAROT_STR_TUTORIAL_STEP);
    }
}

void StellarTarotTutorial::OnLogout(Player* player)
{
    if (player)
        Opened().erase(player->GetGUID());
}

void StellarTarotTutorial::OnSecond(Player* player)
{
    // seule une quete terminee qui attend sa recompense se regarde ici -- rien d'autre ne lit la base
    if (!player || !player->GetSession())
        return;
    uint32 const step = Current(player);
    if (step && player->GetQuestStatus(step) == QUEST_STATUS_COMPLETE)
        Advance(player);
}
