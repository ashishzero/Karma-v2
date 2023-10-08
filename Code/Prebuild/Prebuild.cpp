#include "kPrebuild.h"
#include "kPlatform.h"

void Main(int argc, const char **argv)
{
	if (!kExecutePrebuild())
	{
		kTerminate(1);
	}
}
