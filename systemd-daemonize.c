#define _GNU_SOURCE

#include <dlfcn.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-id128.h>
#include <unistd.h>

int daemon(int nochdir, int noclose) {
  int r = 0;

  /* call original daemon function */
  int (*daemon_orig)(int, int) = dlsym(RTLD_NEXT, "daemon");
  if (daemon_orig == NULL) {
    fprintf(stderr,
            "systemd-daemonize: FATAL ERROR: daemon() NOT FOUND, ABORTING");
    abort();
  }
  r = daemon_orig(nochdir, noclose);
  if (r < 0)
    return r;

  __attribute__((cleanup(sd_bus_unrefp))) sd_bus *bus = NULL;
  r = sd_bus_open_user(&bus);
  if (r < 0) {
    return 0; /* fail silently if cannot connect to dbus */
  }

  char unit_name[512];
  pid_t pid = getpid();
  sd_id128_t scope_rand = SD_ID128_NULL;
  r = sd_id128_randomize(&scope_rand);
  if (r < 0)
    return 0;
  sprintf(unit_name, "systemd-daemonize-r" SD_ID128_FORMAT_STR ".scope",
          SD_ID128_FORMAT_VAL(scope_rand));

  __attribute__((cleanup(sd_bus_message_unrefp))) sd_bus_message *reply = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  r = sd_bus_call_method(
      bus, "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
      "org.freedesktop.systemd1.Manager", "StartTransientUnit", &error, &reply,
      "ssa(sv)a(sa(sv))", unit_name, "fail", 2, "PIDs", "au", 1, (uint32_t)pid,
      "Slice", "s", "systemd_daemonize.slice", 0);
  if (r < 0) {
    return 0;
  }

  return 0;
}
