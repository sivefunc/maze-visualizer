// TODO: Have better documentation.
//       current documentation is not great.

// Libraries needed, here's a quick summary:
#include <stdbool.h>    // bool, true and false macros.
#include <stdlib.h>     // malloc, free, rand and srand.
#include <inttypes.h>   // intN_t and uintN_t.
#include <errno.h>      // global error variable "errno" and error macros.
#include <time.h>       // time(NULL) as a seed for generating "randomness".
#include <argp.h>       // parsing of arguments on command line.
#include "SDL.h"        // graphics.

// Concatenate string with number, e.g "Pi is: " STR(3.14159)
// on preprocessing.
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Default argument values for the options on command line.
#define DEFAULT_FPS 60
#define DEFAULT_FULLSCREEN false
#define DEFAULT_SCREEN_WIDTH 640
#define DEFAULT_SCREEN_HEIGHT 480
#define DEFAULT_MAZE_ROWS 63
#define DEFAULT_MAZE_COLUMNS 63
#define DEFAULT_SHOW_BODY true
#define DEFAULT_SHOW_DEAD_HEAD true
// End of default values for options

// SDL poll events
#define USER_QUIT_EVENT 0
#define USER_PAUSE_EVENT 1
#define USER_UNKNOWN_EVENT 2
// End of SDL poll events

// Enum declaration
enum SHORT_OPTION_KEYCODES
{
    OPTION_SCREEN_WIDTH=777,
    OPTION_SCREEN_HEIGHT,
    OPTION_FULLSCREEN,
    OPTION_SHOW_BODY,
    OPTION_SHOW_FULLSCREEN,
    OPTION_SHOW_DEAD_HEAD,
};

enum MAZE_LEGEND
{
    EMPTY,
    WALL,
    BODY,
    DEAD_HEAD,
    LIVE_HEAD,
    START,
    END,
    WIN_BLOCK,
    VISITED     // Visited is only used on generating the maze
                // recursive_backtracker()
};

enum MAZE_MOVES {NONE, LEFT, RIGHT, DOWN, UP};
enum MOVE_STATES {NODE_END_REACHED, NODE_MOVED, NODE_CANT_MOVE};

// Typedef struct declaration
typedef struct Cell
{
    enum MAZE_LEGEND type;
    int32_t distance_runned;
} Cell;

typedef struct Maze
{
    Cell **matrix;
    int16_t rows;
    int16_t columns;
    int16_t start_x, start_y;
    int16_t end_x, end_y;

} Maze;

typedef struct Tree
{
    int16_t parent_move;        /* Parent move that got u here */
    int16_t head_x;
    int16_t head_y;
    struct Tree *parent;
    int16_t parent_index;       // Idx of tree on the parent children array
    int16_t children_count;     /* On a maze it would be 4 available moves */
    int32_t distance_runned;
    struct Tree **children;
} Tree;

typedef struct Arguments
{
    int16_t fps;
    int screen_width;
    int screen_height;
    uint16_t maze_rows;
    uint16_t maze_columns;
    bool fullscreen;
    bool show_body;
    bool show_dead_head;
} Arguments;

// Global variables used by argp.h
// General message containing prog_name, author, license and year.
const char *argp_program_version =
"maze-visualizer v1.0.0\n"
"Copyright (C) 2024 Sivefunc\n"
"License GPLv3+: GNU GPL version 3 or later"
    "<https://gnu.org/licenses/gpl.html>\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"\n"
"Written by a human\n";

// CLI options.
static struct argp_option options[] =
{
    // Quick summary
    {
        "help",                         // Long name of option
        'h',                            // Short name of option
        0,                              // Name of the arg, e.g NUM.
        0,                              // Flags
        "display this help and exit.",  // Doc. of option useful for help.
        -1                              // Order of appearance in help
                                        // -1 means last, pos. value get first.
    },

    {"version", 'v', 0, 0, "output version information and exit.", -1},

    {"fps", 'f', "NUM", 0,
        "frames per second: " STR(DEFAULT_FPS) " by default.", 1},
    
    {"fullscreen", OPTION_FULLSCREEN, "BOOL[0 or 1]", 0,
        "occupies all screen: " STR(DEFAULT_FULLSCREEN) " by default.", 1},

    {"screen_width", OPTION_SCREEN_WIDTH, "PIXELS", 0,
        "screen width: " STR(DEFAULT_SCREEN_WIDTH) " by default.", 2},

    {"screen_height", OPTION_SCREEN_HEIGHT, "PIXELS", 0,
        "screen height: " STR(DEFAULT_SCREEN_HEIGHT) " by default.", 2},

    {"maze_columns", 'c', "NUM", 0,
        "maze columns: " STR(DEFAULT_MAZE_COLUMNS) " by default.", 3},

    {"maze_rows", 'r', "NUM", 0,
        "maze rows: " STR(DEFAULT_MAZE_ROWS) " by default.", 3},

    {"show_body", OPTION_SHOW_BODY, "BOOL[0 or 1]", 0,
        "show all the trail that nodes have walked: "
            STR(DEFAULT_SHOW_BODY) " by default.", 4},

    {"show_dead_head", OPTION_SHOW_DEAD_HEAD, "BOOL[0 or 1]", 0,
        "show head that can't move: " STR(DEFAULT_SHOW_DEAD_HEAD)
            " by default.", 5},

    {0}
};

// Prototypes
// Tree related
Tree * create_node(void);
bool find_path(
        SDL_Window *window, SDL_Renderer *renderer,
        Tree *root, Maze *maze,
        const Arguments *args);

enum MOVE_STATES move_node(Tree *node, Maze *maze);
Tree * find_left_leaf(Tree * node);
Tree * find_next_leaf(Tree *node);
void nodes_info(
        const Maze *maze,
        int32_t *total, int32_t *live, int32_t *dead,
        int32_t *distance_runned);


// Graphics
void hsl_to_rgb(
        double hue, double saturation, double lightness,
        int *r, int *g, int *b);

void draw_maze(
        SDL_Window *window,
        SDL_Renderer *renderer,
        const Maze *maze,
        const Arguments *args);

// Maze generation
int SO_random(int min, int max);
Maze * recursive_backtracker(int16_t rows, int16_t columns);

// Getting user input from terminal and keyboard.
static error_t parse_opt(int32_t key, char *arg, struct argp_state *state);
uint8_t get_key(void);

/////////////////
// Entry point //
/////////////////
int32_t main(int32_t argc, char *argv[])
{
    struct argp argp =
    {
        options,
        parse_opt,
        0,
        "2D Maze solving Visualizer\n\v"
        "Written by Sivefunc",
        0,
        0,
        0
    };

    Arguments args =
    {
        .fps = DEFAULT_FPS,
        .fullscreen = DEFAULT_FULLSCREEN,
        .screen_width = DEFAULT_SCREEN_WIDTH,
        .screen_height = DEFAULT_SCREEN_HEIGHT,
        .maze_rows = DEFAULT_MAZE_ROWS,
        .maze_columns = DEFAULT_MAZE_COLUMNS,
        .show_body = DEFAULT_SHOW_BODY,
        .show_dead_head = DEFAULT_SHOW_DEAD_HEAD,
    };

    // Succesfull parsing
    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &args) == 0)
    {
        long seed = time(NULL);
        printf("Seed: %lu\n", seed);
        srand(seed);


        // Unsuccessfull creation of video.
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            return 1;
        }

        SDL_Window *window = NULL;
        SDL_Renderer *renderer = NULL;
        
        // Unsuccessfull creation of window and renderer.
        if (SDL_CreateWindowAndRenderer(
                    args.screen_width, args.screen_height,
                    (args.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
                    SDL_WINDOW_RESIZABLE,
                    &window, &renderer) < 0)
        {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
            return 1;
        }

        Tree * root = create_node();
        if (root == NULL)
        {
            perror("Couldn't crate initial root\n");
            return 1;
        }
        
        Maze *maze = recursive_backtracker(args.maze_rows, args.maze_columns);
        root -> head_x = maze -> start_x;
        root -> head_y = maze -> start_y;

        bool result = find_path(window, renderer, root, maze, &args);
        int32_t total, live_head, dead_head, distance_runned;
        total = live_head = dead_head = distance_runned = 0;
        nodes_info(maze, &total, &live_head, &dead_head, &distance_runned);
        printf("%s\n", result ? "FOUND" : "NO FOUND");
        printf("Total nodes: %d\n"
            "Live head:   %d\n"
            "Dead head:   %d\n"
            "Distance:    %d\n",
            total, live_head, dead_head, distance_runned);

        
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_SUCCESS;
    }
}

Tree * create_node(void)
{
    Tree * node = malloc(sizeof(Tree));
    if (node == NULL)
    {
        perror("Failed to allocate memory for node\n");
        return NULL;
    }

    node -> parent = NULL;
    node -> children_count = 0;
    node -> children = NULL;
    node -> head_x = -1;
    node -> head_y = -1;
    node -> parent_index = -1;
    node -> parent_move = NONE;
    node -> distance_runned = 0;
    return node;
}

enum MOVE_STATES move_node(Tree *node, Maze *maze)
{

    enum MAZE_MOVES moves[] = {LEFT, UP, DOWN, RIGHT};
    size_t move_quantity = sizeof(moves) / sizeof(enum MAZE_MOVES);
    int16_t tx, ty;

    bool end_reached = false;
    maze -> matrix[node -> head_y][node -> head_x].type = DEAD_HEAD;

    for (size_t move = 0; move < move_quantity; move++)
    {
        // Checking for out of bounds
        if ((moves[move] == LEFT && node -> head_x <= 0) ||
            (moves[move] == RIGHT &&
                node -> head_x >= maze -> columns - 1) ||
            (moves[move] == UP && node -> head_y <= 0) ||
            (moves[move] == DOWN && node -> head_y >= maze -> rows - 1))
        {
            continue;
        }

        tx = node -> head_x + (moves[move] == LEFT ? -1 :
                                moves[move] == RIGHT ? 1 : 0);
        ty = node -> head_y + (moves[move] == UP ? -1 :
                                moves[move] == DOWN ? 1 : 0);

        // Can't move to a WALL or
        // A part where already moved to avoid infinite recursion.
        if (maze -> matrix[ty][tx].type != EMPTY &&
            !(ty == maze -> end_y && tx == maze -> end_x))
        {
            continue;
        }

        // Succesfull move
        // Initializing a new node
        node -> children_count += 1;
        node -> children = realloc(node -> children,
                node -> children_count * sizeof(Tree **));
        
        node -> children[node -> children_count - 1] = create_node();
        node -> children[node -> children_count - 1] -> parent_move = 
            moves[move];

        node -> children[node -> children_count - 1] -> head_x = tx;
        node -> children[node -> children_count - 1] -> head_y = ty;

        node -> children[node -> children_count - 1] -> parent = node;

        node -> children[node -> children_count - 1] -> parent_index =
            node -> children_count - 1;
        
        node -> children[node -> children_count - 1] -> distance_runned =
            node -> distance_runned + 1;

        maze -> matrix[node -> head_y][node -> head_x].type = BODY;
        maze -> matrix[ty][tx].type = LIVE_HEAD;
        maze -> matrix[ty][tx].distance_runned = node -> distance_runned + 1;
        
        if (ty == maze -> end_y && tx == maze -> end_x)
        {
            end_reached = true;
        }
    }
    
    return end_reached ? NODE_END_REACHED : node -> children_count > 0 ?
                NODE_MOVED : NODE_CANT_MOVE;
}

bool find_path(
        SDL_Window *window, SDL_Renderer *renderer,
        Tree *root, Maze *maze,
        const Arguments *args)
{
    // FPS calculation (on miliseconds)
    const int32_t ms_per_frame = 1000 / args -> fps;
    int32_t last_frame_time;
    int32_t delay;
    int32_t time_to_wait;
    uint8_t key;

    // FLAGS
    bool pause = false;
    bool running = true; 
    bool end_reached = false;
    bool atleast_one_node_moved = true;
    Tree *winner_node = NULL;

    draw_maze(window, renderer, maze, args);
    while (running)
    {
        last_frame_time = SDL_GetTicks();
        key = get_key();
        if (key == USER_QUIT_EVENT)
        {
            running = false;
        }

        else if (key == USER_PAUSE_EVENT)
        {
            pause = !(pause);
        }

        if (pause == false)
        {
            if (end_reached)
            {
                if (winner_node != NULL)
                {
                    maze -> matrix
                        [winner_node -> head_y][winner_node -> head_x].type =
                            WIN_BLOCK;
                    
                    winner_node = winner_node -> parent;
                }
            }

            else if (atleast_one_node_moved)
            {
                atleast_one_node_moved = false;
                Tree *node_to_mv = find_left_leaf(root);
                do
                {
                    enum MOVE_STATES result = move_node(node_to_mv, maze);
                    if (result == NODE_END_REACHED)
                    {
                        // Do not break yet, we want each node to move or be
                        // dead on each turn.
                        end_reached = true;
                        winner_node = node_to_mv;
                    }

                    if (result == NODE_MOVED)
                    {
                        atleast_one_node_moved = true;
                    }

                } while ((node_to_mv = find_next_leaf(node_to_mv)) != NULL);
            }
        }

        // It needs to be render even if it's paused due to window resizes.
        draw_maze(window, renderer, maze, args);

        delay = SDL_GetTicks() - last_frame_time;
        time_to_wait = ms_per_frame - delay;
        if (time_to_wait > 0 && time_to_wait <= ms_per_frame)
            SDL_Delay(time_to_wait);
    }

    return end_reached;
}

void draw_maze(
        SDL_Window *window,
        SDL_Renderer *renderer,
        const Maze *maze,
        const Arguments *args)
{
    int32_t window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
    SDL_RenderClear(renderer);
    int cell_lenght = fmin(window_width / maze -> columns,
                            window_height / maze -> rows);
    int padding_x = (window_width - cell_lenght * maze -> columns) / 2;
    int padding_y = (window_height - cell_lenght * maze -> rows) / 2;

    SDL_Rect square =
    {
        .x = padding_x,
        .y = padding_y,
        .w = cell_lenght,
        .h = cell_lenght,
    };

    int r, g, b;

    // The distance between the start node and end node can be actually bigger
    // or smaller due to 'S' shaped mazes, I decided it that using this number
    // for coloring the distance runned by a node is good.
    int16_t max_distance_runned = (maze -> rows * maze -> columns) / 5;

    for (int16_t row = 0; row < maze -> rows; row++)
    {
        for (int16_t column = 0; column < maze -> columns; column++)
        {
            r = 255, g = 255, b = 255;
            if (maze -> matrix[row][column].type == EMPTY)
            {
                r = 255, g = 255, b = 255;
            }

            // START of maze
            else if (maze -> start_x == column && maze -> start_y == row)
            {
                r = 255, g = 127, b = 127;
            }
            
            // END of maze
            else if (maze -> end_x == column && maze -> end_y == row)
            {
                r = 255, g = 0, b = 0;
            }

            else if (maze -> matrix[row][column].type == WALL)
            {
                r = 0, g = 0, b = 0;
            }

            else if (maze -> matrix[row][column].type == LIVE_HEAD)
            {
                r = 255, g = 128, b = 255;
            }

            else if (maze -> matrix[row][column].type == DEAD_HEAD &&
                    args -> show_dead_head)
            {
                r = 255, g = 0, b = 255;
            }

           else if (maze -> matrix[row][column].type == WIN_BLOCK)
            {
                r = 255, g = 255, b = 0;
            }

            else if(args -> show_body)
            {
                r = 0, g = 255, b = 0;
                if (0.2 + maze -> matrix[row][column].distance_runned /
                         (double)max_distance_runned <= 1.0)
                {
                    hsl_to_rgb(
                            120,
                            0.2 + maze -> matrix[row][column].distance_runned /
                                (double)(max_distance_runned),
                            0.5,
                            &r, &g, &b);
                }
            }

            else // No body show aka equal to EMPTY square.
            {
                r = 255, g = 255, b = 255;
            }

            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderFillRect(renderer, &square);
            square.x += square.w;
        }
        square.x = padding_x;
        square.y += square.h;
    }

    SDL_RenderPresent(renderer);
}

// From a given node finds the leftmost leaf.
Tree * find_left_leaf(Tree * node)
{
    while (node -> children_count != 0)
    {
        node = node -> children[0];
    }

    return node;
}

// Must be a leaf the node. (It works without being a leaf, but in the context
// of a maze solving I prefer it being leaf)
Tree * find_next_leaf(Tree *node)
{
    // A next leaf doesn't exist.
    if (node -> parent == NULL)
    {
        return NULL;
    }

    // Finding a parent node which is not the last child of his parent.
    // Does that makes sense?
    while(node -> parent_index == node -> parent -> children_count - 1)
    {
        node = node -> parent;
        if (node -> parent == NULL)
        {
            return NULL;
        }
    }

    return find_left_leaf(node -> parent -> children[node -> parent_index +1]);
}

void nodes_info(
        const Maze *maze,
        int32_t *total, int32_t *live_head, int32_t *dead_head,
        int32_t *distance_runned)
{
    for (int32_t row = 0; row < maze -> rows; row++)
    {
        for (int32_t column = 0; column < maze -> columns; column++)
        {
            Cell cell = maze -> matrix[row][column];
            if (cell.type == EMPTY || cell.type == WALL || cell.type == END)
            {
                continue;
            }

            *total += 1;
            if (cell.type == DEAD_HEAD)
            {
                *dead_head += 1;
            }

            else if(cell.type == LIVE_HEAD)
            {
                *live_head += 1;
            }

            if (cell.distance_runned >= *distance_runned)
            {
                *distance_runned = cell.distance_runned;
            }
        }
    }
}

Maze * recursive_backtracker(int16_t rows, int16_t columns)
{
    Maze * maze = (Maze *) malloc(sizeof(Maze));
    maze -> rows = rows - (rows % 2 == 0);
    maze -> columns = columns - (columns % 2 == 0);
    maze -> matrix = malloc(sizeof(Cell*) * maze -> rows);
    for (int16_t row = 0; row < maze -> rows; row++)
    {
        maze -> matrix[row] = malloc(sizeof(Cell) * maze -> columns);
    }
    
    for (int16_t row = 0; row < maze -> rows; row++)
    {
        for (int16_t column = 0; column < maze -> columns; column++)
        {
            maze -> matrix[row][column].type =
                row % 2 || column % 2 ? WALL : EMPTY;

            maze -> matrix[row][column].distance_runned = 0;
        }
    }

    // TODO: Optimize max_size formula, it's actually less.
    // X O X O X
    // O X O X O
    // X O X O X
    // I could also just use realloc.
    int32_t max_size = maze -> rows * maze -> columns;
    int32_t **backtrack = malloc(sizeof(int32_t*) * max_size);
    for (int32_t coord = 0; coord < max_size; coord++)
    {
        // Pair of coordinates [X, Y]
        backtrack[coord] = malloc(sizeof(int32_t) * 2);

        // Coord doesn't exist
        backtrack[coord][0] = -1;
        backtrack[coord][1] = -1;
    }

    int32_t backtrack_size = 1;
    backtrack[0][0] = 0; // X
    backtrack[0][1] = 0; // Y
    maze -> matrix[0][0].type = VISITED;

    // VALID and INVALID moves that a cell can do.
    enum MAZE_MOVES moves[] = {LEFT, UP, DOWN, RIGHT};
    size_t pos_move_quantity = sizeof(moves) / sizeof(enum MAZE_MOVES);
    
    // Moves that actually the user can do
    enum MAZE_MOVES valid_moves[] = {-1, -1, -1, -1};
    size_t valid_moves_count = 0;

    int32_t current_x;
    int32_t current_y;
    int32_t new_x;
    int32_t new_y;
    int32_t wall_x; // The wall is the block between current and new cell.
    int32_t wall_y;

    do
    {
        valid_moves_count = 0;
        current_x = backtrack[backtrack_size - 1][0];
        current_y = backtrack[backtrack_size - 1][1];

        for (size_t move = 0; move < pos_move_quantity; move++)
        {
            // Checking for out of bounds
            if ((moves[move] == LEFT && current_x <= 0) ||
                (moves[move] == RIGHT &&
                    current_x >= maze -> columns - 1) ||
                (moves[move] == UP && current_y <= 0) ||
                (moves[move] == DOWN && current_y >= maze -> rows - 1))
            {
                continue;
            }

            new_x = current_x + (moves[move] == LEFT ? -2 :
                                moves[move] == RIGHT ? 2 : 0);

            new_y = current_y + (moves[move] == UP ? -2 :
                                moves[move] == DOWN ? 2 : 0);

            if (maze -> matrix[new_y][new_x].type == EMPTY)
            {
                valid_moves[valid_moves_count] = moves[move];
                valid_moves_count++;
            }
        }

        if (valid_moves_count == 0)
        {
            backtrack_size--;
        }

        else
        {
            enum MAZE_MOVES mv = valid_moves[
                                    SO_random(0, valid_moves_count-1)];

            new_x = current_x + (mv == LEFT ? -2 : mv == RIGHT ? 2 : 0);
            new_y = current_y + (mv == UP ? -2 : mv == DOWN ? 2 : 0);

            backtrack[backtrack_size][0] = new_x;
            backtrack[backtrack_size][1] = new_y;
            maze -> matrix[new_y][new_x].type = VISITED;
            wall_x = current_x + (mv == LEFT ? -1 : mv == RIGHT ? 1 : 0);
            wall_y = current_y + (mv == UP ? -1 : mv == DOWN ? 1 : 0);
            maze -> matrix[wall_y][wall_x].type = EMPTY;
            backtrack_size++;
        }
    }
    while (backtrack_size > 1);

    // Set the visited cells used for backtracking to empty cells.
    for (int16_t row = 0; row < maze -> rows; row++)
    {
        for (int16_t column = 0; column < maze -> columns; column++)
        {
            if (maze -> matrix[row][column].type == VISITED)
            {
                maze -> matrix[row][column].type = EMPTY;
            }
        }
    }

    // Freeing 2d backtrack
    for (int32_t coord = 0; coord < max_size; coord++)
    {
        free(backtrack[coord]);
    }
    free(backtrack);

    // Setting start and end of the maze.
    // Start is top left corner
    // End is bottom right corner.
    maze -> start_x = 0;
    maze -> start_y = 0;
    maze -> end_x = maze -> columns - 1;
    maze -> end_y = maze -> rows - 1;
    maze -> matrix[maze -> start_y][maze -> start_x].type = START;
    maze -> matrix[maze -> end_y][maze -> end_x].type = END;
    
    return maze;
}

static error_t parse_opt(int32_t key, char *arg, struct argp_state *state)
{
    Arguments *args = state -> input;
    errno = 0;
    char *endptr = NULL;
    switch (key) 
    {
        case 'h':
            argp_state_help(state, state -> out_stream, ARGP_HELP_STD_HELP);
            break;

        case 'v':
            fprintf(state -> out_stream, "%s", argp_program_version);
            exit(EXIT_SUCCESS);
            break;
        
        case OPTION_SCREEN_HEIGHT: case OPTION_SCREEN_WIDTH:
        case 'c': case 'r': case 'f':
            long val = strtol(arg, &endptr, 10);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (val <= 0 || val > INT16_MAX)
            {
                fprintf(state -> out_stream,
                        "Value must be between [1, %"PRIi16"]\n", INT16_MAX);
                exit(EXIT_FAILURE);
            }

            if (key == 'c')
            {
                args -> maze_columns = val;
            }

            else if (key == 'f')
            {
                args -> fps = val;
            }
            
            else if (key == 'r')
            {
                args -> maze_rows = val;
            }

            else if (key == OPTION_SCREEN_HEIGHT)
            {
                args -> screen_height = val;
            }

            else if (key == OPTION_SCREEN_WIDTH)
            {
                args -> screen_width = val;
            }
            break;

        case OPTION_FULLSCREEN: case OPTION_SHOW_BODY:
        case OPTION_SHOW_DEAD_HEAD:
            long bool_value = strtol(arg, &endptr, 10);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }

            else if (bool_value < 0 || bool_value > 1)
            {
                fprintf(state -> out_stream,
                        "Bool value must be [0 or 1]\n");
                exit(EXIT_FAILURE);
            }

            if (key == OPTION_FULLSCREEN)
            {
                args -> fullscreen = bool_value;
            }

            else if (key == OPTION_SHOW_BODY)
            {
                args -> show_body = bool_value;
            }
            else if (key == OPTION_SHOW_DEAD_HEAD)
            {
                args -> show_dead_head = bool_value;
            }

            break;

        case ARGP_KEY_ARG:
            fprintf(state -> out_stream,
                "program doesn't accept arguments without optionn\n");
            exit(EXIT_FAILURE);
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

// https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
// Recommendations for good rainbows
void hsl_to_rgb(double hue, double saturation, double lightness,
        int *r, int *g, int *b)
{
    double a = saturation * fmin(lightness, 1 - lightness);
    double k = (int)(0 + hue / 30.0) % 12;
    *r = 255 * (lightness - a * fmax(-1, fmin(fmin(k - 3, 9 - k), 1)));

    k = (int)(8 + hue / 30.0) % 12;
    *g = 255 * (lightness - a * fmax(-1, fmin(fmin(k - 3, 9 - k), 1)));

    k = (int)(4 + hue / 30.0) % 12;
    *b = 255 * (lightness - a * fmax(-1, fmin(fmin(k - 3, 9 - k), 1)));

    // Multiply r, g, b by 255 because these were on [0, 1] interval.
}

/*
 * Function: get_key
 * ----------------------
 * Check for the events close window and key presses using SDL_PollEvent.
 *
 * Parameters:
 * -----------
 *  none aka void.
 *
 * returns: uint8_t constant indicating action of event
 * (QUIT, PAUSE or UNKNOWN)
 *
 */
uint8_t get_key(void)
{
    SDL_Event event;
    uint8_t result = USER_UNKNOWN_EVENT;
    while (SDL_PollEvent(&event)) 
    {
        switch (event.type)
        {
            case SDL_QUIT:
                result = USER_QUIT_EVENT;
                break;

            case SDL_KEYDOWN:
                {
                    SDL_Keycode key_code = event.key.keysym.sym;
                    if (key_code == SDLK_ESCAPE || key_code == SDLK_q)
                    {
                        result = USER_QUIT_EVENT;
                    }
                    
                    else if (key_code == SDLK_SPACE ||
                            key_code == SDLK_KP_ENTER ||
                            key_code == SDLK_RETURN)
                    {
                        result = USER_PAUSE_EVENT;
                    }
                    break;
                }
            default: {}
        }
    }
    return result;
}

// I was using prior to this the typical rand() % N
// https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand#1202706
int SO_random(int min, int max)
{
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}
