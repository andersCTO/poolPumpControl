#include "pool_pump/app.h"

void app_main(void)
{
    if (pool_pump_app_init() != ESP_OK) {
        return;
    }

    pool_pump_app_run();
}
