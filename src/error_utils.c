#include "error_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* List of all keywords from lexer.l */
static const char *keywords[] = {
    "int", "void", "char", "const", "struct", "virtual", "class",
    "public", "private", "if", "else", "while", "for", "return",
    "switch", "case", "default", "break", "continue", "printf",
    "scanf", "new", "delete", "try", "catch", "throw", ":"
};
static const int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

/* Compute Levenshtein distance between two strings */
int levenshtein_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int matrix[len1 + 1][len2 + 1];

    for (int i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }
    for (int j = 0; j <= len2; j++) {
        matrix[0][j] = j;
    }

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            matrix[i][j] = matrix[i - 1][j - 1] + cost; // substitution
            if (matrix[i][j] > matrix[i - 1][j] + 1) matrix[i][j] = matrix[i - 1][j] + 1; // deletion
            if (matrix[i][j] > matrix[i][j - 1] + 1) matrix[i][j] = matrix[i][j - 1] + 1; // insertion
        }
    }

    return matrix[len1][len2];
}

/* Find the closest keyword to a typo, within max_distance edits */
const char* find_closest_keyword(const char *typo, int max_distance) {
    int min_distance = max_distance + 1;
    const char *closest = NULL;

    for (int i = 0; i < num_keywords; i++) {
        int dist = levenshtein_distance(typo, keywords[i]);
        if (dist < min_distance) {
            min_distance = dist;
            closest = keywords[i];
        }
    }

    return (min_distance <= max_distance) ? closest : NULL;
}

/* Enhanced semantic error with suggestion */
void semantic_error_with_suggestion(int line, const char *msg, const char *typo) {
    printf("Semantic Error (line %d): %s\n", line, msg);
    const char *suggestion = find_closest_keyword(typo, 2); // threshold of 2 edits
    if (suggestion) {
        printf("Did you mean '%s'?\n", suggestion);
    }
}