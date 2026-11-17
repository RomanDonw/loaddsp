/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "dspmodule.h"

const unsigned short dspmodule_requiredAPIversion = 4;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    if (!lapi->addport("dummy", NULL, DSPPortDirection_Input, 0))
    { lapi->log(DSPLogLevel_Fatal, "failed adding dummy input port"); return DSPModuleReturnCode_Stop; }
    if (!lapi->addport("dummy", NULL, DSPPortDirection_Output, 0))
    { lapi->log(DSPLogLevel_Fatal, "failed adding dummy output port"); return DSPModuleReturnCode_Stop; }

    opts->sysname = "test module 1";
    return DSPModuleReturnCode_Continue;
}

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{ return DSPModuleReturnCode_Continue; }

DSPModuleReturnCode dspmodule_statechanged(const DSPLoaderAPI * restrict lapi, DSPModuleStateChangedFunctionOptions * restrict opts)
{
    lapi->log(DSPLogLevel_Info, "prevstate: %s\ncurrstate: %s",
        opts->prevstate == DSPState_Paused ? "paused" : "streaming",
        opts->currstate == DSPState_Paused ? "paused" : "streaming");
    return DSPModuleReturnCode_Continue;
}