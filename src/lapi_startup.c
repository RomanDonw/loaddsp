/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "lapi_startup.h"

#include <pipewire/impl-port.h>

#include "main.h"

static const char *lapif_getfiltersysname(void)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, NULL)->dict), PW_KEY_NODE_NAME); }

static const char *lapif_getfilterdispname(void)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, NULL)->dict), PW_KEY_NODE_DESCRIPTION); }

static void lapif_setfilterdispname(const char *name)
{
    struct spa_dict_item dictitem = SPA_DICT_ITEM_INIT(PW_KEY_NODE_DESCRIPTION, name);
    struct spa_dict propsdict = SPA_DICT_INIT(&dictitem, 1);
    pw_filter_update_properties(filter, NULL, &propsdict);
}

static void *lapif_addport(const char *sysname, const char *dispname, DSPPortDirection direction, size_t userdatasize)
{
    enum pw_direction dir;
    switch (direction)
    {
        case DSPPortDirection_Input:
            dir = PW_DIRECTION_INPUT;
            break;

        case DSPPortDirection_Output:
            dir = PW_DIRECTION_OUTPUT;
            break;

        default:
            return NULL;
    }

    struct pw_properties *props = pw_properties_new(
            PW_KEY_FORMAT_DSP, "32 bit float mono audio",
            PW_KEY_PORT_NAME, sysname,
            PW_KEY_PORT_ALIAS, dispname,
        NULL);
    if (!props) return NULL;

    void *ret = pw_filter_add_port(filter, dir, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0, props, NULL, 0);
    if (!ret) return NULL;

    return ret;
}

static const char *lapif_getportsysname(void *port)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, port ? port : (void *)1 /* protection from passing NULL. */))->dict, PW_KEY_PORT_NAME); }

static const char *lapif_getportdispname(void *port)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, port ? port : (void *)1 /* protection from passing NULL. */))->dict, PW_KEY_PORT_ALIAS); }

static void lapif_setportdispname(void *port, const char *dispname)
{
    struct spa_dict_item dictitem = SPA_DICT_ITEM_INIT(PW_KEY_PORT_ALIAS, dispname);
    struct spa_dict propsdict = SPA_DICT_INIT(&dictitem, 1);
    pw_filter_update_properties(filter, port ? port : (void *)1 /* protection from passing NULL. */, &propsdict);
    pw_impl_port_update_properties(port ? port : (void *)1, &propsdict);
}

const DSPLoaderAPI lapi_startup =
{
    .getfiltersysname = lapif_getfiltersysname,
    .getfilterdispname = lapif_getfilterdispname,
    .setfilterdispname = lapif_setfilterdispname,

    .addport = lapif_addport,
    .removeport = (void (*)(void *))pw_filter_remove_port,
    .getportsysname = lapif_getportsysname,
    .getportdispname = lapif_getportdispname,
    .setportdispname = lapif_setportdispname
};
