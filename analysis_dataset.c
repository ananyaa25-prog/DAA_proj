#include "common.h"

/*
 * analysis_dataset.c
 * ---------------------------------------------------------------
 * Dataset analysis utility for SETG project.
 *
 * Responsibilities:
 *   - Read one or more input datasets
 *   - Build the conflict graph
 *   - Run available algorithms
 *   - Print comparative analysis
 *
 * NOTE:
 *   This file does NOT define main().
 *   Call analyze_default_datasets() or analyze_dataset_file()
 *   from main.c whenever needed.
 * ---------------------------------------------------------------
 */

/* Uncomment only when branch_and_bound_coloring is implemented */
/* #define ENABLE_BNB */

/* Forward declarations for algorithms */
void greedy_coloring(const Graph *g, Timetable *t);
void exact_backtracking_coloring(const Graph *g, Timetable *t);
void dsatur_coloring(const Graph *g, Timetable *t);

#ifdef ENABLE_BNB
void branch_and_bound_coloring(const Graph *g, Timetable *t);
#endif

/* ───────────────────────── Local helpers ───────────────────────── */

static int read_input_file(const char *path,
                           Graph *g,
                           StudentEnrollment *enrollments,
                           int *num_students);

static void print_dataset_header(const char *path);
static void print_dataset_summary(const Graph *g, int num_students);
static void print_result_row(const Timetable *t);
static void print_analysis_table(Timetable results[], int count);

/* ───────────────────────── Core API ────────────────────────────── */

/*
 * analyze_dataset_file
 * ---------------------------------------------------------------
 * Analyze one dataset file by running all available algorithms.
 *
 * Returns:
 *   SETG_OK  on success
 *   SETG_ERR on failure
 */
int analyze_dataset_file(const char *path)
{
    Graph g;
    StudentEnrollment enrollments[MAX_STUDENTS];
    int num_students = 0;

    print_dataset_header(path);

    if (read_input_file(path, &g, enrollments, &num_students) != SETG_OK) {
        SETG_ERR_LOG("Dataset analysis failed for '%s'.", path);
        return SETG_ERR;
    }

    build_conflict_graph(&g, enrollments, num_students);
    print_dataset_summary(&g, num_students);

    Timetable results[4];
    int count = 0;

    greedy_coloring(&g, &results[count++]);
    exact_backtracking_coloring(&g, &results[count++]);
    dsatur_coloring(&g, &results[count++]);

#ifdef ENABLE_BNB
    branch_and_bound_coloring(&g, &results[count++]);
#endif

    print_analysis_table(results, count);

    graph_free(&g);
    return SETG_OK;
}

/*
 * analyze_default_datasets
 * ---------------------------------------------------------------
 * Analyze the built-in sample datasets one by one.
 */
void analyze_default_datasets(void)
{
    const char *datasets[] = {
        "sample_input_small.txt",
        "sample_input_medium.txt",
        "sample_input_large.txt"
    };

    int total = (int)(sizeof(datasets) / sizeof(datasets[0]));

    printf("\n============================================================\n");
    printf("                 DATASET ANALYSIS MODE\n");
    printf("============================================================\n");

    for (int i = 0; i < total; i++) {
        analyze_dataset_file(datasets[i]);
    }
}

/* ───────────────────────── Input reader ────────────────────────── */

static int read_input_file(const char *path,
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
        num_slots <= 0 || num_slots > MAX_SLOTS) {
        SETG_ERR_LOG("Invalid dataset header in '%s'.", path);
        fclose(fp);
        return SETG_ERR;
    }

    graph_init(g, num_courses, num_slots);

    for (int i = 0; i < num_courses; i++) {
        int idx;
        char name[MAX_NAME_LEN];

        if (fscanf(fp, "%d %63s", &idx, name) != 2 ||
            idx < 0 || idx >= num_courses) {
            SETG_ERR_LOG("Invalid course entry in '%s'.", path);
            fclose(fp);
            return SETG_ERR;
        }

        strncpy(g->course_names[idx], name, MAX_NAME_LEN - 1);
        g->course_names[idx][MAX_NAME_LEN - 1] = '\0';
    }

    int ns;
    if (fscanf(fp, "%d", &ns) != 1 || ns < 0 || ns > MAX_STUDENTS) {
        SETG_ERR_LOG("Invalid student count in '%s'.", path);
        fclose(fp);
        return SETG_ERR;
    }

    *num_students = ns;

    for (int s = 0; s < ns; s++) {
        StudentEnrollment *e = &enrollments[s];
        int sid, n;

        if (fscanf(fp, "%d %d", &sid, &n) != 2 ||
            n < 0 || n > MAX_COURSES) {
            SETG_ERR_LOG("Invalid student record in '%s'.", path);
            fclose(fp);
            return SETG_ERR;
        }

        e->student_id = sid;
        e->num_courses = 0;

        for (int c = 0; c < n; c++) {
            int cid;

            if (fscanf(fp, "%d", &cid) != 1 ||
                cid < 0 || cid >= num_courses) {
                SETG_ERR_LOG("Invalid course id for student %d in '%s'.",
                             sid, path);
                fclose(fp);
                return SETG_ERR;
            }

            e->course_ids[e->num_courses++] = cid;
        }
    }

    fclose(fp);
    return SETG_OK;
}

/* ───────────────────────── Output helpers ──────────────────────── */

static void print_dataset_header(const char *path)
{
    printf("\n============================================================\n");
    printf("Analyzing Dataset: %s\n", path);
    printf("============================================================\n");
}

static void print_dataset_summary(const Graph *g, int num_students)
{
    printf("Courses        : %d\n", g->num_courses);
    printf("Students       : %d\n", num_students);
    printf("Available slots: %d\n", g->num_slots);
    printf("Conflict edges : %d\n", g->edge_count);
    printf("------------------------------------------------------------\n");
}

static void print_result_row(const Timetable *t)
{
    printf("%-28s | %5d | %9d | %10.2f\n",
           t->label,
           t->num_colors_used,
           t->conflict_count,
           t->elapsed_ms);
}

static void print_analysis_table(Timetable results[], int count)
{
    printf("%-28s | %5s | %9s | %10s\n",
           "Algorithm", "Slots", "Conflicts", "Time (ms)");
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < count; i++) {
        print_result_row(&results[i]);
    }

    int best_idx = -1;
    int best_slots = MAX_SLOTS + 1;

    for (int i = 0; i < count; i++) {
        if (results[i].conflict_count == 0 &&
            results[i].num_colors_used > 0 &&
            results[i].num_colors_used < best_slots) {
            best_slots = results[i].num_colors_used;
            best_idx = i;
        }
    }

    printf("------------------------------------------------------------\n");

    if (best_idx >= 0) {
        printf("Best Result    : %s (%d slots, %.2f ms)\n",
               results[best_idx].label,
               results[best_idx].num_colors_used,
               results[best_idx].elapsed_ms);
    } else {
        printf("Best Result    : No conflict-free timetable found\n");
    }
}
