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

static bool gTickTock = false;

static void timerCb(void *arg)
{
    (void) arg;

    LOG(LL_INFO,
        ("%s uptime: %.2lf, RAM: %lu, %lu free", (gTickTock ? "Tick" : "Tock"),
         mgos_uptime(), (unsigned long) mgos_get_heap_size(),
         (unsigned long) mgos_get_free_heap_size()));

    gTickTock = !gTickTock;
}

static void netCb(int ev, void *evd, void *arg)
{
  (void) evd;
  (void) arg;

    switch (ev) {
        case MGOS_NET_EV_DISCONNECTED:
            LOG(LL_INFO, ("%s", "Net disconnected."));
        break;
        case MGOS_NET_EV_CONNECTING:
            LOG(LL_INFO, ("%s", "Net connecting..."));
        break;
        case MGOS_NET_EV_CONNECTED:
            LOG(LL_INFO, ("%s", "Net connected."));
        break;
        case MGOS_NET_EV_IP_ACQUIRED:
            LOG(LL_INFO, ("%s", "Net got IP address."));
        break;
        default:
        break;
    }
}

#ifdef MGOS_HAVE_WIFI

static void wifiCb(int ev, void *evd, void *arg)
{

  (void) arg;

    switch (ev) {
        case MGOS_WIFI_EV_STA_DISCONNECTED: {
            struct mgos_wifi_sta_disconnected_arg *da = (struct mgos_wifi_sta_disconnected_arg *) evd;
            LOG(LL_INFO, ("WiFi standalone disconnected, reason %d.", da->reason));
        }
        break;
        case MGOS_WIFI_EV_STA_CONNECTING:
            LOG(LL_INFO, ("WiFi standalone connecting %p.", arg));
        break;
        case MGOS_WIFI_EV_STA_CONNECTED:
            LOG(LL_INFO, ("WiFi standalone connected %p.", arg));
        break;
        case MGOS_WIFI_EV_STA_IP_ACQUIRED:
            LOG(LL_INFO, ("WiFi standalone IP address acquired %p.", arg));
        break;
        case MGOS_WIFI_EV_AP_STA_CONNECTED: {
            struct mgos_wifi_ap_sta_connected_arg *aa = (struct mgos_wifi_ap_sta_connected_arg *) evd;
            LOG(LL_INFO, ("WiFi AP connected MAC %02x:%02x:%02x:%02x:%02x:%02x.",
                aa->mac[0], aa->mac[1], aa->mac[2], aa->mac[3], aa->mac[4],
                aa->mac[5]));
        }
        break;
        case MGOS_WIFI_EV_AP_STA_DISCONNECTED: {
            struct mgos_wifi_ap_sta_disconnected_arg *aa = (struct mgos_wifi_ap_sta_disconnected_arg *) evd;
            LOG(LL_INFO, ("WiFi AP disconnected MAC %02x:%02x:%02x:%02x:%02x:%02x.",
                aa->mac[0], aa->mac[1], aa->mac[2], aa->mac[3], aa->mac[4],
                aa->mac[5]));
        }
        break;
        default:
        break;
    }
}
#endif /* MGOS_HAVE_WIFI */

static void cloudCb(int ev, void *evd, void *arg)
{
    struct mgos_cloud_arg *ca = (struct mgos_cloud_arg *) evd;

    (void) arg;

    switch (ev) {
        case MGOS_EVENT_CLOUD_CONNECTED:
            LOG(LL_INFO, ("Cloud connected (%d).", ca->type));
        break;
        case MGOS_EVENT_CLOUD_DISCONNECTED:
            LOG(LL_INFO, ("Cloud disconnected (%d).", ca->type));
        break;
        default:
        break;
    }
}
enum mgos_app_init_result mgos_app_init(void)
{
    printf("Starting up...\n");
    mgos_set_timer(1000 /* ms */, true /* repeat */, timerCb, NULL);

    /* Network connectivity events */
    mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, netCb, NULL);

#ifdef MGOS_HAVE_WIFI
    mgos_event_add_group_handler(MGOS_WIFI_EV_BASE, wifiCb, NULL);
#endif

    mgos_event_add_handler(MGOS_EVENT_CLOUD_CONNECTED, cloudCb, NULL);
    mgos_event_add_handler(MGOS_EVENT_CLOUD_DISCONNECTED, cloudCb, NULL);

    printf("Start-up completed.\n");

    return MGOS_APP_INIT_SUCCESS;
}