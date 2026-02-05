#include "unity.h"
#include "unity_fixture.h"

static void RunAllTests(void) {
    RUN_TEST_GROUP(relay_control_tests);
    RUN_TEST_GROUP(pump_controller_tests);
    RUN_TEST_GROUP(wifi_manager_tests);
    RUN_TEST_GROUP(nvs_storage_tests);
    RUN_TEST_GROUP(price_fetcher_tests);
    RUN_TEST_GROUP(web_server_tests);
}

int main(int argc, const char *argv[]) { return UnityMain(argc, argv, RunAllTests); }
