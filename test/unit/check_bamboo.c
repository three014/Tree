#include "../../include/arena.h"

#include <check.h>
#include <stdlib.h>

START_TEST(arena_allocates) {
    int32_t *a_single_number = arena_alloc(sizeof(int32_t));
    ck_assert_ptr_nonnull(a_single_number);

    *a_single_number = 4;
    arena_clear();
}
END_TEST

START_TEST(big_alloc) {
    size_t __big_number = 100000000;
    int64_t *arr = arena_alloc(sizeof(int64_t) * __big_number);
    ck_assert_ptr_nonnull(arr);
    arena_clear();
}
END_TEST

Suite *arena_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Hashmap");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, arena_allocates);
    tcase_add_test(tc_core, big_alloc);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int num_failed;
    Suite *s;
    SRunner *sr;

    s = arena_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    num_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    arena_delete();
    return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
