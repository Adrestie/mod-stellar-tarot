# mod-stellar-tarot

**WORK IN PROGRESS**

Cards laid on a board, for AzerothCore 3.3.5a: where two edges meet on the same number, both cards wake, and the more edges match, the stronger their effect. A crossing of Hades II's arcana cards and Final Fantasy VIII's Triple Triad.

## Key features

- 372 cards, each with four levels, and 25 boards of 2x2 to 5x5 cells.
- A binder for the whole account: a card or a board used from the bags joins the collection.
- The `/tarot` window: equip a board, lay cards by drag and drop, save layouts as presets.
- Effects at each level: auras and scripts of the module.
- Three cards, or three boards, fused into one at the workbench.
- Cards and boards dropped across the game; an illustration and an icon per card.

## Dependencies

| Dependency | For |
|---|---|
| [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) (WotLK 3.3.5a) | the server |
| [mod-ale](https://github.com/azerothcore/mod-ale) | runs the interface |
| [AIO](https://github.com/Rochet2/AIO) | the interface: `AIO_Server` in `lua_scripts`, and the `AIO_Client` addon on every client |
| [dynamicloot](https://github.com/Adrestie/WoW-mods-dynamicloot) | places the module's drops across the game; install it first, with the installer too |
| [The workbench](https://github.com/Adrestie/WoW-mods-workbench) | fusing and rerolling; shipped inside this module, the installer places it |

## How to install

1. Download `installer.exe` from the [WoW-mods-installer releases](https://github.com/Adrestie/WoW-mods-installer/releases).
2. Stop the worldserver and close the game.
3. Run `installer.exe`, give it this folder and the folders it asks for, click **Check**, then **Install**.
4. Rebuild the worldserver. On its next start, the core applies the module's SQL.
5. The installer writes the game files into `Data\patch-Z.MPQ`: give that archive to the other players.
6. Put a workbench down in your world (game object 810000), where you want it.

## How to update

Same steps with the new version's folder, choosing **Update** instead of **Install**. The accounts' binders, boards, layouts and presets, and your `mod-stellar-tarot.conf` settings, are kept. Rebuild the worldserver, then give the players the new `patch-Z.MPQ`.

## How to uninstall

Same steps, choosing **Remove**. Everything goes, database included. The workbench stays while another module uses it. Rebuild the worldserver.

## Configuration

`mod-stellar-tarot.conf` sets:

- the module on or off, and the check of its data at startup;
- the level at which the tarot unlocks, and the tutorial;
- the drops of cards, boards and containers (DynamicLoot lines), and one rate on top.

## Commands

| Command | |
|---|---|
| `/tarot` | opens the window |
| `.tarot study <entry>` | studies a card or a board from the bags |
| `.tarot board <id\|0>` | equips a board, or takes it off |
| `.tarot place <row> <col> <card>` | lays a card |
| `.tarot remove <row> <col>` | takes a card off |
| `.tarot move <row> <col> <row> <col>` | moves or swaps a card |
| `.tarot clear` | takes every card off |
| `.tarot preset save <name> \| load <id> \| delete <id>` | the account's layouts |
| `.tarot fuse <a> <b> <c>` | three cards, or three boards, become one |
| `.tarot info` | game master: the loaded catalogue |
| `.tarot binder \| layout \| effects [player]` | game master: a player's binder, board and effects |
| `.tarot reload` | administrator: reads the tables again |
