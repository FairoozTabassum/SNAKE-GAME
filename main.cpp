#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <fstream>
using namespace std;

// Snake Part Structure
struct Point {
    int x, y;
};

enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };

const int WIDTH = 40;
const int HEIGHT = 20;
bool gameOver;
Direction dir;
vector<Point> snake;
Point fruit;
Point bonusFruit;
bool bonusActive = false;
Point powerUp;
bool powerUpActive = false;
int score;
int speed = 100;
int level = 1;

bool fireMode = false;
int fireModeDuration = 0;
bool paused = false;

const int levelThreshold = 50; // score required to level up

vector<Point> walls = {
    {10, 5}, {11, 5}, {12, 5}, {13, 5},
    {25, 10}, {26, 10}, {27, 10},
    {5, 15}, {6, 15}, {7, 15}, {8, 15}
};

int highScore = 0;

// --------- Color ----------
void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void GotoXY(int x, int y) {
    COORD pos = { (short)x, (short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void ClearScreen() {
    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hConsole, ' ', dwConSize, coordScreen, &cCharsWritten);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
    SetConsoleCursorPosition(hConsole, coordScreen);
}

void LoadHighScore() {
    ifstream in("score.txt");
    if (in >> highScore) {}
    in.close();
}

void SaveHighScore() {
    if (score > highScore) {
        ofstream out("score.txt");
        out << score;
        out.close();
    }
}

void ShowTitle() {
    ClearScreen();
    SetColor(11);
    GotoXY(5, 2);
    cout << R"(
  ____   _   _    _    _  __  _____
 / ___| | \ | |  / \  | |/ / | ____|
 \___ \ |  \| | / _ \ | ' /  |  _|
  ___) || |\  |/ ___ \| . \  | |___
 |____/ |_| \_/_/   \_\_|\_\ |_____|
)";
    SetColor(10);
    GotoXY(5, 10);
    cout << "Use WASD keys to move the snake.";
    GotoXY(5, 11);
    cout << "Press 'P' to pause/resume. 'X' to quit.";
    GotoXY(5, 12);
    cout << "High Score: " << highScore;
    GotoXY(5, 14);
    SetColor(14);
    cout << "Press any key to start...";
    SetColor(7);
    _getch();
}

void ShowGameOver() {
    ClearScreen();
    SetColor(12);
    GotoXY(4, 3);
    cout << R"(
   ____                         ___
  / ___| __ _ _ __ ___   ___   / _ \__   _____ _ __
 | |  _ / _` | '_ ` _ \ / _ \ | | | \ \ / / _ \ '__|
 | |_| | (_| | | | | | |  __/ | |_| |\ V /  __/ |
  \____|\__,_|_| |_| |_|\___|  \___/  \_/ \___|_|
)";
    SetColor(14);
    GotoXY(10, 12);
    cout << "Your Final Score: " << score << endl;
    if (score > highScore) GotoXY(10, 13), cout << "New High Score!";

    // Game over beep sequence
    Beep(800, 300);
    Beep(600, 300);
    Beep(400, 600);

    GotoXY(10, 15);
    cout << "Press any key to exit...";
    _getch();
}

void Setup() {
    LoadHighScore();
    gameOver = false;
    dir = STOP;
    snake.clear();
    snake.push_back({WIDTH / 2, HEIGHT / 2});
    fruit = {rand() % WIDTH, rand() % HEIGHT};
    bonusActive = false;
    powerUpActive = false;
    fireMode = false;
    fireModeDuration = 0;
    score = 0;
    speed = 100;
    level = 1;
    system("cls");
}

void Draw() {
    GotoXY(0, 0);
    SetColor(9);
    for (int i = 0; i < WIDTH+2; i++) cout << "#";
    cout << endl;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == 0) cout << "#";
            bool printed = false;

            if (x == fruit.x && y == fruit.y) { SetColor(14); cout << "F"; SetColor(9); printed = true; }
            else if (bonusActive && x == bonusFruit.x && y == bonusFruit.y) { SetColor(13); cout << "B"; SetColor(9); printed = true; }
            else if (powerUpActive && x == powerUp.x && y == powerUp.y) { SetColor(11); cout << "P"; SetColor(9); printed = true; }
            else {
                for (int k = 0; k < snake.size(); k++) {
                    if (snake[k].x == x && snake[k].y == y) {
                        SetColor(k == 0 ? (fireMode ? 12 : 10) : 2);
                        cout << (k == 0 ? "O" : "o");
                        SetColor(9);
                        printed = true; break;
                    }
                }
            }

            if (!printed) {
                for (auto& w : walls) {
                    if (w.x == x && w.y == y) {
                        SetColor(7); cout << "X"; SetColor(9); printed = true; break;
                    }
                }
            }

            if (!printed) cout << ".";
            if (x == WIDTH - 1) cout << "#";
        }
        cout << endl;
    }
    for (int i = 0; i < WIDTH+2; i++) cout << "#";

    SetColor(11);
    cout << "\nScore: " << score << "  Level: " << level << "  High: " << highScore;
    if (fireMode) cout << "   🔥 Fire Mode Active (" << fireModeDuration << ")";
    if (paused) cout << "   [PAUSED]";
    cout << endl;
    SetColor(7);
}

void Input() {
    if (_kbhit()) {
        switch (_getch()) {
            case 'a': if(dir != RIGHT) dir = LEFT; break;
            case 'd': if(dir != LEFT) dir = RIGHT; break;
            case 'w': if(dir != DOWN) dir = UP; break;
            case 's': if(dir != UP) dir = DOWN; break;
            case 'x': gameOver = true; break;
            case 'p': paused = !paused; break;
        }
    }
}

void Logic() {
    if (paused || dir == STOP) return;

    Point prev = snake[0], next;
    switch (dir) {
        case LEFT:  snake[0].x--; break;
        case RIGHT: snake[0].x++; break;
        case UP:    snake[0].y--; break;
        case DOWN:  snake[0].y++; break;
        default: break;
    }

    for (int i = 1; i < snake.size(); i++) { next = snake[i]; snake[i] = prev; prev = next; }

    if (snake[0].x < 0 || snake[0].x >= WIDTH || snake[0].y < 0 || snake[0].y >= HEIGHT) {
        gameOver = true;
        Beep(400, 700);
    }

    for (auto& w : walls) {
        if (snake[0].x == w.x && snake[0].y == w.y) {
            gameOver = true;
            Beep(400, 700);
        }
    }

    if (!fireMode) {
        for (int i = 1; i < snake.size(); i++) {
            if (snake[i].x == snake[0].x && snake[i].y == snake[0].y) {
                gameOver = true;
                Beep(400, 700);
            }
        }
    }

    if (snake[0].x == fruit.x && snake[0].y == fruit.y) {
        score += 10;
        snake.push_back({-1, -1});
        fruit = {rand() % WIDTH, rand() % HEIGHT};
        Beep(800, 100);  // Eat fruit sound

        if (score / levelThreshold + 1 > level) {
            level++;
            speed = max(20, speed - 10);
        }
    }

    if (bonusActive && snake[0].x == bonusFruit.x && snake[0].y == bonusFruit.y) {
        score += 30;
        snake.push_back({-1, -1});
        bonusActive = false;
        Beep(1000, 150);  // Bonus fruit sound
    }

    if (powerUpActive && snake[0].x == powerUp.x && snake[0].y == powerUp.y) {
        powerUpActive = false;
        fireMode = true;
        fireModeDuration = 50;
        speed = max(30, speed - 40);
        Beep(600, 150);  // Power-up sound
    }

    if (fireMode && --fireModeDuration <= 0) {
        fireMode = false;
        speed = 100 - (level - 1) * 10;
    }

    if (!bonusActive && (rand() % 50 == 0)) {
        bonusFruit = {rand() % WIDTH, rand() % HEIGHT}; bonusActive = true;
    }
    if (!powerUpActive && (rand() % 80 == 0)) {
        powerUp = {rand() % WIDTH, rand() % HEIGHT}; powerUpActive = true;
    }
}

int main() {
    srand(time(0));
    Setup();
    ShowTitle();
    while (!gameOver) {
        Draw();
        Input();
        Logic();
        Sleep(speed);
    }
    SaveHighScore();
    ShowGameOver();
    return 0;
}
