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

#include "StellarTarotLoot.h"

#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Field.h"
#include "Log.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "QueryResult.h"
#include "Random.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StellarTarotBinder.h"
#include "StellarTarotMgr.h"
#include "WorldSession.h"

#include <unordered_set>

#include <fmt/format.h>
#include <vector>

namespace
{
    // CE QU'UNE CAISSE CONTIENT : la table de butin d'un COFFRET, choisi dans
    // la liste que l'auteur a arretee -- un coffret par tranche de niveau, du
    // bronze au titane. C'est leur table que la caisse joue, telle quelle,
    // avec les chances du jeu.
    uint32 const CRATE_BOXES[3][8] =
    {
        // l'ancien monde
        { 4633,     // Heavy Bronze Lockbox
          4634,     // Iron Lockbox
          4636,     // Strong Iron Lockbox
          4637,     // Steel Lockbox
          4638,     // Reinforced Steel Lockbox
          5758,     // Mithril Lockbox
          5759,     // Thorium Lockbox
          5760 },   // Eternium Lockbox
        { 31952, 0, 0, 0, 0, 0, 0, 0 },     // Outreterre : coffret en khorium
        { 43622,    // Norfendre : coffret en acier de Givre
          43624,    // coffret en titane
          0, 0, 0, 0, 0, 0 },
    };
    bool fillingCrate = false;          // une caisse qui se remplit ne se remplit pas elle-meme

    // LES OBJETS GRIS que le monde fait tomber, ranges par age : de quoi
    // ajouter une babiole a un cadavre. Le niveau d'objet les date proprement
    // (<= 55 l'ancien monde, 56-69 l'Outreterre, au-dela Norfendre).
    std::vector<uint32> junk[3];

    // LA RESERVE D'EQUIPEMENT, par qualite : rare (indice 0) et epique
    // (indice 1). On n'y met que des ARMES et des ARMURES que le monde fait
    // deja tomber -- pas un objet de quete, pas une piece liee a une carte, pas
    // une recette. Chaque entree garde le niveau requis et le niveau d'objet :
    // une carte qui promet « un objet de votre niveau » choisit dedans.
    struct Gear
    {
        uint32 entry = 0;
        uint16 itemLevel = 0;
        uint16 required = 0;
    };
    std::vector<Gear> gear[2];

    // LES HERBES que le monde fait pousser, rangees par age comme les
    // babioles : de quoi rendre « une autre plante de la meme extension ».
    std::vector<uint32> herbs[3];

    uint8 CrateAgeOf(uint8 level)
    {
        if (level <= 57) return 0;
        if (level <= 67) return 1;
        return 2;
    }
}

void StellarTarotLoot::Load()
{
    // La reserve de babioles : tout objet GRIS qu'une creature laisse.
    for (auto& age : junk)
        age.clear();
    if (QueryResult greys = WorldDatabase.Query(
        "SELECT i.entry, i.ItemLevel FROM item_template i WHERE i.Quality = 0 AND i.bonding = 0 "
        "AND i.startquest = 0 AND i.map = 0 AND i.area = 0 AND (i.Flags & 4) = 0 "
        "AND i.entry IN (SELECT DISTINCT Item FROM creature_loot_template)"))
    {
        do
        {
            Field* g = greys->Fetch();
            uint32 const entry = g[0].Get<uint32>();
            uint32 const itemLevel = g[1].Get<uint32>();
            junk[itemLevel <= 55 ? 0 : (itemLevel <= 69 ? 1 : 2)].push_back(entry);
        } while (greys->NextRow());
    }
    LOG_INFO("module", "StellarTarot: {} / {} / {} grey item(s) a card may add to a corpse.",
             junk[0].size(), junk[1].size(), junk[2].size());

    // CE QUE LE MONDE FAIT TOMBER, une bonne fois : les numeros d'objet que les
    // tables des creatures et celles des objets du decor nomment. Demander
    // cette liste EN SOUS-REQUETE coutait plus d'une minute par appel -- la
    // base la relisait pour chaque objet du catalogue. Lue une fois et tenue
    // ici, elle ne coute plus rien.
    std::unordered_set<uint32> tombe;
    for (char const* table : { "creature_loot_template", "gameobject_loot_template" })
        if (QueryResult lignes = WorldDatabase.Query("SELECT DISTINCT Item FROM {}", table))
            do
            {
                tombe.insert((*lignes)[0].Get<uint32>());
            } while (lignes->NextRow());

    // L'EQUIPEMENT RARE ET EPIQUE que le monde donne deja : armes et armures,
    // les deux qualites d'un seul coup, triees en memoire.
    for (auto& q : gear)
        q.clear();
    if (QueryResult pieces = WorldDatabase.Query(
        "SELECT entry, ItemLevel, RequiredLevel, Quality FROM item_template "
        "WHERE Quality IN (3, 4) AND class IN (2, 4) AND startquest = 0 AND map = 0 "
        "AND area = 0 AND (Flags & 4) = 0 AND ItemLevel > 0"))
    {
        do
        {
            Field* g = pieces->Fetch();
            Gear piece;
            piece.entry = g[0].Get<uint32>();
            if (!tombe.count(piece.entry))
                continue;
            piece.itemLevel = uint16(g[1].Get<uint32>());
            piece.required = uint16(g[2].Get<uint32>());
            gear[g[3].Get<uint32>() - ITEM_QUALITY_RARE].push_back(piece);
        } while (pieces->NextRow());
    }
    LOG_INFO("module", "StellarTarot: {} rare and {} epic piece(s) a card may hand out.",
             gear[0].size(), gear[1].size());

    // LES HERBES : ce que les noeuds du decor donnent vraiment, date par le
    // niveau d'objet comme le reste.
    for (auto& age : herbs)
        age.clear();
    if (QueryResult plantes = WorldDatabase.Query(
        "SELECT entry, ItemLevel FROM item_template WHERE class = 7 AND subclass = 9"))
    {
        do
        {
            Field* h = plantes->Fetch();
            uint32 const entry = h[0].Get<uint32>();
            if (!tombe.count(entry))
                continue;
            uint32 const itemLevel = h[1].Get<uint32>();
            herbs[itemLevel <= 55 ? 0 : (itemLevel <= 69 ? 1 : 2)].push_back(entry);
        } while (plantes->NextRow());
    }
    LOG_INFO("module", "StellarTarot: {} / {} / {} herb(s) a card may hand out.",
             herbs[0].size(), herbs[1].size(), herbs[2].size());
}

// The crate of goods, opened: the module plays the loot table of one of the
// LOCKBOXES the author named for that expansion -- the game's own table, with
// the game's own chances -- and lays a purse on top of it.
void StellarTarotLoot::FillCrate(Player* player, Loot* loot)
{
    if (!player || !loot || fillingCrate)
        return;
    uint8 const age = CrateAgeOf(player->GetLevel());
    uint32 boxes[8] = { 0 };
    uint8 count = 0;
    for (uint32 box : CRATE_BOXES[age])
        if (box)
            boxes[count++] = box;
    if (!count)
        return;
    uint32 const chosen = boxes[urand(0, count - 1)];
    fillingCrate = true;                // la table jouee ne doit pas rappeler ceci
    loot->clear();
    loot->FillLoot(chosen, LootTemplates_Item, player, true, true);
    fillingCrate = false;
    // Les pieces au fond de la caisse, a la mesure de l'extension : de 5 pa a
    // 1 po pour l'ancien monde, de 2 a 5 po pour l'Outreterre, de 5 a 10 po
    // pour Norfendre.
    static uint32 const purse[3][2] = { { 500, 10000 }, { 20000, 50000 }, { 50000, 100000 } };
    loot->gold = urand(purse[age][0], purse[age][1]);
}

// UNE PIECE D'EQUIPEMENT de la qualite demandee, a la mesure du joueur : son
// niveau requis ne doit pas le depasser, et son niveau d'objet doit tenir dans
// une fenetre autour du sien. La fenetre s'ouvre jusqu'a ce que quelque chose
// y tienne ; 0 quand la reserve est vide.
uint32 StellarTarotLoot::GearFor(uint8 quality, uint8 level)
{
    if (quality != ITEM_QUALITY_RARE && quality != ITEM_QUALITY_EPIC)
        return 0;
    std::vector<Gear> const& reserve = gear[quality - ITEM_QUALITY_RARE];
    if (reserve.empty())
        return 0;
    std::vector<uint32> good;
    for (uint16 window = 10; window <= 60 && good.empty(); window += 10)
        for (Gear const& piece : reserve)
            if (piece.required <= level && piece.itemLevel + window >= level
                && piece.itemLevel <= level + window)
                good.push_back(piece.entry);
    // Rien dans aucune fenetre : ce que le joueur peut porter de plus haut.
    if (good.empty())
    {
        uint16 best = 0;
        for (Gear const& piece : reserve)
            if (piece.required <= level && piece.itemLevel > best)
                best = piece.itemLevel;
        for (Gear const& piece : reserve)
            if (piece.required <= level && piece.itemLevel == best)
                good.push_back(piece.entry);
    }
    return good.empty() ? 0 : good[urand(0, uint32(good.size()) - 1)];
}

// UNE HERBE de l'age du joueur, 0 quand la reserve est vide.
uint32 StellarTarotLoot::HerbFor(uint8 level)
{
    std::vector<uint32> const& age = herbs[CrateAgeOf(level)];
    return age.empty() ? 0 : age[urand(0, uint32(age.size()) - 1)];
}

// One grey trinket of the player's own age, 0 when the reserve is empty.
uint32 StellarTarotLoot::GreyItemFor(uint8 level)
{
    std::vector<uint32> const& age = junk[CrateAgeOf(level)];
    return age.empty() ? 0 : age[urand(0, uint32(age.size()) - 1)];
}
