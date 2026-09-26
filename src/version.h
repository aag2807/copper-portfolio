#ifndef VERSION_H
#define VERSION_H

// Single source for the version shown in the startup banner and /api/status.
#define COPPER_NAME "c-copper"
#define COPPER_VERSION "0.5"

// Baked in by the Makefile (`git rev-parse --short HEAD`) or the Dockerfile
// build arg; "dev" when neither supplied one.
#ifndef BUILD_SHA
#define BUILD_SHA "dev"
#endif

#endif // !VERSION_H
