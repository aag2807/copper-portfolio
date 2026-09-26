#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "request.h"
#include "response.h"
#include "str.h"

#include <stddef.h>
#include <time.h>

// Placeholders a view may put in its HTML; filled in just before the response
// is flushed. Absent tokens cost one memmem() over the body.
#define TELEMETRY_TOKEN_RENDER "<!--c:render_us-->"
#define TELEMETRY_TOKEN_REQUESTS "<!--c:requests-->"
#define TELEMETRY_TOKEN_UPTIME "<!--c:uptime-->"

// Number of recent render durations kept for p50/p99.
#define TELEMETRY_RING 1000

// Records the process start time. Called once from main; safe to call again.
void telemetry_init(void);
// Counts one request served by this process; returns the new total.
unsigned long long telemetry_count_request(void);
unsigned long long telemetry_requests(void);
long telemetry_uptime_s(void);

// Adds one render duration (milliseconds) to the ring buffer.
void telemetry_record(double ms);
// Median and 99th percentile of the ring. Returns the sample count (0 → both 0).
int telemetry_percentiles(double* p50, double* p99);

// "0.18" (ms, 2 decimals), "1,284", "3h 12m" / "41s" / "2d 4h".
void telemetry_format_ms(double ms, char* out, size_t cap);
void telemetry_format_count(unsigned long long n, char* out, size_t cap);
void telemetry_format_uptime(long secs, char* out, size_t cap);

// Replaces every telemetry token in `body` with the given strings. Returns
// how many were replaced; body is untouched (no allocation) when there are none.
int telemetry_substitute(String* body, const char* render_ms, const char* requests, const char* uptime);

// Appends the /api/status JSON document to `out`.
void telemetry_status_json(String* out);

// Final pass over a response before response_flush(): fills the telemetry
// tokens in HTML bodies, records the duration since `start` (CLOCK_MONOTONIC),
// adds Server-Timing and Vary, and gzips the body when `req` accepts it.
// `req` may be NULL (no content negotiation then).
void telemetry_finish(Request* req, Response* res, const struct timespec* start);

#endif // !TELEMETRY_H
