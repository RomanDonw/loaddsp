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
#include <spa/utils/dict.h>

#include "dspmodule.h"

static struct pw_main_loop *mainloop = NULL;
static struct pw_filter *filter = NULL;
static DSPModuleProcessFunctionPrototype *modfunc_process;

const char *lapif_getfiltername(void);
void lapif_setfiltername(const char *name);
DSPPort *lapif_addport(const char *name, DSPPortDirection direction);
bool lapif_removeport(DSPPort *port);

static DSPLoaderAPI lapi_startup =
{
    .getfiltername = lapif_getfiltername,
    .setfiltername = lapif_setfiltername,

    .addport = lapif_addport,
    .removeport = lapif_removeport,
    //.getportname = ,
    //.setportname = ,
};

static DSPLoaderAPI lapi_process = { .getportbuffer = (float *(*)(DSPPort *, unsigned long))pw_filter_get_dsp_buffer };

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

    pw_init(NULL, NULL);

    if (!(mainloop = pw_main_loop_new(NULL))) goto errorquit_oncreatemainloop;
    struct pw_loop *loop = pw_main_loop_get_loop(mainloop);

    if (!(pw_loop_add_signal(loop, SIGINT, quitsignal, NULL) && pw_loop_add_signal(loop, SIGTERM, quitsignal, NULL))) goto errorquit_aftercreatemainloop;

    struct pw_properties *props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Filter", PW_KEY_MEDIA_ROLE, "DSP", NULL);
    if (!props) goto errorquit_aftercreatemainloop;
    filter = pw_filter_new_simple(loop, ""/*"Digital Sound Processor"*/, props, &filterevents, NULL);
    if (!filter) { pw_properties_free(props); goto errorquit_aftercreatemainloop; }

    // ===============================================================

    {
        unsigned short ret = modfunc_startup(&lapi_startup, argc - 1, &argv[1]);
        if (ret) { fputs("\nmodule internal initialization error\n", stderr); exitcode = ret; goto errorquit_aftercreatefilter; }
    }

    if (pw_filter_connect(filter, 0, NULL, 0)) { fputs("error connecting filter.", stderr); goto errorquit_aftermodulestartup; }

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
    unsigned short ret = modfunc_process(&lapi_process, position->clock.position, position->clock.duration, position->clock.rate.denom, position->clock.nsec);
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

const char *lapif_getfiltername(void) { return pw_filter_get_name(filter); }
void lapif_setfiltername(const char *name)
{
    static struct spa_dict_item items[2];
    items[0] = SPA_DICT_ITEM_INIT(PW_KEY_NODE_DESCRIPTION, name);
    items[1] = SPA_DICT_ITEM_INIT(PW_KEY_NODE_NICK, name);

    static struct spa_dict props;
    props = SPA_DICT_INIT(items, 2);
    pw_filter_update_properties(filter, NULL, &props);
}

DSPPort *lapif_addport(const char *name, DSPPortDirection direction)
{
    enum pw_direction dir;
    char *strdir;
    switch (direction)
    {
        case DSPPortDirection_Input:
            dir = PW_DIRECTION_INPUT;
            strdir = "input";
            break;

        case DSPPortDirection_Output:
            dir = PW_DIRECTION_OUTPUT;
            strdir = "output";
            break;

        default:
            //fprintf(stderr, "invalid port direction (port name: \"%s\")\n", name)
            return NULL;
    }

    struct pw_properties *props = pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME, name, NULL);
    if (!props)
    {
        //fprintf(stderr, "error creating properties of %s port with name \"%s\"\n", strdir, name);
        return NULL;
    }

    DSPPort *ret = pw_filter_add_port(filter, dir, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0, props, NULL, 0);
    if (!ret)
    {
        //fprintf(stderr, "error adding %s port with name \"%s\"\n", strdir, name);
        pw_properties_free(props);
        return NULL;
    }

    return ret;
}

bool lapif_removeport(DSPPort *port) { return !pw_filter_remove_port(port); }