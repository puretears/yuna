#ifndef __LINKAGE_H__
#define __LINKAGE_H__

#define SYMBOL_NAME(name) name
#define SYMBOL_NAME_LABEL(name) name##:

#define ENTRY(name) \
.global SYMBOL_NAME(name); \
SYMBOL_NAME_LABEL(name)

#endif
