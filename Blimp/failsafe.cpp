#include "Blimp.h"

//
//  failsafe support
//  Andrew Tridgell, December 2011
//
//  our failsafe strategy is to detect main loop lockup and disarm the motors
//

static bool failsafe_enabled = false;
static uint16_t failsafe_last_ticks;
static uint32_t failsafe_last_timestamp;
static bool in_failsafe;

//
// failsafe_enable - enable failsafe
//
void Blimp::failsafe_enable()
{
    failsafe_enabled = true;
    failsafe_last_timestamp = micros();
}

//
// failsafe_disable - used when we know we are going to delay the mainloop significantly
//
void Blimp::failsafe_disable()
{
    failsafe_enabled = false;
}

//
//  failsafe_check - this function is called from the core timer interrupt at 1kHz.
//
void Blimp::failsafe_check()
{
    uint32_t tnow = AP_HAL::micros();

    const uint16_t ticks = scheduler.ticks();
    if (ticks != failsafe_last_ticks) {
        // the main loop is running, all is OK
        failsafe_last_ticks = ticks;
        failsafe_last_timestamp = tnow;
        if (in_failsafe) {
            in_failsafe = false;
            AP::logger().Write_Error(LogErrorSubsystem::CPU, LogErrorCode::FAILSAFE_RESOLVED);
        }
        return;
    }

    if (!in_failsafe && failsafe_enabled && AP_HAL::Calclate_dt(tnow,failsafe_last_timestamp) > 2000000) {
        // motors are running but we have gone 2 second since the
        // main loop ran. That means we're in trouble and should
        // disarm the motors->
        in_failsafe = true;
        // reduce motors to minimum (we do not immediately disarm because we want to log the failure)
        if (motors->armed()) {
            motors->output_min();
            //TODO: this may not work correctly.
        }

        AP::logger().Write_Error(LogErrorSubsystem::CPU, LogErrorCode::FAILSAFE_OCCURRED);
    }

    if (failsafe_enabled && in_failsafe && AP_HAL::Calclate_dt(tnow,failsafe_last_timestamp) > 1000000) {
        // disarm motors every second
        failsafe_last_timestamp = tnow;
        if (motors->armed()) {
            motors->armed(false);
            motors->output();
        }
    }
}
