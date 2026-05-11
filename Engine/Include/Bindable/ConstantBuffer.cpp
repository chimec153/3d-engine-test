// Template implementations moved to ConstantBuffer.h so consumers outside
// Engine.dll (e.g., Editor instantiating ConstantBuffer with editor-local
// structs like OUTLINECBUFFER) can produce all needed symbols locally
// rather than relying on Engine.dll's pre-instantiated set.
#include "ConstantBuffer.h"
