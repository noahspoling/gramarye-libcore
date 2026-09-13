#include "test_common.h"
#include "except.h"
#include "table.h"
#include <string.h>
#include <stdint.h>

/* int_hash.h, double_coord_hash.h, int_coord_hash.h, and uuid_hash.h all
   declare ordinary externally-linked functions -- call them directly. */
#include "int_hash.h"
#include "double_coord_hash.h"
#include "int_coord_hash.h"
#include "hash/uuid_hash.h"

/* simple_string_hash.h defines str_cmp/str_hash inline (full bodies, not
   just prototypes), so including it is enough to use them directly. */
#include "simple_string_hash.h"

static void test_int_hash_cmp(void) {
    printf("  Testing int_cmp/int_hash pair...\n");

    int a = 5, b = 5, c = 9;
    TEST_ASSERT_EQ(int_cmp(&a, &b), 0, "Equal ints should compare equal (0)");
    TEST_ASSERT_NE(int_cmp(&a, &c), 0, "Different ints should not compare equal");
    TEST_ASSERT_EQ(int_hash(&a), int_hash(&b), "Equal ints must hash identically");
    TEST_ASSERT_EQ(int_hash(&a), (unsigned)a, "int_hash should just be the int value itself");
}

static void test_double_coord_hash_cmp(void) {
    printf("  Testing double_coord_cmp/double_coord_hash pair...\n");

    double_coord_t p1 = { 1.5, 2.5 };
    double_coord_t p2 = { 1.5, 2.5 };
    double_coord_t p3 = { 1.5, 9.9 };

    TEST_ASSERT_EQ(double_coord_cmp(&p1, &p2), 0, "Equal coords should compare equal");
    TEST_ASSERT_NE(double_coord_cmp(&p1, &p3), 0, "Different coords should not compare equal");
    TEST_ASSERT_EQ(double_coord_hash(&p1), double_coord_hash(&p2), "Equal coords must hash identically");
}

static void test_int_coord_hash_cmp(void) {
    printf("  Testing IntCoord_hash/IntCoord_cmp pair...\n");

    IntCoord a = { 3, 4 };
    IntCoord b = { 3, 4 };
    IntCoord c = { 3, 5 };
    IntCoord d = { 4, 4 };

    TEST_ASSERT_EQ(IntCoord_cmp(&a, &b), 0, "Equal IntCoords should compare equal");
    TEST_ASSERT_NE(IntCoord_cmp(&a, &c), 0, "IntCoords differing in y should not compare equal");
    TEST_ASSERT_NE(IntCoord_cmp(&a, &d), 0, "IntCoords differing in x should not compare equal");
    TEST_ASSERT_EQ(IntCoord_hash(&a), IntCoord_hash(&b), "Equal IntCoords must hash identically");
}

static void test_uuid_hash(void) {
    printf("  Testing hash_uuid/hash_uuid_bytes...\n");

    uuid_key_t u1 = { 0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL };
    uuid_key_t u2 = { 0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL };
    uuid_key_t u3 = { 0x1122334455667788ULL, 0x0000000000000001ULL };

    TEST_ASSERT_EQ(hash_uuid(u1), hash_uuid(u2), "Equal UUIDs should hash identically (hash_uuid)");
    TEST_ASSERT_NE(hash_uuid(u1), hash_uuid(u3), "Different UUIDs should (very likely) hash differently");

    TEST_ASSERT_EQ(hash_uuid_bytes(u1), hash_uuid_bytes(u2), "Equal UUIDs should hash identically (hash_uuid_bytes)");
    TEST_ASSERT_NE(hash_uuid_bytes(u1), hash_uuid_bytes(u3), "Different UUIDs should (very likely) hash differently (bytes variant)");

    /* zero uuid should not crash and should be self-consistent */
    uuid_key_t zero = { 0, 0 };
    TEST_ASSERT_EQ(hash_uuid(zero), hash_uuid(zero), "Hashing should be a pure/deterministic function of its input");
}

static void test_string_hash_cmp(void) {
    printf("  Testing str_cmp/str_hash pair...\n");

    const char *s1 = "hello world";
    const char *s2 = "hello world";
    const char *s3 = "goodbye world";

    TEST_ASSERT_EQ(str_cmp(s1, s2), 0, "Equal strings should compare equal");
    TEST_ASSERT_NE(str_cmp(s1, s3), 0, "Different strings should not compare equal");
    TEST_ASSERT_EQ(str_hash(s1), str_hash(s2), "Equal strings must hash identically");

    TEST_ASSERT_EQ(str_hash(""), 5381u, "djb2 hash of empty string should be the seed value 5381");
}

static void test_table_integration_int_coord(void) {
    printf("  Testing IntCoord_hash/IntCoord_cmp plugged into a real Table_T...\n");

    Table_T table = Table_new(0, IntCoord_cmp, IntCoord_hash);
    TEST_ASSERT_NOT_NULL(table, "Table_new should succeed");

    IntCoord keys[] = { {0, 0}, {1, 0}, {0, 1}, {5, 5}, {-3, 7} };
    int values[] = { 100, 200, 300, 400, 500 };

    for (int i = 0; i < 5; i++) {
        Table_put(table, &keys[i], &values[i]);
    }
    TEST_ASSERT_EQ(Table_length(table), 5, "Table should contain all 5 distinct-key entries");

    for (int i = 0; i < 5; i++) {
        int *found = (int *)Table_get(table, &keys[i]);
        TEST_ASSERT_NOT_NULL(found, "Every inserted key should be retrievable");
        TEST_ASSERT_EQ(*found, values[i], "Retrieved value should match what was inserted for that key");
    }

    /* A distinct-but-equal-valued key object should still find the same entry. */
    IntCoord same_as_first = { 0, 0 };
    int *found_alias = (int *)Table_get(table, &same_as_first);
    TEST_ASSERT_NOT_NULL(found_alias, "A structurally-equal key should hit the same table entry");
    TEST_ASSERT_EQ(*found_alias, 100, "Structurally-equal key lookup should return the original value");

    IntCoord missing = { 999, 999 };
    TEST_ASSERT_NULL(Table_get(table, &missing), "A key that was never inserted should not be found");

    int *removed = (int *)Table_remove(table, &keys[0]);
    TEST_ASSERT_NOT_NULL(removed, "Removing an existing key should return its value");
    TEST_ASSERT_EQ(Table_length(table), 4, "Table length should decrease after removal");
    TEST_ASSERT_NULL(Table_get(table, &keys[0]), "Removed key should no longer be found");

    Table_free(&table);
}

static void test_table_integration_string(void) {
    printf("  Testing str_cmp/str_hash plugged into a real Table_T...\n");

    Table_T table = Table_new(0, str_cmp, str_hash);

    const char *k1 = "alpha";
    const char *k2 = "beta";
    const char *k3 = "gamma";
    int v1 = 1, v2 = 2, v3 = 3;

    Table_put(table, k1, &v1);
    Table_put(table, k2, &v2);
    Table_put(table, k3, &v3);

    TEST_ASSERT_EQ(Table_length(table), 3, "Table should contain 3 entries");

    /* Look up with a different (heap-distinct) string with the same contents. */
    char lookup_key[16];
    strcpy(lookup_key, "beta");
    int *found = (int *)Table_get(table, lookup_key);
    TEST_ASSERT_NOT_NULL(found, "String lookup should use str_cmp for equality, not pointer identity");
    TEST_ASSERT_EQ(*found, 2, "Looked-up value should match what was stored for that string");

    Table_free(&table);
}

bool test_hash(void) {
    printf("\n=== Hash Function Tests ===\n");

    bool result = true;
    TRY
        test_int_hash_cmp();
        test_double_coord_hash_cmp();
        test_int_coord_hash_cmp();
        test_uuid_hash();
        test_string_hash_cmp();
        test_table_integration_int_coord();
        test_table_integration_string();
    EXCEPT(Test_Failed)
        result = false;
    END_TRY;

    return result;
}
