/*
 * greedy_core.c
 * SETG – Smart Exam Timetable Generator
 * ---------------------------------------------------------------
 * Graph construction helpers and Greedy Graph Coloring algorithm.
 *
 * Responsibilities (Member 1):
 *   • Implementing graph_init / graph_add_edge / graph_free / etc.
 *   • Building the conflict graph from enrolment data.
 *   • Greedy coloring (first-fit, sorted by degree).
 *   • Timetable verification and printing.
 * ---------------------------------------------------------------
 */

#include "common.h"

/* ═══════════════════════════════════════════════════════════════════
 *  GRAPH IMPLEMENTATION
 * ═══════════════════════════════════════════════════════════════════ */

void graph_init(Graph *g, int num_courses, int num_slots)
{
    if (!g || num_courses <= 0 || num_courses > MAX_COURSES) {
        SETG_ERR_LOG("graph_init: invalid parameters.");
        return;
    }

    g->num_courses = num_courses;
    g->num_slots   = num_slots;
    g->edge_count  = 0;

    for (int i = 0; i < num_courses; i++) {
        g->adj[i]    = NULL;
        g->degree[i] = 0;
        for (int j = 0; j < num_courses; j++)
            g->adj_matrix[i][j] = 0;
    }
}

void graph_add_edge(Graph *g, int a, int b)
{
    if (!g || a == b) return;
    if (a < 0 || a >= g->num_courses || b < 0 || b >= g->num_courses) return;
    if (g->adj_matrix[a][b]) return;   /* duplicate guard */

    /* mark in matrix */
    g->adj_matrix[a][b] = 1;
    g->adj_matrix[b][a] = 1;

    /* prepend to adjacency lists (O(1)) */
    AdjNode *na = (AdjNode *)malloc(sizeof(AdjNode));
    AdjNode *nb = (AdjNode *)malloc(sizeof(AdjNode));
    if (!na || !nb) { SETG_ERR_LOG("graph_add_edge: malloc failed."); return; }

    na->course_id = b;  na->next = g->adj[a];  g->adj[a] = na;
    nb->course_id = a;  nb->next = g->adj[b];  g->adj[b] = nb;

    g->degree[a]++;
    g->degree[b]++;
    g->edge_count++;
}

int graph_has_edge(const Graph *g, int a, int b)
{
    if (!g || a < 0 || a >= g->num_courses || b < 0 || b >= g->num_courses)
        return 0;
    return g->adj_matrix[a][b];
}

void graph_free(Graph *g)
{
    if (!g) return;
    for (int i = 0; i < g->num_courses; i++) {
        AdjNode *cur = g->adj[i];
        while (cur) {
            AdjNode *tmp = cur->next;
            free(cur);
            cur = tmp;
        }
        g->adj[i] = NULL;
    }
}
void graph_print(const Graph *g)
{
    if (!g) return;

    printf("\n==================================================\n");
    printf("              CONFLICT GRAPH STRUCTURE\n");
    printf("==================================================\n");
    printf("Courses: %d   Conflicts (edges): %d\n",
           g->num_courses, g->edge_count);
    printf("--------------------------------------------------\n");

    for (int i = 0; i < g->num_courses; i++) {
        printf("[%2d] %-15s (deg %2d) -> ",
               i, g->course_names[i], g->degree[i]);

        AdjNode *cur = g->adj[i];
        int printed = 0;
        while (cur) {
            printf("%s%d", printed ? ", " : "", cur->course_id);
            cur = cur->next;
            printed++;
        }

        if (!printed) printf("(no conflicts)");
        printf("\n");
    }

    printf("==================================================\n\n");
}


/* ═══════════════════════════════════════════════════════════════════
 *  CONFLICT GRAPH BUILDER
 *  Reads enrolment records and adds an edge between every pair of
 *  courses that share at least one common student.
 * ═══════════════════════════════════════════════════════════════════ */

/*
 * build_conflict_graph
 *
 * enrollments : array of StudentEnrollment records
 * num_students: length of the array
 * g           : pre-initialised Graph (via graph_init)
 *
 * Strategy: for each student iterate over all course pairs they are
 * enrolled in and add a conflict edge.  The adj_matrix duplicate
 * guard in graph_add_edge makes repeated adds safe and O(1).
 */
void build_conflict_graph(Graph *g,
                          const StudentEnrollment *enrollments,
                          int num_students)
{
    if (!g || !enrollments || num_students <= 0) return;

    SETG_LOG("Building conflict graph from %d student records...", num_students);

    for (int s = 0; s < num_students; s++) {
        const StudentEnrollment *e = &enrollments[s];
        for (int i = 0; i < e->num_courses; i++) {
            for (int j = i + 1; j < e->num_courses; j++) {
                graph_add_edge(g, e->course_ids[i], e->course_ids[j]);
            }
        }
    }

    SETG_LOG("Conflict graph built: %d courses, %d conflict edges.",
             g->num_courses, g->edge_count);
}

/* ═══════════════════════════════════════════════════════════════════
 *  GREEDY GRAPH COLORING
 * ═══════════════════════════════════════════════════════════════════ */

/*
 * Helper: build a processing order sorted by degree (largest first).
 * Ties are broken by course index for determinism.
 */
static void degree_sorted_order(const Graph *g, int *order)
{
    for (int i = 0; i < g->num_courses; i++) order[i] = i;

    /* simple insertion sort – n ≤ 100 so O(n²) is fine */
    for (int i = 1; i < g->num_courses; i++) {
        int key = order[i];
        int j   = i - 1;
        while (j >= 0 && g->degree[order[j]] < g->degree[key]) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }
}

/*
 * greedy_coloring
 *
 * Assigns each course the lowest-numbered slot that is not used by
 * any of its neighbours (first-fit greedy).
 *
 * Processing order: courses sorted by degree (largest first) to
 * reduce the total number of slots required in practice.
 *
 * Time  : O(V + E)
 * Space : O(V)  (forbidden[] scratch array)
 */
void greedy_coloring(const Graph *g, Timetable *t)
{
    if (!g || !t) return;

    timetable_init(t, "Greedy (Degree-Sorted First-Fit)");
    SETG_LOG("Running %s coloring...", t->label);

    TIMER_START(ts);

    int order[MAX_COURSES];
    degree_sorted_order(g, order);

    /* forbidden[c] = 1 means slot c is already used by a neighbour */
    int forbidden[MAX_SLOTS];

    for (int idx = 0; idx < g->num_courses; idx++) {
        int v = order[idx];

        /* reset forbidden array */
        memset(forbidden, 0, sizeof(int) * g->num_slots);

        /* mark all slots occupied by neighbours */
        AdjNode *cur = g->adj[v];
        while (cur) {
            int nbr_slot = t->slot_assignment[cur->course_id];
            if (nbr_slot != UNCOLORED && nbr_slot < g->num_slots)
                forbidden[nbr_slot] = 1;
            cur = cur->next;
        }

        /* assign the first free slot */
        int assigned = UNCOLORED;
        for (int s = 0; s < g->num_slots; s++) {
            if (!forbidden[s]) { assigned = s; break; }
        }

        if (assigned == UNCOLORED) {
            /* More slots needed than available – use overflow slot */
            SETG_ERR_LOG("Course %d (%s): no free slot within %d available. "
                         "Consider increasing NUM_SLOTS.",
                         v, g->course_names[v], g->num_slots);
            assigned = g->num_slots; /* overflow – noted as conflict */
        }

        t->slot_assignment[v] = assigned;
        if (assigned + 1 > t->num_colors_used)
            t->num_colors_used = assigned + 1;
    }

    t->elapsed_ms = TIMER_END_MS(ts);

    timetable_verify(t, g);

    SETG_LOG("Greedy done: %d slots used, %d conflicts, %.2f ms.",
             t->num_colors_used, t->conflict_count, t->elapsed_ms);
}

/* ═══════════════════════════════════════════════════════════════════
 *  TIMETABLE IMPLEMENTATION
 * ═══════════════════════════════════════════════════════════════════ */

void timetable_init(Timetable *t, const char *label)
{
    if (!t) return;
    for (int i = 0; i < MAX_COURSES; i++)
        t->slot_assignment[i] = UNCOLORED;
    t->num_colors_used = 0;
    t->conflict_count  = 0;
    t->elapsed_ms      = 0.0;
    strncpy(t->label, label ? label : "Unknown", MAX_NAME_LEN - 1);
    t->label[MAX_NAME_LEN - 1] = '\0';
}

int timetable_verify(Timetable *t, const Graph *g)
{
    if (!t || !g) return SETG_ERR;

    t->conflict_count = 0;

    /* Check every edge: both endpoints must have different slots */
    for (int u = 0; u < g->num_courses; u++) {
        AdjNode *cur = g->adj[u];
        while (cur) {
            int v = cur->course_id;
            if (v > u) { /* count each edge once */
                if (t->slot_assignment[u] == t->slot_assignment[v] &&
                    t->slot_assignment[u] != UNCOLORED) {
                    t->conflict_count++;
                }
            }
            cur = cur->next;
        }
    }

    return (t->conflict_count == 0) ? SETG_OK : SETG_ERR;
}

void timetable_print(const Timetable *t, const Graph *g)
{
    if (!t || !g) return;

    printf("\n============================================================\n");
    printf("EXAM TIMETABLE - %s\n", t->label);
    printf("============================================================\n");
    printf("Slots used: %d   Conflicts: %d   Time: %.2f ms\n",
           t->num_colors_used, t->conflict_count, t->elapsed_ms);
    printf("------------------------------------------------------------\n");
    printf("%-6s %-25s %-10s\n", "ID", "Course Name", "Slot");
    printf("------------------------------------------------------------\n");

    for (int slot = 0; slot < t->num_colors_used; slot++) {
        for (int c = 0; c < g->num_courses; c++) {
            if (t->slot_assignment[c] == slot) {
                printf("%-6d %-25s Slot %-3d\n",
                       c, g->course_names[c], slot + 1);
            }
        }
        printf("------------------------------------------------------------\n");
    }

    if (t->conflict_count == 0) {
        printf("CONFLICT-FREE TIMETABLE GENERATED\n");
    } else {
        printf("WARNING: %d conflict(s) detected!\n", t->conflict_count);
    }

    printf("============================================================\n\n");
}

