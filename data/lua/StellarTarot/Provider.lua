--[[
    This file is part of mod-stellar-tarot.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
    Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <http://www.gnu.org/licenses/>.
]]

--[[----------------------------------------------------------------------------
    The tarot at the workbench: what the shared bench (lua_scripts/Workbench)
    can do with cards and boards.

      fuse_cards    3 cards, any of them   -> 1 card drawn from the whole catalogue
      fuse_boards   3 boards, any of them  -> 1 board drawn from the whole catalogue

    THIS FILE DECIDES NOTHING: it says which items are the tarot's and which
    combinations the bench may offer; the module's `.tarot fuse` command, run
    as the player, checks everything again, does the exchange and speaks.
------------------------------------------------------------------------------]]

local fmt = string.format

if not Workbench then
    print("StellarTarot: the shared workbench (lua_scripts/Workbench/Workbench.ext) is not loaded; no recipe registered.")
    return
end

-- The identifiers the module allocates (StellarTarotMgr.h): card N is item
-- CARD_BASE + N, board B is item BOARD_BASE + B.
local CARD_BASE = 87000
local BOARD_BASE = 87500

-- Which entries exist, read once from the catalogue: a card added later
-- needs a `.reload ale` to be known here.
local cards, boards = {}, {}
local q = WorldDBQuery("SELECT card_id FROM mod_stellar_tarot_card")
if q then
    repeat cards[CARD_BASE + q:GetUInt32(0)] = true until not q:NextRow()
end
q = WorldDBQuery("SELECT board_id FROM mod_stellar_tarot_board")
if q then
    repeat boards[BOARD_BASE + q:GetUInt32(0)] = true until not q:NextRow()
end

local function OnlyCards(entries)
    for _, e in ipairs(entries) do
        if not cards[e] then return false end
    end
    return true
end

local function OnlyBoards(entries)
    for _, e in ipairs(entries) do
        if not boards[e] then return false end
    end
    return true
end

local function AllCards(entries)
    return OnlyCards(entries) and #entries == 3
end

local function AllBoards(entries)
    return OnlyBoards(entries) and #entries == 3
end

-- What the bench shows as the result: a draw, so no particular card would
-- be honest -- the type's icon under the game's red question mark (the
-- module's own textures, data/art/Interface/mod-Tarot/Icons), and a word.
-- `texts`: { enUS, frFR, deDE, esES, ruRU }, read in the player's language
-- (esMX reads Spanish; any other, English).
local DB_LOCALES = { [2] = "frFR", [3] = "deDE", [6] = "esES", [7] = "esES", [8] = "ruRU" }
local function Draw(icon, texts)
    return function(player)
        local locale = DB_LOCALES[player:GetDbLocaleIndex()] or "enUS"
        return { icon = "Interface\\mod-Tarot\\Icons\\" .. icon,
                 text = texts[locale] or texts.enUS }
    end
end

Workbench.Register({
    module = "mod-stellar-tarot",
    Owns = function(entry)
        return cards[entry] or boards[entry] or false
    end,
    kinds = {
        { key = "card", name = { enUS = "Cards", frFR = "Cartes", deDE = "Karten", esES = "Cartas",
                                 esMX = "Cartas", ruRU = "Карты" },
          Of = function(entry) return cards[entry] == true end },
        { key = "board", name = { enUS = "Boards", frFR = "Plateaux", deDE = "Spielbretter", esES = "Tableros",
                                  esMX = "Tableros", ruRU = "Поля" },
          Of = function(entry) return boards[entry] == true end },
    },
    recipes = {
        {
            key = "fuse_cards", slots = 3,
            name = { enUS = "Fusion: three cards, any of them, become one drawn at random.",
                     frFR = "Fusion : trois cartes, quelles qu'elles soient, deviennent une carte tirée au hasard.",
                     deDE = "Fusion: drei beliebige Karten werden zu einer zufällig gezogenen Karte.",
                     esES = "Fusión: tres cartas cualesquiera se convierten en una carta elegida al azar.",
                     esMX = "Fusión: tres cartas cualesquiera se convierten en una carta elegida al azar.",
                     ruRU = "Слияние: три любые карты превращаются в одну случайную карту." },
            Fits = function(_, placed, entry) return cards[entry] == true and OnlyCards(placed) end,
            Accepts = function(_, entries) return AllCards(entries) end,
            Preview = Draw("random_card", { enUS = "A card drawn at random from the whole catalogue.",
                                            frFR = "Une carte tirée au hasard dans tout le catalogue.",
                                            deDE = "Eine zufällig gezogene Karte aus dem gesamten Katalog.",
                                            esES = "Una carta elegida al azar de todo el catálogo.",
                                            ruRU = "Случайная карта из всего каталога." }),
            Run = function(player, entries)
                player:RunCommand(fmt("tarot fuse %d %d %d", entries[1], entries[2], entries[3]))
            end,
        },
        {
            key = "fuse_boards", slots = 3,
            name = { enUS = "Fusion: three boards, any of them, become one drawn at random.",
                     frFR = "Fusion : trois plateaux, quels qu'ils soient, deviennent un plateau tiré au hasard.",
                     deDE = "Fusion: drei beliebige Spielbretter werden zu einem zufällig gezogenen Spielbrett.",
                     esES = "Fusión: tres tableros cualesquiera se convierten en un tablero elegido al azar.",
                     esMX = "Fusión: tres tableros cualesquiera se convierten en un tablero elegido al azar.",
                     ruRU = "Слияние: три любых поля превращаются в одно случайное поле." },
            Fits = function(_, placed, entry) return boards[entry] == true and OnlyBoards(placed) end,
            Accepts = function(_, entries) return AllBoards(entries) end,
            Preview = Draw("random_board", { enUS = "A board drawn at random from the whole catalogue.",
                                             frFR = "Un plateau tiré au hasard dans tout le catalogue.",
                                             deDE = "Ein zufällig gezogenes Spielbrett aus dem gesamten Katalog.",
                                             esES = "Un tablero elegido al azar de todo el catálogo.",
                                             ruRU = "Случайное поле из всего каталога." }),
            Run = function(player, entries)
                player:RunCommand(fmt("tarot fuse %d %d %d", entries[1], entries[2], entries[3]))
            end,
        },
    },
})
