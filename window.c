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
static float _cubeSize = 4.f;

/*!\brief Surface représentant une sphère */
static surface_t * _sphere = NULL;

/* Variable d'état pour activer/désactiver la synchronisation verticale */
static int _use_vsync = 1;

/* Variable d'état pour activer/désactiver le debug */
static int _debug = 0;

/* Grille de positions où il y aura des cubes
 * 0 -> Vide
 * 1 -> Mur
 * 2 (valeur reservée) -> Joueur A (défini automatiquement par le programme)
 * 3 (valeur reservée) -> Joueur B (défini automatiquement par le programme) */
static int * _grille = NULL;
static int _grilleW;
static int _grilleH;

/* Définition d'un personnage */
typedef struct perso_t {
    float x, y, z; // coordonées
    int position; // position dans la grille
    surface_t * perso; // objet
} perso_t;

/* Définition de nos deux joueurs */
perso_t _herosA = { 4.f, 0.f, -6.f, -1, NULL }; // à droite
perso_t _herosB = { -4.f, 0.f, -1.f, -1, NULL }; // à gauche

/* Clavier virtuel */
enum {
    /* Héros A */
    VK_RIGHT = 0,
    VK_UP,
    VK_LEFT,
    VK_DOWN,

    /* Héros B */
    VK_d,
    VK_z,
    VK_q,
    VK_s,

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
    GLuint id;
    /* Création d'un screen GL4Dummies (texture dans laquelle nous
     * pouvons dessiner) aux dimensions de la fenêtre. IMPORTANT de
     * créer le screen avant d'utiliser les fonctions liées au
     * textures */
    gl4dpInitScreen();

    /* Création du cube */
    _cube   = mk_cube(); /* ça fait 2x6 triangles */
    _sphere = mk_sphere(12, 12); /* ça fait 12x12 triangles */

    /* Rajoute la texture */
    id = get_texture_from_BMP("images/tex.bmp");
    set_texture_id(_cube, id);
    set_texture_id(_sphere, id);

    /* Affichage des textures */
    enable_surface_option(_cube, SO_USE_TEXTURE);
    enable_surface_option(_sphere, SO_USE_TEXTURE);

    /* Affichage des ombres */
    enable_surface_option(_cube, SO_USE_LIGHTING);
    enable_surface_option(_sphere, SO_USE_LIGHTING);

    /* Si _use_vsync != 0, on active la synchronisation verticale */
    SDL_GL_SetSwapInterval(_use_vsync);

    /* Génération du plateau */
    srand(time(NULL));

    /* TODO
     * Génération pour _grilleH aléatoire aussi
     * Corriger la position des joueurs en fonction de la map pour pas spawn dans un mur
     * Zoom de la caméra en fonction de la taille de la map */

    _grilleW = 10 + (rand() % 10);
    _grilleH = _grilleW;

    if ((_grille = malloc((_grilleW * _grilleH) * sizeof(int))) == NULL) {
        printf("Impossible d'allouer de la mémoire supplémentaire pour générer le plateau.\n");
        sortie();
        exit(1);
    }

    int _curseur = 0;
    for(int i = 0; i < _grilleH; i++)
        for(int j = 0; j < _grilleW; j++) {
            if (i == 0) {
                _grille[_curseur] = 1;
                _curseur++;
                continue;
            }
            if (i == (_grilleH - 1)) {
                _grille[_curseur] = 1;
                _curseur++;
                continue;
            }
            if (j == 0) {
                _grille[_curseur] = 1;
                _curseur++;
                continue;
            }
            if (j == (_grilleW - 1)) {
                _grille[_curseur] = 1;
                _curseur++;
                continue;
            }
            if ((j % 2) == 0 && (i % 2) == 0) {
                _grille[_curseur] = 1;
                _curseur++;
                continue;
            }
            _grille[_curseur] = 0;
            _curseur++;
        }

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

    float vitesse = 10.f; // vitesse des joueurs

    /* Calcul du décalage */
    float decalageAutorisee = .15f; // décalage autorisé, modifier cette valeur si nécessaire
    float decalageGB = .0f + decalageAutorisee; // décalage pour la gauche et le bas
    float decalageDH = 1.f - decalageAutorisee; // décalage pour la droite et le haut

    /* Mouvements du héros A */
    /* Coordonées x, z */
    float zA = (float)((_herosA.z + _cubeSize * _grilleH / 2) / _cubeSize); // ligne   - hauteur
    float xA = (float)((_herosA.x + _cubeSize * _grilleW / 2) / _cubeSize); // colonne - longueur

    /* Coordonnées joueur A */
    int coorJoueurA = round(zA) * _grilleH + round(xA);
    int coorDroiteA = round(zA) * _grilleH + ceil(xA);
    int coorHautA   = floor(zA) * _grilleH + round(xA);
    int coorGaucheA = round(zA) * _grilleH + floor(xA);
    int coorBasA    = ceil(zA)  * _grilleH + round(xA);

    /* Décalage pour éviter bug de collisions */
    float decalageLargeurA  = zA - floor(zA);
    float decalageLongueurA = xA - floor(xA);

    /* Déplacement */
    if(_vkeyboard[VK_RIGHT])
        if((_grille[coorDroiteA] == 0 || _grille[coorDroiteA] == 2) && (decalageLargeurA < decalageGB || decalageLargeurA > decalageDH)) // collision à droite du plateau
            _herosA.x += vitesse * dt;
    if(_vkeyboard[VK_UP])
        if((_grille[coorHautA] == 0 || _grille[coorHautA] == 2) && (decalageLongueurA < decalageGB || decalageLongueurA > decalageDH)) // collision en haut du plateau
            _herosA.z -= vitesse * dt;
    if(_vkeyboard[VK_LEFT])
        if((_grille[coorGaucheA] == 0 || _grille[coorGaucheA] == 2) && (decalageLargeurA < decalageGB || decalageLargeurA > decalageDH)) // collision à gauche du plateau
            _herosA.x -= vitesse * dt;
    if(_vkeyboard[VK_DOWN])
        if((_grille[coorBasA] == 0 || _grille[coorBasA] == 2) && (decalageLongueurA < decalageGB || decalageLongueurA > decalageDH)) // collision en bas du plateau
            _herosA.z += vitesse * dt;

    /* Affichage Debug */
    if(_debug) {
        printf("\n========= Héros A =========\n");
        printf(" li = %d, col = %d, idx = %d\n", (int)(zA + .5f), (int)(xA + .5f), _herosA.position);
        printf(" zA=%f xA=%f\n", zA, xA);
        printf(" d=%d h=%d g=%d b=%d\n", coorDroiteA, coorHautA, coorGaucheA, coorBasA);
    }

    /* Anti-collision entre joueurs */
    if(_herosA.position != coorJoueurA) {
        if(_herosA.position != -1)
            _grille[_herosA.position] = 0;
        _herosA.position = coorJoueurA;
        _grille[coorJoueurA] = 2;
    }


    /* Mouvements du héros B */
    /* Coordonées x, z */
    float zB = (float)((_herosB.z + _cubeSize * _grilleH / 2) / _cubeSize); // ligne - hauteur
    float xB = (float)((_herosB.x + _cubeSize * _grilleW / 2) / _cubeSize); // colonne - longueur

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
        if((_grille[coorDroiteB] == 0 || _grille[coorDroiteB] == 3) && (decalageLargeurB < decalageGB || decalageLargeurB > decalageDH)) // collision à droite du plateau
            _herosB.x += vitesse * dt;
    if(_vkeyboard[VK_z])
        if((_grille[coorHautB] == 0 || _grille[coorHautB] == 3) && (decalageLongueurB < decalageGB || decalageLongueurB > decalageDH)) // collision en haut du plateau
            _herosB.z -= vitesse * dt;
    if(_vkeyboard[VK_q])
        if((_grille[coorGaucheB] == 0 || _grille[coorGaucheB] == 3) && (decalageLargeurB < decalageGB || decalageLargeurB > decalageDH)) // collision à gauche du plateau
            _herosB.x -= vitesse * dt;
    if(_vkeyboard[VK_s])
        if((_grille[coorBasB] == 0 || _grille[coorBasB] == 3) && (decalageLongueurB < decalageGB || decalageLongueurB > decalageDH)) // collision en bas du plateau
            _herosB.z += vitesse * dt;

    /* Affichage Debug */
    if(_debug) {
        printf("========= Héros B =========\n");
        printf(" li = %d, col = %d, idx = %d\n", (int)(zB + .5f), (int)(xB + .5f), _herosB.position);
        printf(" zA=%f xA=%f\n", zB, xB);
        printf(" d=%d h=%d g=%d b=%d\n", coorDroiteB, coorHautB, coorGaucheB, coorBasB);
        printf("===========================\n");
    }

    /* Anti-collision entre joueurs */
    if(_herosB.position != coorJoueurB) {
        if(_herosB.position != -1)
            _grille[_herosB.position] = 0;
        _herosB.position = coorJoueurB;
        _grille[coorJoueurB] = 3;
    }
}

/*!\brief Fonction appelée à chaque display. */
void draw(void) {
    vec4 couleurPlateau = {0.2, 0.2, 0.2, 1} /* Gris */,
         couleurHerosA  = {0.15, 0.5, 0.15, 1} /* Vert */,
         couleurHerosB  = {0.2, 0.2, 0.7, 1} /* Bleu */;

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
    lookAt(model_view_matrix, 0, 100 /* zoom */, 30 /* inclinaison */, 0, 0, 0, 0, 0, -1);

    /* Pour centrer la grille par rapport au monde */
    float cX = -_cubeSize * _grilleW / 2.0f;
    float cZ = -_cubeSize * _grilleH / 2.0f;

    /* On change la couleur */
    _cube->dcolor = couleurPlateau;

    /* Pour toutes les cases de la grille, afficher un cube quand il y a
     * un 1 dans la grille */
    for(int i = 0; i < _grilleW; ++i)
        for(int j = 0; j < _grilleH; ++j)
            if(_grille[i * _grilleW + j] == 1) {
                /* copie model_view_matrix dans nmv */
                memcpy(nmv, model_view_matrix, sizeof(nmv));

                /* pour convertir les coordonnées i, j de la grille en x, z du monde */
                translate(nmv, _cubeSize * j + cX, 0.0f, _cubeSize * i + cZ);
                scale(nmv, _cubeSize / 2.0f, _cubeSize / 2.0f, _cubeSize / 2.0f);
                transform_n_rasterize(_cube, nmv, projection_matrix);
            }

    /* Dessine le héros A */
    _cube->dcolor = couleurHerosA;
    _sphere->dcolor = couleurHerosA;
    memcpy(nmv, model_view_matrix, sizeof(nmv));
    /* Corps */
    translate(nmv, _herosA.x, _herosA.y, _herosA.z);
    scale(nmv, _cubeSize / 3.0f, _cubeSize / 3.0f, _cubeSize / 3.0f);
    transform_n_rasterize(_cube, nmv, projection_matrix);
    /* Tête */
    translate(nmv, 0.0f, 2.0f, 0.0f);
    transform_n_rasterize(_sphere, nmv, projection_matrix);

    /* Dessine le héros B */
    _cube->dcolor = couleurHerosB;
    _sphere->dcolor = couleurHerosB;
    memcpy(nmv, model_view_matrix, sizeof(nmv));
    /* Corps */
    translate(nmv, _herosB.x, _herosB.y, _herosB.z);
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
        /* 'v' utiliser la sync Verticale */
        case GL4DK_v:
            _use_vsync = !_use_vsync;
            if(_use_vsync)
                SDL_GL_SetSwapInterval(1);
            else
                SDL_GL_SetSwapInterval(0);
            break;

        /* 'h' afficher ou non les infos de debug */
        case GL4DK_h:
            _debug = !_debug;
            break;

        /* Héros A */
        case GL4DK_RIGHT:
            _vkeyboard[VK_RIGHT] = 1;
            break;

        case GL4DK_UP:
            _vkeyboard[VK_UP] = 1;
            break;

        case GL4DK_LEFT:
            _vkeyboard[VK_LEFT] = 1;
            break;

        case GL4DK_DOWN:
            _vkeyboard[VK_DOWN] = 1;
            break;

        /* Héros B */
        case GL4DK_d:
            _vkeyboard[VK_d] = 1;
            break;

        case GL4DK_z:
            _vkeyboard[VK_z] = 1;
            break;

        case GL4DK_q:
            _vkeyboard[VK_q] = 1;
            break;

        case GL4DK_s:
            _vkeyboard[VK_s] = 1;
            break;

        /* Par défaut on ne fais rien */
        default: break;
    }
}

/*!\brief Intercepte l'évènement clavier pour modifier les options (au relâchement d'une touche). */
void keyu(int keycode) {
    switch(keycode) {
        /* Cas où l'héros A on arrête de bouger */
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

        /* Cas où l'héros A on arrête de bouger */
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
    /* on libère le cube */
    if(_cube) {
        free_surface(_cube);
        _cube = NULL;
    }

    /* on libère la sphère */
    if(_sphere) {
        free_surface(_sphere);
        _sphere = NULL;
    }

    /* libère tous les objets produits par GL4Dummies, ici
     * principalement les screen */
    gl4duClean(GL4DU_ALL);
}
