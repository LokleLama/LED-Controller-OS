// Tests for dotMatrix8x8 device with console-emulated output
#include <gtest/gtest.h>

#include "devices/dotMatrix8x8.h"
#include "devices/WS2812.h"
#include "devices/PIODevice.h"
#include "devices/MatrixChar8x8.h"
#include "Mainloop.h"

#include "mock_pio.h"
#include "dot_matrix_renderer.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

class DotMatrixTest : public ::testing::Test {
protected:
    std::shared_ptr<PIODevice> pio;
    std::shared_ptr<WS2812> ws2812;

    void SetUp() override {
        mock_pio_reset();
        Mainloop::getInstance().reset();

        pio = std::make_shared<PIODevice>(0);
        ws2812 = std::make_shared<WS2812>(pio, /*pin=*/0, /*num_leds=*/64);
        // PIODevice must be Assigned for transfer() to work
        pio->assignToUser(ws2812);
    }

    void TearDown() override {
        Mainloop::getInstance().reset();
        mock_pio_reset();
    }

    // Advance mainloop enough to trigger the 100ms scrolling task
    void triggerScroll(int times = 1) {
        for (int i = 0; i < times; i++) {
            Mainloop::getInstance().tick(100);
        }
    }
};

// ---------------------------------------------------------------------------
//  Construction and initialization
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, Construction) {
    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test_matrix", "A");
    EXPECT_EQ(matrix->getName(), "test_matrix");
    EXPECT_EQ(matrix->getType(), "dotMatrix8x8");
    EXPECT_EQ(matrix->getStatus(), IDevice::DeviceStatus::Initialized);
}

// ---------------------------------------------------------------------------
//  Single character rendering — verify the 'A' glyph appears on the matrix
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, RenderSingleCharA) {
    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "A");
    triggerScroll();

    auto& frame = mock_pio_get_last_frame();
    ASSERT_EQ(frame.size(), 64u);

    // Print the rendered frame to console
    DotMatrixRenderer::print(frame, "Character 'A'");

    // Verify the frame matches the MatrixChar8x8 font for 'A'
    auto actual = DotMatrixRenderer::toGrid(frame);
    auto expected = DotMatrixRenderer::getExpectedChar(MatrixChar8x8::getChar('A'));

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            EXPECT_EQ(actual[row][col], expected[row][col])
                << "Mismatch at row=" << row << " col=" << col;
        }
    }
}

// ---------------------------------------------------------------------------
//  Render various characters and print them to console
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, RenderMultipleChars) {
    const std::string chars = "HELLO";
    std::cout << "\n=== Individual character glyphs ===" << std::endl;

    for (char c : chars) {
        std::string s(1, c);
        mock_pio_reset();
        Mainloop::getInstance().reset();
        pio = std::make_shared<PIODevice>(0);
        ws2812 = std::make_shared<WS2812>(pio, 0, 64);
        pio->assignToUser(ws2812);

        auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", s);
        triggerScroll();

        auto& frame = mock_pio_get_last_frame();
        if (frame.size() == 64) {
            DotMatrixRenderer::print(frame, std::string("Character '") + c + "'");

            auto actual = DotMatrixRenderer::toGrid(frame);
            auto expected = DotMatrixRenderer::getExpectedChar(MatrixChar8x8::getChar(c));
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    EXPECT_EQ(actual[row][col], expected[row][col])
                        << "Char '" << c << "' mismatch at row=" << row << " col=" << col;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  Scrolling animation — print successive frames of scrolling text
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, ScrollingAnimation) {
    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "Hi");

    std::cout << "\n=== Scrolling animation: \"Hi\" ===" << std::endl;

    // Capture and display several scroll steps
    for (int step = 0; step < 18; step++) {
        triggerScroll();
        auto& frame = mock_pio_get_last_frame();
        if (frame.size() == 64) {
            std::cout << "--- Step " << step << " ---" << std::endl;
            std::cout << DotMatrixRenderer::render(frame);
        }
    }
    // Just verify frames are being produced
    EXPECT_EQ(mock_pio_get_last_frame().size(), 64u);
}

// ---------------------------------------------------------------------------
//  setValue changes the displayed text
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, SetValueChangesText) {
    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "X");
    triggerScroll();
    auto frame1 = mock_pio_get_last_frame();  // copy
    ASSERT_EQ(frame1.size(), 64u);

    // Change text
    matrix->setValue("O");
    triggerScroll();
    auto& frame2 = mock_pio_get_last_frame();
    ASSERT_EQ(frame2.size(), 64u);

    // The two frames should differ
    EXPECT_NE(frame1, frame2);

    std::cout << std::endl;
    DotMatrixRenderer::print(frame1, "Before: 'X'");
    DotMatrixRenderer::print(frame2, "After: 'O'");
}

// ---------------------------------------------------------------------------
//  Color is applied to lit LEDs
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, ColorApplied) {
    uint32_t color = 0x00FF0000;  // red
    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "A", color);
    triggerScroll();

    auto& frame = mock_pio_get_last_frame();
    ASSERT_EQ(frame.size(), 64u);

    // Every lit pixel should have exactly the specified color
    for (auto pixel : frame) {
        if (pixel != 0) {
            EXPECT_EQ(pixel, color);
        }
    }
}

// ---------------------------------------------------------------------------
//  Digits render correctly
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, RenderDigits) {
    std::cout << "\n=== Digit glyphs ===" << std::endl;

    for (char c = '0'; c <= '9'; c++) {
        mock_pio_reset();
        Mainloop::getInstance().reset();
        pio = std::make_shared<PIODevice>(0);
        ws2812 = std::make_shared<WS2812>(pio, 0, 64);
        pio->assignToUser(ws2812);

        auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", std::string(1, c));
        triggerScroll();

        auto& frame = mock_pio_get_last_frame();
        if (frame.size() == 64) {
            DotMatrixRenderer::print(frame, std::string("Digit '") + c + "'");
            auto actual = DotMatrixRenderer::toGrid(frame);
            auto expected = DotMatrixRenderer::getExpectedChar(MatrixChar8x8::getChar(c));
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    EXPECT_EQ(actual[row][col], expected[row][col])
                        << "Digit '" << c << "' mismatch at row=" << row << " col=" << col;
                }
            }
        }
    }
}

// ===========================================================================
//  8xN daisy-chain tests — multiple modules
// ===========================================================================

class DotMatrix8xNTest : public ::testing::Test {
protected:
    int num_modules;
    int total_leds;
    int display_cols;
    std::shared_ptr<PIODevice> pio;
    std::shared_ptr<WS2812> ws2812;

    void createChain(int modules) {
        num_modules = modules;
        total_leds = modules * 64;
        display_cols = modules * 8;
        mock_pio_reset();
        Mainloop::getInstance().reset();

        pio = std::make_shared<PIODevice>(0);
        ws2812 = std::make_shared<WS2812>(pio, 0, total_leds);
        pio->assignToUser(ws2812);
    }

    void TearDown() override {
        Mainloop::getInstance().reset();
        mock_pio_reset();
    }

    void triggerScroll(int times = 1) {
        for (int i = 0; i < times; i++) {
            Mainloop::getInstance().tick(100);
        }
    }
};

// ---------------------------------------------------------------------------
//  Single module backward compat — still works as 8x8
// ---------------------------------------------------------------------------

TEST_F(DotMatrix8xNTest, SingleModule_StillWorks) {
    createChain(1);

    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "A");
    EXPECT_EQ(matrix->getDisplayColumns(), 8);
    triggerScroll();

    auto& frame = mock_pio_get_last_frame();
    ASSERT_EQ(frame.size(), 64u);

    std::cout << "\n=== 8xN: 1 module, 'A' ===" << std::endl;
    DotMatrixRenderer::print(frame, "", 8);

    auto actual = DotMatrixRenderer::toGrid(frame);
    auto expected = DotMatrixRenderer::getExpectedChar(MatrixChar8x8::getChar('A'));
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            EXPECT_EQ(actual[row][col], expected[row][col]);
        }
    }
}

// ---------------------------------------------------------------------------
//  Two modules — 16 columns, should show 2+ characters at once
// ---------------------------------------------------------------------------

TEST_F(DotMatrix8xNTest, TwoModules_ShowsTwoChars) {
    createChain(2);

    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "AB");
    EXPECT_EQ(matrix->getDisplayColumns(), 16);
    triggerScroll();

    auto& frame = mock_pio_get_last_frame();
    ASSERT_EQ(frame.size(), 128u) << "2 modules = 128 LEDs";

    std::cout << "\n=== 8xN: 2 modules, 'AB' ===" << std::endl;
    DotMatrixRenderer::print(frame, "", 16);

    // First 8 columns should match 'A'
    auto expectedA = DotMatrixRenderer::getExpectedChar(MatrixChar8x8::getChar('A'));
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            bool actual = frame[DotMatrixRenderer::frameIndex(row, col)] != 0;
            EXPECT_EQ(actual, expectedA[row][col])
                << "'A' mismatch at row=" << row << " col=" << col;
        }
    }
}

// ---------------------------------------------------------------------------
//  Four modules — wide display showing "HELLO"
// ---------------------------------------------------------------------------

TEST_F(DotMatrix8xNTest, FourModules_WideDisplay) {
    createChain(4);

    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "HELLO");
    EXPECT_EQ(matrix->getDisplayColumns(), 32);
    triggerScroll();

    auto& frame = mock_pio_get_last_frame();
    ASSERT_EQ(frame.size(), 256u) << "4 modules = 256 LEDs";

    std::cout << "\n=== 8xN: 4 modules (32 cols), 'HELLO' ===" << std::endl;
    DotMatrixRenderer::print(frame, "", 32);
}

// ---------------------------------------------------------------------------
//  Four modules scrolling — watch "HELLO" scroll across 32 columns
// ---------------------------------------------------------------------------

TEST_F(DotMatrix8xNTest, FourModules_ScrollAnimation) {
    createChain(4);

    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "Hallo Christian");

    std::cout << "\n=== 8xN: 4 modules scrolling 'Hallo Christian' ===" << std::endl;

    for (int step = 0; step < 116; step++) {
        triggerScroll();
        auto& frame = mock_pio_get_last_frame();
        if (frame.size() == 256 && step % 9 == 0) {
            std::cout << "--- step " << std::setw(2) << step << " ---" << std::endl;
            DotMatrixRenderer::print(frame, "", 32);
        }
    }
    EXPECT_EQ(mock_pio_get_last_frame().size(), 256u);
}

// ---------------------------------------------------------------------------
//  Two modules scrolling — "Hi!" across 16 columns
// ---------------------------------------------------------------------------

TEST_F(DotMatrix8xNTest, TwoModules_ScrollHi) {
    createChain(2);

    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "Hi!");

    std::cout << "\n=== 8xN: 2 modules scrolling 'Hi!' ===" << std::endl;

    for (int step = 0; step < 36; step++) {
        triggerScroll();
        auto& frame = mock_pio_get_last_frame();
        if (frame.size() == 128 && step % 4 == 0) {
            std::cout << "--- step " << std::setw(2) << step << " ---" << std::endl;
            DotMatrixRenderer::print(frame, "", 16);
        }
    }
    EXPECT_EQ(mock_pio_get_last_frame().size(), 128u);
}

// ---------------------------------------------------------------------------
//  Color is correctly applied across all modules
// ---------------------------------------------------------------------------

TEST_F(DotMatrix8xNTest, TwoModules_ColorApplied) {
    createChain(2);
    uint32_t color = 0x00FF0000;

    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", "AB", color);
    triggerScroll();

    auto& frame = mock_pio_get_last_frame();
    ASSERT_EQ(frame.size(), 128u);

    for (auto pixel : frame) {
        if (pixel != 0) {
            EXPECT_EQ(pixel, color);
        }
    }
}

// ---------------------------------------------------------------------------
//  Verify every character in a long string at its exact scroll offset
// ---------------------------------------------------------------------------

TEST_F(DotMatrixTest, LongString_EveryCharVerified) {
    const std::string text = "ABCDEFGHIJ";
    auto matrix = std::make_shared<dotMatrix8x8>(ws2812, "test", text);

    // Each character occupies columns [charIndex*9 .. charIndex*9+7].
    // After (charIndex*9 + 1) scroll ticks, offset = charIndex*9,
    // and display columns 0-7 show that character.
    int ticks_so_far = 0;

    std::cout << "\n=== Long string verification: '" << text << "' ===" << std::endl;

    for (size_t ci = 0; ci < text.size(); ci++) {
        int target_offset = ci * 9;
        int ticks_needed = target_offset + 1 - ticks_so_far;
        if (ticks_needed > 0) {
            triggerScroll(ticks_needed);
            ticks_so_far += ticks_needed;
        }

        auto& frame = mock_pio_get_last_frame();
        ASSERT_EQ(frame.size(), 64u);

        // Print the character
        std::cout << "--- char " << ci << " ('" << text[ci]
                  << "') at offset " << target_offset << " ---" << std::endl;
        std::cout << DotMatrixRenderer::render(frame);

        // Verify first 8 columns match the font
        auto expected = DotMatrixRenderer::getExpectedChar(MatrixChar8x8::getChar(text[ci]));
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                bool actual = frame[DotMatrixRenderer::frameIndex(row, col)] != 0;
                EXPECT_EQ(actual, expected[row][col])
                    << "Char '" << text[ci] << "' (index " << ci
                    << ") mismatch at row=" << row << " col=" << col;
            }
        }
    }
}
