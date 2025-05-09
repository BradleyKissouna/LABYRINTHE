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

// Echelles
struct Ladder {
    int level;
    int x, y;
    int object;
};

vector<vector<string>> mazes, solvedMazes;
vector<vector<Ladder>> ladders;
thread_local Position startPos, endPos;  // Utilisation de thread_local pour éviter les conflits
bool collected[3] = {false, false, false};

// Directions
const int dx[] = {-1, 1, 0, 0};
const int dy[] = {0, 0, -1, 1};
const char OBJECTS[3] = {'E', 'B', 'C'};

mutex mtx;

// Chargement des labyrinthes depuis un fichier texte
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
    solvedMazes = mazes;
}

// Sauvegarde
void saveSolvedMaze(int level, const vector<string> &maze) {
    lock_guard<mutex> lock(mtx);
    solvedMazes[level] = maze;
}

// Affichage 
void displaySolvedMaze(int level) {
    cout << "\nLabyrinthe (niveau " << level << ") r\x82solu :\n";
    for (const auto &row : solvedMazes[level]) {
        for (char ch : row) {
            if (ch == '*')
                cout << "\033[32m*\033[0m";
            else
                cout << ch;
        }
        cout << endl;
    }
}

bool isValidMove(int x, int y, int level, const vector<string>& maze) {
    return (x >= 0 && x < maze.size() &&
            y >= 0 && y < maze[0].size() &&
            maze[x][y] != '#' &&
            maze[x][y] != 'M' &&
            maze[x][y] != '*');
}

//Exo 1
bool solveMaze(int x, int y, int level) {
    if (mazes[level][x][y] == 'A' && collected[0] && collected[1] && collected[2]) {
        mazes[level][x][y] = '*';
        saveSolvedMaze(level, mazes[level]);
        return true;
    }

    char temp = mazes[level][x][y];
    bool collectedHere = false;
    int collectedIndex = -1;

    // Objet
    auto object = find(begin(OBJECTS), end(OBJECTS), temp);
    if (object != end(OBJECTS)) {
        collectedIndex = distance(begin(OBJECTS), object);
        if (!collected[collectedIndex]) {
            collected[collectedIndex] = true;
            collectedHere = true;
        }
    }

    // TNT
    if (temp == 'T') {
        size_t posTab = mazes[level][x].find('\t');
        if (posTab != string::npos && y < posTab) {
            y = (int)(posTab + 1 + y);
            endPos.y = (int)posTab + 1 + endPos.y;
            cout << "Teortation de (" << x << ", " << y << ") \xE0 (" << x << ", " << y << ")\n";
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

    // Passage à l'étage supérieur si on atteint la fin
    if (x == endPos.x && y == endPos.y && level + 1 < (int)mazes.size()) {
        saveSolvedMaze(level, mazes[level]);
        return true;
    }

    // Backtrack
    mazes[level][x][y] = temp;
    if (collectedHere)
        collected[collectedIndex] = false;
    return false;
}

// Recherche du départ et de l'arrivée
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

    // Valeurs par défaut
    if (!startFound && !ladders[level].empty())
        startPos = {ladders[level][0].x, ladders[level][0].y};

    if (!endFound && !ladders[level].empty()) {
        endPos = ladders[level].size() > 1 ? Position{ladders[level][1].x, ladders[level][1].y} : Position{ladders[level][0].x, ladders[level][0].y};
    }
}

void solveLabyrinth_V1(int level) {
    initializePositions(level);
    cout << "Depart level " << level << " : (" << startPos.x << "," << startPos.y << ") | Arriver : (" << endPos.x << "," << endPos.y << ")\n";
    if (!solveMaze(startPos.x, startPos.y, level)) {
        lock_guard<mutex> lock(mtx);
        solvedMazes[level] = { "Impossible de resoudre le labyrinthe de niveau " + to_string(level) };
    }
}

int main() {
    mazes.clear();
    solvedMazes.clear();
    ladders.clear();

    loadLabyrinths("labyrinthe.txt");

    int choice = 0;
    cout << "Choix de l'exo' :\n";
    cout << "1. Exo 1\n";
    cout << "2. Exo 2)\n";
    cout << "Votre choix : ";
    cin >> choice;

    vector<thread> threads;
    if (choice == 1) {
        for (int level = 0; level < mazes.size(); ++level)
            threads.push_back(thread(solveLabyrinth_V1, level));
    } else if (choice == 2) {
        // TODO
    } else {
        cout << "Choix invalide.\n";
        return 1;
    }

    for (auto &t : threads)
        t.join();

    // Affichage final
    for (int level = 0; level < solvedMazes.size(); ++level)
        displaySolvedMaze(level);

    return 0;
}
