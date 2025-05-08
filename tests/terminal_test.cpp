#include <gtest/gtest.h>
#include "../src/Buffer.hpp"
#include <utility>

class BufferTest : public testing::Test {
protected:

    TermBuffer buffer{900, 600, 20, 10};
};

TEST_F(BufferTest, MaxYTest) {
    ASSERT_EQ(buffer.get_max_y(), 0);
}

TEST_F(BufferTest, MaxYTestAfterAdding) {
    auto width = buffer.get_buffer().size();
    for (auto i = 0; i <= width; ++i) {
        buffer.add_cells({Cell{}});
    }
    ASSERT_EQ(buffer.get_max_y(), 1);
}

TEST_F(BufferTest, TestCursorPos) {
    auto width = buffer.get_buffer()[0].size();
    for (auto i = 0; i < width; ++i) {
        buffer.add_cells({Cell{42}});
    }
    ASSERT_EQ(buffer.get_cursor_pos().first, 0);
    ASSERT_EQ(buffer.get_cursor_pos().second, 1);
}
TEST_F(BufferTest, TestNewLine) {
    buffer.add_cells({Cell{32}, Cell{'\n'}});
    auto cursor_pos = buffer.get_cursor_pos();
    ASSERT_EQ(cursor_pos.first, 0);
    ASSERT_EQ(cursor_pos.second, 1);
}