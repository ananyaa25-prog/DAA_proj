/*
 * main.c
 * SETG – Smart Exam Timetable Generator
 * ---------------------------------------------------------------
 * Entry point.  Handles:
 *   • Reading courses and student-enrolment data (file or stdin)
 *   • Building the conflict graph (via greedy_core.c)
 *   • Running all algorithm modules and collecting results
 *   • Printing a side-by-side performance comparison table
 *
 * Input file format (plain text):
 * ────────────────────────────────
 *   Line 1 : <num_courses> <num_slots>
 *   Lines 2 … num_courses+1 : <course_index> <course_name>
 *   Next   : <num_students>
 *   Per student: <student_id> <num_enrolled> <c1> <c2> … <cN>
 *
 * Example (sample_input_small.txt):
 *   5 4
 *   0 Maths
 *   1 Physics
 *   2 Chemistry
 *   3 Biology
 *   4 ComputerScience
 *   3
 *   1 3 0 1 2
 *   2 2 1 3
 *   3 3 2 3 4
 * ---------------------------------------------------------------
 */

#include "common.h"

/* ── Forward declarations for other algorithm modules ─────────────── */
/* These are implemented by other team members.  Weak stubs are        */
/* compiled if their .c files are absent (see bottom of this file).   */
extern void exact_backtracking_coloring(const Graph *g, Timetable *t);
extern void branch_and_bound_coloring  (const Graph *g, Timetable *t);
extern void dsatur_coloring            (const Graph *g, Timetable *t);

/* ── Local helpers ──────────────────────────────────────────────────*/
static int  read_input (const char *path, Graph *g,
                        StudentEnrollment *enrollments, int *num_students);
static void print_comparison(Timetable results[], int count);
static void print_banner(void);

/* ═══════════════════════════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════════════════════════ */

int main(int argc, char *argv[])
{
    print_banner();

    /* ── 1. Determine input source ─────────────────────────────── */
    const char *input_path = (argc >= 2) ? argv[1] : "sample_input_small.txt";
    SETG_LOG("Input file: %s", input_path);

    /* ── 2. Allocate graph + enrolment buffer ──────────────────── */
    Graph              g;
    StudentEnrollment  enrollments[MAX_STUDENTS];
    int                num_students = 0;

    /* graph_init called inside read_input once we know sizes */
    if (read_input(input_path, &g, enrollments, &num_students) != SETG_OK) {
        SETG_ERR_LOG("Failed to read input. Aborting.");
        return EXIT_FAILURE;
    }

    /* ── 3. Build conflict graph ────────────────────────────────── */
    build_conflict_graph(&g, enrollments, num_students);
    graph_print(&g);

    /* ── 4. Run all algorithms ──────────────────────────────────── */
    Timetable results[4];
    int       num_results = 0;

    /* (a) Greedy – always runs */
    greedy_coloring(&g, &results[num_results++]);

    /* (b) Exact backtracking */
    exact_backtracking_coloring(&g, &results[num_results++]);

    /* (c) Branch & Bound */
    branch_and_bound_coloring(&g, &results[num_results++]);

    /* (d) DSATUR heuristic */
    dsatur_coloring(&g, &results[num_results++]);

    /* ── 5. Print individual timetables ────────────────────────── */
    for (int i = 0; i < num_results; i++)
        timetable_print(&results[i], &g);

    /* ── 6. Performance comparison table ───────────────────────── */
    print_comparison(results, num_results);

    /* ── 7. Cleanup ────────────────────────────────────────────── */
    graph_free(&g);

    SETG_LOG("SETG completed successfully.");
    return EXIT_SUCCESS;
}

/* ═══════════════════════════════════════════════════════════════════
 *  INPUT READER
 * ═══════════════════════════════════════════════════════════════════ */

static int read_input(const char *path,
                      Graph *g,
                      StudentEnrollment *enrollments,
                      int *num_students)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        SETG_ERR_LOG("Cannot open '%s': %s", path, strerror(errno));
        return SETG_ERR;
    }

    int num_courses, num_slots;
    if (fscanf(fp, "%d %d", &num_courses, &num_slots) != 2 ||
        num_courses <= 0 || num_courses > MAX_COURSES ||
        num_slots   <= 0 || num_slots   > MAX_SLOTS) {
        SETG_ERR_LOG("Invalid header: expected <num_courses> <num_slots>.");
        fclose(fp);
        return SETG_ERR;
    }

    graph_init(g, num_courses, num_slots);

    /* Read course names */
    for (int i = 0; i < num_courses; i++) {
        int idx;
        char name[MAX_NAME_LEN];
        if (fscanf(fp, "%d %63s", &idx, name) != 2 ||
            idx < 0 || idx >= num_courses) {
            SETG_ERR_LOG("Bad course entry at line %d.", i + 2);
            fclose(fp);
            return SETG_ERR;
        }
        strncpy(g->course_names[idx], name, MAX_NAME_LEN - 1);
    }

    /* Read student enrolments */
    int ns;
    if (fscanf(fp, "%d", &ns) != 1 || ns < 0 || ns > MAX_STUDENTS) {
        SETG_ERR_LOG("Invalid student count.");
        fclose(fp);
        return SETG_ERR;
    }
    *num_students = ns;

    for (int s = 0; s < ns; s++) {
        StudentEnrollment *e = &enrollments[s];
        int sid, n;
        if (fscanf(fp, "%d %d", &sid, &n) != 2) {
            SETG_ERR_LOG("Bad student record %d.", s);
            fclose(fp);
            return SETG_ERR;
        }
        e->student_id   = sid;
        e->num_courses  = 0;

        for (int c = 0; c < n; c++) {
            int cid;
            if (fscanf(fp, "%d", &cid) != 1 ||
                cid < 0 || cid >= num_courses) {
                SETG_ERR_LOG("Bad course id for student %d.", sid);
                fclose(fp);
                return SETG_ERR;
            }
            e->course_ids[e->num_courses++] = cid;
        }
    }

    fclose(fp);
    SETG_LOG("Input read: %d courses, %d slots, %d students.",
             num_courses, num_slots, ns);
    return SETG_OK;
}

/* ═══════════════════════════════════════════════════════════════════
 *  COMPARISON TABLE
 * ═══════════════════════════════════════════════════════════════════ */

static void print_comparison(Timetable results[], int count)
{
    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║              ALGORITHM PERFORMANCE COMPARISON                    ║\n");
    printf("╠══════════════════════════════════════╦═══════╦═══════════╦══════╣\n");
    printf("║  %-36s ║ Slots ║ Conflicts ║ ms   ║\n", "Algorithm");
    printf("╠══════════════════════════════════════╬═══════╬═══════════╬══════╣\n");

    for (int i = 0; i < count; i++) {
        const Timetable *t = &results[i];
        /* Skip algorithms that did not run (slot_assignment all UNCOLORED) */
        if (t->num_colors_used == 0) continue;

        printf("║  %-36s ║  %3d  ║    %3d    ║%5.1f ║\n",
               t->label,
               t->num_colors_used,
               t->conflict_count,
               t->elapsed_ms);
    }

    printf("╚══════════════════════════════════════╩═══════╩═══════════╩══════╝\n");

    /* Highlight winner */
    int best_slots = MAX_SLOTS + 1;
    int best_idx   = -1;
    for (int i = 0; i < count; i++) {
        if (results[i].num_colors_used > 0 &&
            results[i].conflict_count == 0 &&
            results[i].num_colors_used < best_slots) {
            best_slots = results[i].num_colors_used;
            best_idx   = i;
        }
    }

    if (best_idx >= 0) {
        printf("\n  ★  Best (fewest slots, conflict-free): %s  [%d slots]\n\n",
               results[best_idx].label,
               results[best_idx].num_colors_used);
    } else {
        printf("\n  ✗  No fully conflict-free solution found with given slot count.\n\n");
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  BANNER
 * ═══════════════════════════════════════════════════════════════════ */

static void print_banner(void)
{
    printf("\n");
    printf("  ███████╗███████╗████████╗ ██████╗ \n");
    printf("  ██╔════╝██╔════╝╚══██╔══╝██╔════╝ \n");
    printf("  ███████╗█████╗     ██║   ██║  ███╗\n");
    printf("  ╚════██║██╔══╝     ██║   ██║   ██║\n");
    printf("  ███████║███████╗   ██║   ╚██████╔╝\n");
    printf("  ╚══════╝╚══════╝   ╚═╝    ╚═════╝ \n");
    printf("  Smart Exam Timetable Generator\n");
    printf("  ─────────────────────────────────────────────\n\n");
}

/* ═══════════════════════════════════════════════════════════════════
 *  WEAK STUBS
 *  Compiled only if the other members' .c files are not yet linked.
 *  Each stub records elapsed time of 0 and leaves slots UNCOLORED so
 *  the comparison table simply skips them.
 * ═══════════════════════════════════════════════════════════════════ */

#ifdef SETG_COMPILE_STUBS

void exact_backtracking_coloring(const Graph *g, Timetable *t)
{
    (void)g;
    timetable_init(t, "Exact Backtracking (stub)");
    SETG_LOG("exact_backtracking_coloring: stub – not yet linked.");
}

void branch_and_bound_coloring(const Graph *g, Timetable *t)
{
    (void)g;
    timetable_init(t, "Branch & Bound (stub)");
    SETG_LOG("branch_and_bound_coloring: stub – not yet linked.");
}

void dsatur_coloring(const Graph *g, Timetable *t)
{
    (void)g;
    timetable_init(t, "DSATUR (stub)");
    SETG_LOG("dsatur_coloring: stub – not yet linked.");
}

#endif /* SETG_COMPILE_STUBS */
