这是一个用于进行内存管理的库，可以有效的查找内存泄漏和内存越界，并且能在很大程度上避免内存碎片
在不需要进行内存泄漏和内存越界的定位时，请注释掉CMemoryMgr.h文件中的以下三个宏，以避免性能问题
#define MEMORY_LEASK
#define CHECK_MEMORY_ERROR
#define CHECK_MEMORY_BOUND

This is a library for memory management, can effectively locate memory leaks and memory out of bounds, and can largely avoid memory fragmentation.
To avoid performance problems, comment out the following three macros in the CMemoryMgr.h when you do not need to locate memory leaks and out of bounds.
#define MEMORY_LEASK
#define CHECK_MEMORY_ERROR
#define CHECK_MEMORY_BOUND