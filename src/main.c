/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>
#include <stdbool.h>

#include <pipewire/pipewire.h>
#include <pipewire/filter.h>
#include <pipewire/impl-port.h>

#include <spa/pod/builder.h>
#include <spa/param/latency-utils.h>
#include <spa/utils/dict.h>

#include "dspmodule.h"

static struct pw_main_loop *mainloop = NULL;
static struct pw_filter *filter = NULL;
static DSPModuleProcessFunctionPrototype *modfunc_process;

const char *lapif_getfiltersysname(void);
const char *lapif_getfilterdispname(void);
void lapif_setfilterdispname(const char *name);

void *lapif_addport(const char *sysname, const char *dispname, DSPPortDirection direction, size_t userdatasize);
bool lapif_removeport(void *port);
const char *lapif_getportsysname(void *port);
const char *lapif_getportdispname(void *port);
void lapif_setportdispname(void *port, const char *dispname);

#define LOADERAPIVERSION 1
#define LOADERAPIVERSION_STR "1"

static DSPLoaderAPI lapi_startup =
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
static DSPLoaderAPI lapi_process = { .getportbuffer = (float *(*)(void *, unsigned long))pw_filter_get_dsp_buffer };

static void procdsp(void *userdata, struct spa_io_position *position);
static void chstatedsp(void *data, enum pw_filter_state old, enum pw_filter_state state, const char *error);

static struct pw_filter_events filterevents =
{
    PW_VERSION_FILTER_EVENTS,
    .process = procdsp,
    .state_changed = chstatedsp
};

static void quitsignal(void *userdata, int signum)
{ putchar('\n'); pw_main_loop_quit(mainloop); }

static int exitcode = -1;
static bool quiet = false;

int main(int argc, char *argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "-q")) != -1)
        {
            switch (p)
            {
                case 1:
                    optind--;
                    goto parseoptsend;

                case 'q':
                    quiet = true;
                    break;
            }
        }
    }
    parseoptsend:

    if (optind >= argc)
    {
        fprintf(stderr, "Too few arguments. Basic command line arguments scheme: \n"
            "\t%s [optional loader parametres] <path to DSP .so module> [additional args for module].\n", argv[0]);
        return -1;
    }

    // ===============================================================

    void *module = dlmopen(LM_ID_NEWLM, argv[optind], RTLD_NOW);
    if (!module) { fprintf(stderr, "dlopen(): %s\n", dlerror()); return -1; }

    {
        const unsigned short *modreqver = dlsym(module, "dspmodule_requiredAPIversion");
        if (!modreqver) { fprintf(stderr, "dlsym(\"dspmodule_requiredAPIversion\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }
        if (*modreqver != LOADERAPIVERSION)
        { fprintf(stderr, "incompatible module version (mod. ver.: %hu, loader ver.: " LOADERAPIVERSION_STR ")\n", *modreqver); goto errorquit_afteropenmodule; }
    }

    DSPModuleStartupFunctionPrototype *modfunc_startup = dlsym(module, "dspmodule_startup");
    if (!modfunc_startup) { fprintf(stderr, "dlsym(\"dspmodule_startup\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }

    DSPModuleCleanupFunctionPrototype *modfunc_cleanup = dlsym(module, "dspmodule_cleanup");
    if (!modfunc_cleanup) { fprintf(stderr, "dlsym(\"dspmodule_cleanup\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }

    modfunc_process = dlsym(module, "dspmodule_process");
    if (!modfunc_process) { fprintf(stderr, "dlsym(\"dspmodule_process\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }

    // ===============================================================

    pw_init(NULL, NULL);

    if (!(mainloop = pw_main_loop_new(NULL))) goto errorquit_oncreatemainloop;
    struct pw_loop *loop = pw_main_loop_get_loop(mainloop);

    if (!(pw_loop_add_signal(loop, SIGINT, quitsignal, NULL) && pw_loop_add_signal(loop, SIGTERM, quitsignal, NULL))) goto errorquit_aftercreatemainloop;

    struct pw_properties *props = pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Filter",
            PW_KEY_MEDIA_ROLE, "DSP",
        NULL);
    if (!props) goto errorquit_aftercreatemainloop;
    if (quiet) filterevents.state_changed = NULL;
    if (!(filter = pw_filter_new_simple(loop, NULL, props, &filterevents, NULL)))
    { pw_properties_free(props); goto errorquit_aftercreatemainloop; }

    // ===============================================================

    {
        const char *sysname = NULL, *dispname = NULL;
        //int firstidx = optind;
        //RESETGETOPT();
        unsigned short ret = modfunc_startup(&lapi_startup, argc - optind, &argv[optind], &sysname, &dispname);
        if (ret) { fputs("\nmodule internal initialization error\n", stderr); exitcode = ret; goto errorquit_aftercreatefilter; }

        struct spa_dict_item dictitems[2] =
        {
            SPA_DICT_ITEM_INIT(PW_KEY_NODE_NAME, sysname),
            SPA_DICT_ITEM_INIT(PW_KEY_NODE_DESCRIPTION, dispname),
        };
        struct spa_dict propsdict = SPA_DICT_INIT(dictitems, 2);
        pw_filter_update_properties(filter, NULL, &propsdict);
    }

    // ===============================================================

    if (pw_filter_connect(filter, 0, NULL, 0)) { fputs("error connecting filter.\n", stderr); goto errorquit_aftermodulestartup; }

    exitcode = 0;
    pw_main_loop_run(mainloop);

    errorquit_aftermodulestartup:
        modfunc_cleanup();
    errorquit_aftercreatefilter:
        pw_filter_destroy(filter);
    errorquit_aftercreatemainloop:
        pw_main_loop_destroy(mainloop);
    errorquit_oncreatemainloop:
        pw_deinit();
    errorquit_afteropenmodule:
        dlclose(module);
    return exitcode;
}

static void procdsp(void *userdata, struct spa_io_position *position)
{
    uint32_t duration;
    if (position->clock.duration > UINT32_MAX)
    {
        duration = UINT32_MAX;
        printf("too big size of current cycle (position: %llu, duration: %llu, max duration: 2 ^ 32 - 1). processing only 2 ^ 32 - 1 samples.\n",
            position->clock.position, position->clock.duration);
    }
    else duration = position->clock.duration;

    unsigned short ret = modfunc_process(&lapi_process, position->clock.position, duration, position->clock.rate.denom, position->clock.nsec);
    if (ret) { exitcode = ret; pw_main_loop_quit(mainloop); }
}

static void chstatedsp(void *data, enum pw_filter_state old, enum pw_filter_state state, const char *error)
{
    printf("(state changed to '");
    switch (state)
    {
        case PW_FILTER_STATE_ERROR:
            printf("error");
            break;

        case PW_FILTER_STATE_UNCONNECTED:
            printf("unconnected");
            break;

        case PW_FILTER_STATE_CONNECTING:
            printf("connecting");
            break;

        case PW_FILTER_STATE_PAUSED:
            printf("paused");
            break;
        
        case PW_FILTER_STATE_STREAMING:
            printf("streaming");
            break;
        
        default:
            printf("(unknown)");
    }
    puts("')");
}

const char *lapif_getfiltersysname(void)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, NULL)->dict), PW_KEY_NODE_NAME); }

const char *lapif_getfilterdispname(void)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, NULL)->dict), PW_KEY_NODE_DESCRIPTION); }

void lapif_setfilterdispname(const char *name)
{
    struct spa_dict_item dictitem = SPA_DICT_ITEM_INIT(PW_KEY_NODE_DESCRIPTION, name);
    struct spa_dict propsdict = SPA_DICT_INIT(&dictitem, 1);
    pw_filter_update_properties(filter, NULL, &propsdict);
}

void *lapif_addport(const char *sysname, const char *dispname, DSPPortDirection direction, size_t userdatasize)
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

const char *lapif_getportsysname(void *port)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, port ? port : (void *)1 /* protection from passing NULL. */))->dict, PW_KEY_PORT_NAME); }

const char *lapif_getportdispname(void *port)
{ return spa_dict_lookup(&(pw_filter_get_properties(filter, port ? port : (void *)1 /* protection from passing NULL. */))->dict, PW_KEY_PORT_ALIAS); }

void lapif_setportdispname(void *port, const char *dispname)
{
    struct spa_dict_item dictitem = SPA_DICT_ITEM_INIT(PW_KEY_PORT_ALIAS, dispname);
    struct spa_dict propsdict = SPA_DICT_INIT(&dictitem, 1);
    pw_filter_update_properties(filter, port ? port : (void *)1 /* protection from passing NULL. */, &propsdict);
    pw_impl_port_update_properties(port ? port : (void *)1, &propsdict);
}
