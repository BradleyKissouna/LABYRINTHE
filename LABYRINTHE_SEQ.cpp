#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <set>


using namespace std;

struct Position {
    int x, y;
};

struct Ladder {
    int level;
    int x, y;
    int object;
};

vector<vector<string>> mazes, solvedMazes;
Position startPos,endPos;
vector<vector<Ladder>> ladders;
bool collected[3] = {false, false, false};

const int dx[] = {-1, 1, 0, 0};
const int dy[] = {0, 0, -1, 1};
const char OBJECTS[3] = {'E', 'B', 'C'};

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
    ladders.resize(mazes.size());
    solvedMazes.resize(mazes.size());
}

void saveSolvedMaze(int level) {
    solvedMazes[level] = mazes[level];
}

bool isValidMove(int x, int y, int level) {
    return (x >= 0 && x < mazes[level].size() &&
            y >= 0 && y < mazes[level][0].size() &&
            mazes[level][x][y] != '#' &&
            mazes[level][x][y] != 'M' &&
            mazes[level][x][y] != '*');
}


// Afficher le labyrinthe
void displaySolvedMaze(int level) {
    cout << "\nDisplay du labyrinthe (niveau " << level << ") :\n";
    for (const auto& row : solvedMazes[level]) {
        for (char ch : row) {
            if (ch == '*') cout << "\033[31m*\033[0m";
            else cout << ch;
        }
        cout << endl;
    }
}

// Résolution du labyrinthe
bool solveMaze(int x, int y, int level) {
    if (level == (int)mazes.size() - 1 && x == endPos.x && y == endPos.y &&
        collected[0] && collected[1] && collected[2]) {
        mazes[level][x][y] = '*';
        saveSolvedMaze(level);
        return true;
    }

    char temp = mazes[level][x][y];
    bool collectedHere = false;
    int collectedIndex = -1;

    // Vérification objet
    auto object = std::find(std::begin(OBJECTS), std::end(OBJECTS), temp);
    if (object != std::end(OBJECTS)) {
        collectedIndex = std::distance(std::begin(OBJECTS), object);
        if (!collected[collectedIndex]) {
            collected[collectedIndex] = true;
            collectedHere = true;
        }
    }

    // TNT
    if (temp == 'T') {
        size_t posTab = mazes[level][x].find('\t');
        if (posTab != string::npos && y < posTab) {;
            y = (int)posTab + 1 + y;
            endPos.y = (int)posTab + 1 + endPos.y;
            return solveMaze(x, y, level);
        }
        saveSolvedMaze(level);
        return true;
    }

    // Marquer la case
    mazes[level][x][y] = '*';

    // Mouvements
    for (int i = 0; i < 4; ++i) {
        int nx = x + dx[i], ny = y + dy[i];
        if (isValidMove(nx, ny, level)) {
            if (solveMaze(nx, ny, level)) return true;
        }
    }

    // Echelle
    if (x == endPos.x && y == endPos.y && level + 1 < (int)mazes.size()) {
        if (collected[level]) {
            saveSolvedMaze(level);
            return true;
        }
    }

    // Backtracking
    mazes[level][x][y] = temp;
    if (collectedHere) {
        collected[collectedIndex] = false;
    }
    return false;
}

// Init depart et arrivée
void initializePositions(int level) {
    bool startFound = false, endFound = false;

    for (int i = 0; i < mazes[level].size(); i++) {
        // Si il y a un espace je coupe la ligne en 2
        size_t posTab = mazes[level][i].find('\t');
        size_t maxJ = (posTab != string::npos) ? posTab : mazes[level][i].size();

        for (int j = 0; j < (int)maxJ; j++) {
            char cell = mazes[level][i][j];

            if (cell == 'D') { // Départ
                startPos = {i, j};
                startFound = true;
            }
            else if (cell == 'A') { // Arrivée
                endPos = {i, j};
                endFound = true;
            }
            else if (isdigit(cell)) { // Echelle
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

bool askContinue() {
    string response;
    cout << "\n- Passer au labyrinthe suivant ? (y/n): ";
    cin >> response;
    return (response == "y" || response == "Y");
}

int main() {
    loadLabyrinths("./labyrinthe.txt");
    for (int level = 0; level < mazes.size(); ++level) {
        initializePositions(level);

        cout << "Départ : (" << startPos.x << "," << startPos.y << ")";
        cout << " | Arrivée : (" << endPos.x << "," << endPos.y << ")\n";

        bool solved = solveMaze(startPos.x, startPos.y, level);

        if (solved) {
            displaySolvedMaze(level);
        } else {
            cout << "Impossible de résoudre le labyrinthe de niveau " << level << endl;
        }

        // Demander à l'utilisateur s'il veut continuer vers le niveau suivant
        if (level < (int)mazes.size() - 1 && !askContinue()) {
            cout << "Exit.\n";
            break;
        }
    }
    return 0;
}
