#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "test_common.h"

const Except_T Test_Failed = { "Test_Failed" };

typedef struct {
    const char *name;
    bool (*test_func)(void);
} TestCase;

extern bool test_heap(void);
extern bool test_kdtree(void);
extern bool test_sync(void);
extern bool test_atomic(void);
extern bool test_thread(void);
extern bool test_chan(void);
extern bool test_hash(void);

static TestCase test_registry[] = {
    { "heap", test_heap },
    { "kdtree", test_kdtree },
    { "sync", test_sync },
    { "atomic", test_atomic },
    { "thread", test_thread },
    { "chan", test_chan },
    { "hash", test_hash },
    { NULL, NULL }
};

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static bool run_test(const char *name, bool (*test_func)(void)) {
    printf("\n=== Running test: %s ===\n", name);
    tests_run++;

    bool result = test_func();

    if (result) {
        printf("✓ PASSED: %s\n", name);
        tests_passed++;
        return true;
    } else {
        printf("✗ FAILED: %s\n", name);
        tests_failed++;
        return false;
    }
}

static void run_all_tests(void) {
    printf("Running all tests...\n");
    printf("========================================\n");

    for (int i = 0; test_registry[i].name != NULL; i++) {
        run_test(test_registry[i].name, test_registry[i].test_func);
    }
}

static bool run_test_by_name(const char *name) {
    for (int i = 0; test_registry[i].name != NULL; i++) {
        if (strcmp(test_registry[i].name, name) == 0) {
            return run_test(name, test_registry[i].test_func);
        }
    }

    printf("Error: Test '%s' not found\n", name);
    printf("Available tests:\n");
    for (int i = 0; test_registry[i].name != NULL; i++) {
        printf("  - %s\n", test_registry[i].name);
    }
    return false;
}

static void list_tests(void) {
    printf("Available tests:\n");
    for (int i = 0; test_registry[i].name != NULL; i++) {
        printf("  - %s\n", test_registry[i].name);
    }
}

static void print_usage(const char *program_name) {
    printf("Usage: %s [options] [test_name]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help        Show this help message\n");
    printf("  -l, --list        List all available tests\n");
    printf("  -a, --all         Run all tests (default if no arguments)\n");
    printf("  --heap            Run heap tests only\n");
    printf("  --kdtree          Run kdtree tests only\n");
    printf("  --sync            Run sync (mutex/rwlock) tests only\n");
    printf("  --atomic          Run atomic ops tests only\n");
    printf("  --thread          Run thread tests only\n");
    printf("  --chan            Run channel tests only\n");
    printf("  --hash            Run hash function tests only\n");
    printf("\nExamples:\n");
    printf("  %s                    # Run all tests\n", program_name);
    printf("  %s --all              # Run all tests\n", program_name);
    printf("  %s heap               # Run specific test by name\n", program_name);
    printf("  %s --heap             # Run heap tests\n", program_name);
    printf("  %s --list             # List all tests\n", program_name);
}

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("  Gramarye Libcore Test Suite\n");
    printf("========================================\n");

    if (argc == 1) {
        run_all_tests();
    } else if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "--list") == 0) {
            list_tests();
            return 0;
        } else if (strcmp(argv[1], "-a") == 0 || strcmp(argv[1], "--all") == 0) {
            run_all_tests();
        } else if (strcmp(argv[1], "--heap") == 0) {
            run_test_by_name("heap");
        } else if (strcmp(argv[1], "--kdtree") == 0) {
            run_test_by_name("kdtree");
        } else if (strcmp(argv[1], "--sync") == 0) {
            run_test_by_name("sync");
        } else if (strcmp(argv[1], "--atomic") == 0) {
            run_test_by_name("atomic");
        } else if (strcmp(argv[1], "--thread") == 0) {
            run_test_by_name("thread");
        } else if (strcmp(argv[1], "--chan") == 0) {
            run_test_by_name("chan");
        } else if (strcmp(argv[1], "--hash") == 0) {
            run_test_by_name("hash");
        } else {
            run_test_by_name(argv[1]);
        }
    } else {
        printf("Error: Too many arguments\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("\n========================================\n");
    printf("Test Summary:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
