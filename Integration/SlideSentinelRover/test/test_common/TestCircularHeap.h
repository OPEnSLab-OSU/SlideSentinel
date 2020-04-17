//
// Created by Noah on 4/17/2020.
//
#if !defined(SLIDESENTINELROVER_TESTCIRCULARHEAP_H) && defined(UNIT_TEST)
#define SLIDESENTINELROVER_TESTCIRCULARHEAP_H

#include <array>
#include "CircularHeap.h"

void TestCircularHeapInitial() {
    CircularHeap<64> heap;

    TEST_ASSERT_EQUAL(heap.size(), 0);
}

void TestCircularHeapTestAllocate() {
    CircularHeap<64> heap;

    using TestType1 = struct {
        std::array<char, 25> data{ { 0, 1, 2, 4, 5, 63 } };
    };

    using TestType2 = struct {
        float thing = 0.5f;
        int otherthing = 2;
        char superthing = 'k';
        char superotherthing = 'l';
        double why = 3.14;
        bool t = false;
    };

    using TestType3 = struct {

    };

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1), heap.size());
    const TestType1* ptr = static_cast<const TestType1*>(heap.get_front());
    TEST_ASSERT_TRUE(ptr->data == TestType1{}.data);

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType2>());
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) + sizeof(TestType2), heap.size());
    const TestType1* ptr2 = static_cast<const TestType1*>(heap.get_front());
    TEST_ASSERT_TRUE(ptr2->data == TestType1{}.data);

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType3>());
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) + sizeof(TestType2) + sizeof(TestType3), heap.size());
    const TestType1* ptr3 = static_cast<const TestType1*>(heap.get_front());
    TEST_ASSERT_TRUE(ptr3->data == TestType1{}.data);
}

void TestCircularHeapDeAllocate() {
    CircularHeap<64> heap;

    using TestType1 = struct {
        std::array<char, 25> data{ { 0, 1, 2, 4, 5, 63 } };
    };

    using TestType2 = struct {
        float thing = 0.5f;
        int otherthing = 2;
        char superthing = 'k';
        char superotherthing = 'l';
        double why = 3.14;
        bool t = false;
    };

    using TestType3 = struct {};

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType2>());
    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType3>());

    const TestType1* ptr = static_cast<const TestType1*>(heap.get_front());
    TEST_ASSERT_TRUE(ptr->data == TestType1{}.data);

    heap.deallocate_pop_front();
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType2) + sizeof(TestType3), heap.size());
    const TestType2* ptr2 = static_cast<const TestType2*>(heap.get_front());
    const TestType2 test{};
    TEST_ASSERT_EQUAL_FLOAT(ptr2->thing, test.thing);
    TEST_ASSERT_EQUAL(ptr2->otherthing, test.otherthing);
    TEST_ASSERT_EQUAL_CHAR(ptr2->superthing, test.superthing);
    TEST_ASSERT_EQUAL_CHAR(ptr2->superotherthing, test.superotherthing);
    TEST_ASSERT_EQUAL_FLOAT(ptr2->why, test.why);
    TEST_ASSERT_EQUAL(ptr2->t, test.t);

    heap.deallocate_pop_front();
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType3), heap.size());
    const TestType3* ptr3 = static_cast<const TestType3*>(heap.get_front());
    TEST_ASSERT_NOT_NULL(ptr3);

    heap.deallocate_pop_front();
    TEST_ASSERT_EQUAL(heap.size(), 0);
}

void TestCircularHeapCircular() {
    CircularHeap<64> heap;

    using TestType1 = struct {
        std::array<char, 25> data{ { 0, 1, 2, 4, 5, 63 } };
    };

    using TestType2 = struct {
        float thing = 0.5f;
        int otherthing = 2;
        char superthing = 'k';
        char superotherthing = 'l';
        double why = 3.14;
        bool t = false;
    };

    using TestType3 = struct {

    };

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType2>());
    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType3>());

    heap.deallocate_pop_front();
    heap.deallocate_pop_front();
    heap.deallocate_pop_front();

    TEST_ASSERT_GREATER_OR_EQUAL(0, heap.size());
    // make sure the heap starts in the middle
    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType3>());

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
    heap.deallocate_pop_front();
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1), heap.size());
    const TestType1* ptr = static_cast<const TestType1*>(heap.get_front());
    TEST_ASSERT_TRUE(ptr->data == TestType1{}.data);

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType2>());
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) + sizeof(TestType2), heap.size());
    const TestType1* ptr2 = static_cast<const TestType1*>(heap.get_front());
    TEST_ASSERT_TRUE(ptr2->data == TestType1{}.data);

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType3>());
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) + sizeof(TestType2) + sizeof(TestType3), heap.size());
    const TestType1* ptr3 = static_cast<const TestType1*>(heap.get_front());
    TEST_ASSERT_TRUE(ptr3->data == TestType1{}.data);
}

void TestCircularHeapFill() {
    CircularHeap<64> heap;

    using TestType1 = struct {
        std::array<char, 25> data{ { 0, 1, 2, 4, 5, 63 } };
    };

    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
    TEST_ASSERT_FALSE(heap.allocate_push_back<TestType1>());
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) * 2, heap.size());

    heap.deallocate_pop_front();
    TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) * 2, heap.size());
}

void TestCircularHeapLong() {
    CircularHeap<2048> heap;

    using TestType1 = struct {
        std::array<char, 25> data{ { 0, 1, 2, 4, 5, 63 } };
    };

    using TestType2 = struct {
        std::array<long, 27> data{ { 63, 5, 4, 3, 2, 1, 0 } };
    };

    // empty and fill the buffer 10 times
    for (size_t o = 0; o < 10; o++) {
        TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
        TEST_ASSERT_TRUE(heap.allocate_push_back<TestType2>());

        // cycle though obj1 -> obj2 1000 times
        for (size_t i = 0; i < 1000; i++) {
            constexpr TestType1 type1Test;
            constexpr TestType2 type2Test;
            TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) + sizeof(TestType2), heap.size());
            // first object
            const TestType1 *ptr1 = static_cast<const TestType1 *>(heap.get_front());
            TEST_ASSERT_TRUE(ptr1->data == type1Test.data);
            heap.deallocate_pop_front();
            TEST_ASSERT_TRUE(heap.allocate_push_back<TestType1>());
            TEST_ASSERT_GREATER_OR_EQUAL(sizeof(TestType1) + sizeof(TestType2), heap.size());
            // second object
            const TestType2 *ptr2 = static_cast<const TestType2 *>(heap.get_front());
            TEST_ASSERT_TRUE(ptr2->data == type2Test.data);
            heap.deallocate_pop_front();
            TEST_ASSERT_TRUE(heap.allocate_push_back<TestType2>());
        }

        // deallocate both the events
        heap.deallocate_pop_front();
        heap.deallocate_pop_front();

        // check empty
        TEST_ASSERT_EQUAL(heap.size(), 0);
    }
}

#endif //SLIDESENTINELROVER_TESTCIRCULARHEAP_H
