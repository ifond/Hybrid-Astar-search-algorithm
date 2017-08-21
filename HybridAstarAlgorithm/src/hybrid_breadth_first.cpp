/*
 * hybrid_breadth_first.cpp
 *
 *  Created on: Aug 19, 2017
 *      Author: ramiz
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include <vector>
#include "hybrid_breadth_first.h"
#include "utils.h"

using namespace std;

/**
 * Initializes HBF
 */
HBF::HBF() {

}

HBF::~HBF() {
}

int HBF::theta_to_stack_number(double theta) {
  /*
   Takes an angle (in radians) and returns which "stack" in the 3D configuration space
   this angle corresponds to. Angles near 0 go in the lower stacks while angles near
   2 * pi go in the higher stacks.
   */

  double new_theta = fmod((theta + 2 * M_PI), (2 * M_PI));
  int stack_number = (int) (round(new_theta * NUM_THETA_CELLS / (2 * M_PI)))
      % NUM_THETA_CELLS;
  return stack_number;
}

int HBF::idx(double float_num) {
  /*
   Returns the index into the grid for continuous position. So if x is 3.621, then this
   would return 3 to indicate that 3.621 corresponds to array index 3.
   */

  return int(floor(float_num));
}

double HBF::deg2rad(double delta_i) {
  return M_PI / 180.0 * delta_i;
}

vector<vector<double> > HBF::calculate_euclidean_heuristic(
    const vector<vector<int> > &grid, const vector<int> &goal) {
  int goal_x = goal[0];
  int goal_y = goal[1];

  vector<vector<double> > dist_grid(grid[0].size(),
      vector<double>(grid.size()));

  for (int i = 0; i < grid.size(); ++i) {
    for (int j = 0; j < grid[0].size(); ++j) {
      dist_grid[i][j] = Utils::euclidean(goal_x, goal_y, i, j);
      ;
    }
  }

  Utils::print_grid(dist_grid);

  return dist_grid;
}

/**
 * ---- Non-holonomic without obstacles heuristic-----
 *
 * Calculates 3D (x, y, theta) grid of heuristic using euclidean distance. This heuristic can be used to achieve
 * goal in desired heading (theta)
 */
vector<vector<vector<double> > > HBF::calculate_euclidean_heuristic_3d(
    const vector<vector<int> > &grid, const vector<int> &goal) {
  int goal_x = goal[0];
  int goal_y = goal[1];
  double goal_theta = goal[2];

  vector<vector<vector<double> > > dist_grid(NUM_THETA_CELLS,
      vector<vector<double> >(grid[0].size(), vector<double>(grid.size())));

  for (int i = 0; i < grid.size(); ++i) {
    for (int j = 0; j < grid[0].size(); ++j) {
      for (int theta = -35; theta < 40; theta += 5) {
        double theta_rad = deg2rad(theta);
        int theta_stack_number = theta_to_stack_number(theta_rad);
        dist_grid[theta_stack_number][j][i] = Utils::euclidean_3d(goal_x,
            goal_y, goal_theta, j, i, theta_rad);
      }
    }
  }

  cout << endl;
  Utils::print_grid(dist_grid[0]);
  cout << endl;

  return dist_grid;
}

vector<int> HBF::move(const vector<int> &current_cell,
                      const vector<int> &move) {
  vector<int> new_cell(current_cell.size());

  for (int i = 0; i < current_cell.size(); ++i) {
    new_cell[i] = current_cell[i] + move[i];
  }

  return new_cell;
}

int HBF::holonomic_f_shortest_path_cost(vector<vector<int> > &grid,
                                        vector<vector<int> > &cost_grid,
                                        const vector<int> &cell,
                                        const vector<int> &goal) {
  //handle base case
  if (cell == goal) {
    //passed cell is actually goal cell so no cost incurred
    return 0;
  }

  int cell_row = cell[0];
  int cell_col = cell[1];

  //check if cost already calculated
  if (cost_grid[cell_row][cell_col] != -1) {
    //cost has already been calculated for this cell
    return cost_grid[cell_row][cell_col];
  }

  //handle obstacle
  if (grid[cell_row][cell_col] == 1) {
    //assign it maximum cost
    return COST_FOR_OBSTACLE;
  }

  //mark this cell as visited
  grid[cell_row][cell_col] = 1;

  //initialize with a very high value
  int min_cost = COST_FOR_OBSTACLE;

  //perform each valid move and pick the minimum cost from all moves
  for (int i = 0; i < holomonic_moves_.size(); ++i) {
    //make a move
    vector<int> next_cell = move(cell, holomonic_moves_[i]);
    int next_row = next_cell[0];
    int next_col = next_cell[1];

    //check if this move is valid or not
    if (!is_valid_cell(grid, next_cell)) {
      //invalid move, ignore this move
      continue;
    }

    //check if this next cell is an obstacle
    if (grid[next_row][next_col] == 1) {
      //assign maximum cost to it
      cost_grid[next_row][next_col] = COST_FOR_OBSTACLE;
      //as it is obstacle, ignore this move
      continue;
    }

    int cost = cost_grid[next_row][next_col];

    //check if cost for next_cell has already been calculated or not.
    //if cost is -1 then it means cost has not been calculated other yes
    if (cost == -1) {
      //cost not calculated, so calculate it now
      cost = holonomic_f_shortest_path_cost(grid, cost_grid, next_cell, goal);
      //update cost_grid to reflect cost for this cell
      cost_grid[next_row][next_col] = cost;
    }

    //update min_cost if new cost is less
    if (cost < min_cost) {
      min_cost = cost;
    }
  }

  //add +1 to count current cell
  int cost_for_this_cell = min_cost + 1;
  //update cost for this cell
  cost_grid[cell_row][cell_col] = cost_for_this_cell;

  return cost_for_this_cell;
}

vector<vector<int> > HBF::holonomic_min_cost_from_cell(
    const vector<vector<int> > &grid, const vector<int> &start,
    const vector<int> &goal) {
  //vector to hold zeros
  vector<int> cols(grid[0].size(), -1);
  //create a vector to hold shortest path cost for reach cell to goal
  vector<vector<int> > cost_grid(grid.size(), cols);

  //make a copy of grid to pass f_shortest_path_cost() function
  //as it modifies it
  auto grid_copy = grid;

  cost_grid[start[0]][start[1]] = holonomic_f_shortest_path_cost(grid_copy,
      cost_grid, start, goal);

  return cost_grid;
}

vector<vector<int> > HBF::holonomic_min_path_cost_from_each_cell_heuristic(
    const vector<vector<int> > &grid, const vector<int> &goal) {

  //vector to hold zeros
  vector<int> cols(grid[0].size(), -1);
  //create a vector to hold shortest path cost for reach cell to goal
  vector<vector<int> > cost_grid(grid.size(), cols);

  //make a copy of grid to pass f_shortest_path_cost() function
  //as it modifies it
  auto grid_copy = grid;
  for (int i = 0; i < grid.size(); ++i) {
    for (int j = 0; j < grid[0].size(); ++j) {
      //check for obstacle
      if (grid[i][j] == 1) {
        //it is obstacle, replace its cost with very high value
        cost_grid[i][j] = COST_FOR_OBSTACLE;
        continue;
      }

      cost_grid[i][j] = holonomic_f_shortest_path_cost(grid_copy, cost_grid, {
          i, j }, goal);
    }
  }

  return cost_grid;
}

vector<HBF::maze_s> HBF::expand(HBF::maze_s state) {
  int g = state.g;
  double x = state.x;
  double y = state.y;
  double theta = state.theta;

  //update g value for next cells
  int g2 = g + 1;

  vector<HBF::maze_s> next_states;

  //try steering angle from max left to max right to
  //find all possible configurations which will be later
  //checked for validity in `search()` function
  for (double delta_i = -35; delta_i < 40; delta_i += 5) {

    //convert from degree to radian
    double delta = deg2rad(delta_i);
    //calculate rate of change of heading using formula: w = v/L * tan(delta)
    double omega = SPEED / LENGTH * tan(delta);

    //predict new heading based on rate of change of heading
    double theta2 = theta + omega;
    //validate angle value
    if (theta2 > 0) {
      theta2 += 2 * M_PI;
    }

    //predict new (x, y) using motion equations, assuming time change (delta_t)= 1
    //x2 = x1 + v * delta_t * cos(theta)
    //y2 = y1 + v * delta_t * cos(theta)
    double x2 = x + SPEED * cos(theta2);
    double y2 = y + SPEED * sin(theta2);

    //add this predicted state to set of next possible (valid/invalid) states.
    HBF::maze_s state2;
    state2.g = g2;
    state2.x = x2;
    state2.y = y2;
    state2.theta = theta2;
    next_states.push_back(state2);
  }

  return next_states;
}

vector<HBF::maze_s> HBF::reconstruct_path(
    vector<vector<vector<HBF::maze_s> > > came_from, vector<double> start,
    HBF::maze_s final) {

  vector<maze_s> path = { final };

  int stack = theta_to_stack_number(final.theta);

  maze_s current = came_from[stack][idx(final.x)][idx(final.y)];

  stack = theta_to_stack_number(current.theta);

  double x = current.x;
  double y = current.y;
  while (x != start[0] && y != start[1]) {
    path.push_back(current);
    current = came_from[stack][idx(x)][idx(y)];
    x = current.x;
    y = current.y;
    stack = theta_to_stack_number(current.theta);
  }

  return path;

}

bool HBF::is_valid_cell(const vector<vector<int> >& grid,
                        const vector<int> &cell) {
  return is_valid_cell(cell[0], cell[1], grid);
}

bool HBF::is_valid_cell(double x2, double y2,
                        const vector<vector<int> >& grid) {
  return (x2 >= 0 && x2 < grid.size()) && (y2 >= 0 && y2 < grid[0].size());
}

HBF::maze_path HBF::search(vector<vector<int> > grid, vector<double> start,
                           vector<int> goal) {

  /*
   Working Implementation of breadth first search. Does NOT use a heuristic
   and as a result this is pretty inefficient. Try modifying this algorithm
   into hybrid A* by adding heuristics appropriately.
   */

  vector<vector<vector<maze_s> > > closed(NUM_THETA_CELLS,
      vector<vector<maze_s>>(grid[0].size(), vector<maze_s>(grid.size())));
  vector<vector<vector<int> > > closed_value(NUM_THETA_CELLS,
      vector<vector<int>>(grid[0].size(), vector<int>(grid.size())));
  vector<vector<vector<maze_s> > > came_from(NUM_THETA_CELLS,
      vector<vector<maze_s>>(grid[0].size(), vector<maze_s>(grid.size())));

  //initial values
  double theta = start[2];
  int stack = theta_to_stack_number(theta);
  int g = 0;

  //update start node
  maze_s state;
  state.g = g;
  state.x = start[0];
  state.y = start[1];

  //  vector<vector<double> > heuristic = calculate_euclidean_heuristic(grid, goal);
  //  vector<vector<vector<double> > > heuristic1 = calculate_euclidean_heuristic_3d(grid, goal);

  //calculate heuristics
  vector<vector<int>> cost_grid = holonomic_min_cost_from_cell(grid,
      { idx(state.x), idx(state.y) }, goal);
  printf("\nDP cost_grid: \n");
  Utils::print_grid(cost_grid);

  //mark start node as closed and visited
  closed[stack][idx(state.x)][idx(state.y)] = state;
  closed_value[stack][idx(state.x)][idx(state.y)] = 1;
  came_from[stack][idx(state.x)][idx(state.y)] = state;

  //we want to keep a count of total nodes closed/checked
  int total_closed = 1;

  //initialize list of valid next states/configs as there can be
  //invalid configs as well (that lead to obstacles or out of grid or high cost)
  vector<maze_s> opened = { state };

  bool finished = false;
  while (!opened.empty()) {

    maze_s next = opened[0]; //grab first elment
    opened.erase(opened.begin()); //pop first element

    int x = next.x;
    int y = next.y;

    if (idx(x) == goal[0] && idx(y) == goal[1]) {
      cout << "found path to goal in " << total_closed << " expansions" << endl;
      maze_path path;
      path.closed = closed;
      path.came_from = came_from;
      path.final = next;
      return path;

    }
    vector<maze_s> next_state = expand(next);

    for (int i = 0; i < next_state.size(); i++) {
      int g2 = next_state[i].g;
      double x2 = next_state[i].x;
      double y2 = next_state[i].y;
      double theta2 = next_state[i].theta;

      if (!is_valid_cell(x2, y2, grid)) {
        //invalid cell
        continue;
      }

      int stack2 = theta_to_stack_number(theta2);

      //check if
      //1. this cell is in open cells list
      //2. is not an obstacle
      if (closed_value[stack2][idx(x2)][idx(y2)] == 0
          && grid[idx(x2)][idx(y2)] == 0) {

        maze_s state2;
        state2.g = g2;
        state2.x = x2;
        state2.y = y2;
        state2.theta = theta2;

        opened.push_back(state2);

        closed[stack2][idx(x2)][idx(y2)] = next_state[i];
        closed_value[stack2][idx(x2)][idx(y2)] = 1;
        came_from[stack2][idx(x2)][idx(y2)] = next;
        total_closed += 1;
      }

    }

  }
  cout << "no valid path." << endl;
  HBF::maze_path path;
  path.closed = closed;
  path.came_from = came_from;
  path.final = state;
  return path;

}

