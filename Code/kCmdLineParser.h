#pragma once

#include "kCommon.h"

void kCmdLineFlag(kString key, bool *val, kString desc);
void kCmdLineBoolean(kString key, bool def, bool *val, kString desc);
void kCmdLineNumber(kString key, int def, int *val, kString desc);
void kCmdLineString(kString key, kString def, kString *val, kString desc);
void kCmdLineOptions(kString key, int def, kSlice<kString> opts, int *val, kString desc);

void kCmdLinePrintUsage(void);
void kCmdLineParse(int *argc, const char ***argv, bool ignore_invalids = true);
