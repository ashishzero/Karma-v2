#include "kCmdLine.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum kArgType
{
	kArgType_Flag,
	kArgType_Boolean,
	kArgType_Number,
	kArgType_String,
	kArgType_Options,
};

struct kArgDefault
{
	kString Str;
	int     Num;
};

struct kArg
{
	kString        Key;
	kString        Desc;
	kArgType       Type;
	void          *Dest;
	kArgDefault    Default;
	kSpan<kString> Options;
};

#ifndef K_MAX_COMMAND_LINE_ARGS
#define K_MAX_COMMAND_LINE_ARGS 16
#endif

static int  g_CommandLineArgCount;
static kArg g_CommandLineArgs[K_MAX_COMMAND_LINE_ARGS];

//
//
//

static void kCmdLineArg(kString key, kString desc, kSpan<kString> opts, kArgType type, void *dst, kArgDefault def)
{
	if (g_CommandLineArgCount + 1 < K_MAX_COMMAND_LINE_ARGS)
	{
		kArg &arg   = g_CommandLineArgs[g_CommandLineArgCount];
		arg.Key     = key;
		arg.Desc    = desc;
		arg.Options = opts;
		arg.Type    = type;
		arg.Dest    = dst;
		arg.Default = def;
		g_CommandLineArgCount += 1;
	}
}

void kCmdLineFlag(kString key, bool *val, kString desc)
{
	kCmdLineArg(key, desc, {}, kArgType_Flag, val, {});
}

void kCmdLineBoolean(kString key, bool def, bool *val, kString desc)
{
	kArgDefault arg = {.Num = def};
	kCmdLineArg(key, desc, {}, kArgType_Boolean, val, arg);
}

void kCmdLineNumber(kString key, int def, int *val, kString desc)
{
	kArgDefault arg = {.Num = def};
	kCmdLineArg(key, desc, {}, kArgType_Number, val, arg);
}

void kCmdLineString(kString key, kString def, kString *val, kString desc)
{
	kArgDefault arg = {.Str = def};
	kCmdLineArg(key, desc, {}, kArgType_String, val, arg);
}

void kCmdLineOptions(kString key, int def, kSpan<kString> opts, int *val, kString desc)
{
	kAssert(def < opts.Count);
	kArgDefault arg = {.Num = def};
	kCmdLineArg(key, desc, opts, kArgType_Options, val, arg);
}

void kCmdLinePrintUsage(void)
{
	printf("\n Usage: \n\n");

	for (kArg &arg : g_CommandLineArgs)
	{
		printf("  -%-8s: " kStrFmt "\n", arg.Key.Items, kStrArg(arg.Desc));

		if (arg.Type != kArgType_Flag && arg.Type != kArgType_Options)
			printf("    Default: ");

		if (arg.Type == kArgType_Boolean)
		{
			printf("%s", arg.Default.Num ? "true" : "false");
		}
		else if (arg.Type == kArgType_Number)
		{
			printf("%lld", (long long)arg.Default.Num);
		}
		else if (arg.Type == kArgType_String)
		{
			printf(kStrFmt, kStrArg(arg.Default.Str));
		}

		if (arg.Type != kArgType_Flag && arg.Type != kArgType_Options)
			printf("\n");

		if (arg.Type == kArgType_Options && arg.Options.Count)
		{
			printf("    Values : " kStrFmt, kStrArg(arg.Options[0]));
			if (arg.Default.Num == 0)
				printf(" (default)");

			for (imem i = 1; i < arg.Options.Count; ++i)
			{
				printf(", " kStrFmt, kStrArg(arg.Options[i]));
				if (arg.Default.Num == i)
					printf(" (default)");
			}
			printf("\n");
		}
	}

	printf("\n\n");
}

//
//
//

static kString kNextCmdLineArg(int *argc, const char ***argv)
{
	if (*argc)
	{
		const char *arg = **argv;
		(*argc) -= 1;
		(*argv) += 1;
		return kString(arg, strlen(arg));
	}
	return "";
}

static void kHandleBoolean(kArg *arg, kString value)
{
	bool *dst = (bool *)arg->Dest;
	if (value == "1" || value == "true")
	{
		*dst = true;
	}
	else if (value == "0" || value == "false")
	{
		*dst = false;
	}
	else
	{
		printf("  Error: Expected boolean but got \"" kStrFmt "\" for " kStrFmt ".\n", kStrArg(value),
		       kStrArg(arg->Key));
	}
}

static void kHandleNumber(kArg *arg, kString value)
{
	char *endptr = 0;
	int   number = strtol((char *)value.Items, &endptr, 10);

	if (endptr == (char *)value.Items + value.Count)
	{
		int *dst = (int *)arg->Dest;
		*dst     = number;
	}
	else
	{
		printf("  Error: Expected number but got \"" kStrFmt "\" for " kStrFmt ".\n", kStrArg(value),
		       kStrArg(arg->Key));
	}
}

static void kHandleString(kArg *arg, kString value)
{
	kString *dst = (kString *)arg->Dest;
	*dst         = value;
}

static void kHandleOptions(kArg *arg, kString value)
{
	bool valid = false;
	for (int i = 0; i < arg->Options.Count; ++i)
	{
		if (arg->Options[i] == value)
		{
			int *dst = (int *)arg->Dest;
			*dst     = i;
			valid    = true;
			break;
		}
	}
	if (!valid)
	{
		printf("  Invalid value \"" kStrFmt "\" for option " kStrFmt ".\n", kStrArg(value), kStrArg(arg->Key));
		printf("  Possible values: \n");
		printf(kStrFmt, kStrArg(arg->Options[0]));
		for (imem i = 1; i < arg->Options.Count; ++i)
		{
			printf(", " kStrFmt, kStrArg(arg->Options[i]));
		}
		printf("\n");
	}
}

static void kHandleArg(kArg *arg, kString value)
{
	if (arg->Type == kArgType_Boolean)
	{
		kHandleBoolean(arg, value);
	}
	else if (arg->Type == kArgType_Number)
	{
		kHandleNumber(arg, value);
	}
	else if (arg->Type == kArgType_String)
	{
		kHandleString(arg, value);
	}
	else if (arg->Type == kArgType_Options)
	{
		kHandleOptions(arg, value);
	}
}

bool kCmdLineParse(int *argc, const char ***argv, bool ignore_invalids)
{
	bool ok = true;
	for (kArg &carg : g_CommandLineArgs)
	{
		if (carg.Type == kArgType_Flag)
		{
			bool *dst = (bool *)carg.Dest;
			*dst      = false;
		}
		else if (carg.Type == kArgType_Boolean)
		{
			bool *dst = (bool *)carg.Dest;
			*dst      = carg.Default.Num;
		}
		else if (carg.Type == kArgType_Number)
		{
			int *dst = (int *)carg.Dest;
			*dst     = carg.Default.Num;
		}
		else if (carg.Type == kArgType_String)
		{
			kString *dst = (kString *)carg.Dest;
			*dst         = carg.Default.Str;
		}
		else if (carg.Type == kArgType_Options)
		{
			int *dst = (int *)carg.Dest;
			*dst     = carg.Default.Num;
		}
	}

	kNextCmdLineArg(argc, argv);

	while (*argc)
	{
		kString arg     = kNextCmdLineArg(argc, argv);
		bool    handled = false;

		if (kStartsWith(arg, "-"))
		{
			arg = kRemovePrefix(arg, 1);
			kString key, value;
			if (kSplitString(arg, ":", &key, &value))
			{
				for (kArg &carg : g_CommandLineArgs)
				{
					if (carg.Type == kArgType_Flag)
						continue;
					if (carg.Key == key)
					{
						kHandleArg(&carg, value);
						handled = true;
						break;
					}
				}
			}
			else
			{
				printf("  Value for the key \"%.*s\" is not given. Use -key:value syntax to specify value\n",
				       kStrArg(arg));
				handled = true;
				ok      = false;
			}
		}
		else
		{
			for (kArg &carg : g_CommandLineArgs)
			{
				if (carg.Type != kArgType_Flag)
					continue;
				if (carg.Key == arg)
				{
					bool *dst = (bool *)carg.Dest;
					*dst      = true;
					handled   = true;
					break;
				}
			}
		}

		if (!handled)
		{
			ok = false;
			if (ignore_invalids)
			{
				printf("  Unknown argument: \"" kStrFmt "\". Ignoring.\n", kStrArg(arg));
			}
			else
			{
				return false;
			}
		}
	}
	return ok;
}
