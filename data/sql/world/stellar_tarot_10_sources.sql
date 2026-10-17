-- mod-stellar-tarot — where cards and boards came from.
--
-- The loot sources are gone: cards and boards drop through DynamicLoot (the
-- dynamicloot module), from the StellarTarot.Loot.* settings of
-- mod-stellar-tarot.conf. The table they lived in goes with them.
--
-- Idempotent: the file may be replayed.

DROP TABLE IF EXISTS `mod_stellar_tarot_source`;
