//
// Copyright (c) 2018, Enrico Gregori, Alessandro Improta, Luca Sani, Institute
// of Informatics and Telematics of the Italian National Research Council
// (IIT-CNR). All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE IIT-CNR BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <CUnit/CUnit.h>
#include <isolario/log.h>
#include <isolario/vt100.h>
#include <stdlib.h>

void testlog(void)
{
    logopen(NULL, lmodecol | lmodestamp);
    loglevel(logdev);
    logprintf(logdev, "Debug");
    logprintf(loginfo, "Info");
    logprintf(loginfo, VTGRN VTBLD VTITL "Custom Info " VTHLN " one" VTVLN "two?");
    logprintf(logwarn, "Warning");
    logprintf(logerr, "Error");
    logprintf(loginfo, "END OF FIRST");
    logclose();
    
    logopen("logtest_output.log", lmodenocon | lmodecol | lmodecreat | lmodestamp);
    logprintf(loginfo, "This is a normal info");
    logprintf(loginfo, VTGRN "This Info should not be green");
    logprintf(loginfo, VTBLC "This Info should not be preceeded by a bottom left corner");
    logprintf(logwarn, "Warn");
    logclose();
    logprintf(logwarn, "Warn out of log");
    
    logopen(NULL, lmodecol);
    logclose();
}
