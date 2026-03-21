#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/plugin.hpp"
#include "../../examples/alteriom/alteriom_custom_package_template.hpp"

using namespace painlessmesh;
using namespace alteriom;

logger::LogClass Log;

SCENARIO("MpptPackage has correct type and routing") {
    GIVEN("A default-constructed MpptPackage") {
        auto pkg = MpptPackage();

        THEN("Type ID is 203 and routing is BROADCAST") {
            REQUIRE(pkg.type == 203);
            REQUIRE(pkg.routing == router::BROADCAST);
            REQUIRE(pkg.messageType == 203);
        }

        THEN("All numeric fields default to zero") {
            REQUIRE(pkg.solarVoltage   == 0.0f);
            REQUIRE(pkg.solarCurrent   == 0.0f);
            REQUIRE(pkg.solarPower     == 0);
            REQUIRE(pkg.batteryVoltage == 0.0f);
            REQUIRE(pkg.batterySOC     == 0);
            REQUIRE(pkg.loadVoltage    == 0.0f);
            REQUIRE(pkg.loadCurrent    == 0.0f);
            REQUIRE(pkg.chargeState    == 0);
            REQUIRE(pkg.controllerTemp == 0);
            REQUIRE(pkg.deviceId       == 0);
            REQUIRE(pkg.timestamp      == 0);
        }
    }
}

SCENARIO("MpptPackage serialization round-trip preserves all fields") {
    GIVEN("An MpptPackage populated with typical charge-controller values") {
        auto pkg = MpptPackage();
        pkg.from           = 11111;
        pkg.solarVoltage   = 18.5f;
        pkg.solarCurrent   = 5.2f;
        pkg.solarPower     = 96;
        pkg.batteryVoltage = 12.6f;
        pkg.batterySOC     = 80;
        pkg.loadVoltage    = 12.4f;
        pkg.loadCurrent    = 2.1f;
        pkg.chargeState    = CHARGE_MPPT;
        pkg.controllerTemp = 32;
        pkg.deviceId       = 99999;
        pkg.timestamp      = 1700000000;

        REQUIRE(pkg.type == 203);

        WHEN("Converting to and from a protocol::Variant") {
            auto var  = protocol::Variant(&pkg);
            auto pkg2 = var.to<MpptPackage>();

            THEN("All fields are identical after round-trip") {
                REQUIRE(pkg2.from           == pkg.from);
                REQUIRE(pkg2.solarVoltage   == Approx(pkg.solarVoltage));
                REQUIRE(pkg2.solarCurrent   == Approx(pkg.solarCurrent));
                REQUIRE(pkg2.solarPower     == pkg.solarPower);
                REQUIRE(pkg2.batteryVoltage == Approx(pkg.batteryVoltage));
                REQUIRE(pkg2.batterySOC     == pkg.batterySOC);
                REQUIRE(pkg2.loadVoltage    == Approx(pkg.loadVoltage));
                REQUIRE(pkg2.loadCurrent    == Approx(pkg.loadCurrent));
                REQUIRE(pkg2.chargeState    == pkg.chargeState);
                REQUIRE(pkg2.controllerTemp == pkg.controllerTemp);
                REQUIRE(pkg2.deviceId       == pkg.deviceId);
                REQUIRE(pkg2.timestamp      == pkg.timestamp);
                REQUIRE(pkg2.type           == pkg.type);
                REQUIRE(pkg2.routing        == pkg.routing);
                REQUIRE(pkg2.messageType    == pkg.messageType);
            }
        }
    }
}

SCENARIO("MpptPackage handles negative controller temperature") {
    GIVEN("An MpptPackage with a sub-zero controller temperature") {
        auto pkg = MpptPackage();
        pkg.from           = 22222;
        pkg.controllerTemp = -10;  // -10 °C
        pkg.batteryVoltage = 11.8f;
        pkg.batterySOC     = 20;
        pkg.chargeState    = CHARGE_BOOST;

        WHEN("Converting to and from a protocol::Variant") {
            auto var  = protocol::Variant(&pkg);
            auto pkg2 = var.to<MpptPackage>();

            THEN("Negative temperature is preserved") {
                REQUIRE(pkg2.controllerTemp == -10);
                REQUIRE(pkg2.batterySOC     == 20);
                REQUIRE(pkg2.chargeState    == CHARGE_BOOST);
            }
        }
    }
}

SCENARIO("MpptPackage handles edge case values") {
    GIVEN("An MpptPackage with zero / maximum values") {
        auto pkg = MpptPackage();
        pkg.from           = 33333;
        pkg.solarVoltage   = 0.0f;
        pkg.solarCurrent   = 0.0f;
        pkg.solarPower     = 0;
        pkg.batterySOC     = 100;    // fully charged
        pkg.chargeState    = CHARGE_FLOAT;
        pkg.controllerTemp = 0;

        WHEN("Converting to and from a protocol::Variant") {
            auto var  = protocol::Variant(&pkg);
            auto pkg2 = var.to<MpptPackage>();

            THEN("Zero PV values and full charge are preserved") {
                REQUIRE(pkg2.solarVoltage == 0.0f);
                REQUIRE(pkg2.solarCurrent == 0.0f);
                REQUIRE(pkg2.solarPower   == 0);
                REQUIRE(pkg2.batterySOC   == 100);
                REQUIRE(pkg2.chargeState  == CHARGE_FLOAT);
            }
        }
    }
}

SCENARIO("ChargeState enum values are correct") {
    THEN("Enum constants match expected integer values") {
        REQUIRE(CHARGE_OFF      == 0);
        REQUIRE(CHARGE_NORMAL   == 1);
        REQUIRE(CHARGE_MPPT     == 2);
        REQUIRE(CHARGE_EQUALIZE == 3);
        REQUIRE(CHARGE_BOOST    == 4);
        REQUIRE(CHARGE_FLOAT    == 5);
        REQUIRE(CHARGE_LIMITED  == 6);
    }
}
