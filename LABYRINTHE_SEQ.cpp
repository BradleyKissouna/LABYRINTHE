#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

struct Position {
    int x, y;
};

vector<vector<string>> mazes;
Position startPos, endPos;
vector<Position> ladders;
bool collected[3] = {false, false, false}; // C, B, E

// Directions possibles (haut, bas, gauche, droite)
const int dx[] = {-1, 1, 0, 0};
const int dy[] = {0, 0, -1, 1};
const char OBJECTS[3] = {'C', 'B', 'E'};

// Charger les labyrinthes depuis un fichier
void loadLabyrinths(const string &filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Erreur : Impossible d'ouvrir " << filename << endl;
        exit(1);
    }

    vector<string> currentLab;
    string line;
    
    while (getline(file, line)) {
        if (line.empty()) { 
            if (!currentLab.empty()) {
                mazes.push_back(currentLab);
                currentLab.clear();
            }
        } else {
            currentLab.push_back(line);
        }
    }
    
    // Ajouter le dernier labyrinthe
    if (!currentLab.empty()) {
        mazes.push_back(currentLab);
    }
    
    file.close();
}

// Afficher les labyrinthes
void displayLabyrinths() {
    for (size_t i = 0; i < mazes.size(); i++) {
        cout << "Labyrinthe " << i + 1 << " :\n";
        for (const auto& row : mazes[i]) {
            cout << row << endl;
        }
        cout << endl;
    }
}

// Vérifier si une position est valide
bool isValidMove(int x, int y, int level) {
    return (x >= 0 && x < mazes[level].size() &&
            y >= 0 && y < mazes[level][0].size() &&
            mazes[level][x][y] != '#');
}

// Fonction récursive de backtracking
bool solveMaze(int x, int y, int level, int ladderIndex = 0) {
    if (x == endPos.x && y == endPos.y && collected[0] && collected[1] && collected[2]) {
        return true; // Fin atteinte avec tous les objets
    }

    char temp = mazes[level][x][y];
    mazes[level][x][y] = '*'; // Marquer la case visitée

    // Vérifier si un objet a été collecté
    for (int i = 0; i < 3; i++) {
        if (temp == OBJECTS[i]) collected[i] = true;
    }

    // Vérifier si on est sur une échelle
    if (isdigit(temp)) {
        int ladderNum = temp - '0';
        if (ladderIndex < ladders.size() && ladderNum == ladderIndex + 1) {
            ladderIndex++; // On valide cette échelle et passe à la suivante
            cout << "Échelle " << ladderNum << " atteinte !\n";
        }
    }

    // Exploration dans les 4 directions
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i], ny = y + dy[i];
        if (isValidMove(nx, ny, level) && solveMaze(nx, ny, level, ladderIndex)) {
            return true;
        }
    }
    
    mazes[level][x][y] = temp;
    return false;
}

int main() {
    loadLabyrinths("./labyrinthe.txt");

    // displayLabyrinths();
    // Traiter chaque labyrinthe génériquement, sans boucle prédéfinie
    while (size_t mazeIndex = 0 < mazes.size()) {
        vector<string> currentMaze = mazes[mazeIndex];
        bool foundStart = false, foundEnd = false;
        ladders.clear();

        // Initialisation de la position de départ et d'arrivée pour chaque labyrinthe
        for (int i = 0; i < currentMaze.size(); i++) {
            for (int j = 0; j < currentMaze[i].size(); j++) {
                if (currentMaze[i][j] == 'D') {
                    startPos = {i, j};  // Utiliser startPos ici
                    foundStart = true;
                }
                if (currentMaze[i][j] == 'A') {
                    endPos = {i, j};  // Utiliser endPos ici
                    foundEnd = true;
                }
                if (isdigit(currentMaze[i][j])) {  // Vérifier si c'est une échelle
                    ladders.push_back({i, j});
                }
            }
        }

        // Trier les échelles par ordre croissant
        sort(ladders.begin(), ladders.end(), [&](Position a, Position b) {
            return currentMaze[a.x][a.y] < currentMaze[b.x][b.y];
        });

        if (foundStart && foundEnd) {
            cout << "Labyrinthe " << mazeIndex + 1 << " en cours de résolution...\n";
            
            if (solveMaze(startPos.x, startPos.y, mazeIndex)) {
                cout << "Solution trouvée pour le labyrinthe " << mazeIndex + 1 << " !\n";
            } else {
                cout << "Aucun chemin trouvé pour le labyrinthe " << mazeIndex + 1 << ".\n";
            }
        } else {
            cout << "Départ ou arrivée manquants dans le labyrinthe " << mazeIndex + 1 << ".\n";
        }
        // Passer au labyrinthe suivant
        mazeIndex++;
    }

    return 0;
}
