#if defined(__AVX512F__)
#define OBLAS_AVX512
#else
#if defined(__AVX2__)
#define OBLAS_AVX2
#else
#if defined(__aarch64__) // Also used by arm64 Windows
#define OBLAS_NEON
#else
#if defined(__SSSE3__)
#define OBLAS_SSE3
#endif
#endif
#endif
#endif
