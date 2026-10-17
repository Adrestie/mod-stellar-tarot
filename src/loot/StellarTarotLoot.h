/*
 * This file is part of mod-stellar-tarot.
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

#ifndef MOD_STELLAR_TAROT_LOOT_H_
#define MOD_STELLAR_TAROT_LOOT_H_

#include "Define.h"

#include <string>
#include <vector>

class Loot;
class LootStore;
class Player;

// WHERE A CARD OR A BOARD COMES FROM is not here any more: they drop through
// DynamicLoot (the dynamicloot module), from the StellarTarot.Loot.* settings
// of mod-stellar-tarot.conf. What is left are the reserves the CARDS draw from
// when they add something to a loot: the crate of goods, grey trinkets, gear,
// herbs.

namespace StellarTarotLoot
{
    // Reads the reserves from the world database. Called by the manager's
    // Load, so `.tarot reload` reads them again.
    void Load();

    // THE CRATE OF GOODS a card lays in a corpse. What it holds is the loot
    // table of a LOCKED CHEST of the player's own expansion -- the game's own
    // tables, chosen by the lockpicking skill their locks ask for -- and a
    // purse whose weight follows that same expansion.
    void FillCrate(Player* player, Loot* loot);

    // A GREY TRINKET of the player's own age, for a card that adds one to a
    // corpse; 0 when there is none to give.
    [[nodiscard]] uint32 GreyItemFor(uint8 level);

    // UNE PIECE D'EQUIPEMENT rare (3) ou epique (4) a la mesure d'un joueur de
    // ce niveau, prise dans ce que le monde fait deja tomber ; 0 quand il n'y
    // a rien a donner.
    [[nodiscard]] uint32 GearFor(uint8 quality, uint8 level);

    // UNE HERBE de l'age d'un joueur de ce niveau, prise dans ce que les
    // noeuds du decor donnent deja ; 0 quand il n'y a rien a donner.
    [[nodiscard]] uint32 HerbFor(uint8 level);
}

#endif
