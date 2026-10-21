#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	(void)image;
	(void)systab;
	return EFI_SUCCESS;
}
