#include <stdint.h>

#if defined(__x86_64__)
__attribute__((ms_abi))
#endif
uintptr_t efi_main(void *image, void *systab)
{
	(void)image;
	(void)systab;
	return 0;
}
