/*
 * exact_backtracking.c
 * ---------------------------------------------------------------
 * Exact Graph Coloring using Backtracking
 * Finds the minimum number of slots (chromatic number)
 * ---------------------------------------------------------------
 */

#include "common.h"

/*
 * is_safe
 * ---------------------------------------------------------------
 * Check if assigning 'slot' to course 'v' is valid.
 * A slot is valid if no adjacent (conflicting) course
 * has already been assigned the same slot.
 */
static int is_safe(const Graph *g, int v, int slot, int assignment[]) {
    for (int i = 0; i < g->num_courses; i++) {
        if (g->adj_matrix[v][i] == 1 && assignment[i] == slot) {
            return 0; // conflict exists
        }
    }
    return 1; // safe to assign
}

/*
 * backtrack
 * ---------------------------------------------------------------
 * Recursive function to assign slots using backtracking.
 * Tries all possible assignments for each course.
 */
static int backtrack(const Graph *g, int m, int v, int assignment[]) {

    // Base case: all courses assigned successfully
    if (v == g->num_courses) {
        return 1;
    }

    // Try assigning each slot from 0 to m-1
    for (int s = 0; s < m; s++) {

        if (is_safe(g, v, s, assignment)) {

            assignment[v] = s; // assign slot

            // Recur for next course
            if (backtrack(g, m, v + 1, assignment)) {
                return 1;
            }

            // Backtrack if assignment fails
            assignment[v] = UNCOLORED;
        }
    }

    return 0; // no valid assignment found
}

/*
 * exact_backtracking_coloring
 * ---------------------------------------------------------------
 * Main driver function.
 * Tries coloring with 1, 2, 3... slots until a valid solution is found.
 * Stores the result in the Timetable structure.
 */
void exact_backtracking_coloring(const Graph *g, Timetable *t) {

    TIMER_START(start);

    int assignment[MAX_COURSES];

    // Initialize all courses as UNCOLORED
    for (int i = 0; i < g->num_courses; i++) {
        assignment[i] = UNCOLORED;
    }

    int min_slots = 0;

    // Try increasing number of slots (1 → V)
    for (int m = 1; m <= g->num_courses; m++) {

        // Reset assignments before each attempt
        for (int i = 0; i < g->num_courses; i++) {
            assignment[i] = UNCOLORED;
        }

        // Attempt coloring with 'm' slots
        if (backtrack(g, m, 0, assignment)) {
            min_slots = m;
            break; // first success = optimal solution
        }
    }

    // Store result in timetable
    for (int i = 0; i < g->num_courses; i++) {
        t->slot_assignment[i] = assignment[i];
    }

    t->num_colors_used = min_slots;

    // Verify conflicts (should be zero)
    timetable_verify(t, g);

    // Record execution time
    t->elapsed_ms = TIMER_END_MS(start);

    // Set algorithm label
    strcpy(t->label, "Exact Backtracking");
}
