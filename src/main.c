#include "arena.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void asdadfda() {
    printf("Enter the number of numbers you want: \n");
    int number_of_numbers = 0;
    scanf("%d", &number_of_numbers);

    int *array = malloc(sizeof(int) * number_of_numbers);
    char *letter = malloc(sizeof(char));
    char *other_letter = malloc(sizeof(char));

    free(array);
    free(letter);
    free(other_letter);
}


void kjbihebgeiubg() {
    printf("Enter the number of numbers you want: \n");
    int number_of_numbers = 0;
    scanf("%d", &number_of_numbers);

    arena_t *arena = arena_new();
    int *array = arena_alloc(arena, sizeof(int) * number_of_numbers);
    char *letter = arena_alloc(arena, sizeof(char));
    char *other_letter = arena_alloc(arena, sizeof(char));

    arena_delete(arena);
}

int bar(arena_temp_t *temp_arena) {
    double *number = arena_temp_alloc(temp_arena, sizeof(double));
    *number = 12.986954;
    return 23;
}

char *foo(arena_temp_t *a) {
    char *str = "This is a test string!";
    char *ret = arena_temp_alloc(a, sizeof(char) * 23);
    arena_temp_t *b = arena_temp_from_temp(a);
    int random = bar(b);
    arena_temp_delete(b);
    strncpy(ret, str, 23);
    return ret;
}

int main(void) {
    arena_t *arena = arena_new();

    int *number = arena_alloc(arena, sizeof(int));
    *number = 7;

    char *str = arena_alloc(arena, sizeof(char) * 5000);
    strncpy(str, "Hello World!", 28);

    printf("%s\n", str);
    printf("%d\n", *number);

    struct Test {
        int a;
        double b;
        char c;
    };

    struct Test test = {.a = 3, .b = 85.099023, .c = 'g'};
    struct Test *t = arena_alloc(arena, sizeof(struct Test));
    *t = test;

    arena_temp_t *tmp = arena_temp_new(arena);
    printf("%s\n", foo(tmp));
    arena_temp_delete(tmp);

    int *otro = arena_alloc(arena, sizeof(int));
    *otro = 9;
    printf("%d\n", *otro);

    arena_delete(arena);


    return 0;
}