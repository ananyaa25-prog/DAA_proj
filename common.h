/*
 * common.h
 * --------
 * Shared graph data structure used by all graph-coloring algorithms.
 * An undirected graph is represented as an adjacency matrix so that
 * neighbour look-ups are O(1).
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum number of vertices (courses) supported */
#define MAX_VERTICES 100

/* ------------------------------------------------------------------ */
/*  Graph structure                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    int num_vertices;                          /* total number of courses/nodes  */
    int adj[MAX_VERTICES][MAX_VERTICES];       /* adjacency matrix (0 or 1)      */
    int degree[MAX_VERTICES];                  /* pre-computed ordinary degree    */
} Graph;

/* ------------------------------------------------------------------ */
/*  Graph helpers                                                       */
/* ------------------------------------------------------------------ */

/* Initialise an empty graph with v vertices */
static inline void graph_init(Graph *g, int v) {
    g->num_vertices = v;
    memset(g->adj,    0, sizeof(g->adj));
    memset(g->degree, 0, sizeof(g->degree));
}

/* Add an undirected edge between vertices u and v */
static inline void graph_add_edge(Graph *g, int u, int v) {
    if (g->adj[u][v] == 0) {          /* avoid duplicate edges */
        g->adj[u][v] = 1;
        g->adj[v][u] = 1;
        g->degree[u]++;
        g->degree[v]++;
    }
}

/* Print the graph's adjacency matrix (for debugging) */
static inline void graph_print(const Graph *g) {
    printf("Graph (%d vertices):\n", g->num_vertices);
    for (int i = 0; i < g->num_vertices; i++) {
        printf("  v%d (deg=%d): ", i, g->degree[i]);
        for (int j = 0; j < g->num_vertices; j++)
            if (g->adj[i][j]) printf("v%d ", j);
        printf("\n");
    }
}

#endif /* COMMON_H */
