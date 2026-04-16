/*
 * dsatur.c
 * --------
 * Graph colouring using the DSATUR heuristic (Brélaz, 1979).
 *
 * ALGORITHM OVERVIEW
 * ==================
 * DSATUR (Degree of SATURation) colours graph vertices one at a time.
 * At each step it picks the UNCOLOURED vertex with:
 *   1. Highest SATURATION DEGREE  (number of distinct colours already
 *      seen among its neighbours).
 *   2. Ties broken by highest ORDINARY DEGREE (number of edges).
 *   3. Further ties broken by lowest vertex index (arbitrary but stable).
 *
 * Then it assigns the LOWEST valid colour (slot) not used by any
 * already-coloured neighbour.
 *
 * For exam / course timetabling:
 *   • Vertices  = courses
 *   • Edges     = two courses share at least one student (conflict)
 *   • Colours   = exam time-slots
 */

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
    int neighbour_colours[MAX_VERTICES]; /* bitmask-style: nc[c]=1 if colour c
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

    for (int v = 0; v < g->num_vertices; v++) {
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
    for (int c = 0; c < g->num_vertices; c++) {  /* c can never exceed n-1 */
        if (state[v].neighbour_colours[c] == 0)
            return c;
    }
    return g->num_vertices; /* should never reach here for a valid graph */
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
    for (int u = 0; u < g->num_vertices; u++) {
        if (!g->adj[v][u])              /* not a neighbour – skip */
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
int dsatur_colour(const Graph *g, int *slot_out) {
    int n = g->num_vertices;

    /* ---- Initialise per-vertex state -------------------------------- */
    VertexState state[MAX_VERTICES];
    for (int v = 0; v < n; v++) {
        state[v].colour     = -1;   /* uncoloured */
        state[v].saturation =  0;
        memset(state[v].neighbour_colours, 0,
               sizeof(state[v].neighbour_colours));
    }

    int num_colours_used = 0;

    /* ---- Main DSATUR loop ------------------------------------------- */
    for (int step = 0; step < n; step++) {

        /* Step 1 – choose the next vertex to colour */
        int v = pick_next_vertex(g, state);

        /* Step 2 – assign the lowest valid colour */
        int col = lowest_valid_colour(g, state, v);
        state[v].colour = col;

        /* Track how many distinct colours we have used */
        if (col + 1 > num_colours_used)
            num_colours_used = col + 1;

        /* Step 3 – propagate new colour info to uncoloured neighbours */
        update_saturation(g, state, v, col);
    }

    /* ---- Copy results into caller's array ---------------------------- */
    for (int v = 0; v < n; v++)
        slot_out[v] = state[v].colour;

    return num_colours_used;
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
int main(void) {
    Graph g;
    graph_init(&g, 8);

    /* Add conflict edges */
    graph_add_edge(&g, 0, 1);
    graph_add_edge(&g, 1, 2);
    graph_add_edge(&g, 0, 3);
    graph_add_edge(&g, 1, 4);
    graph_add_edge(&g, 2, 5);
    graph_add_edge(&g, 3, 4);
    graph_add_edge(&g, 4, 5);
    graph_add_edge(&g, 4, 6);
    graph_add_edge(&g, 6, 7);

    printf("=== DSATUR Graph Colouring ===\n\n");
    graph_print(&g);

    /* Run DSATUR */
    int slots[MAX_VERTICES];
    int num_slots = dsatur_colour(&g, slots);

    /* Display results */
    dsatur_print_result(g.num_vertices, slots, num_slots);

    /* Verify no two adjacent vertices share the same slot */
    printf("Verification (checking all edges):\n");
    int ok = 1;
    for (int u = 0; u < g.num_vertices; u++) {
        for (int v = u + 1; v < g.num_vertices; v++) {
            if (g.adj[u][v] && slots[u] == slots[v]) {
                printf("  CONFLICT: course %d and course %d both in slot %d!\n",
                       u, v, slots[u]);
                ok = 0;
            }
        }
    }
    if (ok)
        printf("  All conflicts resolved – colouring is valid.\n\n");

    return 0;
}
