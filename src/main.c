/*
 * Copyright (C) u-blox Melbourn Ltd
 * u-blox Melbourn Ltd, Melbourn, UK
 *
 * All rights reserved.
 *
 * This source file is the sole property of u-blox Melbourn Ltd.
 * Reproduction or utilisation of this source in whole or part is
 * forbidden without the written consent of u-blox Melbourn Ltd.
 */

#include "stdio.h"
#include "mgos.h"

static void timer_cb(void *arg) {
  static int current_level = 1;
  LOG(LL_INFO, ("%s", (current_level ? "Tick" : "Tock")));
  current_level ^= 1;
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  printf("Hello, world!\n");
  mgos_set_timer(1000 /* ms */, true /* repeat */, timer_cb, NULL);
  return MGOS_APP_INIT_SUCCESS;
}