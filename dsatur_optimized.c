#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

/* ------------------------------------------------------------------ */
/*  Types                                                               */
/* ------------------------------------------------------------------ */

/*
 * Per-vertex runtime state maintained while DSATUR runs.
 */
typedef struct {
    int colour;          /* assigned colour (-1 = uncoloured yet)         */
    int saturation;      /* number of DISTINCT colours among neighbours    */
    int neighbour_colours[MAX_COURSES]; /* bitmask-style: nc[c]=1 if colour c
                                            is used by some neighbour      */
} VertexState;

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/*
 * pick_next_vertex
 * ----------------
 * Scan all uncoloured vertices and return the index of the one with:
 *   (1) maximum saturation degree,
 *   (2) maximum ordinary degree as tie-break,
 *   (3) minimum index as final tie-break (deterministic).
 */
static int pick_next_vertex(const Graph *g, const VertexState *state) {
    int best   = -1;
    int best_sat = -1;
    int best_deg = -1;

    for (int v = 0; v < g->num_courses; v++) {
        if (state[v].colour != -1)   /* already coloured – skip */
            continue;

        int sat = state[v].saturation;
        int deg = g->degree[v];

        /* Rule 1: prefer higher saturation */
        if (sat > best_sat) {
            best = v; best_sat = sat; best_deg = deg;
        }
        /* Rule 2: tie on saturation → prefer higher ordinary degree */
        else if (sat == best_sat && deg > best_deg) {
            best = v; best_deg = deg;
        }
        /* Rule 3: tie on both → keep lower index (already guaranteed by
           scanning in order, so no action needed here)                   */
    }
    return best;
}

/*
 * lowest_valid_colour
 * -------------------
 * Return the smallest colour c >= 0 that is NOT used by any
 * already-coloured neighbour of vertex v.
 */
static int lowest_valid_colour(const Graph *g,
                                const VertexState *state,
                                int v) {
    /* neighbour_colours[c] tells us if colour c is forbidden */
    for (int c = 0; c < g->num_courses; c++) {  /* c can never exceed n-1 */
        if (state[v].neighbour_colours[c] == 0)
            return c;
    }
    return g->num_courses; /* should never reach here for a valid graph */
}

/*
 * update_saturation
 * -----------------
 * After colouring vertex v with colour col, update the saturation
 * of every uncoloured neighbour of v.
 */
static void update_saturation(const Graph *g,
                               VertexState *state,
                               int v,
                               int col) {
    for (int u = 0; u < g->num_courses; u++) {
        if (!g->adj_matrix[v][u])              /* not a neighbour – skip */
            continue;
        if (state[u].colour != -1)      /* already coloured – skip */
            continue;

        /* If colour 'col' is newly seen by u, raise its saturation */
        if (state[u].neighbour_colours[col] == 0) {
            state[u].neighbour_colours[col] = 1;
            state[u].saturation++;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Public DSATUR function                                              */
/* ------------------------------------------------------------------ */

/*
 * dsatur_colour
 * -------------
 * Colour graph *g using the DSATUR heuristic.
 *
 * Parameters:
 *   g          – pointer to the shared Graph structure (read-only)
 *   slot_out   – caller-allocated array of size g->num_vertices;
 *                on return, slot_out[v] holds the time-slot (0-based)
 *                assigned to vertex v.
 *
 * Returns:
 *   Total number of distinct slots (colours) used.
 */

    void dsatur_coloring(const Graph *g, Timetable *t) {
    if (!g || !t) return;

    timetable_init(t, "DSATUR");
    TIMER_START(start);

    int n = g->num_courses;
    VertexState state[MAX_COURSES];

    for (int v = 0; v < n; v++) {
        state[v].colour = UNCOLORED;
        state[v].saturation = 0;
        memset(state[v].neighbour_colours, 0, sizeof(state[v].neighbour_colours));
    }

    int num_colours_used = 0;

    for (int step = 0; step < n; step++) {
        int v = pick_next_vertex(g, state);
        int col = lowest_valid_colour(g, state, v);
        state[v].colour = col;

        if (col + 1 > num_colours_used)
            num_colours_used = col + 1;

        update_saturation(g, state, v, col);
    }

    for (int v = 0; v < n; v++) {
        t->slot_assignment[v] = state[v].colour;
    }

    t->num_colors_used = num_colours_used;
    timetable_verify(t, g);
    t->elapsed_ms = TIMER_END_MS(start);
}


/* ------------------------------------------------------------------ */
/*  Utility – print slot assignment                                     */
/* ------------------------------------------------------------------ */

void dsatur_print_result(int n, const int *slot, int num_slots) {
    printf("\n=== DSATUR Result ===\n");
    printf("Total time-slots used: %d\n\n", num_slots);
    printf("%-10s  %s\n", "Course", "Slot");
    printf("%-10s  %s\n", "------", "----");
    for (int v = 0; v < n; v++)
        printf("%-10d  %d\n", v, slot[v]);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/*  Demo main – builds a small conflict graph and colours it           */
/* ------------------------------------------------------------------ */

/*
 * Example conflict graph (8 courses):
 *
 *   0 -- 1 -- 2
 *   |    |    |
 *   3 -- 4 -- 5
 *        |
 *        6 -- 7
 *
 * Edges represent shared-student conflicts (courses that cannot be
 * scheduled in the same time-slot).
 */

