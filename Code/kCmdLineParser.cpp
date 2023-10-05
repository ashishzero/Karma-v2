#include "kCmdLineParser.h"
#include "kArray.h"

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
	kString str;
	int		num;
};

struct kArg
{
	kString			key;
	kString			desc;
	kArgType		type;
	void		   *dst;
	kArgDefault		def;
	kSlice<kString> opts;
};

static kArray<kArg> cmd_args;

//
//
//

static void kCmdLineArg(kString key, kString desc, kSlice<kString> opts, kArgType type, void *dst, kArgDefault def)
{
	kArg *arg = cmd_args.Add();
	arg->key  = key;
	arg->desc = desc;
	arg->opts = opts;
	arg->type = type;
	arg->dst  = dst;
	arg->def  = def;
}

void kCmdLineFlag(kString key, bool *val, kString desc)
{
	kCmdLineArg(key, desc, {}, kArgType_Flag, val, {});
}

void kCmdLineBoolean(kString key, bool def, bool *val, kString desc)
{
	kArgDefault arg = {.num = def};
	kCmdLineArg(key, desc, {}, kArgType_Boolean, val, arg);
}

void kCmdLineNumber(kString key, int def, int *val, kString desc)
{
	kArgDefault arg = {.num = def};
	kCmdLineArg(key, desc, {}, kArgType_Number, val, arg);
}

void kCmdLineString(kString key, kString def, kString *val, kString desc)
{
	kArgDefault arg = {.str = def};
	kCmdLineArg(key, desc, {}, kArgType_String, val, arg);
}

void kCmdLineOptions(kString key, int def, kSlice<kString> opts, int *val, kString desc)
{
	kAssert(def < opts.count);
	kArgDefault arg = {.num = def};
	kCmdLineArg(key, desc, opts, kArgType_Options, val, arg);
}

void kCmdLinePrintUsage(void)
{
	printf("\n Usage: \n\n");

	for (kArg &arg : cmd_args)
	{
		printf("  -%-8s: " kStrFmt "\n", arg.key.data, kStrArg(arg.desc));

		if (arg.type != kArgType_Flag && arg.type != kArgType_Options)
			printf("    Default: ");

		if (arg.type == kArgType_Boolean)
		{
			printf("%s", arg.def.num ? "true" : "false");
		}
		else if (arg.type == kArgType_Number)
		{
			printf("%lld", (long long)arg.def.num);
		}
		else if (arg.type == kArgType_String)
		{
			printf(kStrFmt, kStrArg(arg.def.str));
		}

		if (arg.type != kArgType_Flag && arg.type != kArgType_Options)
			printf("\n");

		if (arg.type == kArgType_Options && arg.opts.count)
		{
			printf("    Values : " kStrFmt, kStrArg(arg.opts[0]));
			if (arg.def.num == 0) printf(" (default)");

			for (imem i = 1; i < arg.opts.count; ++i)
			{
				printf(", " kStrFmt, kStrArg(arg.opts[i]));
				if (arg.def.num == i) printf(" (default)");
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
	bool *dst = (bool *)arg->dst;
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
			   kStrArg(arg->key));
	}
}

static void kHandleNumber(kArg *arg, kString value)
{
	char *endptr = 0;
	int	  number = strtol((char *)value.data, &endptr, 10);

	if (endptr == (char *)value.data + value.count)
	{
		int *dst = (int *)arg->dst;
		*dst	 = number;
	}
	else
	{
		printf("  Error: Expected number but got \"" kStrFmt "\" for " kStrFmt ".\n", kStrArg(value),
			   kStrArg(arg->key));
	}
}

static void kHandleString(kArg *arg, kString value)
{
	kString *dst = (kString *)arg->dst;
	*dst		 = value;
}

static void kHandleOptions(kArg *arg, kString value)
{
	bool valid = false;
	for (int i = 0; i < arg->opts.count; ++i)
	{
		if (arg->opts[i] == value)
		{
			int *dst = (int *)arg->dst;
			*dst	 = i;
			valid	 = true;
			break;
		}
	}
	if (!valid)
	{
		printf("  Invalid value \"" kStrFmt "\" for option " kStrFmt ".\n", kStrArg(value), kStrArg(arg->key));
		printf("  Possible values: \n");
		printf(kStrFmt, kStrArg(arg->opts[0]));
		for (imem i = 1; i < arg->opts.count; ++i)
		{
			printf(", " kStrFmt, kStrArg(arg->opts[i]));
		}
		printf("\n");
	}
}

static void kHandleArg(kArg *arg, kString value)
{
	if (arg->type == kArgType_Boolean)
	{
		kHandleBoolean(arg, value);
	}
	else if (arg->type == kArgType_Number)
	{
		kHandleNumber(arg, value);
	}
	else if (arg->type == kArgType_String)
	{
		kHandleString(arg, value);
	}
	else if (arg->type == kArgType_Options)
	{
		kHandleOptions(arg, value);
	}
}

void kCmdLineParse(int *argc, const char ***argv, bool ignore_invalids)
{
	for (kArg &carg : cmd_args)
	{
		if (carg.type == kArgType_Flag)
		{
			bool *dst = (bool *)carg.dst;
			*dst	  = false;
		}
		else if (carg.type == kArgType_Boolean)
		{
			bool *dst = (bool *)carg.dst;
			*dst	  = carg.def.num;
		}
		else if (carg.type == kArgType_Number)
		{
			int *dst = (int *)carg.dst;
			*dst	 = carg.def.num;
		}
		else if (carg.type == kArgType_String)
		{
			kString *dst = (kString *)carg.dst;
			*dst		 = carg.def.str;
		}
		else if (carg.type == kArgType_Options)
		{
			int *dst = (int *)carg.dst;
			*dst	 = carg.def.num;
		}
	}

	kNextCmdLineArg(argc, argv);

	while (*argc)
	{
		kString arg		= kNextCmdLineArg(argc, argv);
		bool	handled = false;

		if (kStartsWith(arg, "-"))
		{
			arg = kRemovePrefix(arg, 1);
			kString key, value;
			if (kSplitString(arg, ":", &key, &value))
			{
				for (kArg &carg : cmd_args)
				{
					if (carg.type == kArgType_Flag)
						continue;
					if (carg.key == key)
					{
						kHandleArg(&carg, value);
						handled = true;
						break;
					}
				}
			}
		}
		else
		{
			for (kArg &carg : cmd_args)
			{
				if (carg.type != kArgType_Flag)
					continue;
				if (carg.key == arg)
				{
					bool *dst = (bool *)carg.dst;
					*dst	  = true;
					handled	  = true;
					break;
				}
			}
		}

		if (!handled)
		{
			if (ignore_invalids)
			{
				printf("  Unknown argument: \"" kStrFmt "\". Ignoring.\n", kStrArg(arg));
			}
			else
			{
				return;
			}
		}
	}
}
