#include "__hashmap_private.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

START_TEST(new_hashmap_works) {
  hashmap_t *map = hashmap_new();
  ck_assert(map != NULL);
}
END_TEST

START_TEST(hash_works) {
  size_t len = 10;
  size_t seed = 0;
  size_t index = __calc_index(seed, 25, len);
  ck_assert(index < len);
}
END_TEST

Suite *hashmap_suite(void) {
  Suite *s;
  TCase *tc_core;

  s = suite_create("Hashmap");

  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, new_hashmap_works);
  tcase_add_test(tc_core, hash_works);
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
