/*
Multiplication Game
A simple multiplication game where two players (one human and one computer)
take turns selecting numbers.The goal is to form a line of four in a row
(horizontally, vertically, or diagonally).
The game uses the ncurses library for terminal graphics.

    Name : Abrar Hossain Zahin
    ID: 2022-2-60-040 Roll: 25

    Compile with:  gcc -o Multiplication Multiplication.c -lncurses
    Run with:      ./Multiplication
*/

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>

#define GRID_SIZE 6
#define WIN_COUNT 4
#define EMPTY 0
#define PLAYER1 1
#define PLAYER2 2

int board[GRID_SIZE][GRID_SIZE] = {
    {1, 2, 3, 4, 5, 6},
    {7, 8, 9, 10, 12, 14},
    {15, 16, 18, 20, 21, 24},
    {25, 27, 28, 30, 32, 35},
    {36, 40, 42, 45, 48, 49},
    {54, 56, 63, 64, 72, 81}};

typedef struct
{
    int wins;
    int moves;
    int prev_top;
    int prev_bottom;
} Player;
// Player structure to hold wins, moves, and previous selectors
Player player1 = {0, 0, 1, 1};
Player player2 = {0, 0, 1, 1};

// RegisterSet structure to hold the state of the game
typedef struct
{
    int A;    // Accumulator or operand 1
    int B;    // Operand 2
    int RES;  // Result
    int FLAG; // For conditions like zero, win detection, etc.
} RegisterSet;

RegisterSet reg;

// Global variables
int marks[GRID_SIZE][GRID_SIZE];
int top_selector = 1, bottom_selector = 1;
int tempTop = 0, tempBottm = 0;
int starting = 1;
void draw_board(int current_player);
void init_game();
void handle_input(int *current_player, int *game_over);
void computer_move(int *game_over);
int heuristic_score(int x, int y, int player);
int is_valid_cell(int val);
void load_stats();
void show_stats();
int prompt_retry();
void make_move(int val, int player);
int check_winner(int player);
int check_draw();
void animate_computer_choice(int top, int bottom, int current_player);
void save_stats();

int main()
{
    initscr();
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    load_stats();

    while (1)
    {
        init_game();
        int current_player = PLAYER1;
        int game_over = 0;

        while (!game_over)
        {
            clear();
            draw_board(current_player);
            show_stats();

            if (check_draw())
            {
                attron(COLOR_PAIR(3));
                mvprintw(GRID_SIZE * 2 + 12, 0, "Game is a draw! Press any key...");
                getch();
                break;
            }

            if (current_player == PLAYER1)
            {
                handle_input(&current_player, &game_over);
            }
            else
            {
                computer_move(&game_over);
                current_player = PLAYER1;
            }
        }

        save_stats();
        clear();
        draw_board(game_over);
        show_stats();
        if (game_over == PLAYER1)
        {
            attron(COLOR_PAIR(1));
            mvprintw(16, 14, "Player 1 wins!");
        }
        else if (game_over == PLAYER2)
        {
            attron(COLOR_PAIR(2));
            mvprintw(16, 14, "Computer wins!");
        }
        else
        {
            attron(COLOR_PAIR(3));
            mvprintw(16, 14, "Match Cancled!");
        }

        int retry = prompt_retry();
        if (!retry)
            break;
    }

    endwin();
    return 0;
}

void save_stats()
{
    FILE *f = fopen("stats.txt", "w");
    if (f)
    {
        fprintf(f, "%d %d %d %d\n", player1.wins, player2.wins, player1.moves, player2.moves);
        fclose(f);
    }
}

void load_stats()
{
    FILE *f = fopen("stats.txt", "r");
    if (f)
    {
        fscanf(f, "%d %d %d %d", &player1.wins, &player2.wins, &player1.moves, &player2.moves);
        fclose(f);
    }
}

void show_stats()
{
    attron(COLOR_PAIR(3));
    mvprintw(17, 17, "Stats:");

    attron(COLOR_PAIR(1));
    mvprintw(18, 11, "Player 1 - Wins: %d", player1.wins);

    attron(COLOR_PAIR(2));
    mvprintw(19, 11, "Computer - Wins: %d", player2.wins);
}

int prompt_retry()
{
    attron(COLOR_PAIR(3));
    mvprintw(20, 5, "Press 'R' to Retry or ESC to Exit.");
    while (1)
    {
        int ch = getch();
        if (ch == 27)
            return 0;
        if (ch == 'r' || ch == 'R')
            return 1;
    }
}

void init_game()
{
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            marks[i][j] = EMPTY;

    top_selector = 1;
    bottom_selector = 1;
    tempTop = 1;
    tempBottm = 1;

    srand(time(NULL));

    // NEW: Animate to random top selector
    int random_top = (rand() % 9) + 1; // Random between 1 and 9
    while (top_selector != random_top)
    {
        if (top_selector < random_top)
            top_selector++;
        else
            top_selector--;
        clear();
        draw_board(PLAYER1);
        show_stats();
        refresh();
        usleep(150000); // 150ms delay for animation
    }

    // Save as P1's initial selector

    usleep(200000);
    starting = 0;
    player1.prev_top = top_selector;
}

void draw_board(int current_player)
{
    // Show controls at the top (row 0)
    attron(COLOR_PAIR(4));
    mvprintw(0, 0, "***************** MULTIPLICATION GAME ****************");

    attron(COLOR_PAIR(3));
    mvprintw(1, 0, "<- / ->: Select Top Option | A/D: Select Bottom Option \nEnter: Confirm         | R: Retry |         ESC: Exit");

    // Player info (row 1)
    attron(COLOR_PAIR(1));
    mvprintw(2, 0, "Your Move, Player 1: [P1]           ");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2));
    mvprintw(2, 25, "        Computer's Move: [CP]");
    attroff(COLOR_PAIR(2));

    // Turn indicator (row 3)

    if (starting == 1)
    {
        attron(COLOR_PAIR(3));
        mvprintw(4, 0, "            Randomizing!                         ");
        attroff(COLOR_PAIR(3));
    }
    else if (current_player == PLAYER1)
    {
        attron(COLOR_PAIR(1));
        mvprintw(4, 0, "            Player 1's Turn                      ");
        attroff(COLOR_PAIR(1));
    }
    else if (current_player == PLAYER2)
    {
        attron(COLOR_PAIR(2));
        mvprintw(4, 0, "            Computer's Turn                      ");
        attroff(COLOR_PAIR(2));
    }

    // Top selector (row 4)
    mvprintw(5, 0, "   ");
    for (int i = 1; i <= 9; i++)
    {
        if (i == top_selector)
        {
            attron(COLOR_PAIR(3));
            printw("  | ");
            attroff(COLOR_PAIR(3));
        }
        else
        {
            printw("    ");
        }
    }

    // Column numbers (row 5)
    mvprintw(6, 0, "   ");
    for (int i = 1; i <= 9; i++)
    {
        printw(" %2d ", i);
    }

    // **Bottom selector - Kept at row 6**
    mvprintw(7, 0, "   ");
    for (int i = 1; i <= 9; i++)
    {
        if (i == bottom_selector)
        {
            attron(COLOR_PAIR(3));
            printw("  | ");
            attroff(COLOR_PAIR(3));
        }
        else
        {
            printw("    ");
        }
    }

    // Game grid (starting row 8)
    for (int i = 0; i < GRID_SIZE; i++)
    {
        mvprintw(9 + i, 8, ""); // Grid now starts from row 8 (below the selectors)
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (marks[i][j] == PLAYER1)
            {
                attron(COLOR_PAIR(1));
                printw("[P1]");
                attroff(COLOR_PAIR(1));
            }
            else if (marks[i][j] == PLAYER2)
            {
                attron(COLOR_PAIR(2));
                printw("[CP]");
                attroff(COLOR_PAIR(2));
            }
            else
            {
                printw(" %2d ", board[i][j]);
            }
        }
    }

    // Draw a box around the game grid
    int box_top = 8;                   // Box starts at row 7 (just above the grid)
    int box_left = 7;                  // Box starts 8 columns to the right
    int box_height = GRID_SIZE + 2;    // One row above and below the grid
    int box_width = GRID_SIZE * 4 + 2; // Each cell is 4 chars + left/right padding

    // Top border
    mvaddch(box_top, box_left, ACS_ULCORNER);
    for (int i = 1; i < box_width - 1; i++)
        addch(ACS_HLINE);
    addch(ACS_URCORNER);

    // Middle vertical borders
    for (int i = 1; i < box_height - 1; i++)
    {
        mvaddch(box_top + i, box_left, ACS_VLINE);
        mvaddch(box_top + i, box_left + box_width - 1, ACS_VLINE);
    }

    // Bottom border
    mvaddch(box_top + box_height - 1, box_left, ACS_LLCORNER);
    for (int i = 1; i < box_width - 1; i++)
        addch(ACS_HLINE);
    addch(ACS_LRCORNER);
}
void handle_input(int *current_player, int *game_over)
{
    int ch;
    int selector_changed = 0; // 0 = none, 1 = top changed, 2 = bottom changed

    // Start from previous positions
    // Start from the last positions used by the computer
    top_selector = player2.prev_top;
    bottom_selector = player2.prev_bottom;

    while (1)
    {
        ch = getch();

        if (ch == KEY_LEFT || ch == KEY_RIGHT)
        {
            if (selector_changed == 2)
            {
                bottom_selector = player1.prev_bottom;
            }
            selector_changed = 1;
            if (ch == KEY_LEFT)
                top_selector = (top_selector == 1) ? 9 : top_selector - 1;
            else
                top_selector = (top_selector == 9) ? 1 : top_selector + 1;
        }
        else if (ch == 'a' || ch == 'd')
        {
            if (selector_changed == 1)
            {
                top_selector = player1.prev_top;
            }
            selector_changed = 2;
            if (ch == 'a')
                bottom_selector = (bottom_selector == 1) ? 9 : bottom_selector - 1;
            else
                bottom_selector = (bottom_selector == 9) ? 1 : bottom_selector + 1;
        }
        else if (ch == '\n')
        {
            int prod = top_selector * bottom_selector;
            if (is_valid_cell(prod))
            {
                make_move(prod, *current_player);
                player1.moves++;

                player1.prev_top = top_selector;
                player1.prev_bottom = bottom_selector;

                if (check_winner(*current_player))
                {
                    *game_over = PLAYER1;
                    player1.wins++;
                }

                *current_player = PLAYER2;
                break;
            }
        }
        else if (ch == 27)
        {
            endwin();
            exit(0);
        }
        else if (ch == 'r')
        {
            *game_over = -1;
            break;
        }

        clear();
        draw_board(*current_player);
        show_stats();
    }
}

int is_valid_cell(int val)
{
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            if (board[i][j] == val && marks[i][j] == EMPTY)
                return 1;
    return 0;
}
void make_move(int val, int player)
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {

            reg.A = board[i][j];
            reg.B = val;

            if (reg.A == reg.B && marks[i][j] == EMPTY)
            {
                reg.RES = player;
                marks[i][j] = reg.RES;
                return;
            }
        }
    }
}

void animate_computer_choice(int top, int bottom, int current_player)
{
    while (top_selector != top || bottom_selector != bottom)
    {
        if (top_selector < top)
            top_selector++;
        else if (top_selector > top)
            top_selector--;

        if (bottom_selector < bottom)
            bottom_selector++;
        else if (bottom_selector > bottom)
            bottom_selector--;

        clear(); // Ensure screen is fully redrawn
        draw_board(current_player);
        show_stats();
        refresh();
        usleep(150000); // 150ms = 0.15 second pause
    }

    // Final frame
    clear();
    draw_board(current_player);
    show_stats();
    refresh();
    usleep(200000); // Optional: Hold the final position for a moment
}

void computer_move(int *game_over)
{
    int best_score = INT_MIN, best_val = -1;
    int best_top = top_selector, best_bottom = bottom_selector;

    for (int a = 1; a <= 9; a++)
    {
        int prod = a * bottom_selector;
        if (!is_valid_cell(prod))
            continue;
        for (int i = 0; i < GRID_SIZE; i++)
            for (int j = 0; j < GRID_SIZE; j++)
                if (board[i][j] == prod && marks[i][j] == EMPTY)
                {
                    int score = heuristic_score(i, j, PLAYER2);
                    if (score > best_score)
                    {
                        best_score = score;
                        best_val = prod;
                        best_top = a;
                        best_bottom = bottom_selector;
                    }
                }
    }

    for (int b = 1; b <= 9; b++)
    {
        int prod = top_selector * b;
        if (!is_valid_cell(prod))
            continue;
        for (int i = 0; i < GRID_SIZE; i++)
            for (int j = 0; j < GRID_SIZE; j++)
                if (board[i][j] == prod && marks[i][j] == EMPTY)
                {
                    int score = heuristic_score(i, j, PLAYER2);
                    if (score > best_score)
                    {
                        best_score = score;
                        best_val = prod;
                        best_top = top_selector;
                        best_bottom = b;
                    }
                }
    }

    if (best_val != -1)
    {
        animate_computer_choice(best_top, best_bottom, PLAYER2);
        make_move(best_val, PLAYER2);
        player2.moves++;

        // Save selector state for Computer
        player2.prev_top = best_top;
        player2.prev_bottom = best_bottom;

        top_selector = best_top;
        bottom_selector = best_bottom;

        if (check_winner(PLAYER2))
        {
            *game_over = PLAYER2;
            player2.wins++;
        }
    }
}

int check_draw()
{
    for (int i = 1; i <= 9; i++)
        for (int j = 1; j <= 9; j++)
            if (is_valid_cell(i * j))
                return 0;
    return 1;
}
int check_winner(int player)
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {

            // Horizontal scan
            if (j <= GRID_SIZE - WIN_COUNT)
            {
                reg.FLAG = 1;
                for (int k = 0; k < WIN_COUNT; k++)
                {
                    reg.A = marks[i][j + k];
                    if (reg.A != player)
                        reg.FLAG = 0;
                }
                if (reg.FLAG)
                    return 1;
            }

            // Vertical scan
            if (i <= GRID_SIZE - WIN_COUNT)
            {
                reg.FLAG = 1;
                for (int k = 0; k < WIN_COUNT; k++)
                {
                    reg.A = marks[i + k][j];
                    if (reg.A != player)
                        reg.FLAG = 0;
                }
                if (reg.FLAG)
                    return 1;
            }

            // Diagonal scan (top-left to bottom-right)
            if (i <= GRID_SIZE - WIN_COUNT && j <= GRID_SIZE - WIN_COUNT)
            {
                reg.FLAG = 1;
                for (int k = 0; k < WIN_COUNT; k++)
                {
                    reg.A = marks[i + k][j + k];
                    if (reg.A != player)
                        reg.FLAG = 0;
                }
                if (reg.FLAG)
                    return 1;
            }

            // Diagonal scan (top-right to bottom-left)
            if (i <= GRID_SIZE - WIN_COUNT && j >= WIN_COUNT - 1)
            {
                reg.FLAG = 1;
                for (int k = 0; k < WIN_COUNT; k++)
                {
                    reg.A = marks[i + k][j - k];
                    if (reg.A != player)
                        reg.FLAG = 0;
                }
                if (reg.FLAG)
                    return 1;
            }
        }
    }
    return 0;
}

int heuristic_score(int x, int y, int player)
{
    int score = 0;
    int opponent = (player == PLAYER1) ? PLAYER2 : PLAYER1;

    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0)
                continue;

            int player_count = 0, opponent_count = 0;

            for (int k = 1; k < WIN_COUNT; k++)
            {
                int nx = x + dx * k;
                int ny = y + dy * k;

                if (nx >= 0 && ny >= 0 && nx < GRID_SIZE && ny < GRID_SIZE)
                {
                    if (marks[nx][ny] == player)
                        player_count++;
                    else if (marks[nx][ny] == opponent)
                        opponent_count++;
                }
            }

            // Prioritize extending own line and blocking opponent
            if (opponent_count == 0)
                score += (1 << player_count); // exponential reward
            if (player_count == 0)
                score += (1 << opponent_count); // defensive reward
        }
    }

    return score;
}