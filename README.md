# Bomberman en vue isométrique

|                     |                                             |
|--------------------:|---------------------------------------------|
|                 Nom | Anri KENNEL                                 |
|              Classe | L2-A                                        |
|   Numéro d'étudiant | 20010664                                    |
|                Mail | anri.kennel@etud.univ-paris8.fr             |
| Cycle universitaire | 2021-2022                                   |
|              Module | Algorithmes pour la Programmation Graphique |

## Fonctionnalités
- [x] Modélisation aléatoire de chaque labyrinthes (avec différents types de cubes)
- [x] Modélisation des joueurs avec un cône (carré dans ce jeu) surmonté d'une sphère, de couleurs différentes
- [x] Modélisation de sphères noires en guise de bombes (avec changement de couleur et grossisement)
- [ ] Modélisation du rayon de l'explosion par des sphères jaunes dont le rayon décroit avec la distance (pas fait car fait autrement)
- [x] Collisions et interactions logiques
- [ ] IA ennemies
- [x] Mode multijoueur local
- [ ] Mode multijoueur "en ligne" (sur différentes machine)

## Récupération du projet
Pour récuperer le projet :
### En SSH
```bash
git clone git@code.up8.edu:Anri/bomberman.git bomberman-akennel
```

### En HTTPS
```bash
git clone https://code.up8.edu/Anri/bomberman.git bomberman-akennel
```

## Build & lancer le jeu
Pour build le jeu (vous aurez besoin de [GL4Dummies](https://github.com/noalien/GL4Dummies) et ces [dépendances](https://github.com/noalien/GL4Dummies#dependencies)) [[ici un script](https://git.kennel.ml/Anri/myLinuxConfiguration/raw/branch/main/installgl4D.sh) pour installer GL4D tout seul]:
```bash
make
```

Puis pour lancer le jeu :
```bash
./bomberman
```

## Présentation
La présentation se trouve dans le [dossier de documentation](presentation/presentation.tex) du projet.

## Commandes
- Déplacement du joueur A avec les flèches directionnelles `↑ ← → ↓`
- Déplacement du joueur B avec les touches `Z Q S D`
- Activation/Désactivation de la synchronisation verticale à l'appuie de la touche `v`
- Activation/Désactivation des logs de debug dans la console à l'appuie de la touche `h`

## Sources
- [Image du dépôt](https://pixabay.com/vectors/bomb-cartoon-iconic-2025548/)
- [Image du bois](https://pixabay.com/vectors/crate-box-wood-pattern-wooden-147618/) (convertis en `bmp` avec [Gimp](https://www.gimp.org/))
