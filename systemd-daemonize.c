#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-id128.h>
#include <unistd.h>

#define FATAL(...)                                                             \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    abort();                                                                   \
  } while (0)
#define FAIL_SILENT                                                            \
  do {                                                                         \
    errno = 0;                                                                 \
    return 0;                                                                  \
  } while (0)
#define TRY_NOFAIL(x)                                                          \
  if (x)                                                                       \
  FAIL_SILENT
#define TRY(x)                                                                 \
  if (x)                                                                       \
  return x
#define SDD_CLEANUP(x) __attribute__((cleanup(x##_unrefp))) x

int
daemon(int nochdir, int noclose)
{
  /* call original daemon function */
  int (*daemon_orig)(int, int) = dlsym(RTLD_NEXT, "daemon");
  if (daemon_orig == NULL)
    FATAL("systemd-daemonize: FATAL ERROR: symbol \"daemon\" not found");
  TRY(daemon_orig(nochdir, noclose));

  SDD_CLEANUP(sd_bus)* bus = NULL;
  TRY_NOFAIL(sd_bus_open_user(&bus));

  sd_id128_t scope_rand = SD_ID128_NULL;
  TRY_NOFAIL(sd_id128_randomize(&scope_rand));

  char unit_name[512];
  sprintf(unit_name,
          "systemd-daemonize-r" SD_ID128_FORMAT_STR ".scope",
          SD_ID128_FORMAT_VAL(scope_rand));

  SDD_CLEANUP(sd_bus_message)* reply = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  TRY_NOFAIL(sd_bus_call_method(bus,
                                "org.freedesktop.systemd1",
                                "/org/freedesktop/systemd1",
                                "org.freedesktop.systemd1.Manager",
                                "StartTransientUnit",
                                &error,
                                &reply,
                                "ssa(sv)a(sa(sv))",
                                unit_name,
                                "fail",
                                2,
                                "PIDs",
                                "au",
                                1,
                                (uint32_t)getpid(),
                                "Slice",
                                "s",
                                "systemd_daemon.slice",
                                0));

  return 0;
}
