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
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "StellarTarotBinder.h"
#include "StellarTarotLayout.h"
#include "StellarTarotMgr.h"
#include "StellarTarotStrings.h"
#include "WorldSession.h"
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

    bool Ready(Player const* player)
    {
        return player && player->GetSession() && sStellarTarotMgr->Enabled() && sStellarTarotMgr->TutorialEnabled()
            && StellarTarotTutorial::Unlocked(player);
    }
}

bool StellarTarotTutorial::Unlocked(Player const* player)
{
    return player && player->GetLevel() >= sStellarTarotMgr->UnlockLevel();
}

void StellarTarotTutorial::Offer(Player* player)
{
    if (!Ready(player))
        return;
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
        if (!player->CanRewardQuest(quest, 0, true))
            return;                         // les sacs sont pleins : la quete attend, terminee
        player->RewardQuest(quest, 0, player);
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
