#include "CircularBuffer.h"
#include "unity.h"

#ifdef UNIT_TEST

class Int {
public:
	Int(int i, int& dcount) : m_i(i), m_dcount(dcount) { m_dcount++; }
	Int(const Int& rhs) : m_i(rhs.m_i), m_dcount(rhs.m_dcount) { m_dcount++; }
	Int(Int&& rhs) = delete;
	~Int() { m_dcount--; }

	int m_i;
	int& m_dcount;
};

constexpr static int m_answer[] = { 8, 7, 6, 5, 1, 2, 3, 4 };
static CircularBuffer<Int, 8> m_buf;
static int m_dcount = 0;

static void CircularBufferSetup() {
    m_buf.reset();
    m_dcount = 0;
    
    m_buf.emplace_back(1, m_dcount);
    m_buf.emplace_back(2, m_dcount);
    m_buf.add_back(Int(3, m_dcount));
    m_buf.add_back(Int(4, m_dcount));
        
    m_buf.emplace_front(5, m_dcount);
    m_buf.emplace_front(6, m_dcount);
    m_buf.add_front(Int(7, m_dcount));
    m_buf.add_front(Int(8, m_dcount));
}

void TestInsertion() {
    int dcount = 0;
    {
        CircularBuffer<Int, 8> buf;
		TEST_ASSERT_FALSE(buf.full());
		TEST_ASSERT_TRUE(buf.empty());

		TEST_ASSERT_TRUE(buf.emplace_back(1, dcount));
		TEST_ASSERT_TRUE(buf.emplace_back(2, dcount));

		TEST_ASSERT_TRUE(buf.add_back(Int(3, dcount)));
		TEST_ASSERT_TRUE(buf.add_back(Int(4, dcount)));

		TEST_ASSERT_TRUE(buf.emplace_front(5, dcount));
		TEST_ASSERT_TRUE(buf.emplace_front(6, dcount));

		TEST_ASSERT_TRUE(buf.add_front(Int(7, dcount)));
		TEST_ASSERT_TRUE(buf.add_front(Int(8, dcount)));

		TEST_ASSERT_TRUE(buf.full());
		TEST_ASSERT_FALSE(buf.empty());

		TEST_ASSERT_FALSE(buf.emplace_back(0, dcount));
		TEST_ASSERT_FALSE(buf.add_back(Int(0, dcount)));

		TEST_ASSERT_FALSE(buf.emplace_front(0, dcount));
		TEST_ASSERT_FALSE(buf.add_front(Int(0, dcount)));

        TEST_ASSERT_EQUAL_MESSAGE(buf.size(), 8, "miscalculated size!");

        constexpr int answer[] = { 8, 7, 6, 5, 1, 2, 3, 4 };
		for (auto i = 0U; i < buf.size(); i++)
			TEST_ASSERT_EQUAL_MESSAGE(answer[i], buf[i].m_i, "Unexpected insertion value at index");
    }
    TEST_ASSERT_EQUAL_MESSAGE(dcount, 0, "improper cleanup");
}

void TestIteration() {
    CircularBufferSetup();

    int k = 0;
	for (Int& temp : m_buf) {
		TEST_ASSERT_EQUAL(m_answer[k], temp.m_i);
		k++;
	}
	k = 0;
	for (const Int& temp : m_buf.crange()) {
		TEST_ASSERT_EQUAL(m_answer[k], temp.m_i);
		k++;
	}
}

void TestRemove() {
    CircularBufferSetup();

    auto iter = m_buf.begin();
	++iter; ++iter; ++iter; ++iter;
	m_buf.remove(iter);
	TEST_ASSERT_EQUAL(m_buf.size(), 7);
	constexpr int answer[] = { 8, 7, 6, 5, 2, 3, 4 };
	for (auto i = 0U; i < m_buf.size(); i++)
		TEST_ASSERT_EQUAL(answer[i], m_buf[i].m_i);
		
	auto citer = m_buf.crange().begin();
	++citer; ++citer; ++citer;
	m_buf.remove(citer);
	TEST_ASSERT_EQUAL(m_buf.size(), 6);
	constexpr int answer2[] = { 8, 7, 6, 2, 3, 4 };
	for (auto i = 0U; i < m_buf.size(); i++)
		TEST_ASSERT_EQUAL(answer2[i], m_buf[i].m_i);

	TEST_ASSERT_TRUE(m_buf.destroy_back());
	constexpr int answer3[] = { 8, 7, 6, 2, 3 };
	for (auto i = 0U; i < m_buf.size(); i++)
		TEST_ASSERT_EQUAL(answer3[i], m_buf[i].m_i);

	TEST_ASSERT_TRUE(m_buf.destroy_front());
	constexpr int answer4[] = { 7, 6, 2, 3 };
	for (auto i = 0U; i < m_buf.size(); i++)
		TEST_ASSERT_EQUAL(answer4[i], m_buf[i].m_i);

	m_buf.reset();
	TEST_ASSERT_EQUAL(m_dcount, 0);

	TEST_ASSERT_FALSE(m_buf.destroy_back());
	TEST_ASSERT_FALSE(m_buf.destroy_front());
}

void TestFront() {
    CircularBufferSetup();
    
    TEST_ASSERT_EQUAL(m_buf.front().m_i, m_answer[0]);
	TEST_ASSERT_EQUAL(m_buf.back().m_i, m_answer[7]);
}

#endif