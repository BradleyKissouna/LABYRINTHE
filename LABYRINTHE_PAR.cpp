#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <set>
#include <thread>
#include <mutex>
#include <cstring>

using namespace std;

struct Position {
    int x, y;
};

struct Ladder {
    int level;
    int x, y;
    int object;
};

// Variables globales communes
vector<vector<string>> mazes, solvedMazes;
vector<vector<Ladder>> ladders;
Position startPos, endPos;
// Utilisés dans la Variante 1 (backtracking séquentiel)
bool collected[3] = {false, false, false};

const int dx[] = {-1, 1, 0, 0};
const int dy[] = {0, 0, -1, 1};
const char OBJECTS[3] = {'E', 'B', 'C'};

mutex mtx;  // Protège l'accès à solvedMazes

// --- Chargement du labyrinthe ---
void loadLabyrinths(const string &filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Erreur : Impossible d'ouvrir " << filename << endl;
        exit(1);
    }
    vector<string> currentLab;
    string line;
    while(getline(file, line)) {
        if (line.empty()) {
            if (!currentLab.empty()) {
                mazes.push_back(currentLab);
                currentLab.clear();
            }
        } else {
            currentLab.push_back(line);
        }
    }
    if (!currentLab.empty())
        mazes.push_back(currentLab);
    file.close();
    
    ladders.resize(mazes.size());
    solvedMazes = mazes; // On initialise solvedMazes avec l'état initial
}

// --- Sauvegarde et affichage ---
void saveSolvedMaze(int level, const vector<string> &maze) {
    lock_guard<mutex> lock(mtx);
    solvedMazes[level] = maze;
}

void displaySolvedMaze(int level) {
    cout << "\nLabyrinthe (niveau " << level << ") résolu :\n";
    for (const auto &row : solvedMazes[level]) {
        for (char ch : row) {
            if (ch == '*')
                cout << "\033[32m*\033[0m"; // Chemin en vert
            else
                cout << ch;
        }
        cout << endl;
    }
}

// --- Vérifier un mouvement dans un labyrinthe donné ---
bool isValidMove(int x, int y, int level, const vector<string>& maze) {
    return (x >= 0 && x < maze.size() &&
            y >= 0 && y < maze[0].size() &&
            maze[x][y] != '#' &&
            maze[x][y] != 'M' &&
            maze[x][y] != '*');
}

// ---------------------
// Variante 1 : Backtracking séquentiel par labyrinthe
bool solveMaze(int x, int y, int level) {
    if (mazes[level][x][y] == 'A' && collected[0] && collected[1] && collected[2]) {
        mazes[level][x][y] = '*';
        saveSolvedMaze(level, mazes[level]);
        return true;
    }
    
    char temp = mazes[level][x][y];
    bool collectedHere = false;
    int collectedIndex = -1;
    
    auto object = find(begin(OBJECTS), end(OBJECTS), temp);
    if (object != end(OBJECTS)) {
        collectedIndex = distance(begin(OBJECTS), object);
        if (!collected[collectedIndex]) {
            collected[collectedIndex] = true;
            collectedHere = true;
        }
    }
    
    // Gestion du TNT (Variante 1) : si on rencontre 'T', on téléporte
    if (temp == 'T') {
        size_t posTab = mazes[level][x].find('\t');
        if (posTab != string::npos && y < posTab) {
            y = (int)posTab + 1 + y;
            return solveMaze(x, y, level);
        }
        saveSolvedMaze(level, mazes[level]);
        return true;
    }
    
    mazes[level][x][y] = '*';
    
    for (int i = 0; i < 4; ++i) {
        int nx = x + dx[i], ny = y + dy[i];
        if (isValidMove(nx, ny, level, mazes[level])) {
            if (solveMaze(nx, ny, level))
                return true;
        }
    }
    
    if (x == endPos.x && y == endPos.y && level + 1 < (int)mazes.size()) {
        saveSolvedMaze(level, mazes[level]);
        return true;
    }
    
    mazes[level][x][y] = temp;
    if (collectedHere)
        collected[collectedIndex] = false;
    return false;
}

void initializePositions(int level) {
    bool startFound = false, endFound = false;
    for (int i = 0; i < mazes[level].size(); ++i) {
        size_t posTab = mazes[level][i].find('\t');
        size_t maxJ = (posTab != string::npos) ? posTab : mazes[level][i].size();
        for (int j = 0; j < (int)maxJ; ++j) {
            char cell = mazes[level][i][j];
            if (cell == 'D') {
                startPos = {i, j};
                startFound = true;
            } else if (cell == 'A') {
                endPos = {i, j};
                endFound = true;
            } else if (isdigit(cell)) {
                ladders[level].push_back({level, i, j, cell});
            }
        }
    }
    // Si on n'a pas trouvé l'arrivée ('A'), on utilise la première échelle si elle existe
    if (!startFound && !ladders[level].empty()) {
        startPos = {ladders[level][0].x, ladders[level][0].y};
        startFound = true;
    }

    // Si on n'a pas trouvé l'arrivée ('A'), on utilise la deuxième échelle si elle existe
    if (!endFound && !ladders[level].empty()) {
        // Si la deuxième échelle existe, on l'utilise pour l'arrivée
        if (ladders[level].size() > 1) {
            endPos = {ladders[level][1].x, ladders[level][1].y};
            endFound = true;
        }
        // Si l'échelle est la seule trouvée, on utilise la même pour l'arrivée
        else {
            endPos = {ladders[level][0].x, ladders[level][0].y};
            endFound = true;
        }
    }
}

// Fonction de résolution pour la Variante 1
void solveLabyrinth_V1(int level) {
    initializePositions(level);
    if (!solveMaze(startPos.x, startPos.y, level)) {
        lock_guard<mutex> lock(mtx);
        solvedMazes[level] = { "Impossible de résoudre le labyrinthe de niveau " + to_string(level) };
    }
}

// ---------------------
// Variante 2 : Backtracking parallèle intra-labyrinthe
// Chaque appel lance 4 threads pour explorer simultanément les directions, en travaillant sur des copies locales.
bool solveMazeParallel(int x, int y, int level, vector<string> maze, bool localCollected[3]) {
    if (maze[x][y] == 'A' && localCollected[0] && localCollected[1] && localCollected[2]) {
        maze[x][y] = '*';
        saveSolvedMaze(level, maze);
        return true;
    }
    
    char temp = maze[x][y];
    bool collectedHere = false;
    int collectedIndex = -1;
    auto object = find(begin(OBJECTS), end(OBJECTS), temp);
    if (object != end(OBJECTS)) {
        collectedIndex = distance(begin(OBJECTS), object);
        if (!localCollected[collectedIndex]) {
            localCollected[collectedIndex] = true;
            collectedHere = true;
        }
    }
    
    // Gestion du TNT : vérifie qu'on effectue bien une téléportation effective.
    if (temp == 'T') {
        size_t posTab = maze[x].find('\t');
        if (posTab != string::npos && y < posTab) {
            int newY = (int)posTab + 1 + y;
            // Vérifier que newY est strictement supérieur à y pour éviter de rester bloqué.
            if (newY <= y) {
                // Si ce n'est pas le cas, on traite la cellule comme normale.
            } else {
                maze[x][y] = '*'; // Marquer la cellule pour éviter de revenir dessus
                return solveMazeParallel(x, newY, level, maze, localCollected);
            }
        }
    }
    
    maze[x][y] = '*';
    
    bool found = false;
    vector<thread> threads;
    mutex foundMutex;
    
    for (int i = 0; i < 4; ++i) {
        int nx = x + dx[i], ny = y + dy[i];
        if (isValidMove(nx, ny, level, maze)) {
            threads.push_back(thread([=, &found, &foundMutex]() mutable {
                vector<string> mazeCopy = maze;
                bool collectedCopy[3];
                memcpy(collectedCopy, localCollected, 3 * sizeof(bool));
                if (solveMazeParallel(nx, ny, level, mazeCopy, collectedCopy)) {
                    lock_guard<mutex> lock(foundMutex);
                    found = true;
                }
            }));
        }
    }
    
    for (auto &t : threads)
        t.join();
    return found;
}

void solveLabyrinth_V2(int level) {
    bool localCollected[3] = {false, false, false};
    initializePositions(level);
    if (!solveMazeParallel(startPos.x, startPos.y, level, mazes[level], localCollected)) {
        lock_guard<mutex> lock(mtx);
        solvedMazes[level] = { "Impossible de résoudre le labyrinthe de niveau " + to_string(level) };
    }
}

int main() {
    loadLabyrinths("labyrinthe.txt");
    
    int choice = 0;
    cout << "Choix de la variante parallèle à utiliser :\n";
    cout << "1. Parallélisation par labyrinthe (Variante 1)\n";
    cout << "2. Parallélisation intra-labyrinthe (Variante 2)\n";
    cout << "Votre choix : ";
    cin >> choice;
    
    vector<thread> threads;
    if (choice == 1) {
        for (int level = 0; level < mazes.size(); ++level)
            threads.push_back(thread(solveLabyrinth_V1, level));
    } else if (choice == 2) {
        for (int level = 0; level < mazes.size(); ++level)
            threads.push_back(thread(solveLabyrinth_V2, level));
    } else {
        cout << "Choix invalide.\n";
        return 1;
    }
    
    for (auto &t : threads)
        t.join();
    
    // Affichage final de tous les labyrinthes dans l'ordre
    for (int level = 0; level < solvedMazes.size(); ++level)
        displaySolvedMaze(level);
    
    return 0;
}
