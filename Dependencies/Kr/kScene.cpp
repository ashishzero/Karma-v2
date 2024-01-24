#include "kScene.h"
#include "kContext.h"

#define CGLTF_MALLOC(size) kAlloc(size)
#define CGLTF_FREE(ptr)    kFree(ptr, 0)
#define CGLTF_IMPLEMENTATION
#include "glTF/cgltf.h"
