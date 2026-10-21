/*
 * Copyright (C) Emily <info@emy.sh>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library under certain conditions as described in each individual source file,
 * and distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all
 * of the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the
 * file(s), but you are not obligated to do so. If you do not wish to do
 * so, delete this exception statement from your version. If you delete
 * this exception statement from all source files in the program, then
 * also delete it here.
 */

#ifndef SBSIGNTOOLS_PE_H
#define SBSIGNTOOLS_PE_H

#define IMAGE_FILE_MACHINE_I386 0x014c
#define IMAGE_FILE_MACHINE_THUMB 0x01c2
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_AARCH64 0xaa64
#define IMAGE_FILE_MACHINE_RISCV64 0x5064

struct external_PEI_DOS_hdr {
	char e_magic[2];
	char reserved[58];
	char e_lfanew[4];
	char dos_message[64];
};

struct external_PEI_IMAGE_hdr {
	char nt_signature[4];
	char f_magic[2];
	char f_nscns[2];
	char f_timdat[4];
	char f_symptr[4];
	char f_nsyms[4];
	char f_opthdr[2];
	char f_flags[2];
};

struct pe_optional_standard32 {
	char magic[2];
	char vstamp[2];
	char tsize[4];
	char dsize[4];
	char bsize[4];
	char entry[4];
	char text_start[4];
	char data_start[4];
};

struct pe_optional_standard64 {
	char magic[2];
	char vstamp[2];
	char tsize[4];
	char dsize[4];
	char bsize[4];
	char entry[4];
	char text_start[4];
};

typedef struct {
	struct pe_optional_standard32 standard;
	char ImageBase[4];
	char SectionAlignment[4];
	char FileAlignment[4];
	char MajorOperatingSystemVersion[2];
	char MinorOperatingSystemVersion[2];
	char MajorImageVersion[2];
	char MinorImageVersion[2];
	char MajorSubsystemVersion[2];
	char MinorSubsystemVersion[2];
	char Reserved1[4];
	char SizeOfImage[4];
	char SizeOfHeaders[4];
	char CheckSum[4];
	char Subsystem[2];
	char DllCharacteristics[2];
	char SizeOfStackReserve[4];
	char SizeOfStackCommit[4];
	char SizeOfHeapReserve[4];
	char SizeOfHeapCommit[4];
	char LoaderFlags[4];
	char NumberOfRvaAndSizes[4];
	char DataDirectory[16][2][4];
} PEAOUTHDR;

typedef struct {
	struct pe_optional_standard64 standard;
	char ImageBase[8];
	char SectionAlignment[4];
	char FileAlignment[4];
	char MajorOperatingSystemVersion[2];
	char MinorOperatingSystemVersion[2];
	char MajorImageVersion[2];
	char MinorImageVersion[2];
	char MajorSubsystemVersion[2];
	char MinorSubsystemVersion[2];
	char Reserved1[4];
	char SizeOfImage[4];
	char SizeOfHeaders[4];
	char CheckSum[4];
	char Subsystem[2];
	char DllCharacteristics[2];
	char SizeOfStackReserve[8];
	char SizeOfStackCommit[8];
	char SizeOfHeapReserve[8];
	char SizeOfHeapCommit[8];
	char LoaderFlags[4];
	char NumberOfRvaAndSizes[4];
	char DataDirectory[16][2][4];
} PEPAOUTHDR;

struct external_scnhdr {
	char s_name[8];
	char s_paddr[4];
	char s_vaddr[4];
	char s_size[4];
	char s_scnptr[4];
	char s_relptr[4];
	char s_lnnoptr[4];
	char s_nreloc[2];
	char s_nlnno[2];
	char s_flags[4];
};

#endif
