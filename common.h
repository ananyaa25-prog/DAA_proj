/*
 * common.h
 * SETG – Smart Exam Timetable Generator
 * ---------------------------------------------------------------
 * Shared types, constants, and the central graph / timetable
 * structures used by every module in the project.
 * ---------------------------------------------------------------
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* ───────────────────────────── Limits ───────────────────────────── */
#define MAX_COURSES      100       /* maximum number of courses        */
#define MAX_STUDENTS     500       /* maximum number of students       */
#define MAX_SLOTS        20        /* maximum time slots               */
#define MAX_NAME_LEN     64        /* course / student name length     */
#define UNCOLORED        -1        /* sentinel: slot not yet assigned  */

/* ─────────────────────────── Return codes ───────────────────────── */
#define SETG_OK          0
#define SETG_ERR        -1

/* ──────────────────────── Data structures ───────────────────────── */

/*
 * AdjNode – singly-linked list node for one adjacency entry.
 */

typedef struct AdjNode {
    int              course_id;   /* neighbour course index           */
    struct AdjNode  *next;
} AdjNode;

/*
 * Graph – conflict graph.
 *   nodes      : number of courses (vertices)
 *   adj        : adjacency list heads (size = nodes)
 *   adj_matrix : O(1) edge lookup (backed by adj list)
 *   degree     : pre-computed degree of each vertex
 */
typedef struct {
    int       num_courses;
    int       num_slots;
    char      course_names[MAX_COURSES][MAX_NAME_LEN];
    AdjNode  *adj[MAX_COURSES];
    int       adj_matrix[MAX_COURSES][MAX_COURSES]; /* 1 = conflict */
    int       degree[MAX_COURSES];
    int       edge_count;
} Graph;

/*
 * Timetable – result of a coloring run.
 *   slot_assignment[i] : slot index (0-based) assigned to course i,
 *                        or UNCOLORED if not yet assigned.
 *   num_colors_used    : chromatic number found by this run.
 *   conflict_count     : edges where both endpoints share a slot
 *                        (should be 0 for a valid coloring).
 *   label              : human-readable algorithm name.
 *   elapsed_ms         : wall-clock time in milliseconds.
 */
typedef struct {
    int    slot_assignment[MAX_COURSES];
    int    num_colors_used;
    int    conflict_count;
    double elapsed_ms;
    char   label[MAX_NAME_LEN];
} Timetable;

/*
 * StudentEnrollment – raw enrolment data used to build the graph.
 */
typedef struct {
    int student_id;
    int course_ids[MAX_COURSES];   /* courses this student takes       */
    int num_courses;
} StudentEnrollment;

/* ──────────────────────── Graph API ─────────────────────────────── */

/* Initialise an empty graph for num_courses courses. */
void graph_init(Graph *g, int num_courses, int num_slots);

/* Add an undirected conflict edge between course a and course b.
   No-op if the edge already exists or a == b.                        */
void graph_add_edge(Graph *g, int a, int b);

/* Return 1 if (a, b) is a conflict edge, 0 otherwise. */
int  graph_has_edge(const Graph *g, int a, int b);

/* Free all dynamically allocated adjacency-list nodes. */
void graph_free(Graph *g);

/* Pretty-print the adjacency list to stdout. */
void graph_print(const Graph *g);
void build_conflict_graph(Graph *g,
                          const StudentEnrollment *enrollments,
                          int num_students);

void greedy_coloring(const Graph *g, Timetable *t);
/* ─────────────────────── Timetable API ──────────────────────────── */

/* Initialise a timetable (all slots set to UNCOLORED). */
void timetable_init(Timetable *t, const char *label);

/* Verify a completed timetable and set conflict_count.
   Returns SETG_OK if conflict-free, SETG_ERR otherwise.             */
int  timetable_verify(Timetable *t, const Graph *g);

/* Pretty-print the timetable to stdout. */
void timetable_print(const Timetable *t, const Graph *g);

/* ─────────────────────── Utility macros ─────────────────────────── */

#define SETG_LOG(fmt, ...)  fprintf(stdout, "[SETG] " fmt "\n", ##__VA_ARGS__)
#define SETG_ERR_LOG(fmt, ...) fprintf(stderr, "[SETG ERROR] " fmt "\n", ##__VA_ARGS__)

/* Wall-clock timer helpers (portable via time.h clock()). */
#define TIMER_START(t_start)  clock_t t_start = clock()
#define TIMER_END_MS(t_start) (1000.0 * (double)(clock() - (t_start)) / CLOCKS_PER_SEC)

#endif /* COMMON_H */
