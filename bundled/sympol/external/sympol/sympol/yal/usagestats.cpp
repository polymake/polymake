// ---------------------------------------------------------------------------
//
// This file is part of SymPol
//
// Copyright (C) 2006-2010  Thomas Rehn <thomas@carmen76.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// ---------------------------------------------------------------------------

#include "usagestats.h"

#include <cstdio>
#include <unistd.h>
#include <sys/resource.h>

#include "config.h"

ulong yal::UsageStats::processSize() {
#if HAVE_SYSCONF_PROTO
    char buf[30];
    snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
    FILE* pf = fopen(buf, "r");
    if (pf) {
        unsigned size; //       total program size
        unsigned resident;//   resident set size
        //unsigned share;//      shared pages
        //unsigned text;//       text (code)
        //unsigned lib;//        library
        //unsigned data;//       data/stack
        //unsigned dt;//         dirty pages (unused in Linux 2.6)
        if (fscanf(pf, "%u %u" /* %u %u %u %u"*/, &size, &resident/*, &share, &text, &lib, &data*/) == EOF) {
            size = 0;
            resident = 0;
        }
        fclose(pf);
        return resident * sysconf(_SC_PAGESIZE);
    }
    fclose(pf);
#endif
    return -1;
}

double yal::UsageStats::processTimeUser() {
#if HAVE_GETRUSAGE_PROTO
    struct rusage usage;
    int ret = getrusage( RUSAGE_SELF, &usage);
    if (!ret) {
        return double(usage.ru_utime.tv_sec) 
            + double(usage.ru_utime.tv_usec) / 1000000.0;
    }
#endif
    return 0.0;
}
