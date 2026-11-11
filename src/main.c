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

#include <spa/pod/builder.h>
#include <spa/param/latency-utils.h>
#include <spa/utils/dict.h>

#include "dspmodule.h"
#include "lapi_generic.h"
#include "lapi_rtsafe.h"
#include "util.h"

static struct pw_main_loop *mainloop = NULL;
struct pw_filter *filter = NULL;
static DSPModuleProcessFunctionPrototype *modfunc_process;
static DSPModuleStateChangedFunctionPrototype *modfunc_chstate;

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
static bool logstate = true;

int main(int argc, char *argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "-ql:")) != -1)
        {
            switch (p)
            {
                case 1:
                    optind--;
                    goto parseoptsend;

                case 'q':
                    logstate = false;
                    break;

                case 'l':
                    switch (*optarg)
                    {
                        case 'i':
                            currloglevel = DSPLogLevel_Info;
                            break;

                        case 'w':
                            currloglevel = DSPLogLevel_Warning;
                            break;

                        case 'e':
                            currloglevel = DSPLogLevel_Error;
                            break;

                        case 'f':
                            currloglevel = DSPLogLevel_Fatal;
                            break;

                        default:
                            puts("invalid min log level. allowed: 'i' (info), 'w' (warning), 'e' (error), 'f' (fatal)");
                            return 1;
                    }
                    break;
            }
        }
    }
    parseoptsend:

    if (optind >= argc)
    {
        printf("Too few arguments. Basic command line arguments scheme: \n"
            "\t%s [optional loader parametres] <path to DSP .so module> [additional args for module].\n", argv[0]);
        return -1;
    }

    // ===============================================================

    void *module = dlmopen(LM_ID_NEWLM, argv[optind], RTLD_NOW);
    if (!module) { printf("dlopen(): %s\n", dlerror()); return -1; }

    {
        const unsigned short *modreqver = dlsym(module, "dspmodule_requiredAPIversion");
        if (!modreqver) { printf("dlsym(\"dspmodule_requiredAPIversion\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }
        
        if (*modreqver != 4)
        {
            printf("incompatible module version: supported only 4, current %hu\n", *modreqver);
            goto errorquit_afteropenmodule;
        }
    }

    DSPModuleStartupFunctionPrototype *modfunc_startup = dlsym(module, "dspmodule_startup");
    if (!modfunc_startup) { printf("dlsym(\"dspmodule_startup\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }

    modfunc_process = dlsym(module, "dspmodule_process");
    if (!modfunc_process) { printf("dlsym(\"dspmodule_process\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }
    
    DSPModuleCleanupFunctionPrototype *modfunc_cleanup = dlsym(module, "dspmodule_cleanup");
    modfunc_chstate = dlsym(module, "dspmodule_statechanged");

    // ===============================================================

    pw_init(NULL, NULL);

    if (!(mainloop = pw_main_loop_new(NULL))) goto errorquit_oncreatemainloop;
    struct pw_loop *loop = pw_main_loop_get_loop(mainloop);

    if (!(pw_loop_add_signal(loop, SIGINT, quitsignal, NULL) && pw_loop_add_signal(loop, SIGTERM, quitsignal, NULL))) goto errorquit_aftercreatemainloop;

    {
        struct pw_properties *props = pw_properties_new(
                PW_KEY_MEDIA_TYPE, "Audio",
                PW_KEY_MEDIA_CATEGORY, "Filter",
                PW_KEY_MEDIA_ROLE, "DSP",
            NULL);
        if (!props) goto errorquit_aftercreatemainloop;

        if (!(filter = pw_filter_new_simple(loop, NULL, props, &filterevents, NULL)))
        { pw_properties_free(props); goto errorquit_aftercreatemainloop; }
    }

    // ===============================================================

    {
        DSPModuleStartupFunctionOptions opts =
        {
            .argc = argc - optind,
            .argv = argv + optind,
        };
        unsigned short ret = modfunc_startup(&lapi_generic, &opts);
        if (ret)
        {
            loader_log(DSPLogLevel_Fatal, "module internal initialization error");
            exitcode = ret;
            goto errorquit_aftercreatefilter;
        }

        char forceratestr[11];
        if (snprintf(forceratestr, sizeof(forceratestr), "%lu", opts.forcerate) < 0)
        {
            loader_log(DSPLogLevel_Fatal, "failed formatting string (force rate PipeWire option value) through snprintf");
            goto errorquit_aftermodulestartup;
        }

        char forcequantumstr[11];
        if (snprintf(forcequantumstr, sizeof(forcequantumstr), "%lu", opts.forcequantum) < 0)
        {
            loader_log(DSPLogLevel_Fatal, "failed formatting string (force quantum PipeWire option value) through snprintf");
            goto errorquit_aftermodulestartup;
        }

        struct spa_dict_item dictitems[4] =
        {
            SPA_DICT_ITEM_INIT(PW_KEY_NODE_NAME, opts.sysname),
            SPA_DICT_ITEM_INIT(PW_KEY_NODE_DESCRIPTION, opts.dispname),
            SPA_DICT_ITEM_INIT(PW_KEY_NODE_FORCE_RATE, forceratestr),
            SPA_DICT_ITEM_INIT(PW_KEY_NODE_FORCE_QUANTUM, forcequantumstr)
        };
        struct spa_dict propsdict = SPA_DICT_INIT(dictitems, 2);
        pw_filter_update_properties(filter, NULL, &propsdict);
    }
    
    // ===============================================================

    if (pw_filter_connect(filter, 0, NULL, 0))
    { loader_log(DSPLogLevel_Fatal, "failed connecting filter to graph"); goto errorquit_aftermodulestartup; }

    exitcode = 0;
    pw_main_loop_run(mainloop);

    errorquit_aftermodulestartup:
        if (modfunc_cleanup) modfunc_cleanup();
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
        loader_log(DSPLogLevel_Warning, "too big size of current cycle (position: %llu, duration: %llu, max duration: 2 ^ 32 - 1)."
            " processing only 2 ^ 32 - 1 samples.\n", position->clock.position, position->clock.duration);
    }
    else duration = position->clock.duration;

    DSPModuleProcessFunctionOptions opts =
    {
        .position = position->clock.position,
        .duration = duration,
        .rate = position->clock.rate.denom,
        .nsectime = position->clock.nsec
    };

    unsigned short ret = modfunc_process(&lapi_rtsafe, &opts);
    if (ret) { exitcode = ret; pw_main_loop_quit(mainloop); }
}

static void chstatedsp(void *data, enum pw_filter_state old, enum pw_filter_state state, const char *error)
{
    if (logstate)
    {
        const char *statestr;
        switch (state)
        {
            case PW_FILTER_STATE_ERROR:
                statestr = "error";
                break;

            case PW_FILTER_STATE_UNCONNECTED:
                statestr = "unconnected";
                break;

            case PW_FILTER_STATE_CONNECTING:
                statestr = "connecting";
                break;

            case PW_FILTER_STATE_PAUSED:
                statestr = "paused";
                break;
            
            case PW_FILTER_STATE_STREAMING:
                statestr = "streaming";
                break;
            
            default:
                statestr = "(unknown)";
        }
        loader_log(DSPLogLevel_Info, "state changed to '%s'", statestr);
    }

    if (modfunc_chstate &&
        (old == PW_FILTER_STATE_PAUSED || old == PW_FILTER_STATE_STREAMING) &&
        (state == PW_FILTER_STATE_PAUSED || state == PW_FILTER_STATE_STREAMING))
    {
        unsigned short ret = modfunc_chstate(&lapi_generic, old == PW_FILTER_STATE_STREAMING ? DSPState_Streaming : DSPState_Paused,
            state == PW_FILTER_STATE_STREAMING ? DSPState_Streaming : DSPState_Paused);
        if (ret) { exitcode = ret; pw_main_loop_quit(mainloop); }
    }
}