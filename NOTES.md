# This file contains a few notes on what work has to be done in this repo

## Urgent work
- Have examples (usage of most functions on a simple basis)
  - One on Weights???
- Update the doc to be publishable
  - Finish documentation on Weight classes and Rank1Lattice/IntLattice
  - Re-read what has been done to fit with the pdf guide.
- Build projections basis and dual basis
- Check if LLL can be done with all norms.
- Implement LLL construction/reduction.
- Look into the B&B algorithm

## New features
- Implement upper bounds as in Cohn and Elkies and look for higher dimensions
- Choose a license
- Test current implementation for supported types combinations
- Make sure everything that is advertised in the manual is implemented
- Implement a test suite to look for bugs

## Design changes

## Minor changes

# Questions
J'ai pensé à ça, et c'est possible de retirer les switch. Il suffit en fait de
réimplémenter les enum en des classes et créer des sous-classes pour chaque cas
qui elles-même implémentent les fonctions nécessaires, ce qui nous permet de ne
pas avoir de switch et qui fait en sorte que tout ce que les switch font se
décide à la compilation!
- Est-ce qu'on implémente P_alpha? Et globalement, quel traitement est-ce que
  l'on donne aux figures de mérite? Certaines sont implémentées, mais de façon
  primitive. D'un côté ce sont les applications spécifiques qui en ont besoin,
  mais d'un autre côté, c'est intéressant de les avoir ici parce que ça fait de
  LatticeTester un logiciel à part entière. Le statu quo ne semble pas une bonne
  option car LatticeTester a plus l'air d'être un espèce de script écrit sur un
  coin de table. Donc, à mon avis, soit on dégage tout et LatticeTester n'est
  qu'un librarie, soit on implémente un peu mieux. Je ne pense pas que ce soit
  une surcharge de travail si on retire les switch. Je suis plus en faveur du
  deuxième cas. LatticeTester resterait utile en tant que tel sans avoir besoin
  de programmer des trucs trop compliqués.
  - J'ai cru comprendre que non parce que ce n'est pas tellement d'intérêt
    général. Il y a aussi un peu partout des reliques dans le logiciel avec des
    noms qui existent dans des enums mais qui ne servent à rien. On en fait
    quoi?
- Can redLLLNTL convert to ZZ if we pass matrix of long type instead of
  calling our algorithm? We should test which is faster instead of printing a
  warning. This could lead to overflow when converting back, but we should just
  take care and throw exceptions in that case. The algorithm is the same so it
  should overflow anyway if we do not convert and use our implementation.

# Notes personnelles
- Est-ce que LatticeTester devrait avoir des classes pour supporter les réseaux
qui ne sont pas générés par une bases de n vecteur en n dimensions (présentement
le logiciel supporte seulement les matrices de base carrées).
  - R: Probablement
  - Autre R: À force de travailler avec, je viens de me suis rendu compte qu'on
    avait pas de dual sur lequel un peut calculer numériquement, alors bon...
- Est-ce que Lacunary est pertinent dans LatticeTester? Je ne pense pas parce
que c'est principalement un truc qui est utile pour étudier les MRGs.
  - R: Oui. On peut faire la projection d'un réseau sur certaines de ses
    coordonnées de manière générale en prenant simplement un sous-ensemble des
    colonnes pour constituer l'ensemble générateur.
- Le calcul des bornes de Rogers ne vient pas de l'article de 1959 parce que le
terme avec les log donne quelque chose de négatif pour les valeurs que l'on
considère. C'est vraiment de l'article de 58, mais il faut arriver à calculer le truc...