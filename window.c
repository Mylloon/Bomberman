/*!\file window.c
 * \author Farès BELHADJ, amsi@up8.edu
 * \student Anri KENNEL, anri.kennel@etud.univ-paris8.fr
 * \date November 16, 2021.
 */
#include <assert.h>
#include <time.h>

/* Inclusion des entêtes de fonctions de gestion de primitives simples
 * de dessin. La lettre p signifie aussi bien primitive que
 * pédagogique. */
#include <GL4D/gl4dp.h>

/* Inclure la bibliothèque de rendu du cours */
#include "rasterize.h"

/* Inclusion des entêtes de fonctions de création et de gestion de
 * fenêtres système ouvrant un contexte favorable à GL4dummies. Cette
 * partie est dépendante de la bibliothèque SDL2 */
#include <GL4D/gl4duw_SDL2.h>

/* Fonctions locales (static) */
static void init(void);
static void idle(void);
static void draw(void);
static void keyu(int keycode);
static void keyd(int keycode);
static void sortie(void);

/*!\brief Surface représentant un cube */
static surface_t * _cube = NULL;
static surface_t * _cubeBois = NULL;
static float _cubeSize = 4.f;

/*!\brief Surface représentant une sphère */
static surface_t * _sphere = NULL;

/*!\brief Variable d'état pour activer/désactiver la synchronisation verticale */
static int _use_vsync = 1;

/*!\brief Variable d'état pour activer/désactiver le debug */
static int _debug = 0;

/*!\brief Grille de positions où il y aura des cubes
 * 0 -> Vide
 * 1 -> Mur
 * 2 (valeur reservée) -> Joueur A (défini automatiquement par le programme)
 * 3 (valeur reservée) -> Joueur B (défini automatiquement par le programme)
 * 4 -> Bloc destructible */
static int * _plateau = NULL;

/*!\brief Largeur/Nombre de lignes de la grille */
static int _grilleW;

/*!\brief Hauteur/Nombre de colonnes de la grille */
static int _grilleH;

/* Définition d'un personnage */
typedef struct perso_t {
    float x, y, z; /* Coordonées spatiale */
    int position; /* Position dans la grille.
                   * Permet d'éviter aux joueurs
                   * de se rentrer dedans */
} perso_t;

/* Définition de nos deux joueurs */
perso_t _joueurA = { 0.f, 0.f, 0.f, -1 }; // à droite
perso_t _joueurB = { 0.f, 0.f, 6.f, -1 }; // à gauche

/* Clavier virtuel */
enum {
    /* Joueur A */
    VK_RIGHT = 0,
    VK_UP,
    VK_LEFT,
    VK_DOWN,
    VK_RETURN,

    /* Joueur B */
    VK_d,
    VK_z,
    VK_q,
    VK_s,
    VK_SPACE,

    /* Toujours à la fin */
    VK_SIZEOF
};

int _vkeyboard[VK_SIZEOF] = {0, 0, 0, 0, 0, 0, 0, 0};

/*!\brief Paramètre l'application et lance la boucle infinie. */
int main(int argc, char ** argv) {
    /* Tentative de création d'une fenêtre pour GL4Dummies */
    if(!gl4duwCreateWindow(argc, argv, /* args du programme */
             "Bomberman ⋅ A. KENNEL L2-A", /* titre */
             10, 10, 800, 600, /* x, y, largeur, heuteur */
             GL4DW_SHOWN) /* état visible */) {
        /* Ici si échec de la création souvent lié à un problème d'absence
         * de contexte graphique ou d'impossibilité d'ouverture d'un
         * contexte OpenGL (au moins 3.2) */
        return 1;
    }
    init();

    /* Mets en place la fonction d'interception clavier touche pressée */
    gl4duwKeyDownFunc(keyd);

    /* Mets en place la fonction d'interception clavier touche relachée */
    gl4duwKeyUpFunc(keyu);

    /* Mets en place la fonction idle (simulation, au sens physique du terme) */
    gl4duwIdleFunc(idle);

    /* Mets en place la fonction de display */
    gl4duwDisplayFunc(draw);

    /* Boucle infinie pour éviter que le programme ne s'arrête et ferme
     * la fenêtre immédiatement */
    gl4duwMainLoop();
    return 0;
}

/*!\brief init de nos données, spécialement le plateau */
void init(void) {
    /* Création d'un screen GL4Dummies (texture dans laquelle nous
     * pouvons dessiner) aux dimensions de la fenêtre. IMPORTANT de
     * créer le screen avant d'utiliser les fonctions liées au
     * textures */
    gl4dpInitScreen();

    /* Création du cube */
    _cube     = mk_cube(); /* ça fait 2x6 triangles */
    _cubeBois = mk_cube(); /* ça fait 2x6 triangles */
    _sphere   = mk_sphere(12, 12); /* ça fait 12x12 triangles */

    /* Rajoute la texture */
    GLuint tex = get_texture_from_BMP("images/tex.bmp");
    set_texture_id(_cube, tex);
    set_texture_id(_sphere, tex);
    GLuint bois = get_texture_from_BMP("images/bois.bmp");
    set_texture_id(_cubeBois, bois);

    /* Affichage des textures */
    enable_surface_option(_cube, SO_USE_TEXTURE);
    enable_surface_option(_cubeBois, SO_USE_TEXTURE);
    enable_surface_option(_sphere, SO_USE_TEXTURE);

    /* Affichage des ombres */
    enable_surface_option(_cube, SO_USE_LIGHTING);
    enable_surface_option(_cubeBois, SO_USE_LIGHTING);
    enable_surface_option(_sphere, SO_USE_LIGHTING);

    /* Si _use_vsync != 0, on active la synchronisation verticale */
    SDL_GL_SetSwapInterval(_use_vsync);

    /* Génération du plateau
     * et placement des joueurs */
    srand(time(NULL));

    /* Génération des dimensions du plateau */
    _grilleW = 15 + (rand() % 10);
    _grilleH = _grilleW;

    /* Placement des joueurs */
    /* Joueur A */
    int caseA = (_grilleW * _grilleH) - (_grilleW * 2) - 3; // MAX - Wx2 - 3
    _joueurA.x = (caseA / _grilleH) * 1.5;
    _joueurA.z = (caseA / _grilleW) * 1.5;

    /* Joueur B */
    int caseB = (_grilleW * 2) + 3; // Wx2 + 3
    _joueurB.x = -(caseB / _grilleH) * 12;
    _joueurB.z = -(caseB / _grilleW) * 12;

    if((_plateau = malloc((_grilleW * _grilleH) * sizeof(int))) == NULL) {
        printf("Impossible d'allouer de la mémoire supplémentaire pour générer le plateau.\n");
        sortie();
        exit(1);
    }

    /* Génération du plateau */
    for(int i = 0; i < _grilleH; i++)
        for(int j = 0; j < _grilleW; j++) {
            int _case = i * _grilleH + j;
            if(i == 0) { // mur en haut
                _plateau[_case] = 1;
                continue;
            }
            if(i == (_grilleH - 1)) { // mur en bas
                _plateau[_case] = 1;
                continue;
            }
            if(j == 0) { // mur a gauche
                _plateau[_case] = 1;
                continue;
            }
            if(j == (_grilleW - 1)) { // mur a droite
                _plateau[_case] = 1;
                continue;
            }
            if((j % 2) == 0 && (i % 2) == 0) { // mur à l'intérieur
                _plateau[_case] = 1;
                continue;
            }
            _plateau[_case] = 0;
        }

    /* On ajoute les blocs qui sont déstructibles par les joueurs */
    for(int i = 0; i < _grilleH; i++)
        for(int j = 0; j < _grilleW; j++) {
            int _case = i * _grilleH + j;
            if(_plateau[_case] == 0)
                if(rand() % 3 == 0) // 1 chance sur 3
                    _plateau[_case] = 4;
        }

    /* On s'assure que au moins une cases autours du joueurs sont libre
     * ça donne ça :
     * A B C
     * D x E
     * F G H
     * avec A B C D E F G H des positions
     * et x le joueur Attention au bordures !
     * On les fait spawn loin des bordures
     * pour éviter tout problèmes. */
    int caseJoueurA = round((_joueurA.z + _cubeSize * _grilleH / 2) / _cubeSize) * _grilleH + round((_joueurA.x + _cubeSize * _grilleW / 2) / _cubeSize);
    int caseJoueurB = round((_joueurB.z + _cubeSize * _grilleH / 2) / _cubeSize) * _grilleH + round((_joueurB.x + _cubeSize * _grilleW / 2) / _cubeSize);

    /* Joueur A */
    _plateau[caseJoueurA] = 2;                // x (facultatif)
    _plateau[caseJoueurA - _grilleW - 1] = 0; // A
    _plateau[caseJoueurA - _grilleW] = 0;     // B
    _plateau[caseJoueurA - _grilleW + 1] = 0; // C
    _plateau[caseJoueurA - 1] = 0;            // D
    _plateau[caseJoueurA + 1] = 0;            // E
    _plateau[caseJoueurA + _grilleW - 1] = 0; // F
    _plateau[caseJoueurA + _grilleW] = 0;     // G
    _plateau[caseJoueurA + _grilleW + 1] = 0; // H

    /* Joueur B */
    _plateau[caseJoueurB] = 3;                // x (facultatif)
    _plateau[caseJoueurB - _grilleW - 1] = 0; // A
    _plateau[caseJoueurB - _grilleW] = 0;     // B
    _plateau[caseJoueurB - _grilleW + 1] = 0; // C
    _plateau[caseJoueurB - 1] = 0;            // D
    _plateau[caseJoueurB + 1] = 0;            // E
    _plateau[caseJoueurB + _grilleW - 1] = 0; // F
    _plateau[caseJoueurB + _grilleW] = 0;     // G
    _plateau[caseJoueurB + _grilleW + 1] = 0; // H

    /* Mets en place la fonction à appeler en cas de sortie */
    atexit(sortie);
}

/*!\brief Fonction appellée à chaque idle (entre chaque frame) */
void idle(void) {
    /* on récupère le delta-temps */
    static double t0 = 0.0; // le temps à la frame précédente
    double t, dt;
    t = gl4dGetElapsedTime();
    dt = (t - t0) / 1000.0; // diviser par mille pour avoir des secondes
    /* pour le frame d'après, mets à-jour t0 */
    t0 = t;

    float vitesse = 9.f; // vitesse des joueurs

    /* Calcul du décalage */
    float decalageAutorisee = .2f; // décalage autorisé, modifier cette valeur si nécessaire
    float decalageGB = .0f + decalageAutorisee; // décalage pour la gauche et le bas
    float decalageDH = 1.f - decalageAutorisee; // décalage pour la droite et le haut

    /* Mouvements du Joueur A */
    /* Coordonées x, z */
    float zA = (_joueurA.z + _cubeSize * _grilleH / 2) / _cubeSize; // ligne   - hauteur
    float xA = (_joueurA.x + _cubeSize * _grilleW / 2) / _cubeSize; // colonne - longueur

    /* Coordonnées joueur A */
    int coorJoueurA = round(zA) * _grilleH + round(xA);
    int coorDroiteA = round(zA) * _grilleH + ceil(xA);
    int coorHautA   = floor(zA) * _grilleH + round(xA);
    int coorGaucheA = round(zA) * _grilleH + floor(xA);
    int coorBasA    = ceil(zA)  * _grilleH + round(xA);

    /* Décalage pour éviter bug de collisions */
    float decalageLargeurA  = zA - floor(zA);
    float decalageLongueurA = xA - floor(xA);

    /* Déplacement
     * Pour la vérification du "si mur"
     * Dans ce cas de figure :
     * A B C
     * D x E
     * F G H
     * Pour aller à droite on regarde si C et H ne sont pas des murs
     * On part de la coordonnées de droite car coorDroite = E. */
    if(_vkeyboard[VK_RIGHT])
        /* Collision à droite du joueur */
        if(_plateau[coorDroiteA] == 0 || _plateau[coorDroiteA] == 2) // si case vide ou joueur
            /* if(decalageLargeurA < decalageGB || decalageLargeurA > decalageDH) */ _joueurA.x += vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_UP])
        /* Collision en haut du joueur */
        if(_plateau[coorHautA] == 0 || _plateau[coorHautA] == 2) // si case vide ou joueur
            /* if(decalageLongueurA < decalageGB || decalageLongueurA > decalageDH) */ _joueurA.z -= vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_LEFT])
        /* Collision à gauche du joueur */
        if(_plateau[coorGaucheA] == 0 || _plateau[coorGaucheA] == 2) // si case vide ou joueur
            /* if(decalageLargeurA < decalageGB || decalageLargeurA > decalageDH) */ _joueurA.x -= vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_DOWN])
        /* Collision en bas du joueur */
        if(_plateau[coorBasA] == 0 || _plateau[coorBasA] == 2) // si case vide ou joueur
            /* if(decalageLongueurA < decalageGB || decalageLongueurA > decalageDH) */ _joueurA.z += vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_RETURN]) {
        _vkeyboard[VK_RETURN] = 0; // on évite de spam la pose de bombe
        printf("Joueur A pose une bombe!\n"); // temp
    }

    /* Affichage Debug */
    if(_debug) {
        printf("\n========== Joueur A ==========\n");
        printf(" li = %d, col = %d, idx = %d\n", (int)(zA + .5f), (int)(xA + .5f), _joueurA.position);
        printf(" zA=%f xA=%f\n", zA, xA);
        printf(" d=%d h=%d g=%d b=%d\n", coorDroiteA, coorHautA, coorGaucheA, coorBasA);
    }

    /* Anti-collision entre joueurs */
    if(_joueurA.position != coorJoueurA) {
        if(_joueurA.position != -1)
            _plateau[_joueurA.position] = 0;
        _joueurA.position = coorJoueurA;
        _plateau[coorJoueurA] = 2;
    }


    /* Mouvements du Joueur B */
    /* Coordonées x, z */
    float zB = (float)((_joueurB.z + _cubeSize * _grilleH / 2) / _cubeSize); // ligne - hauteur
    float xB = (float)((_joueurB.x + _cubeSize * _grilleW / 2) / _cubeSize); // colonne - longueur

    /* Coordonnées joueur A */
    int coorJoueurB = round(zB) * _grilleH + round(xB);
    int coorDroiteB = round(zB) * _grilleH + ceil(xB);
    int coorHautB   = floor(zB) * _grilleH + round(xB);
    int coorGaucheB = round(zB) * _grilleH + floor(xB);
    int coorBasB    = ceil(zB)  * _grilleH + round(xB);

    /* Décalage pour éviter bug de collisions */
    float decalageLargeurB  = zB - floor(zB);
    float decalageLongueurB = xB - floor(xB);

    /* Déplacement */
    if(_vkeyboard[VK_d])
        /* Collision à droite du joueur */
        if(_plateau[coorDroiteB] == 0 || _plateau[coorDroiteB] == 2) // si case vide ou joueur
            /* if(decalageLargeurB < decalageGB || decalageLargeurB > decalageDH) */ _joueurB.x += vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_s])
        /* Collision en haut du joueur */
        if(_plateau[coorHautB] == 0 || _plateau[coorHautB] == 2) // si case vide ou joueur
            /* if(decalageLongueurB < decalageGB || decalageLongueurB > decalageDH) */ _joueurB.z -= vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_q])
        /* Collision à gauche du joueur */
        if(_plateau[coorGaucheB] == 0 || _plateau[coorGaucheB] == 2) // si case vide ou joueur
            /* if(decalageLargeurB < decalageGB || decalageLargeurB > decalageDH) */ _joueurB.x -= vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_s])
        /* Collision en bas du joueur */
        if(_plateau[coorBasB] == 0 || _plateau[coorBasB] == 2) // si case vide ou joueur
                /* if(decalageLongueurB < decalageGB || decalageLongueurB > decalageDH) */ _joueurB.z += vitesse * dt; // on s'assure d'être aligné

    if(_vkeyboard[VK_SPACE]) {
        _vkeyboard[VK_SPACE] = 0; // on évite de spam la pose de bombe
        printf("Joueur B pose une bombe!\n"); // temp
    }

    /* Affichage Debug */
    if(_debug) {
        printf("========== Joueur B ==========\n");
        printf(" li = %d, col = %d, idx = %d\n", (int)(zB + .5f), (int)(xB + .5f), _joueurB.position);
        printf(" zA=%f xA=%f\n", zB, xB);
        printf(" d=%d h=%d g=%d b=%d\n", coorDroiteB, coorHautB, coorGaucheB, coorBasB);
        printf("===============================\n");
    }

    /* Anti-collision entre joueurs */
    if(_joueurB.position != coorJoueurB) {
        if(_joueurB.position != -1)
            _plateau[_joueurB.position] = 0;
        _joueurB.position = coorJoueurB;
        _plateau[coorJoueurB] = 3;
    }
}

/*!\brief Fonction appelée à chaque display. */
void draw(void) {
    vec4 couleurPlateau = {0.2, 0.2, 0.2, 1},   /* Gris */
         couleurJoueurA = {0.15, 0.5, 0.15, 1}, /* Vert */
         couleurJoueurB = {0.2, 0.2, 0.7, 1},   /* Bleu */
         couleurBois    = {0.6, 0.3, 0, 1};     /* Marron */

    float model_view_matrix[16], projection_matrix[16], nmv[16];

    /* Efface l'écran et le buffer de profondeur */
    gl4dpClearScreen();
    clear_depth_map();

    /* Des macros facilitant le travail avec des matrices et des
     * vecteurs se trouvent dans la bibliothèque GL4Dummies, dans le
     * fichier gl4dm.h */
    /* Charger un frustum dans projection_matrix */
    MFRUSTUM(projection_matrix, -0.05f, 0.05f, -0.05f, 0.05f, 0.1f, 1000.0f);
    /* Charger la matrice identité dans model-view */
    MIDENTITY(model_view_matrix);
    /* On place la caméra en arrière-haut, elle regarde le centre de la scène */
    int coefTaille = -20 + _grilleW * 2.5;
    lookAt(model_view_matrix, 0, 70 + coefTaille /* zoom */, 30 + coefTaille /* inclinaison */, 0, 0, 0, 0, 0, -1);

    /* Pour centrer la grille par rapport au monde */
    float cX = -_cubeSize * _grilleW / 2.0f;
    float cZ = -_cubeSize * _grilleH / 2.0f;

    /* Pour toutes les cases de la grille, afficher un cube quand il y a
     * un 1 dans la grille */
    for(int i = 0; i < _grilleW; ++i)
        for(int j = 0; j < _grilleH; ++j) {
            /* Bloc simple */
            if(_plateau[i * _grilleW + j] == 1) {
                _cube->dcolor = couleurPlateau;
                /* copie model_view_matrix dans nmv */
                memcpy(nmv, model_view_matrix, sizeof(nmv));

                /* pour convertir les coordonnées i, j de la grille en x, z du monde */
                translate(nmv, _cubeSize * j + cX, 0.0f, _cubeSize * i + cZ);
                scale(nmv, _cubeSize / 2.0f, _cubeSize / 2.0f, _cubeSize / 2.0f);
                transform_n_rasterize(_cube, nmv, projection_matrix);
            }
            /* Bloc destructible */
            if(_plateau[i * _grilleW + j] == 4) {
                _cubeBois->dcolor = couleurBois;
                /* copie model_view_matrix dans nmv */
                memcpy(nmv, model_view_matrix, sizeof(nmv));

                /* pour convertir les coordonnées i, j de la grille en x, z du monde */
                translate(nmv, _cubeSize * j + cX, 0.0f, _cubeSize * i + cZ);
                scale(nmv, _cubeSize / 2.6f, _cubeSize / 2.6f, _cubeSize / 2.6f);
                transform_n_rasterize(_cubeBois, nmv, projection_matrix);
            }
        }

    /* Dessine le Joueur A */
    _cube->dcolor = couleurJoueurA;
    _sphere->dcolor = couleurJoueurA;
    memcpy(nmv, model_view_matrix, sizeof(nmv));
    /* Corps */
    translate(nmv, _joueurA.x, _joueurA.y, _joueurA.z);
    scale(nmv, _cubeSize / 3.0f, _cubeSize / 3.0f, _cubeSize / 3.0f);
    transform_n_rasterize(_cube, nmv, projection_matrix);
    /* Tête */
    translate(nmv, 0.0f, 2.0f, 0.0f);
    transform_n_rasterize(_sphere, nmv, projection_matrix);

    /* Dessine le Joueur B */
    _cube->dcolor = couleurJoueurB;
    _sphere->dcolor = couleurJoueurB;
    memcpy(nmv, model_view_matrix, sizeof(nmv));
    /* Corps */
    translate(nmv, _joueurB.x, _joueurB.y, _joueurB.z);
    scale(nmv, _cubeSize / 3.0f, _cubeSize / 3.0f, _cubeSize / 3.0f);
    transform_n_rasterize(_cube, nmv, projection_matrix);
    /* Tête */
    translate(nmv, 0.0f, 2.0f, 0.0f);
    transform_n_rasterize(_sphere, nmv, projection_matrix);

    /* Déclare que l'on a changé des pixels de l'écran (bas niveau) */
    gl4dpScreenHasChanged();

    /* Fonction permettant de raffraîchir l'ensemble de la fenêtre*/
    gl4dpUpdateScreen(NULL);
}

/*!\brief Intercepte l'événement clavier pour modifier les options (à l'appuie d'une touche). */
void keyd(int keycode) {
    switch(keycode) {
        case GL4DK_v: // 'v' utiliser la sync Verticale
            _use_vsync = !_use_vsync;
            if(_use_vsync)
                SDL_GL_SetSwapInterval(1);
            else
                SDL_GL_SetSwapInterval(0);
            break;

        case GL4DK_h: // 'h' afficher ou non les infos de debug
            _debug = !_debug;
            break;

        /* Joueur A */
        case GL4DK_RIGHT: // droite
            _vkeyboard[VK_RIGHT] = 1;
            break;

        case GL4DK_UP: // haut
            _vkeyboard[VK_UP] = 1;
            break;

        case GL4DK_LEFT: // gauche
            _vkeyboard[VK_LEFT] = 1;
            break;

        case GL4DK_DOWN: // bas
            _vkeyboard[VK_DOWN] = 1;
            break;

        case GL4DK_RETURN: // pose de bombe
            _vkeyboard[VK_RETURN] = 1;
            break;

        /* Joueur B */
        case GL4DK_d: // droite
            _vkeyboard[VK_d] = 1;
            break;

        case GL4DK_z: // haut
            _vkeyboard[VK_z] = 1;
            break;

        case GL4DK_q: // gauche
            _vkeyboard[VK_q] = 1;
            break;

        case GL4DK_s: // bas
            _vkeyboard[VK_s] = 1;
            break;

        case GL4DK_SPACE: // pose de bombe
            _vkeyboard[VK_SPACE] = 1;
            break;

        /* Par défaut on ne fais rien */
        default: break;
    }
}

/*!\brief Intercepte l'évènement clavier pour modifier les options (au relâchement d'une touche). */
void keyu(int keycode) {
    switch(keycode) {
        /* Cas où l'Joueur A on arrête de bouger */
        case GL4DK_RIGHT:
            _vkeyboard[VK_RIGHT] = 0;
            break;

        case GL4DK_UP:
            _vkeyboard[VK_UP] = 0;
            break;

        case GL4DK_LEFT:
            _vkeyboard[VK_LEFT] = 0;
            break;

        case GL4DK_DOWN:
            _vkeyboard[VK_DOWN] = 0;
            break;

        /* Cas où l'Joueur A on arrête de bouger */
        case GL4DK_d:
            _vkeyboard[VK_d] = 0;
            break;

        case GL4DK_z:
            _vkeyboard[VK_z] = 0;
            break;

        case GL4DK_q:
            _vkeyboard[VK_q] = 0;
            break;

        case GL4DK_s:
            _vkeyboard[VK_s] = 0;
            break;

        /* Par défaut on ne fais rien */
        default: break;
    }
}

/*!\brief Appeler à la sortie du programme
 * pour libérer les objets de la mémoire. */
void sortie(void) {
    /* On libère le cube */
    if(_cube) {
        free_surface(_cube);
        _cube = NULL;
    }
    if(_cubeBois) {
        free_surface(_cubeBois);
        _cubeBois = NULL;
    }

    /* On libère la sphère */
    if(_sphere) {
        free_surface(_sphere);
        _sphere = NULL;
    }

    /* Libère tous les objets produits par GL4Dummies, ici
     * principalement le screen */
    gl4duClean(GL4DU_ALL);
}
