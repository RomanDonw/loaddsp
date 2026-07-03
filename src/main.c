#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <dlfcn.h>

#include <pipewire/pipewire.h>
#include <pipewire/filter.h>

#include <spa/pod/builder.h>
#include <spa/param/latency-utils.h>

static struct pw_main_loop *mainloop = NULL;
static struct pw_filter *filter = NULL;
static void *inport = NULL, *outport = NULL;

static void quitsignal(void *userdata, int signum)
{ putchar('\n'); pw_main_loop_quit(mainloop); }

static inline float clampf(float in, float min, float max)
{
    if (in > max) return max;
    if (in < min) return min;
    return in;
}

static inline float signf(float in)
{
    if (in > 0) return 1;
    if (in < 0) return -1;
    return 0;
}

static void procdsp(void *userdata, struct spa_io_position *position)
{
    uint32_t count = position->clock.duration;
    const float *in = pw_filter_get_dsp_buffer(inport, count);
    float *out = pw_filter_get_dsp_buffer(outport, count);

    if (!out) return;
    if (!in)
    { memset(out, 0, sizeof(float) * count); return; }

    for (uint32_t i = 0; i < count; i++)
    {
        float sgn = signf(in[i]);
        float val = clampf(in[i] + 0.5 * sgn, -1, 1) * 0.1;
        out[i] = val;
    }
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

int main(void)
{
    int quitcode = 0;
    pw_init(NULL, NULL);

    mainloop = pw_main_loop_new(NULL);
    struct pw_loop *loop = pw_main_loop_get_loop(mainloop);

    pw_loop_add_signal(loop, SIGINT, quitsignal, NULL);
    pw_loop_add_signal(loop, SIGTERM, quitsignal, NULL);

    const struct pw_filter_events filterevents =
    {
        PW_VERSION_FILTER_EVENTS,
        .process = procdsp,
        .state_changed = chstatedsp
    };

    filter = pw_filter_new_simple(loop, "audio-filter",
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Filter", PW_KEY_MEDIA_ROLE, "DSP", NULL),
            &filterevents, NULL);

    inport = pw_filter_add_port(filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
        pw_properties_new( PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME, "input", NULL),
            NULL, 0);

    outport = pw_filter_add_port(filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, 0,
        pw_properties_new( PW_KEY_FORMAT_DSP, "32 bit float mono audio", PW_KEY_PORT_NAME, "output", NULL),
            NULL, 0);

    if (pw_filter_connect(filter, /*PW_FILTER_FLAG_RT_PROCESS*/0, NULL, 0))
    { puts("error connecting filter."); quitcode = -1; goto quit; }

    pw_main_loop_run(mainloop);

    quit:
        pw_filter_destroy(filter);
        pw_main_loop_destroy(mainloop);
        pw_deinit();
    return quitcode;
}
