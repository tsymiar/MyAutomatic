#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static int map[9][9];
static int puzzle[9][9]; // Original puzzle

// Check if num can be placed at (row, col)
int placeable(int row, int col, int num)
{
    for (int i = 0; i < 9; ++i) {
        if (map[row][i] == num || map[i][col] == num)
            return 0;
    }
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (map[startRow + i][startCol + j] == num)
                return 0;
        }
    }
    return 1;
}

// Backtracking solver
int backtrace(int count)
{
    if (count == 81) {
        printf("Result:\n");
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                printf("%d ", map[i][j]);
            }
            printf("\n");
        }
        return 1;
    }
    int row = count / 9;
    int col = count % 9;
    if (map[row][col] == 0) {
        for (int num = 1; num <= 9; ++num) {
            if (placeable(row, col, num)) {
                map[row][col] = num;
                if (backtrace(count + 1))
                    return 1;
                map[row][col] = 0;
            }
        }
    } else {
        if (backtrace(count + 1))
            return 1;
    }
    return 0;
}

// Shuffle array of 9 elements
void shuffle(int* arr, int n)
{
    for (int i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

// Fill diagonal 3x3 boxes
void fill_diagonal_boxes()
{
    for (int k = 0; k < 3; ++k) {
        int nums[9];
        for (int i = 0; i < 9; ++i) nums[i] = i + 1;
        shuffle(nums, 9);
        int row = k * 3, col = k * 3, idx = 0;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                map[row + i][col + j] = nums[idx++];
    }
}

// Generate a complete Sudoku solution
int fill_remaining(int count)
{
    if (count == 81) return 1;
    int row = count / 9, col = count % 9;
    if (map[row][col] != 0) return fill_remaining(count + 1);

    int nums[9];
    for (int i = 0; i < 9; ++i) nums[i] = i + 1;
    shuffle(nums, 9);
    for (int i = 0; i < 9; ++i) {
        if (placeable(row, col, nums[i])) {
            map[row][col] = nums[i];
            if (fill_remaining(count + 1)) return 1;
            map[row][col] = 0;
        }
    }
    return 0;
}

// Remove cells to create puzzle
void remove_cells(int blanks)
{
    int removed = 0;
    while (removed < blanks) {
        int i = rand() % 9;
        int j = rand() % 9;
        if (map[i][j] != 0) {
            map[i][j] = 0;
            ++removed;
        }
    }
}

// Print puzzle
void show_puzzle(int puzzle[9][9])
{
    printf("    ");
    for (int j = 0; j < 9; ++j)
        printf(" %2d", j + 1);
    printf("\n");
    printf("   +----------------------------+\n");
    for (int i = 0; i < 9; ++i) {
        printf("%2d |", i + 1);
        for (int j = 0; j < 9; ++j) {
            if (puzzle[i][j] == 0)
                printf("  .");
            else
                printf(" %2d", puzzle[i][j]);
        }
        printf(" |\n");
    }
    printf("   +----------------------------+\n");
}

// Print current state, highlight empty cells
void show_map(int puzzle[9][9], int map[9][9])
{
    printf("    ");
    for (int j = 0; j < 9; ++j)
        printf(" %2d", j + 1);
    printf("\n");
    printf("   +----------------------------+\n");
    for (int i = 0; i < 9; ++i) {
        printf("%2d |", i + 1);
        for (int j = 0; j < 9; ++j) {
            if (puzzle[i][j] == 0) {
                if (map[i][j] == 0)
                    printf("  .");
                else
                    printf(" \033[1;32;4m%2d\033[0m", map[i][j]); // Highlight filled cell
            } else {
                printf(" %2d", puzzle[i][j]);
            }
        }
        printf(" |\n");
    }
    printf("   +----------------------------+\n");
}

void sudoku()
{
    srand((unsigned int)time(NULL));
    // Clear map
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            map[i][j] = 0;

    fill_diagonal_boxes();
    fill_remaining(0);

    // Remove cells (e.g., 40 blanks)
    remove_cells(40);

    // Save puzzle
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            puzzle[i][j] = map[i][j];

    printf("Random Sudoku puzzle:\n");
    show_puzzle(puzzle);

    // Interactive input
    char cmd[32];
    int row, col, val;
    while (1) {
        printf("Enter \033[1mrow col val\033[0m (1-9) to fill, or 'g'(giveup) to show answer, or 'q'(quit) to exit:\n");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        if (cmd[0] == 'g' || strncmp(cmd, "giveup", 6) == 0) {
            // Show answer
            for (int i = 0; i < 9; ++i)
                for (int j = 0; j < 9; ++j)
                    map[i][j] = puzzle[i][j];
            backtrace(0);
            break;
        }
        if (cmd[0] == 'q' || strncmp(cmd, "quit", 4) == 0) {
            break;
        }
        if (sscanf(cmd, "%d %d %d", &row, &col, &val) == 3) {
            if (row < 1 || row > 9 || col < 1 || col > 9 || val < 1 || val > 9) {
                printf("Please enter numbers between [1, 9].\n");
                continue;
            }
            if (puzzle[row - 1][col - 1] != 0) {
                printf("This cell is Not puzzle.\n");
                continue;
            }
            map[row - 1][col - 1] = val;
            show_map(puzzle, map);
        } else {
            printf("Command invalid!\n");
        }
    }
}
