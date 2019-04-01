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

#define SERVER_ADDRESS "udp://echo.u-blox.com:7"
//#define SERVER_ADDRESS "udp://ciot.it-sgn.u-blox.com:5050"
#define TEST_MESSAGE "Hello world!\n"

typedef enum {
    APP_STATE_NULL,
    APP_STATE_INITIALISED,
    APP_STATE_CONNECTED,
    APP_STATE_COMMUNICATING,
    APP_STATE_DISCONNECTED,
    MAX_NUM_APP_STATES
} AppState;

typedef struct {
    AppState state;
    struct mg_mgr mgr;
    struct mg_connection *pSocket;
} AppContext;

static bool gTickTock = false;
static AppContext *gpContext = NULL;

static void netCb(int event, void *pEventData, void *pArg)
{
  (void) pEventData;
  (void) pArg;

    switch (event) {
        case MGOS_NET_EV_DISCONNECTED:
            LOG(LL_INFO, ("Net: disconnected."));
        break;
        case MGOS_NET_EV_CONNECTING:
            LOG(LL_INFO, ("Net: connecting..."));
        break;
        case MGOS_NET_EV_CONNECTED:
            LOG(LL_INFO, ("Net: connected."));
        break;
        case MGOS_NET_EV_IP_ACQUIRED:
            LOG(LL_INFO, ("Net: got IP address."));
        break;
        default:
            LOG(LL_WARN, ("Net: unknown event (%d).", event));
        break;
    }
}

#ifdef MGOS_HAVE_WIFI

static void wifiCb(int event, void *pEventData, void *pArg)
{
    switch (event) {
        case MGOS_WIFI_EV_STA_DISCONNECTED: {
            struct mgos_wifi_sta_disconnected_arg *pDisconnectedArg =
                        (struct mgos_wifi_sta_disconnected_arg *) pEventData;
            LOG(LL_INFO, ("Wifi: standalone disconnected, reason %d.",
                pDisconnectedArg->reason));
        }
        break;
        case MGOS_WIFI_EV_STA_CONNECTING:
            LOG(LL_INFO, ("Wifi: standalone connecting %p.", pArg));
        break;
        case MGOS_WIFI_EV_STA_CONNECTED:
            LOG(LL_INFO, ("Wifi: standalone connected %p.", pArg));
        break;
        case MGOS_WIFI_EV_STA_IP_ACQUIRED:
            LOG(LL_INFO, ("Wifi: standalone IP address acquired %p.", pArg));
        break;
        case MGOS_WIFI_EV_AP_STA_CONNECTED: {
            struct mgos_wifi_ap_sta_connected_arg *pConnectedArg =
                        (struct mgos_wifi_ap_sta_connected_arg *) pEventData;
            LOG(LL_INFO, ("Wifi: AP connected MAC %02x:%02x:%02x:%02x:%02x:%02x.",
                pConnectedArg->mac[0], pConnectedArg->mac[1], pConnectedArg->mac[2],
                pConnectedArg->mac[3], pConnectedArg->mac[4], pConnectedArg->mac[5]));
        }
        break;
        case MGOS_WIFI_EV_AP_STA_DISCONNECTED: {
            struct mgos_wifi_ap_sta_disconnected_arg *pDisconnectedArg = 
                        (struct mgos_wifi_ap_sta_disconnected_arg *) pEventData;
            LOG(LL_INFO, ("Wifi: AP disconnected MAC %02x:%02x:%02x:%02x:%02x:%02x.",
                pDisconnectedArg->mac[0], pDisconnectedArg->mac[1], pDisconnectedArg->mac[2],
                pDisconnectedArg->mac[3], pDisconnectedArg->mac[4], pDisconnectedArg->mac[5]));
        }
        break;
        default:
            LOG(LL_WARN, ("Wifi: unknown event (%d).\n", event));
        break;
    }
}
#endif /* MGOS_HAVE_WIFI */

static void cloudCb(int event, void *pEventData, void *pArg)
{
    struct mgos_cloud_arg *pCloudArg = (struct mgos_cloud_arg *) pEventData;

    (void) pArg;

    switch (event) {
        case MGOS_EVENT_CLOUD_CONNECTED:
            LOG(LL_INFO, ("Cloud: connected (%d).", pCloudArg->type));
        break;
        case MGOS_EVENT_CLOUD_DISCONNECTED:
            LOG(LL_INFO, ("Cloud: disconnected (%d).", pCloudArg->type));
        break;
        default:
            LOG(LL_WARN, ("Cloud: unknown event (%d).", event));
        break;
    }
}

// Callback for the socket connection
static void socketEventHandler(struct mg_connection *pSocket, int event,
                               void *pEvData, void *pUserData) {
    int connectStatus;
    AppContext *pContext = (AppContext *) pUserData;

    assert(pContext != NULL);

    switch (event) {
        case MG_EV_POLL:
            LOG(LL_INFO, ("Socket: poll."));
        break;
        case MG_EV_ACCEPT:
            pContext->state = APP_STATE_CONNECTED;
            LOG(LL_INFO, ("Socket: connection accepted (from %s).",
                ((union socket_address *) pEvData)->sa.sa_data));
        break;
        case MG_EV_CONNECT:
            connectStatus = *((int *) pEvData);
            if (connectStatus == 0) {
                pContext->state = APP_STATE_CONNECTED;
                LOG(LL_INFO, ("Socket: connected.\n"));
            } else {
                LOG(LL_INFO, ("Socket: connection error: %s.",
                    strerror(connectStatus)));
            }
        break;
        case MG_EV_RECV:
            pContext->state = APP_STATE_COMMUNICATING;
            LOG(LL_INFO, ("Socket: receive (%d byte(s)).",
                *((int *) pEvData)));
        break;
        case MG_EV_SEND:
            pContext->state = APP_STATE_COMMUNICATING;
            LOG(LL_INFO, ("Socket: send (%d byte(s)).",
                *((int *) pEvData)));
        break;
        case MG_EV_CLOSE:
            pContext->state = APP_STATE_DISCONNECTED;
            LOG(LL_INFO, ("Socket: close."));
        break;
        case MG_EV_TIMER:
            LOG(LL_INFO, ("Socket: timer."));
        break;
        default:
            LOG(LL_WARN, ("Socket: unknown event (%d).", event));
        break;
    }
}

// Return true if the context indicates connected, else false
static bool isConnected(AppContext *pContext)
{
    return (pContext->state == APP_STATE_CONNECTED) || (pContext->state == APP_STATE_COMMUNICATING);
}

// Main callback that does stuff
static void timerCb(void *pArg)
{
    AppContext *pContext = (AppContext *) pArg;
    enum mgos_wifi_status wifiStatus;

    LOG(LL_INFO,
        ("%s uptime: %.2lf, RAM: %lu, %lu free", (gTickTock ? "Tick:" : "Tock:"),
         mgos_uptime(), (unsigned long) mgos_get_heap_size(),
         (unsigned long) mgos_get_free_heap_size()));

    gTickTock = !gTickTock;

    if (pContext != NULL) {
        wifiStatus = mgos_wifi_get_status();
        LOG(LL_INFO, ("Wifi status %d.", wifiStatus));
        if ((gpContext->state >= APP_STATE_INITIALISED) && 
            (wifiStatus == MGOS_WIFI_IP_ACQUIRED)) {

            if (pContext->pSocket == NULL) {
                LOG(LL_INFO, ("Connecting to server \"%s\"...", SERVER_ADDRESS));
                pContext->pSocket = mg_connect(&(pContext->mgr), SERVER_ADDRESS, socketEventHandler, pContext);
                if (pContext->pSocket != NULL) {
                    LOG(LL_INFO, ("Socket might be open to \"%s\"...?", SERVER_ADDRESS));
                } else {
                    LOG(LL_ERROR, ("Socket open failed.\n"));
                }
            }

            if (isConnected(gpContext)) {
                assert(pContext->pSocket != NULL);
                LOG(LL_INFO, ("Sending to socket..."));
                mg_send(pContext->pSocket, TEST_MESSAGE, sizeof (TEST_MESSAGE));
            }
        }
    }
}

// Entry point
enum mgos_app_init_result mgos_app_init(void)
{
    enum mgos_app_init_result result = MGOS_APP_INIT_ERROR;

    LOG(LL_INFO, ("Starting up...\n"));
    gpContext = (AppContext *) malloc (sizeof (*gpContext));
    if (gpContext != NULL) {
        memset (gpContext, 0, sizeof(*gpContext));
        gpContext->state = APP_STATE_NULL;
        /* Callback that actually does stuff */
        mgos_set_timer(1000 /* ms */, true /* repeat */, timerCb, gpContext);

        /* Network connectivity events */
        mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, netCb, NULL);

#ifdef MGOS_HAVE_WIFI
        mgos_event_add_group_handler(MGOS_WIFI_EV_BASE, wifiCb, NULL);
#endif

        mgos_event_add_handler(MGOS_EVENT_CLOUD_CONNECTED, cloudCb, NULL);
        mgos_event_add_handler(MGOS_EVENT_CLOUD_DISCONNECTED, cloudCb, NULL);

        LOG(LL_INFO, ("Initialising a connection manager...\n"));
        mg_mgr_init(&(gpContext->mgr), gpContext);
        gpContext->state = APP_STATE_INITIALISED;
        result = MGOS_APP_INIT_SUCCESS;
    } else {
        LOG(LL_ERROR, ("Unable to allocate memory for context.\n"));
    }

    return result;
}