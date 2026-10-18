/*
 * This file is part of mod-stellar-tarot.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * LE TUTORIEL (demande du 2026-10-18 : « Stellar Tarot se debloque au niveau 5 (reglable dans le .conf)
 * et creer une suite de quete qui va servir de tutoriel. Quand le joueur effectue la suite de quete, il
 * doit obtenir un plateau bas niveau ainsi que "The Seed", "The Purse" et "The Blanket"
 * progressivement. Le tutoriel doit donc inciter le joueur a utiliser l'interface et traquer les actions
 * du joueur afin de valider les quetes »).
 *
 * Cinq quetes (87001-87005, stellar_tarot_17_tutorial.sql), sans donneur ni destinataire : le module les
 * met dans le journal et les rend lui-meme, quand le geste qu'elles demandent est fait.
 *
 *   1  ouvrir la fenetre du tarot                      -> Plateau du novice + The Seed
 *   2  etudier le plateau et The Seed                  -> The Purse
 *   3  equiper le plateau
 *   4  poser une carte sur le plateau                  -> The Blanket
 *   5  faire se toucher deux bords (un effet s'active)
 *
 * Chaque etape se lit sur l'etat du joueur (son classeur, son plateau), sauf la premiere, un geste :
 * une etape deja faite d'avance se valide des qu'elle arrive. Une recompense qui ne tient pas dans les
 * sacs attend : la quete reste terminee dans le journal et se rend au prochain geste ou a la connexion.
 */

#ifndef MOD_STELLAR_TAROT_TUTORIAL_H_
#define MOD_STELLAR_TAROT_TUTORIAL_H_

class Player;

namespace StellarTarotTutorial
{
    // Le niveau du joueur ouvre-t-il le tarot ?
    [[nodiscard]] bool Unlocked(Player const* player);

    // A la connexion et au changement de niveau : la premiere etape non rendue entre au journal.
    void Offer(Player* player);

    // La fenetre s'est ouverte (`.tarot open`, que l'interface envoie).
    void OnOpen(Player* player);

    // Apres tout geste du tarot : l'etape en cours est-elle faite ?
    void Advance(Player* player);

    void OnLogout(Player* player);
}

#endif
