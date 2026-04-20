#ifndef ERROR_UTILS_H
#define ERROR_UTILS_H

#include <stdio.h>

/* Compute Levenshtein distance between two strings */
int levenshtein_distance(const char *s1, const char *s2);

/* Find the closest keyword to a typo, within max_distance edits */
const char* find_closest_keyword(const char *typo, int max_distance);

/* Enhanced semantic error with suggestion */
void semantic_error_with_suggestion(int line, const char *msg, const char *typo);

#endif