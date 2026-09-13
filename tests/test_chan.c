#include "test_common.h"
#include "chan.h"
#include "thread.h"
#include "except.h"
#include <string.h>

static void test_send_receive_single_message(void) {
    printf("  Testing single Chan_send/Chan_receive in a producer/consumer pair...\n");

    Chan_T c = Chan_new();
    TEST_ASSERT_NOT_NULL(c, "Chan_new should return a valid channel");

    int producer_apply(void *args) {
        Chan_T chan = (Chan_T)args;
        int value = 123;
        Chan_send(chan, &value, sizeof(value));
        return 0;
    }

    Thread_T producer = Thread_new(producer_apply, c, 0);

    int received = 0;
    int n = Chan_receive(c, &received, sizeof(received));

    TEST_ASSERT_EQ(n, sizeof(received), "Chan_receive should report the number of bytes transferred");
    TEST_ASSERT_EQ(received, 123, "Received value should match what the producer sent");

    Thread_join(producer);
}

#define NUM_MESSAGES 200

typedef struct {
    Chan_T chan;
} ProducerArgs;

static int multi_producer_apply(void *args) {
    ProducerArgs *a = (ProducerArgs *)args;
    for (int i = 0; i < NUM_MESSAGES; i++) {
        Chan_send(a->chan, &i, sizeof(i));
    }
    return 0;
}

static void test_ordered_stream_of_messages(void) {
    printf("  Testing an ordered stream of %d messages across two threads...\n", NUM_MESSAGES);

    Chan_T c = Chan_new();
    ProducerArgs args = { c };

    Thread_T producer = Thread_new(multi_producer_apply, &args, sizeof(args));

    for (int i = 0; i < NUM_MESSAGES; i++) {
        int received = -1;
        int n = Chan_receive(c, &received, sizeof(received));
        TEST_ASSERT_EQ(n, sizeof(received), "Each message should transfer sizeof(int) bytes");
        TEST_ASSERT_EQ(received, i, "Messages should arrive in the order they were sent (Chan is rendezvous/FIFO here)");
    }

    Thread_join(producer);
}

static void test_receive_smaller_buffer_truncates(void) {
    printf("  Testing Chan_receive with a smaller destination buffer truncates safely...\n");

    Chan_T c = Chan_new();

    int producer_apply(void *args) {
        Chan_T chan = (Chan_T)args;
        long value = 0x1122334455667788L;
        Chan_send(chan, &value, sizeof(value));
        return 0;
    }

    Thread_T producer = Thread_new(producer_apply, c, 0);

    int small_buf = 0;
    int n = Chan_receive(c, &small_buf, sizeof(small_buf));
    TEST_ASSERT_EQ(n, sizeof(small_buf), "Chan_receive should copy min(size, sent_size) bytes");

    Thread_join(producer);
}

static void test_multiple_round_trips_same_channel(void) {
    printf("  Testing multiple sequential round trips reusing the same channel...\n");

    Chan_T c = Chan_new();

    int producer_apply(void *args) {
        Chan_T chan = (Chan_T)args;
        for (int i = 0; i < 10; i++) {
            int v = i * i;
            Chan_send(chan, &v, sizeof(v));
        }
        return 0;
    }

    Thread_T producer = Thread_new(producer_apply, c, 0);

    for (int i = 0; i < 10; i++) {
        int received;
        Chan_receive(c, &received, sizeof(received));
        TEST_ASSERT_EQ(received, i * i, "Round trip value should be the square of the index");
    }

    Thread_join(producer);
}

bool test_chan(void) {
    printf("\n=== Chan Tests ===\n");

    /* Chan uses Sem internally, which relies on Thread_self() identity. */
    Thread_init(0);

    bool result = true;
    TRY
        test_send_receive_single_message();
        test_ordered_stream_of_messages();
        test_receive_smaller_buffer_truncates();
        test_multiple_round_trips_same_channel();
    EXCEPT(Test_Failed)
        result = false;
    END_TRY;

    return result;
}
