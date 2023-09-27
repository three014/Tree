#include "__hashmap_private.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

START_TEST(new_hashmap_works) {
    hashmap_t *map = hashmap_new();
    ck_assert_ptr_nonnull(map);
    hashmap_delete(map, NULL);
}
END_TEST

START_TEST(hash_works) {
    size_t len = 10;
    size_t seed = 0;
    size_t index = __calc_index(seed, 25, len);
    ck_assert(index < len);
}
END_TEST

START_TEST(map_add_get) {
    int *data = malloc(sizeof(int));
    hashmap_t *map = hashmap_new();
    hashmap_insert(map, 5, data);
    int *got = hashmap_get(map, 5);
    ck_assert_int_eq(*data, *got);
    ck_assert_ptr_eq(data, got);
    hashmap_delete(map, NULL);
    free(data);
}
END_TEST

static uint32_t __counter = 0;
void __special_free(void *ptr) {
    __counter--;
    double *actual_ptr = ptr;
    free(actual_ptr);
}

START_TEST(map_delete_andfree) {
    hashmap_t *map = hashmap_new();

    __counter = 30;
    for (size_t i = 0; i < 30; i++) {
        double *d = malloc(sizeof(double));
        *d = 739.1234;
        (void)hashmap_insert(map, i, d);
    }

    hashmap_delete(map, __special_free);
    ck_assert_uint_eq(__counter, 0);
}
END_TEST

Suite *hashmap_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Hashmap");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, new_hashmap_works);
    tcase_add_test(tc_core, hash_works);
    tcase_add_test(tc_core, map_add_get);
    tcase_add_test(tc_core, map_delete_andfree);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int num_failed;
    Suite *s;
    SRunner *sr;

    s = hashmap_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    num_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
