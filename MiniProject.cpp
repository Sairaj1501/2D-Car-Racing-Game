#include <graphics.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib> 
#include <ctime>   

using namespace std;

// --- Global Variables and Constants ---
int laneOffset = 0;
int laneSpeed = 15;
const int MIN_SPEED = 5;
const int MAX_SPEED = 35;
const int SPEED_CHANGE = 5;
// Constant for automatic speed up
const int SPEED_UP_INTERVAL = 2000; 

// Line drawing constants
const int DASH_HEIGHT = 20;
const int GAP_HEIGHT = 20;
const int SEGMENT_TOTAL = DASH_HEIGHT + GAP_HEIGHT;

// Obstacle constants
const int MAX_OBSTACLES = 5;
const int OBSTACLE_WIDTH = 50;
const int OBSTACLE_HEIGHT = 30;
// Spawn just above the screen
const int OBSTACLE_SPAWN_Y = -50; 

// Score and Game State
long score = 0;
int obstacle_hit = 0; 
long last_speed_up_score = 0; 

// Car dimensions (Global for the reset function)
int carWidth = 40;
int carHeight = 60;

// Car position (Global for the reset function)
int carX; 
int carY;

// Structure to define an obstacle
struct Obstacle {
    int x;
    int y;
    bool active;
};

Obstacle obstacles[MAX_OBSTACLES];

// --- Linux Terminal I/O Functions (Unchanged) ---

// Check if a key is pressed
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

// Get the pressed character
int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// --- Game Logic Functions (Unchanged) ---

bool checkCollision(int carX_center, int carY_bottom, int carW, int carH, int obsX_center, int obsY_top, int obsW, int obsH) {
    int carLeft = carX_center - carW/2;
    int carRight = carX_center + carW/2;
    int carTop = carY_bottom - carH;
    int carBottom = carY_bottom;
    int obsLeft = obsX_center - obsW/2;
    int obsRight = obsX_center + obsW/2;
    int obsTop = obsY_top;
    int obsBottom = obsY_top + obsH;
    return (carLeft < obsRight && carRight > obsLeft && 
            carTop < obsBottom && carBottom > obsTop);
}


void initializeObstacles(int roadLeft, int roadRight) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        obstacles[i].active = false;
        obstacles[i].y = 0;
        obstacles[i].x = roadLeft + OBSTACLE_WIDTH/2 + (rand() % (roadRight - roadLeft - OBSTACLE_WIDTH));
    }
}

void resetGame(int initCarX, int initCarY, int roadLeft, int roadRight) {
    score = 0;
    obstacle_hit = 0;
    laneSpeed = 15; 
    last_speed_up_score = 0; 
    laneOffset = 0;
    carX = initCarX;
    carY = initCarY;
    initializeObstacles(roadLeft, roadRight); 
}


// --- Main Game Loop ---

int main() {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, NULL);

    int maxX = getmaxx();
    int maxY = getmaxy();

    int roadLeft = maxX / 3;
    int roadRight = 2 * maxX / 3;

    carX = maxX / 2;
    carY = maxY - carHeight - 30;
    
    const int initCarX = carX;
    const int initCarY = carY;

    // *** Increased for faster side movement when key is held ***
    int sideSpeed = 25; 
    
    bool exitGame = false;
    
    srand(time(NULL));
    initializeObstacles(roadLeft, roadRight);

    while (!exitGame) {
        
        if (obstacle_hit == 0) { // GAME IS RUNNING
            cleardevice();
            
            // 1. SCROLLING and SCORING LOGIC
            laneOffset = (laneOffset + laneSpeed) % SEGMENT_TOTAL;
            score += laneSpeed / 10; 
            
            // AUTOMATIC SPEED INCREASE LOGIC
            if (score >= last_speed_up_score + SPEED_UP_INTERVAL && laneSpeed < MAX_SPEED) {
                laneSpeed += 1; 
                last_speed_up_score = score; 
            }

            // Draw Road
            setcolor(DARKGRAY);
            bar(roadLeft, 0, roadRight, maxY);
            setcolor(WHITE);
            for (int y = -SEGMENT_TOTAL + laneOffset; y < maxY; y += SEGMENT_TOTAL) {
                line(maxX / 2, y, maxX / 2, y + DASH_HEIGHT);
            }

            // 2. OBSTACLE LOGIC (Unchanged)
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                if (obstacles[i].active) {
                    obstacles[i].y += laneSpeed;
                    setcolor(BLACK);
                    bar(obstacles[i].x - OBSTACLE_WIDTH/2, obstacles[i].y, 
                        obstacles[i].x + OBSTACLE_WIDTH/2, obstacles[i].y + OBSTACLE_HEIGHT);

                    if (obstacles[i].y > maxY) {
                        obstacles[i].active = false;
                    }

                    // 3. COLLISION DETECTION
                    if (checkCollision(carX, carY, carWidth, carHeight, 
                                       obstacles[i].x, obstacles[i].y, 
                                       OBSTACLE_WIDTH, OBSTACLE_HEIGHT)) {
                        obstacle_hit = 1; 
                        break;
                    }
                } else {
                    // Spawning Logic
                    if (rand() % 500 < (10 + laneSpeed)) { 
                        bool canSpawn = true;
                        for (int j = 0; j < MAX_OBSTACLES; ++j) {
                            if (obstacles[j].active && obstacles[j].y < OBSTACLE_HEIGHT * 3) {
                                canSpawn = false;
                                break;
                            }
                        }

                        if (canSpawn) {
                            obstacles[i].active = true;
                            obstacles[i].y = OBSTACLE_SPAWN_Y; 
                            obstacles[i].x = roadLeft + OBSTACLE_WIDTH/2 + (rand() % (roadRight - roadLeft - OBSTACLE_WIDTH));
                        }
                    }
                }
            }
            
            // Draw car (red rectangle)
            setcolor(RED);
            bar(carX - carWidth/2, carY - carHeight, carX + carWidth/2, carY);

            // Display Score and Speed (Unchanged)
            setcolor(YELLOW);
            char score_str[50];
            sprintf(score_str, "SCORE: %ld", score);
            outtextxy(maxX - 150, 10, score_str);
            
            char speed_str[50];
            sprintf(speed_str, "SPEED: %d", laneSpeed);
            outtextxy(maxX - 150, 30, speed_str);
            
            // Handle WASD user input
            if (kbhit()) {
                char ch = getch();
                
                if (ch == 27) { // ESC key to exit
                    exitGame = true;
                } 
                else if (ch == 'a' || ch == 'A') { 
                    carX -= sideSpeed;
                    if (carX - carWidth/2 < roadLeft) carX = roadLeft + carWidth/2;
                } 
                else if (ch == 'd' || ch == 'D') { 
                    carX += sideSpeed;
                    if (carX + carWidth/2 > roadRight) carX = roadRight - carWidth/2;
                }
                else if (ch == 'w' || ch == 'W') { 
                    laneSpeed += SPEED_CHANGE;
                    if (laneSpeed > MAX_SPEED) laneSpeed = MAX_SPEED;
                }
                else if (ch == 's' || ch == 'S') { 
                    laneSpeed -= SPEED_CHANGE;
                    if (laneSpeed < MIN_SPEED) laneSpeed = MIN_SPEED;
                }
            }

            delay(50); 
            
        } else { // GAME OVER STATE (Unchanged)
            
            cleardevice();
            setcolor(RED);
            outtextxy(maxX/2 - 120, maxY/2 - 50, (char*)"GAME OVER"); 
            setcolor(YELLOW);
            char final_score_str[100];
            sprintf(final_score_str, "FINAL SCORE: %ld", score);
            outtextxy(maxX/2 - 100, maxY/2 + 20, final_score_str);
            setcolor(WHITE);
            outtextxy(maxX/2 - 170, maxY/2 + 70, (char*)"Press R to Restart or ESC to Quit"); 
            
            if (kbhit()) {
                char ch = getch();
                if (ch == 27) {
                    exitGame = true;
                } else if (ch == 'r' || ch == 'R') { 
                    resetGame(initCarX, initCarY, roadLeft, roadRight); 
                }
            }
            delay(50); 
        }
    }

    closegraph();
    return 0;
}
