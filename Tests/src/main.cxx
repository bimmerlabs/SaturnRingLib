// Tests/src/main.cxx
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

// Display utils
#include "display.hpp"

// Test suites
#include "testsASCII.hpp"
#include "testsAngle.hpp"
// #include "testsEulerAngles.hpp" // Include the header for Euler angles tests
#include "testsCD.hpp"
#include "testsCRAM.hpp"
#include "testsFxp.hpp"
#include "testsHighColor.hpp"
#include "testsMath.hpp"
// #include "testsMat33.hpp"        // Include the header for Matrix33 tests
// #include "testsMat43.hpp"        // Include the header for Matrix43 tests
// #include "testsPlane.hpp"        // Include the header for Plane tests
// #include "testsSphere.hpp"       // Include the header for Sphere tests
// #include "testsCollision.hpp"    // Include the header for Collision tests
// #include "testsFrustum.hpp"      // Include the header for Frustum tests
#include "testsMemory.hpp"        // Include the header for memory tests
#include "testsBase.hpp"          // Include the header for SGL tests
#include "testsBitmap.hpp"        // Include the header for bitmap tests
// #include "testsAABB.hpp"          // Include the header for AABB tests
// #include "testsVector2D.hpp"      // Include the header for Vector2D tests
// #include "testsVector3D.hpp"      // Include the header for Vector3D tests
// #include "testsMatrixStack.hpp"   // Include the header for MatrixStack tests
// #include "testsPrecision.hpp"     // Include the header for Precision tests
// #include "testsRandom.hpp"        // Include the header for Random tests
// #include "testsSortOrder.hpp"     // Include the header for SortOrder tests
// #include "testsTrigonometry.hpp"  // Include the header for Trigonometry tests
// #include "testsUtils.hpp"         // Include the header for Utils tests
#include "testsMemoryHWRam.hpp"   // Include the header for memory HWRam tests
#include "testsMemoryLWRam.hpp"   // Include the header for memory LWRam tests
#include "testsMemoryCartRam.hpp" // Include the header for memory Cart Ram tests
#include "testsString.hpp"        // Include the header for string tests
#include "testsSystem.hpp"        // Include the header for system tests
#include "testsInterrupt.hpp"     // Include the header for vector tests
#include "testsTimer.hpp"         // Include the header for vector tests

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

// Define tags for test start and end
const char *const StrStart = "***UT_START***";
const char *const StrEnd = "***UT_END***";

/**
 * Main program entry
 *
 * This function initializes the SRL core, runs various test suites,
 * and displays the results.
 *
 * @return int
 */
int main()
{
    SRL::Input::Digital pad(0);
    size_t startIndex = 0;

    // Initialize SRL core with a high color
    SRL::Core::Initialize(HighColor(20, 10, 50));
    ASCII::Clear();

    // Tag the beginning of the tests
    LogInfo(StrStart);

    PushResultLine(StrStart);

    // RUN_AND_DISPLAY_SUITE(aabb_test_suite);
    // RUN_AND_DISPLAY_SUITE(angle_test_suite);
    RUN_AND_DISPLAY_SUITE(ascii_test_suite);

    // Run angle test suite
    RUN_AND_DISPLAY_SUITE(angle_test_suite);

    // Run CD test suite
    RUN_AND_DISPLAY_SUITE(cd_test_suite);

    // Run CRAM test suite
    RUN_AND_DISPLAY_SUITE(cram_test_suite);

    // Run FXP test suite
    RUN_AND_DISPLAY_SUITE(fxp_test_suite);

    // Run HighColor test suite
    RUN_AND_DISPLAY_SUITE(highcolor_test_suite);

    // Run Math test suite
    RUN_AND_DISPLAY_SUITE(math_test_suite);

    // Run Memory test suite
    // RUN_AND_DISPLAY_SUITE(memory_test_suite);

    // Run Base test suite (SGL)
    RUN_AND_DISPLAY_SUITE(base_test_suite);

    // Run Bitmap test suite
    RUN_AND_DISPLAY_SUITE(bitmap_test_suite);

    // Run Memory HWRam test suite
    RUN_AND_DISPLAY_SUITE(memory_HWRam_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_LWRam_test_suite);

    // Run Memory CartRam test suite
    // RUN_AND_DISPLAY_SUITE(memory_CartRam_test_suite);

    // Run Interrupt test suite
    RUN_AND_DISPLAY_SUITE(interrupt_test_suite);

    // Run System test suite
    // RUN_AND_DISPLAY_SUITE(system_test_suite);

    // Run Timer test suite
    RUN_AND_DISPLAY_SUITE(test_timer_suite);

    // Generate tests report
    MU_REPORT();

    // Display test statistics
    BuildStatsLine(resultsBuffer, BufferSize,
        static_cast<unsigned int>(minunit_run),
        static_cast<unsigned int>(minunit_assert),
        static_cast<unsigned int>(minunit_fail));

    PushResultLine(resultsBuffer);

    // Tag the end of the tests
    LogInfo(StrEnd);
    PushResultLine(StrEnd);

    if (gResults.size() > DisplayLines)
    {
        startIndex = gResults.size() - DisplayLines;
    }

    RenderResults(startIndex);

    // Main program loop
    uint8_t upHoldFrames = 0;
    uint8_t downHoldFrames = 0;
    const uint8_t repeatDelay = 20;
    const uint8_t repeatRate = 3;

    while (1)
    {
        SRL::Core::Synchronize();
        bool refresh = false;

        const bool upHeld = pad.IsHeld(SRL::Input::Digital::Button::Up);
        const bool downHeld = pad.IsHeld(SRL::Input::Digital::Button::Down);

        if (upHeld && !downHeld)
        {
            if (upHoldFrames < 255)
            {
                ++upHoldFrames;
            }
            downHoldFrames = 0;
        }
        else if (downHeld && !upHeld)
        {
            if (downHoldFrames < 255)
            {
                ++downHoldFrames;
            }
            upHoldFrames = 0;
        }
        else
        {
            upHoldFrames = 0;
            downHoldFrames = 0;
        }

        if (pad.WasPressed(SRL::Input::Digital::Button::Up))
        {
            if (startIndex > 0)
            {
                --startIndex;
                refresh = true;
            }
        }
        else if (upHeld && upHoldFrames > repeatDelay &&
                 ((upHoldFrames - repeatDelay) % repeatRate == 0))
        {
            if (startIndex > 0)
            {
                --startIndex;
                refresh = true;
            }
        }

        if (pad.WasPressed(SRL::Input::Digital::Button::Down))
        {
            if (startIndex + DisplayLines < gResults.size())
            {
                ++startIndex;
                refresh = true;
            }
        }
        else if (downHeld && downHoldFrames > repeatDelay &&
                 ((downHoldFrames - repeatDelay) % repeatRate == 0))
        {
            if (startIndex + DisplayLines < gResults.size())
            {
                ++startIndex;
                refresh = true;
            }
        }

        if (refresh)
        {
            RenderResults(startIndex);
        }
    }

    return 0;
}