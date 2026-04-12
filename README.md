# SETG – Smart Exam Timetable Generator

## Problem Statement
SETG models exam scheduling as a graph coloring problem.  
Each course is represented as a node, and an edge exists between two courses if at least one student is enrolled in both. The aim is to assign exam slots so that no conflicting courses are scheduled at the same time.

## Objectives
- Model exam scheduling using graph theory
- Implement multiple coloring strategies
- Compare exact and heuristic approaches
- Generate conflict-free exam timetables

## Algorithms Implemented
1. Greedy Graph Coloring
2. Exact Graph Coloring using Backtracking
3. DSATUR / Optimized Coloring
4. Performance Comparison on multiple datasets

## Team Structure
- Ananya: Graph construction + Greedy Coloring + Integration
- Shivani: Exact Backtracking Coloring
- Ishaan: DSATUR / Optimized Coloring
- Tanya: Dataset Generation + Analysis

## Project Structure
- `common.h` → shared declarations
- `main.c` → driver code
- `greedy_core.c` → graph construction and greedy coloring
- `exact_backtracking.c` → exact coloring
- `dsatur_optimized.c` → optimized heuristic
- `analysis_dataset.c` → datasets and performance analysis

## Compilation
```bash
gcc main.c greedy_core.c exact_backtracking.c dsatur_optimized.c analysis_dataset.c -o setg
./setg