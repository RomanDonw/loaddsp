#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <pipewire/pipewire.h>
#include <pipewire/filter.h>

#include <spa/pod/builder.h>
#include <spa/param/latency-utils.h>

#include "dspmodule.h"

static struct pw_main_loop *mainloop = NULL;
static DSPModuleProcessFunctionPrototype *modfunc_process;
static float **inbuffers = NULL, **outbuffers = NULL;
static void **inports = NULL, **outports = NULL;
static unsigned short inportscount = 0, outportscount = 0;

static void procdsp(void *userdata, struct spa_io_position *position);
static void chstatedsp(void *data, enum pw_filter_state old, enum pw_filter_state state, const char *error);

static const struct pw_filter_events filterevents =
{
    PW_VERSION_FILTER_EVENTS,
    .process = procdsp,
    .state_changed = chstatedsp
};

static void quitsignal(void *userdata, int signum)
{ putchar('\n'); pw_main_loop_quit(mainloop); }

static int exitcode = -1;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments. Basic command line arguments scheme: %s <path to DSP .so module> [additional args for module].\n", argv[0]);
        return -1;
    }

    // ===============================================================

    void *module = dlopen(argv[1], RTLD_LAZY);
    if (!module) { fprintf(stderr, "dlopen(): %s\n", dlerror()); return -1; }

    DSPModuleStartupFunctionPrototype *modfunc_startup = dlsym(module, "dspmodule_startup");
    if (!modfunc_startup) { fprintf(stderr, "dlsym(\"dspmodule_startup\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }

    DSPModuleCleanupFunctionPrototype *modfunc_cleanup = dlsym(module, "dspmodule_cleanup");
    if (!modfunc_cleanup) { fprintf(stderr, "dlsym(\"dspmodule_cleanup\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }

    modfunc_process = dlsym(module, "dspmodule_process");
    if (!modfunc_process) { fprintf(stderr, "dlsym(\"dspmodule_process\"): %s\n", dlerror()); goto errorquit_afteropenmodule; }

    // ===============================================================
    
    const char *modname = "";
    {
        unsigned short ret = modfunc_startup(&modname, &inportscount, &outportscount, argc - 1, argv + sizeof(char *));
        if (ret) { fprintf(stderr, "module internal initialization error"); exitcode = ret; goto errorquit_afteropenmodule; }
    }

    // ===============================================================

    if (inportscount)
    {
        if (!(
            (inbuffers = malloc(sizeof(float *) * inportscount)) &&\
            (inports = malloc(sizeof(void *) * inportscount))))
                { fputs("memory allocation failed", stderr); goto errorquit_onalloc; }
    }

    if (outportscount)
    {
        if (!(
            (outbuffers = malloc(sizeof(float *) * outportscount)) &&\
            (outports = malloc(sizeof(void *) * outportscount))))
                { fputs("memory allocation failed", stderr); goto errorquit_onalloc; }
    }

    // ===============================================================

    pw_init(NULL, NULL);

    if (!(mainloop = pw_main_loop_new(NULL))) goto errorquit_oncreatemainloop;
    struct pw_loop *loop = pw_main_loop_get_loop(mainloop);

    if (!(pw_loop_add_signal(loop, SIGINT, quitsignal, NULL) && pw_loop_add_signal(loop, SIGTERM, quitsignal, NULL))) goto errorquit_aftercreatemainloop;

    struct pw_properties *props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Filter", PW_KEY_MEDIA_ROLE, "DSP", NULL);
    if (!props) goto errorquit_aftercreatemainloop;
    struct pw_filter *filter = pw_filter_new_simple(loop, modname, props, &filterevents, NULL);
    if (!filter) goto errorquit_aftercreatemainloop;

    // ===============================================================

    void *port;
    for (unsigned short i = 0; i < inportscount; i++)
    {
        if (!(props = pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME, "input", NULL)))
        { fputs("error creating input port properties", stderr); goto errorquit_aftercreatefilter; }

        if (!(port = pw_filter_add_port(filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0, props, NULL, 0)))
        { fputs("error adding input port", stderr); goto errorquit_aftercreatefilter; }

        inports[i] = port;
    }

    for (unsigned short i = 0; i < outportscount; i++)
    {
        if (!(props = pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME, "output", NULL)))
        { fputs("error creating output port properties", stderr); goto errorquit_aftercreatefilter; }

        if (!(port = pw_filter_add_port(filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0, props, NULL, 0)))
        { fputs("error adding output port", stderr); goto errorquit_aftercreatefilter; }

        outports[i] = port;
    }

    // ===============================================================

    if (pw_filter_connect(filter, 0, NULL, 0)) { fputs("error connecting filter.", stderr); goto errorquit_aftercreatefilter; }

    exitcode = 0;
    pw_main_loop_run(mainloop);
    
    errorquit_aftercreatefilter:
        pw_filter_destroy(filter);
    errorquit_aftercreatemainloop:
        pw_main_loop_destroy(mainloop);
    errorquit_oncreatemainloop:
        pw_deinit();
    errorquit_onalloc:
        free(outports);
        free(outbuffers);
        free(inports);
        free(inbuffers);
        modfunc_cleanup();
    errorquit_afteropenmodule:
        dlclose(module);
    return exitcode;
}

static void procdsp(void *userdata, struct spa_io_position *position)
{
    for (unsigned short i = 0; i < inportscount; i++) inbuffers[i] = pw_filter_get_dsp_buffer(inports[i], position->clock.duration);
    for (unsigned short i = 0; i < outportscount; i++) outbuffers[i] = pw_filter_get_dsp_buffer(outports[i], position->clock.duration);

    unsigned short ret = modfunc_process((const float * const *)inbuffers, (float * const *)outbuffers, position->clock.duration);
    if (ret) { exitcode = ret; pw_main_loop_quit(mainloop); }
}

static void chstatedsp(void *data, enum pw_filter_state old, enum pw_filter_state state, const char *error)
{
    printf("state changed to ");
    switch (state)
    {
        case PW_FILTER_STATE_ERROR:
            puts("error");
            break;

        case PW_FILTER_STATE_UNCONNECTED:
            puts("unconnected");
            break;

        case PW_FILTER_STATE_CONNECTING:
            puts("connecting");
            break;

        case PW_FILTER_STATE_PAUSED:
            puts("paused");
            break;
        
        case PW_FILTER_STATE_STREAMING:
            puts("streaming");
            break;
        
        default:
            puts("(unknown)");
    }
}