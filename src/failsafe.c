#include <stdbool.h>
#include <stdint.h>

// FIXME a solution to the dependency problems here is to roll up the failsafe configuration into a structure in failsafe.h and supply it using failsafeInit()

#include "common/axis.h" // FIXME this file should not have this dependency

#include "rx_common.h"

#include "drivers/serial_common.h" // FIXME this file should not have this dependency
#include "serial_common.h" // FIXME this file should not have this dependency
#include "flight_mixer.h" // FIXME this file should not have this dependency
#include "flight_common.h" // FIXME this file should not have this dependency
#include "sensors_common.h" // FIXME this file should not have this dependency
#include "boardalignment.h" // FIXME this file should not have this dependency
#include "battery.h" // FIXME this file should not have this dependency

#include "runtime_config.h"
#include "config.h"
#include "config_storage.h"

int16_t failsafeCnt = 0;
int16_t failsafeEvents = 0;

bool isFailsafeIdle(void)
{
    return failsafeCnt == 0;
}

bool hasFailsafeTimerElapsed(void)
{
    return failsafeCnt > (5 * cfg.failsafe_delay);
}

bool shouldFailsafeForceLanding(bool armed)
{
    return hasFailsafeTimerElapsed() && armed;
}

bool shouldFailsafeHaveCausedLandingByNow(void)
{
    return failsafeCnt > 5 * (cfg.failsafe_delay + cfg.failsafe_off_delay);
}

void failsafeAvoidRearm(void)
{
    mwDisarm();             // This will prevent the automatic rearm if failsafe shuts it down and prevents
    f.OK_TO_ARM = 0;        // to restart accidently by just reconnect to the tx - you will have to switch off first to rearm
}


void updateFailsafeState(void)
{
    uint8_t i;

    if (!feature(FEATURE_FAILSAFE)) {
        return;
    }

    if (hasFailsafeTimerElapsed()) {

        if (shouldFailsafeForceLanding(f.ARMED)) { // Stabilize, and set Throttle to specified level
            for (i = 0; i < 3; i++) {
                rcData[i] = mcfg.rxConfig.midrc;      // after specified guard time after RC signal is lost (in 0.1sec)
            }
            rcData[THROTTLE] = cfg.failsafe_throttle;
            failsafeEvents++;
        }

        if (shouldFailsafeHaveCausedLandingByNow() || !f.ARMED) {
            failsafeAvoidRearm();
        }
    }
    failsafeCnt++;
}

