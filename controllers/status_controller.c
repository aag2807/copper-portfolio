#include "../src/controller.h"
#include "../src/telemetry.h"

// Live numbers for this instance: uptime, requests, render percentiles, memory.
ACTION(api_status)
{
    String json = str_new();
    telemetry_status_json(&json);
    response_header(res, "Cache-Control", "no-store");
    response_json(res, str_cstr(&json));
    str_free(&json);
}
