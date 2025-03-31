#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

struct Position {
    int x, y;
};

vector<vector<string>> mazes;
Position startPos, endPos;
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
    
    // Ajouter le dernier labyrinthe s'il y en a un en attente
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

int main() {
    loadLabyrinths("./labyrinthe.txt");

    displayLabyrinths();

    return 0;
}
