-- mod-stellar-tarot — the companion auras, linked by the core.
--
-- A spell carries but THREE effects. When a card level asks for more, the
-- surplus goes into a companion spell, and this table tells the core to lay it
-- with the level's own aura and to take it off with it (type 2, SPELL_LINK_AURA):
-- `caster->AddAura(companion, target)` on apply, `target->RemoveAura(companion)`
-- on removal, and the stack count kept in step. No module code is involved.
--
-- GENERATED from the design workbook by the author's tooling. Regenerable.

DELETE FROM `spell_linked_spell` WHERE ABS(`spell_trigger`) BETWEEN 87000 AND 89999;
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `comment`) VALUES
(88221, 88985, 2, 'The Grin level 2: the rest of the aura'),
(88251, 88984, 2, 'The Familiar level 4: the rest of the aura'),
(88564, 88961, 2, 'The Artisan level 1: the rest of the aura'),
(88564, 88960, 2, 'The Artisan level 1: the rest of the aura'),
(88584, 88959, 2, 'The Scarecrow level 1: the price'),
(88644, 88958, 2, 'The Patron level 1: the rest of the aura'),
(88644, 88957, 2, 'The Patron level 1: the rest of the aura'),
(88659, 88956, 2, 'The Lunatic level 4: the price'),
(88668, 88955, 2, 'The Paranoia level 1: the rest of the aura'),
(88670, 88954, 2, 'The Paranoia level 3: the rest of the aura'),
(88705, 88953, 2, 'The Hysteria level 2: the rest of the aura'),
(88707, 88952, 2, 'The Hysteria level 4: the rest of the aura'),
(88791, 88946, 2, 'The Maenad level 4: the price'),
(89097, 88929, 2, 'The Guild level 2: the rest of the aura'),
(89097, 88928, 2, 'The Guild level 2: the rest of the aura'),
(89102, 88927, 2, 'The Trance level 3: the price'),
(89208, 88914, 2, 'The Workshop level 1: the rest of the aura'),
(89208, 88913, 2, 'The Workshop level 1: the rest of the aura'),
(89274, 88903, 2, 'The Effigy level 3: the price'),
(89389, 88898, 2, 'The Hanged Man level 2: the price'),
(89393, 88896, 2, 'The Tower level 2: the price'),
(89443, 88895, 2, 'The Chimera level 4: the price'),
(89516, 88879, 2, 'The Gorgon level 1: the price'),
(89560, 87993, 2, 'The Torment level 1: the rest of the aura'),
(89561, 87992, 2, 'The Torment level 2: the rest of the aura'),
(89628, 87984, 2, 'The Oblivion level 1: the rest of the aura'),
(89629, 87983, 2, 'The Oblivion level 2: the rest of the aura'),
(89630, 87982, 2, 'The Oblivion level 3: the rest of the aura'),
(89631, 87981, 2, 'The Oblivion level 4: the rest of the aura');
