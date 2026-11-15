/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "dspmodule.h"

const unsigned short dspmodule_requiredAPIversion = 4;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    lapi->log(DSPLogLevel_Debug, "debug\ntext");
    lapi->log(DSPLogLevel_Info, "info\ntext");
    lapi->log(DSPLogLevel_Warning, "warning\ntext");
    lapi->log(DSPLogLevel_Error, "error\ntext");
    lapi->log(DSPLogLevel_Fatal, "fatal error\ntext");
    return DSPModuleReturnCode_Quit;
}

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{ return DSPModuleReturnCode_Quit; }