/* 
   Unix SMB/CIFS implementation.

   Test LDB attribute functions

   Copyright (C) Andrew Bartlet <abartlet@samba.org> 2008
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "includes.h"
#include "lib/events/events.h"
#include <ldb.h>
#include <ldb-samba/ldb_wrap.h>
#include <ldb_errors.h>
#include <ldb_module.h>
#include "lib/ldb-samba/ldif_handlers.h"
#include "ldb_wrap.h"
#include "dsdb/samdb/samdb.h"
#include "param/param.h"
#include "torture/smbtorture.h"
#include "torture/local/proto.h"

static const char *sid = "S-1-5-21-4177067393-1453636373-93818737";
static const char *hex_sid = "01040000000000051500000081fdf8f815bba456718f9705";
static const char *guid = "975ac5fa-35d9-431d-b86a-845bcd34fff9";
static const char *guid2 = "{975ac5fa-35d9-431d-b86a-845bcd34fff9}";
static const char *hex_guid = "fac55a97d9351d43b86a845bcd34fff9";

static const char *prefix_map_newline = "2:1.2.840.113556.1.2\n5:2.16.840.1.101.2.2.3";
static const char *prefix_map_semi = "2:1.2.840.113556.1.2;5:2.16.840.1.101.2.2.3";

/**
 * This is the hex code derived from the tdbdump for
 * "st/ad_dc/private/sam.ldb.d/DC=ADDC,DC=SAMBA,DC=EXAMPLE,DC=COM.ldb"
 * key "DN=CN=DDA1D01D-4BD7-4C49-A184-46F9241B560E,CN=OPERATIONS,CN=DOMAINUPDATES,CN=SYSTEM,DC=ADDC,DC=SAMBA,DC=EXAMPLE,DC=COM\00"
 *   -- adrianc
 */

static const uint8_t dda1d01d_bin[] = {
	0x67, 0x19, 0x01, 0x26, 0x0d, 0x00, 0x00, 0x00, 0x43, 0x4e, 0x3d, 0x64, 0x64, 0x61, 0x31, 0x64,
	0x30, 0x31, 0x64, 0x2d, 0x34, 0x62, 0x64, 0x37, 0x2d, 0x34, 0x63, 0x34, 0x39, 0x2d, 0x61, 0x31,
	0x38, 0x34, 0x2d, 0x34, 0x36, 0x66, 0x39, 0x32, 0x34, 0x31, 0x62, 0x35, 0x36, 0x30, 0x65, 0x2c,
	0x43, 0x4e, 0x3d, 0x4f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x2c, 0x43, 0x4e,
	0x3d, 0x44, 0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x73, 0x2c, 0x43,
	0x4e, 0x3d, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x2c, 0x44, 0x43, 0x3d, 0x61, 0x64, 0x64, 0x63,
	0x2c, 0x44, 0x43, 0x3d, 0x73, 0x61, 0x6d, 0x62, 0x61, 0x2c, 0x44, 0x43, 0x3d, 0x65, 0x78, 0x61,
	0x6d, 0x70, 0x6c, 0x65, 0x2c, 0x44, 0x43, 0x3d, 0x63, 0x6f, 0x6d, 0x00, 0x6f, 0x62, 0x6a, 0x65,
	0x63, 0x74, 0x43, 0x6c, 0x61, 0x73, 0x73, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x74, 0x6f, 0x70, 0x00, 0x09, 0x00, 0x00, 0x00, 0x63, 0x6f, 0x6e, 0x74, 0x61, 0x69, 0x6e, 0x65,
	0x72, 0x00, 0x63, 0x6e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x64, 0x64, 0x61,
	0x31, 0x64, 0x30, 0x31, 0x64, 0x2d, 0x34, 0x62, 0x64, 0x37, 0x2d, 0x34, 0x63, 0x34, 0x39, 0x2d,
	0x61, 0x31, 0x38, 0x34, 0x2d, 0x34, 0x36, 0x66, 0x39, 0x32, 0x34, 0x31, 0x62, 0x35, 0x36, 0x30,
	0x65, 0x00, 0x69, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x54, 0x79, 0x70, 0x65, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x34, 0x00, 0x77, 0x68, 0x65, 0x6e, 0x43, 0x72, 0x65,
	0x61, 0x74, 0x65, 0x64, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x32, 0x30, 0x31,
	0x35, 0x30, 0x37, 0x30, 0x38, 0x32, 0x32, 0x34, 0x33, 0x31, 0x30, 0x2e, 0x30, 0x5a, 0x00, 0x77,
	0x68, 0x65, 0x6e, 0x43, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x32, 0x30, 0x31, 0x35, 0x30, 0x37, 0x30, 0x38, 0x32, 0x32, 0x34, 0x33, 0x31,
	0x30, 0x2e, 0x30, 0x5a, 0x00, 0x75, 0x53, 0x4e, 0x43, 0x72, 0x65, 0x61, 0x74, 0x65, 0x64, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x33, 0x34, 0x36, 0x37, 0x00, 0x75, 0x53, 0x4e,
	0x43, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x33, 0x34, 0x36, 0x37, 0x00, 0x73, 0x68, 0x6f, 0x77, 0x49, 0x6e, 0x41, 0x64, 0x76, 0x61, 0x6e,
	0x63, 0x65, 0x64, 0x56, 0x69, 0x65, 0x77, 0x4f, 0x6e, 0x6c, 0x79, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x54, 0x52, 0x55, 0x45, 0x00, 0x6e, 0x54, 0x53, 0x65, 0x63, 0x75, 0x72,
	0x69, 0x74, 0x79, 0x44, 0x65, 0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x6f, 0x72, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x18, 0x05, 0x00, 0x00, 0x01, 0x00, 0x17, 0x8c, 0x14, 0x00, 0x00, 0x00, 0x30, 0x00,
	0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x9a, 0xbd, 0x91, 0x7d, 0xd5, 0xe0, 0x11, 0x3c, 0x6e, 0x5e,
	0x1a, 0x4b, 0x00, 0x02, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00,
	0x00, 0x00, 0x9a, 0xbd, 0x91, 0x7d, 0xd5, 0xe0, 0x11, 0x3c, 0x6e, 0x5e, 0x1a, 0x4b, 0x00, 0x02,
	0x00, 0x00, 0x04, 0x00, 0x78, 0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x5a, 0x38, 0x00, 0x20, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xbe, 0x3b, 0x0e, 0xf3, 0xf0, 0x9f, 0xd1, 0x11, 0xb6, 0x03,
	0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1, 0xa5, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85,
	0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x07, 0x5a, 0x38, 0x00, 0x20, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xbf, 0x3b,
	0x0e, 0xf3, 0xf0, 0x9f, 0xd1, 0x11, 0xb6, 0x03, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1, 0xa5, 0x7a,
	0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x54, 0x04, 0x17, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0xff, 0x01, 0x0f, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x9a, 0xbd, 0x91, 0x7d, 0xd5, 0xe0, 0x11, 0x3c, 0x6e, 0x5e,
	0x1a, 0x4b, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0xff, 0x01, 0x0f, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x94, 0x00,
	0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0b, 0x00, 0x00, 0x00, 0x05, 0x1a,
	0x3c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x42, 0x16, 0x4c, 0xc0, 0x20,
	0xd0, 0x11, 0xa7, 0x68, 0x00, 0xaa, 0x00, 0x6e, 0x05, 0x29, 0x14, 0xcc, 0x28, 0x48, 0x37, 0x14,
	0xbc, 0x45, 0x9b, 0x07, 0xad, 0x6f, 0x01, 0x5e, 0x5f, 0x28, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x3c, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x42, 0x16, 0x4c, 0xc0, 0x20, 0xd0, 0x11, 0xa7, 0x68,
	0x00, 0xaa, 0x00, 0x6e, 0x05, 0x29, 0xba, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85,
	0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00,
	0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x3c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x10, 0x20, 0x20, 0x5f, 0xa5, 0x79, 0xd0, 0x11, 0x90, 0x20, 0x00, 0xc0, 0x4f, 0xc2,
	0xd4, 0xcf, 0x14, 0xcc, 0x28, 0x48, 0x37, 0x14, 0xbc, 0x45, 0x9b, 0x07, 0xad, 0x6f, 0x01, 0x5e,
	0x5f, 0x28, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02,
	0x00, 0x00, 0x05, 0x1a, 0x3c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10, 0x20,
	0x20, 0x5f, 0xa5, 0x79, 0xd0, 0x11, 0x90, 0x20, 0x00, 0xc0, 0x4f, 0xc2, 0xd4, 0xcf, 0xba, 0x7a,
	0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a,
	0x3c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x40, 0xc2, 0x0a, 0xbc, 0xa9, 0x79,
	0xd0, 0x11, 0x90, 0x20, 0x00, 0xc0, 0x4f, 0xc2, 0xd4, 0xcf, 0x14, 0xcc, 0x28, 0x48, 0x37, 0x14,
	0xbc, 0x45, 0x9b, 0x07, 0xad, 0x6f, 0x01, 0x5e, 0x5f, 0x28, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x3c, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x40, 0xc2, 0x0a, 0xbc, 0xa9, 0x79, 0xd0, 0x11, 0x90, 0x20,
	0x00, 0xc0, 0x4f, 0xc2, 0xd4, 0xcf, 0xba, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85,
	0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00,
	0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x3c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x42, 0x2f, 0xba, 0x59, 0xa2, 0x79, 0xd0, 0x11, 0x90, 0x20, 0x00, 0xc0, 0x4f, 0xc2,
	0xd3, 0xcf, 0x14, 0xcc, 0x28, 0x48, 0x37, 0x14, 0xbc, 0x45, 0x9b, 0x07, 0xad, 0x6f, 0x01, 0x5e,
	0x5f, 0x28, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02,
	0x00, 0x00, 0x05, 0x1a, 0x3c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x42, 0x2f,
	0xba, 0x59, 0xa2, 0x79, 0xd0, 0x11, 0x90, 0x20, 0x00, 0xc0, 0x4f, 0xc2, 0xd3, 0xcf, 0xba, 0x7a,
	0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a,
	0x3c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x88, 0x70, 0x03, 0xe1, 0x0a,
	0xd2, 0x11, 0xb4, 0x22, 0x00, 0xa0, 0xc9, 0x68, 0xf9, 0x39, 0x14, 0xcc, 0x28, 0x48, 0x37, 0x14,
	0xbc, 0x45, 0x9b, 0x07, 0xad, 0x6f, 0x01, 0x5e, 0x5f, 0x28, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x3c, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x88, 0x70, 0x03, 0xe1, 0x0a, 0xd2, 0x11, 0xb4, 0x22,
	0x00, 0xa0, 0xc9, 0x68, 0xf9, 0x39, 0xba, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85,
	0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00,
	0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x38, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x6d, 0x9e, 0xc6, 0xb7, 0xc7, 0x2c, 0xd2, 0x11, 0x85, 0x4e, 0x00, 0xa0, 0xc9, 0x83,
	0xf6, 0x08, 0x86, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30,
	0x49, 0xe2, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x09, 0x00, 0x00, 0x00, 0x05, 0x1a,
	0x38, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6d, 0x9e, 0xc6, 0xb7, 0xc7, 0x2c,
	0xd2, 0x11, 0x85, 0x4e, 0x00, 0xa0, 0xc9, 0x83, 0xf6, 0x08, 0x9c, 0x7a, 0x96, 0xbf, 0xe6, 0x0d,
	0xd0, 0x11, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x09, 0x00, 0x00, 0x00, 0x05, 0x1a, 0x38, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x6d, 0x9e, 0xc6, 0xb7, 0xc7, 0x2c, 0xd2, 0x11, 0x85, 0x4e, 0x00, 0xa0, 0xc9, 0x83,
	0xf6, 0x08, 0xba, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30,
	0x49, 0xe2, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x09, 0x00, 0x00, 0x00, 0x05, 0x1a,
	0x2c, 0x00, 0x94, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0xcc, 0x28, 0x48, 0x37, 0x14,
	0xbc, 0x45, 0x9b, 0x07, 0xad, 0x6f, 0x01, 0x5e, 0x5f, 0x28, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x2c, 0x00, 0x94, 0x00,
	0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x9c, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85,
	0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00,
	0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x05, 0x1a, 0x2c, 0x00, 0x94, 0x00, 0x02, 0x00, 0x02, 0x00,
	0x00, 0x00, 0xba, 0x7a, 0x96, 0xbf, 0xe6, 0x0d, 0xd0, 0x11, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30,
	0x49, 0xe2, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x2a, 0x02,
	0x00, 0x00, 0x05, 0x12, 0x28, 0x00, 0x30, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xde, 0x47,
	0xe6, 0x91, 0x6f, 0xd9, 0x70, 0x4b, 0x95, 0x57, 0xd6, 0x3f, 0xf4, 0xf3, 0xcc, 0xd8, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x12, 0x24, 0x00, 0xff, 0x01,
	0x0f, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x9a, 0xbd,
	0x91, 0x7d, 0xd5, 0xe0, 0x11, 0x3c, 0x6e, 0x5e, 0x1a, 0x4b, 0x07, 0x02, 0x00, 0x00, 0x00, 0x12,
	0x18, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00,
	0x00, 0x00, 0x2a, 0x02, 0x00, 0x00, 0x00, 0x12, 0x18, 0x00, 0xbd, 0x01, 0x0f, 0x00, 0x01, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x00, 0x6e,
	0x61, 0x6d, 0x65, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x64, 0x64, 0x61, 0x31,
	0x64, 0x30, 0x31, 0x64, 0x2d, 0x34, 0x62, 0x64, 0x37, 0x2d, 0x34, 0x63, 0x34, 0x39, 0x2d, 0x61,
	0x31, 0x38, 0x34, 0x2d, 0x34, 0x36, 0x66, 0x39, 0x32, 0x34, 0x31, 0x62, 0x35, 0x36, 0x30, 0x65,
	0x00, 0x6f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x47, 0x55, 0x49, 0x44, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x57, 0x93, 0x1e, 0x29, 0x25, 0x49, 0xe5, 0x40, 0x9d, 0x98, 0x36, 0x07,
	0x11, 0x9e, 0xbd, 0xe5, 0x00, 0x72, 0x65, 0x70, 0x6c, 0x50, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x74,
	0x79, 0x4d, 0x65, 0x74, 0x61, 0x44, 0x61, 0x74, 0x61, 0x00, 0x01, 0x00, 0x00, 0x00, 0x90, 0x01,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xa9, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x19, 0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0e, 0x03, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7e, 0x38, 0xae, 0x0b, 0x03, 0x00,
	0x00, 0x00, 0x9d, 0xcd, 0xcd, 0x57, 0xee, 0x58, 0x6e, 0x4e, 0x96, 0x99, 0xcc, 0x7d, 0xe1, 0x96,
	0xf1, 0x05, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x6f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x43, 0x61, 0x74, 0x65, 0x67, 0x6f, 0x72,
	0x79, 0x00, 0x01, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x3c, 0x47, 0x55, 0x49, 0x44, 0x3d,
	0x35, 0x32, 0x34, 0x32, 0x39, 0x30, 0x33, 0x38, 0x2d, 0x65, 0x34, 0x33, 0x35, 0x2d, 0x34, 0x66,
	0x65, 0x33, 0x2d, 0x39, 0x36, 0x34, 0x65, 0x2d, 0x38, 0x30, 0x64, 0x61, 0x31, 0x35, 0x34, 0x39,
	0x39, 0x63, 0x39, 0x63, 0x3e, 0x3b, 0x43, 0x4e, 0x3d, 0x43, 0x6f, 0x6e, 0x74, 0x61, 0x69, 0x6e,
	0x65, 0x72, 0x2c, 0x43, 0x4e, 0x3d, 0x53, 0x63, 0x68, 0x65, 0x6d, 0x61, 0x2c, 0x43, 0x4e, 0x3d,
	0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2c, 0x44, 0x43,
	0x3d, 0x61, 0x64, 0x64, 0x63, 0x2c, 0x44, 0x43, 0x3d, 0x73, 0x61, 0x6d, 0x62, 0x61, 0x2c, 0x44,
	0x43, 0x3d, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2c, 0x44, 0x43, 0x3d, 0x63, 0x6f, 0x6d,
	0x00
};

static const char dda1d01d_ldif[] = ""
"dn: CN=dda1d01d-4bd7-4c49-a184-46f9241b560e,CN=Operations,CN=DomainUpdates,CN=System,DC=addc,DC=samba,DC=example,DC=com\n"
"objectClass: top\n"
"objectClass: container\n"
"cn: dda1d01d-4bd7-4c49-a184-46f9241b560e\n"
"instanceType: 4\n"
"whenCreated: 20150708224310.0Z\n"
"whenChanged: 20150708224310.0Z\n"
"uSNCreated: 3467\n"
"uSNChanged: 3467\n"
"showInAdvancedViewOnly: TRUE\n"
"nTSecurityDescriptor: O:S-1-5-21-2106703258-1007804629-1260019310-512G:S-1-5-2\n"
" 1-2106703258-1007804629-1260019310-512D:AI(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;S-\n"
" 1-5-21-2106703258-1007804629-1260019310-512)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;\n"
" SY)(A;;RPLCLORC;;;AU)(OA;CIIOID;RP;4c164200-20c0-11d0-a768-00aa006e0529;4828c\n"
" c14-1437-45bc-9b07-ad6f015e5f28;RU)(OA;CIIOID;RP;4c164200-20c0-11d0-a768-00aa\n"
" 006e0529;bf967aba-0de6-11d0-a285-00aa003049e2;RU)(OA;CIIOID;RP;5f202010-79a5-\n"
" 11d0-9020-00c04fc2d4cf;4828cc14-1437-45bc-9b07-ad6f015e5f28;RU)(OA;CIIOID;RP;\n"
" 5f202010-79a5-11d0-9020-00c04fc2d4cf;bf967aba-0de6-11d0-a285-00aa003049e2;RU)\n"
" (OA;CIIOID;RP;bc0ac240-79a9-11d0-9020-00c04fc2d4cf;4828cc14-1437-45bc-9b07-ad\n"
" 6f015e5f28;RU)(OA;CIIOID;RP;bc0ac240-79a9-11d0-9020-00c04fc2d4cf;bf967aba-0de\n"
" 6-11d0-a285-00aa003049e2;RU)(OA;CIIOID;RP;59ba2f42-79a2-11d0-9020-00c04fc2d3c\n"
" f;4828cc14-1437-45bc-9b07-ad6f015e5f28;RU)(OA;CIIOID;RP;59ba2f42-79a2-11d0-90\n"
" 20-00c04fc2d3cf;bf967aba-0de6-11d0-a285-00aa003049e2;RU)(OA;CIIOID;RP;037088f\n"
" 8-0ae1-11d2-b422-00a0c968f939;4828cc14-1437-45bc-9b07-ad6f015e5f28;RU)(OA;CII\n"
" OID;RP;037088f8-0ae1-11d2-b422-00a0c968f939;bf967aba-0de6-11d0-a285-00aa00304\n"
" 9e2;RU)(OA;CIIOID;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967a86-0de6-11d0-\n"
" a285-00aa003049e2;ED)(OA;CIIOID;RP;b7c69e6d-2cc7-11d2-854e-00a0c983f608;bf967\n"
" a9c-0de6-11d0-a285-00aa003049e2;ED)(OA;CIIOID;RP;b7c69e6d-2cc7-11d2-854e-00a0\n"
" c983f608;bf967aba-0de6-11d0-a285-00aa003049e2;ED)(OA;CIIOID;RPLCLORC;;4828cc1\n"
" 4-1437-45bc-9b07-ad6f015e5f28;RU)(OA;CIIOID;RPLCLORC;;bf967a9c-0de6-11d0-a285\n"
" -00aa003049e2;RU)(OA;CIIOID;RPLCLORC;;bf967aba-0de6-11d0-a285-00aa003049e2;RU\n"
" )(OA;CIID;RPWPCR;91e647de-d96f-4b70-9557-d63ff4f3ccd8;;PS)(A;CIID;RPWPCRCCDCL\n"
" CLORCWOWDSDDTSW;;;S-1-5-21-2106703258-1007804629-1260019310-519)(A;CIID;LC;;;\n"
" RU)(A;CIID;RPWPCRCCLCLORCWOWDSDSW;;;BA)S:AI(OU;CIIOIDSA;WP;f30e3bbe-9ff0-11d1\n"
" -b603-0000f80367c1;bf967aa5-0de6-11d0-a285-00aa003049e2;WD)(OU;CIIOIDSA;WP;f3\n"
" 0e3bbf-9ff0-11d1-b603-0000f80367c1;bf967aa5-0de6-11d0-a285-00aa003049e2;WD)\n"
"name: dda1d01d-4bd7-4c49-a184-46f9241b560e\n"
"objectGUID: 291e9357-4925-40e5-9d98-3607119ebde5\n"
"replPropertyMetaData:: AQAAAAAAAAAIAAAAAAAAAAAAAAABAAAAfjiuCwMAAACdzc1X7lhuTpa\n"
" ZzH3hlvEFiw0AAAAAAACLDQAAAAAAAAEAAgABAAAAfjiuCwMAAACdzc1X7lhuTpaZzH3hlvEFiw0A\n"
" AAAAAACLDQAAAAAAAAIAAgABAAAAfjiuCwMAAACdzc1X7lhuTpaZzH3hlvEFiw0AAAAAAACLDQAAA\n"
" AAAAKkAAgABAAAAfjiuCwMAAACdzc1X7lhuTpaZzH3hlvEFiw0AAAAAAACLDQAAAAAAABkBAgABAA\n"
" AAfjiuCwMAAACdzc1X7lhuTpaZzH3hlvEFiw0AAAAAAACLDQAAAAAAAAEACQABAAAAfjiuCwMAAAC\n"
" dzc1X7lhuTpaZzH3hlvEFiw0AAAAAAACLDQAAAAAAAA4DCQABAAAAfjiuCwMAAACdzc1X7lhuTpaZ\n"
" zH3hlvEFiw0AAAAAAACLDQAAAAAAAAMAAAABAAAAfjiuCwMAAACdzc1X7lhuTpaZzH3hlvEFiw0AA\n"
" AAAAACLDQAAAAAAAA==\n"
"objectCategory: <GUID=52429038-e435-4fe3-964e-80da15499c9c>;CN=Container,CN=Sc\n"
" hema,CN=Configuration,DC=addc,DC=samba,DC=example,DC=com\n\n";

static const char *dda1d01d_ldif_reduced = ""
"dn: CN=dda1d01d-4bd7-4c49-a184-46f9241b560e,CN=Operations,CN=DomainUpdates,CN=System,DC=addc,DC=samba,DC=example,DC=com\n"
"objectClass: top\n"
"objectClass: container\n"
"instanceType: 4\n"
"whenChanged: 20150708224310.0Z\n"
"uSNCreated: 3467\n"
"showInAdvancedViewOnly: TRUE\n"
"name: dda1d01d-4bd7-4c49-a184-46f9241b560e\n\n";

static bool torture_ldb_attrs(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	const struct ldb_schema_attribute *attr;
	struct ldb_val string_sid_blob, binary_sid_blob;
	struct ldb_val string_guid_blob, string_guid_blob2, binary_guid_blob;
	struct ldb_val string_prefix_map_newline_blob, string_prefix_map_semi_blob, string_prefix_map_blob;
	struct ldb_val prefix_map_blob;

	DATA_BLOB sid_blob = strhex_to_data_blob(mem_ctx, hex_sid);
	DATA_BLOB guid_blob = strhex_to_data_blob(mem_ctx, hex_guid);

	torture_assert(torture, 
		       ldb = ldb_init(mem_ctx, torture->ev),
		       "Failed to init ldb");

	torture_assert_int_equal(torture, 
				 ldb_register_samba_handlers(ldb), LDB_SUCCESS,
				 "Failed to register Samba handlers");

	ldb_set_utf8_fns(ldb, NULL, wrap_casefold);

	/* Test SID behaviour */
	torture_assert(torture, attr = ldb_schema_attribute_by_name(ldb, "objectSid"), 
		       "Failed to get objectSid schema attribute");
	
	string_sid_blob = data_blob_string_const(sid);

	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_read_fn(ldb, mem_ctx, 
							    &string_sid_blob, &binary_sid_blob), 0,
				 "Failed to parse string SID");
	
	torture_assert_data_blob_equal(torture, binary_sid_blob, sid_blob, 
				       "Read SID into blob form failed");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_read_fn(ldb, mem_ctx, 
							    &sid_blob, &binary_sid_blob), -1,
				 "Should have failed to parse binary SID");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_write_fn(ldb, mem_ctx, &binary_sid_blob, &string_sid_blob), 0,
				 "Failed to parse binary SID");
	
	torture_assert_data_blob_equal(torture, 
				       string_sid_blob, data_blob_string_const(sid),
				       "Write SID into string form failed");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &binary_sid_blob, &string_sid_blob), 0,
				 "Failed to compare binary and string SID");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &string_sid_blob, &binary_sid_blob), 0,
				 "Failed to compare string and binary binary SID");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &string_sid_blob, &string_sid_blob), 0,
				 "Failed to compare string and string SID");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &binary_sid_blob, &binary_sid_blob), 0,
				 "Failed to compare binary and binary SID");
	
	torture_assert(torture, attr->syntax->comparison_fn(ldb, mem_ctx, &guid_blob, &binary_sid_blob) != 0,
		       "Failed to distinguish binary GUID and binary SID");


	/* Test GUID behaviour */
	torture_assert(torture, attr = ldb_schema_attribute_by_name(ldb, "objectGUID"), 
		       "Failed to get objectGUID schema attribute");
	
	string_guid_blob = data_blob_string_const(guid);

	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_read_fn(ldb, mem_ctx, 
							    &string_guid_blob, &binary_guid_blob), 0,
				 "Failed to parse string GUID");
	
	torture_assert_data_blob_equal(torture, binary_guid_blob, guid_blob, 
				       "Read GUID into blob form failed");
	
	string_guid_blob2 = data_blob_string_const(guid2);
	
	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_read_fn(ldb, mem_ctx, 
							    &string_guid_blob2, &binary_guid_blob), 0,
				 "Failed to parse string GUID");
	
	torture_assert_data_blob_equal(torture, binary_guid_blob, guid_blob, 
				       "Read GUID into blob form failed");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_read_fn(ldb, mem_ctx, 
							    &guid_blob, &binary_guid_blob), 0,
				 "Failed to parse binary GUID");
	
	torture_assert_data_blob_equal(torture, binary_guid_blob, guid_blob, 
				       "Read GUID into blob form failed");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_write_fn(ldb, mem_ctx, &binary_guid_blob, &string_guid_blob), 0,
				 "Failed to print binary GUID as string");

	torture_assert_data_blob_equal(torture, string_sid_blob, data_blob_string_const(sid),
				       "Write SID into string form failed");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &binary_guid_blob, &string_guid_blob), 0,
				 "Failed to compare binary and string GUID");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &string_guid_blob, &binary_guid_blob), 0,
				 "Failed to compare string and binary binary GUID");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &string_guid_blob, &string_guid_blob), 0,
				 "Failed to compare string and string GUID");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &binary_guid_blob, &binary_guid_blob), 0,
				 "Failed to compare binary and binary GUID");
	
	string_prefix_map_newline_blob = data_blob_string_const(prefix_map_newline);
	
	string_prefix_map_semi_blob = data_blob_string_const(prefix_map_semi);
	
	/* Test prefixMap behaviour */
	torture_assert(torture, attr = ldb_schema_attribute_by_name(ldb, "prefixMap"), 
		       "Failed to get prefixMap schema attribute");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &string_prefix_map_newline_blob, &string_prefix_map_semi_blob), 0,
				 "Failed to compare prefixMap with newlines and prefixMap with semicolons");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_read_fn(ldb, mem_ctx, &string_prefix_map_newline_blob, &prefix_map_blob), 0,
				 "Failed to read prefixMap with newlines");
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &string_prefix_map_newline_blob, &prefix_map_blob), 0,
				 "Failed to compare prefixMap with newlines and prefixMap binary");
	
	torture_assert_int_equal(torture, 
				 attr->syntax->ldif_write_fn(ldb, mem_ctx, &prefix_map_blob, &string_prefix_map_blob), 0,
				 "Failed to write prefixMap");
	torture_assert_int_equal(torture, 
				 attr->syntax->comparison_fn(ldb, mem_ctx, &string_prefix_map_blob, &prefix_map_blob), 0,
				 "Failed to compare prefixMap ldif write and prefixMap binary");
	
	torture_assert_data_blob_equal(torture, string_prefix_map_blob, string_prefix_map_semi_blob,
		"Failed to compare prefixMap ldif write and prefixMap binary");
	


	talloc_free(mem_ctx);
	return true;
}

static bool torture_ldb_dn_attrs(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	const struct ldb_dn_extended_syntax *attr;
	struct ldb_val string_sid_blob, binary_sid_blob;
	struct ldb_val string_guid_blob, binary_guid_blob;
	struct ldb_val hex_sid_blob, hex_guid_blob;

	DATA_BLOB sid_blob = strhex_to_data_blob(mem_ctx, hex_sid);
	DATA_BLOB guid_blob = strhex_to_data_blob(mem_ctx, hex_guid);

	torture_assert(torture, 
		       ldb = ldb_init(mem_ctx, torture->ev),
		       "Failed to init ldb");

	torture_assert_int_equal(torture, 
				 ldb_register_samba_handlers(ldb), LDB_SUCCESS,
				 "Failed to register Samba handlers");

	ldb_set_utf8_fns(ldb, NULL, wrap_casefold);

	/* Test SID behaviour */
	torture_assert(torture, attr = ldb_dn_extended_syntax_by_name(ldb, "SID"), 
		       "Failed to get SID DN syntax");
	
	string_sid_blob = data_blob_string_const(sid);

	torture_assert_int_equal(torture, 
				 attr->read_fn(ldb, mem_ctx, 
					       &string_sid_blob, &binary_sid_blob), 0,
				 "Failed to parse string SID");
	
	torture_assert_data_blob_equal(torture, binary_sid_blob, sid_blob, 
				       "Read SID into blob form failed");

	hex_sid_blob = data_blob_string_const(hex_sid);
	
	torture_assert_int_equal(torture, 
				 attr->read_fn(ldb, mem_ctx, 
					       &hex_sid_blob, &binary_sid_blob), 0,
				 "Failed to parse HEX SID");
	
	torture_assert_data_blob_equal(torture, binary_sid_blob, sid_blob, 
				       "Read SID into blob form failed");
	
	torture_assert_int_equal(torture, 
				 attr->read_fn(ldb, mem_ctx, 
					       &sid_blob, &binary_sid_blob), -1,
				 "Should have failed to parse binary SID");
	
	torture_assert_int_equal(torture, 
				 attr->write_hex_fn(ldb, mem_ctx, &sid_blob, &hex_sid_blob), 0,
				 "Failed to parse binary SID");
	
	torture_assert_data_blob_equal(torture, 
				       hex_sid_blob, data_blob_string_const(hex_sid),
				       "Write SID into HEX string form failed");
	
	torture_assert_int_equal(torture, 
				 attr->write_clear_fn(ldb, mem_ctx, &sid_blob, &string_sid_blob), 0,
				 "Failed to parse binary SID");
	
	torture_assert_data_blob_equal(torture, 
				       string_sid_blob, data_blob_string_const(sid),
				       "Write SID into clear string form failed");
	

	/* Test GUID behaviour */
	torture_assert(torture, attr = ldb_dn_extended_syntax_by_name(ldb, "GUID"), 
		       "Failed to get GUID DN syntax");
	
	string_guid_blob = data_blob_string_const(guid);

	torture_assert_int_equal(torture, 
				 attr->read_fn(ldb, mem_ctx, 
					       &string_guid_blob, &binary_guid_blob), 0,
				 "Failed to parse string GUID");
	
	torture_assert_data_blob_equal(torture, binary_guid_blob, guid_blob, 
				       "Read GUID into blob form failed");
	
	hex_guid_blob = data_blob_string_const(hex_guid);
	
	torture_assert_int_equal(torture, 
				 attr->read_fn(ldb, mem_ctx, 
					       &hex_guid_blob, &binary_guid_blob), 0,
				 "Failed to parse HEX GUID");
	
	torture_assert_data_blob_equal(torture, binary_guid_blob, guid_blob, 
				       "Read GUID into blob form failed");
	
	torture_assert_int_equal(torture, 
				 attr->read_fn(ldb, mem_ctx, 
					       &guid_blob, &binary_guid_blob), -1,
				 "Should have failed to parse binary GUID");
	
	torture_assert_int_equal(torture, 
				 attr->write_hex_fn(ldb, mem_ctx, &guid_blob, &hex_guid_blob), 0,
				 "Failed to parse binary GUID");
	
	torture_assert_data_blob_equal(torture, 
				       hex_guid_blob, data_blob_string_const(hex_guid),
				       "Write GUID into HEX string form failed");
	
	torture_assert_int_equal(torture, 
				 attr->write_clear_fn(ldb, mem_ctx, &guid_blob, &string_guid_blob), 0,
				 "Failed to parse binary GUID");
	
	torture_assert_data_blob_equal(torture, 
				       string_guid_blob, data_blob_string_const(guid),
				       "Write GUID into clear string form failed");
	


	talloc_free(mem_ctx);
	return true;
}

static bool torture_ldb_dn_extended(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	struct ldb_dn *dn, *dn2;

	DATA_BLOB sid_blob = strhex_to_data_blob(mem_ctx, hex_sid);
	DATA_BLOB guid_blob = strhex_to_data_blob(mem_ctx, hex_guid);

	const char *dn_str = "cn=admin,cn=users,dc=samba,dc=org";

	torture_assert(torture, 
		       ldb = ldb_init(mem_ctx, torture->ev),
		       "Failed to init ldb");

	torture_assert_int_equal(torture, 
				 ldb_register_samba_handlers(ldb), LDB_SUCCESS,
				 "Failed to register Samba handlers");

	ldb_set_utf8_fns(ldb, NULL, wrap_casefold);

	/* Check behaviour of a normal DN */
	torture_assert(torture, 
		       dn = ldb_dn_new(mem_ctx, ldb, dn_str), 
		       "Failed to create a 'normal' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn),
		       "Failed to validate 'normal' DN");

	torture_assert(torture, ldb_dn_has_extended(dn) == false, 
		       "Should not find plain DN to be 'extended'");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "SID") == NULL, 
		       "Should not find an SID on plain DN");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "GUID") == NULL, 
		       "Should not find an GUID on plain DN");
	
	torture_assert(torture, ldb_dn_get_extended_component(dn, "WKGUID") == NULL, 
		       "Should not find an WKGUID on plain DN");
	
	/* Now make an extended DN */
	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<GUID=%s>;<SID=%s>;%s",
					   guid, sid, dn_str), 
		       "Failed to create an 'extended' DN");

	torture_assert(torture, 
		       dn2 = ldb_dn_copy(mem_ctx, dn), 
		       "Failed to copy the 'extended' DN");
	talloc_free(dn);
	dn = dn2;

	torture_assert(torture, 
		       ldb_dn_validate(dn),
		       "Failed to validate 'extended' DN");

	torture_assert(torture, ldb_dn_has_extended(dn) == true, 
		       "Should find extended DN to be 'extended'");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "SID") != NULL, 
		       "Should find an SID on extended DN");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "GUID") != NULL, 
		       "Should find an GUID on extended DN");
	
	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "SID"), sid_blob, 
				       "Extended DN SID incorect");

	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "GUID"), guid_blob, 
				       "Extended DN GUID incorect");

	torture_assert_str_equal(torture, ldb_dn_get_linearized(dn), dn_str, 
				 "linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_casefold(dn), strupper_talloc(mem_ctx, dn_str), 
				 "casefolded DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_component_name(dn, 0), "cn", 
				 "componet zero incorrect");

	torture_assert_data_blob_equal(torture, *ldb_dn_get_component_val(dn, 0), data_blob_string_const("admin"), 
				 "componet zero incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 1),
				 talloc_asprintf(mem_ctx, "<GUID=%s>;<SID=%s>;%s", 
						 guid, sid, dn_str),
				 "Clear extended linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 0),
				 talloc_asprintf(mem_ctx, "<GUID=%s>;<SID=%s>;%s", 
						 hex_guid, hex_sid, dn_str),
				 "HEX extended linearized DN incorrect");

	torture_assert(torture, ldb_dn_remove_child_components(dn, 1) == true,
				 "Failed to remove DN child");
		       
	torture_assert(torture, ldb_dn_has_extended(dn) == false, 
		       "Extended DN flag should be cleared after child element removal");
	
	torture_assert(torture, ldb_dn_get_extended_component(dn, "SID") == NULL, 
		       "Should not find an SID on DN");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "GUID") == NULL, 
		       "Should not find an GUID on DN");


	/* TODO:  test setting these in the other order, and ensure it still comes out 'GUID first' */
	torture_assert_int_equal(torture, ldb_dn_set_extended_component(dn, "GUID", &guid_blob), 0, 
		       "Failed to set a GUID on DN");
	
	torture_assert_int_equal(torture, ldb_dn_set_extended_component(dn, "SID", &sid_blob), 0, 
		       "Failed to set a SID on DN");

	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "SID"), sid_blob, 
				       "Extended DN SID incorect");

	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "GUID"), guid_blob, 
				       "Extended DN GUID incorect");

	torture_assert_str_equal(torture, ldb_dn_get_linearized(dn), "cn=users,dc=samba,dc=org", 
				 "linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 1),
				 talloc_asprintf(mem_ctx, "<GUID=%s>;<SID=%s>;%s", 
						 guid, sid, "cn=users,dc=samba,dc=org"),
				 "Clear extended linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 0),
				 talloc_asprintf(mem_ctx, "<GUID=%s>;<SID=%s>;%s", 
						 hex_guid, hex_sid, "cn=users,dc=samba,dc=org"),
				 "HEX extended linearized DN incorrect");

	/* Now check a 'just GUID' DN (clear format) */
	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<GUID=%s>",
					   guid), 
		       "Failed to create an 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn),
		       "Failed to validate 'extended' DN");

	torture_assert(torture, ldb_dn_has_extended(dn) == true, 
		       "Should find extended DN to be 'extended'");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "SID") == NULL, 
		       "Should not find an SID on this DN");

	torture_assert_int_equal(torture, ldb_dn_get_comp_num(dn), 0, 
		       "Should not find an 'normal' componet on this DN");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "GUID") != NULL, 
		       "Should find an GUID on this DN");
	
	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "GUID"), guid_blob, 
				       "Extended DN GUID incorect");

	torture_assert_str_equal(torture, ldb_dn_get_linearized(dn), "", 
				 "linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 1),
				 talloc_asprintf(mem_ctx, "<GUID=%s>", 
						 guid),
				 "Clear extended linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 0),
				 talloc_asprintf(mem_ctx, "<GUID=%s>", 
						 hex_guid),
				 "HEX extended linearized DN incorrect");

	/* Now check a 'just GUID' DN (HEX format) */
	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<GUID=%s>",
					   hex_guid), 
		       "Failed to create an 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn),
		       "Failed to validate 'extended' DN");

	torture_assert(torture, ldb_dn_has_extended(dn) == true, 
		       "Should find extended DN to be 'extended'");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "SID") == NULL, 
		       "Should not find an SID on this DN");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "GUID") != NULL, 
		       "Should find an GUID on this DN");
	
	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "GUID"), guid_blob, 
				       "Extended DN GUID incorect");

	torture_assert_str_equal(torture, ldb_dn_get_linearized(dn), "", 
				 "linearized DN incorrect");

	/* Now check a 'just SID' DN (clear format) */
	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<SID=%s>",
					   sid), 
		       "Failed to create an 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn),
		       "Failed to validate 'extended' DN");

	torture_assert(torture, ldb_dn_has_extended(dn) == true, 
		       "Should find extended DN to be 'extended'");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "GUID") == NULL, 
		       "Should not find an SID on this DN");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "SID") != NULL, 
		       "Should find an SID on this DN");
	
	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "SID"), sid_blob, 
				       "Extended DN SID incorect");

	torture_assert_str_equal(torture, ldb_dn_get_linearized(dn), "", 
				 "linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 1),
				 talloc_asprintf(mem_ctx, "<SID=%s>", 
						 sid),
				 "Clear extended linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 0),
				 talloc_asprintf(mem_ctx, "<SID=%s>", 
						 hex_sid),
				 "HEX extended linearized DN incorrect");

	/* Now check a 'just SID' DN (HEX format) */
	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<SID=%s>",
					   hex_sid), 
		       "Failed to create an 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn),
		       "Failed to validate 'extended' DN");

	torture_assert(torture, ldb_dn_has_extended(dn) == true, 
		       "Should find extended DN to be 'extended'");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "GUID") == NULL, 
		       "Should not find an SID on this DN");

	torture_assert(torture, ldb_dn_get_extended_component(dn, "SID") != NULL, 
		       "Should find an SID on this DN");
	
	torture_assert_data_blob_equal(torture, *ldb_dn_get_extended_component(dn, "SID"), sid_blob, 
				       "Extended DN SID incorect");

	torture_assert_str_equal(torture, ldb_dn_get_linearized(dn), "", 
				 "linearized DN incorrect");

	talloc_free(mem_ctx);
	return true;
}


static bool torture_ldb_dn(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	struct ldb_dn *dn;
	struct ldb_dn *child_dn;
	struct ldb_dn *typo_dn;
	struct ldb_dn *special_dn;
	struct ldb_val val;

	torture_assert(torture, 
		       ldb = ldb_init(mem_ctx, torture->ev),
		       "Failed to init ldb");

	torture_assert_int_equal(torture, 
				 ldb_register_samba_handlers(ldb), LDB_SUCCESS,
				 "Failed to register Samba handlers");

	ldb_set_utf8_fns(ldb, NULL, wrap_casefold);

	/* Check behaviour of a normal DN */
	torture_assert(torture, 
		       dn = ldb_dn_new(mem_ctx, ldb, NULL), 
		       "Failed to create a NULL DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn),
		       "Failed to validate NULL DN");

	torture_assert(torture, 
		       ldb_dn_add_base_fmt(dn, "dc=org"), 
		       "Failed to add base DN");

	torture_assert(torture, 
		       ldb_dn_add_child_fmt(dn, "dc=samba"), 
		       "Failed to add base DN");

	torture_assert_str_equal(torture, ldb_dn_get_linearized(dn), "dc=samba,dc=org", 
				 "linearized DN incorrect");

	torture_assert_str_equal(torture, ldb_dn_get_extended_linearized(mem_ctx, dn, 0), "dc=samba,dc=org", 
				 "extended linearized DN incorrect");

	/* Check child DN comparisons */
	torture_assert(torture, 
		       child_dn = ldb_dn_new(mem_ctx, ldb, "CN=users,DC=SAMBA,DC=org"), 
		       "Failed to create child DN");

	torture_assert(torture, 
		       ldb_dn_compare(dn, child_dn) != 0,
		       "Comparison on dc=samba,dc=org and CN=users,DC=SAMBA,DC=org should != 0");

	torture_assert(torture, 
		       ldb_dn_compare_base(child_dn, dn) != 0,
		       "Base Comparison of CN=users,DC=SAMBA,DC=org and dc=samba,dc=org should != 0");

	torture_assert(torture, 
		       ldb_dn_compare_base(dn, child_dn) == 0,
		       "Base Comparison on dc=samba,dc=org and CN=users,DC=SAMBA,DC=org should == 0");

	/* Check comparisons with a truncated DN */
	torture_assert(torture, 
		       typo_dn = ldb_dn_new(mem_ctx, ldb, "c=samba,dc=org"), 
		       "Failed to create 'typo' DN");

	torture_assert(torture, 
		       ldb_dn_compare(dn, typo_dn) != 0,
		       "Comparison on dc=samba,dc=org and c=samba,dc=org should != 0");

	torture_assert(torture, 
		       ldb_dn_compare_base(typo_dn, dn) != 0,
		       "Base Comparison of c=samba,dc=org and dc=samba,dc=org should != 0");

	torture_assert(torture, 
		       ldb_dn_compare_base(dn, typo_dn) != 0,
		       "Base Comparison on dc=samba,dc=org and c=samba,dc=org should != 0");

	/* Check comparisons with a special DN */
	torture_assert(torture,
		       special_dn = ldb_dn_new(mem_ctx, ldb, "@special_dn"),
		       "Failed to create 'special' DN");

	torture_assert(torture,
		       ldb_dn_compare(dn, special_dn) != 0,
		       "Comparison on dc=samba,dc=org and @special_dn should != 0");

	torture_assert(torture,
		       ldb_dn_compare_base(special_dn, dn) > 0,
		       "Base Comparison of @special_dn and dc=samba,dc=org should > 0");

	torture_assert(torture,
		       ldb_dn_compare_base(dn, special_dn) < 0,
		       "Base Comparison on dc=samba,dc=org and @special_dn should < 0");

	/* Check DN based on MS-ADTS:3.1.1.5.1.2 Naming Constraints*/
	torture_assert(torture,
		       dn = ldb_dn_new(mem_ctx, ldb, "CN=New\nLine,DC=SAMBA,DC=org"),
		       "Failed to create a DN with 0xA in it");

	/* this is a warning until we work out how the DEL: CNs work */
	if (ldb_dn_validate(dn) != false) {
		torture_warning(torture,
				"should have failed to validate a DN with 0xA in it");
	}

	/* Escaped comma */
	torture_assert(torture,
		       dn = ldb_dn_new(mem_ctx, ldb, "CN=A\\,comma,DC=SAMBA,DC=org"),
		       "Failed to create a DN with an escaped comma in it");


	val = data_blob_const("CN=Zer\0,DC=SAMBA,DC=org", 23);
	torture_assert(torture,
		       NULL == ldb_dn_from_ldb_val(mem_ctx, ldb, &val),
		       "should fail to create a DN with 0x0 in it");

	talloc_free(mem_ctx);
	return true;
}

static bool torture_ldb_dn_invalid_extended(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	struct ldb_dn *dn;

	const char *dn_str = "cn=admin,cn=users,dc=samba,dc=org";

	torture_assert(torture, 
		       ldb = ldb_init(mem_ctx, torture->ev),
		       "Failed to init ldb");

	torture_assert_int_equal(torture, 
				 ldb_register_samba_handlers(ldb), LDB_SUCCESS,
				 "Failed to register Samba handlers");

	ldb_set_utf8_fns(ldb, NULL, wrap_casefold);

	/* Check behaviour of a normal DN */
	torture_assert(torture, 
		       dn = ldb_dn_new(mem_ctx, ldb, "samba,dc=org"), 
		       "Failed to create a 'normal' invalid DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'normal' invalid DN");

	/* Now make an extended DN */
	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<PID=%s>;%s",
					   sid, dn_str), 
		       "Failed to create an invalid 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'extended' DN");

	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<GUID=%s>%s",
					   sid, dn_str), 
		       "Failed to create an invalid 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'extended' DN");

	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<GUID=%s>;",
					   sid), 
		       "Failed to create an invalid 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'extended' DN");

	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<GUID=%s>;",
					   hex_sid), 
		       "Failed to create an invalid 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'extended' DN");

	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<SID=%s>;",
					   hex_guid), 
		       "Failed to create an invalid 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'extended' DN");

	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<SID=%s>;",
					   guid), 
		       "Failed to create an invalid 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'extended' DN");

	torture_assert(torture, 
		       dn = ldb_dn_new_fmt(mem_ctx, ldb, "<GUID=>"), 
		       "Failed to create an invalid 'extended' DN");

	torture_assert(torture, 
		       ldb_dn_validate(dn) == false,
		       "should have failed to validate 'extended' DN");

	return true;
}

static bool helper_ldb_message_compare(struct torture_context *torture,
				       struct ldb_message *a,
				       struct ldb_message *b)
{
	int i;

	if (a->num_elements != b->num_elements) {
		return false;
	}

	for (i = 0; i < a->num_elements; i++) {
		int j;
		struct ldb_message_element x = a->elements[i];
		struct ldb_message_element y = b->elements[i];

		torture_comment(torture, "#%s\n", x.name);
		torture_assert_int_equal(torture, x.flags, y.flags,
					 "Flags do not match");
		torture_assert_str_equal(torture, x.name, y.name,
					 "Names do not match in field");
		torture_assert_int_equal(torture, x.num_values, y.num_values,
					 "Number of values do not match");

		/*
		 * Records cannot round trip via the SDDL string with a
		 * nTSecurityDescriptor field.
		 *
		 * Parsing from SDDL and diffing the NDR dump output gives the
		 * following:
		 *
		 *          in: struct decode_security_descriptor
		 *             sd: struct security_descriptor
		 *                 revision                 : SECURITY_DESCRIPTOR_REVISION_1 (1)
		 *-                type                     : 0x8c14 (35860)
		 *-                       0: SEC_DESC_OWNER_DEFAULTED
		 *-                       0: SEC_DESC_GROUP_DEFAULTED
		 *+                type                     : 0x8c17 (35863)
		 *+                       1: SEC_DESC_OWNER_DEFAULTED
		 *+                       1: SEC_DESC_GROUP_DEFAULTED
		 *                        1: SEC_DESC_DACL_PRESENT
		 *                        0: SEC_DESC_DACL_DEFAULTED
		 *                        1: SEC_DESC_SACL_PRESENT
		 */
		if (strcmp(x.name, "nTSecurityDescriptor") == 0) {
			continue;
		}
		for (j = 0; j < x.num_values; j++) {
			torture_assert_int_equal(torture, x.values[j].length,
						 y.values[j].length,
						 "Does not match in length");
			torture_assert_mem_equal(torture,
						 x.values[j].data,
						 y.values[j].data,
						 x.values[j].length,
						 "Does not match in data");
		}
	}
	return true;
}

static bool torture_ldb_unpack(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	struct ldb_val data = data_blob_const(dda1d01d_bin, sizeof(dda1d01d_bin));
	struct ldb_message *msg = ldb_msg_new(mem_ctx);
	const char *ldif_text = dda1d01d_ldif;
	struct ldb_ldif ldif;

	ldb = samba_ldb_init(mem_ctx, torture->ev, NULL, NULL, NULL);
	torture_assert(torture,
		       ldb != NULL,
		       "Failed to init ldb");

	torture_assert_int_equal(torture, ldb_unpack_data(ldb, &data, msg), 0,
				 "ldb_unpack_data failed");

	ldif.changetype = LDB_CHANGETYPE_NONE;
	ldif.msg = msg;
	ldif_text = ldb_ldif_write_string(ldb, mem_ctx, &ldif);

	torture_assert_int_equal(torture,
				 strcmp(ldif_text, dda1d01d_ldif), 0,
				 "ldif form differs from binary form");
	return true;
}

static bool torture_ldb_unpack_flags(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	struct ldb_val data = data_blob_const(dda1d01d_bin, sizeof(dda1d01d_bin));
	struct ldb_message *msg = ldb_msg_new(mem_ctx);
	const char *ldif_text = dda1d01d_ldif;
	struct ldb_ldif ldif;
	unsigned int nb_elements_in_db;

	ldb = samba_ldb_init(mem_ctx, torture->ev, NULL, NULL, NULL);
	torture_assert(torture,
		       ldb != NULL,
		       "Failed to init ldb");

	torture_assert_int_equal(torture,
				 ldb_unpack_data_only_attr_list_flags(ldb, &data,
								      msg,
								      NULL, 0,
								      LDB_UNPACK_DATA_FLAG_NO_DATA_ALLOC,
								      &nb_elements_in_db),
				 0,
				 "ldb_unpack_data failed");

	ldif.changetype = LDB_CHANGETYPE_NONE;
	ldif.msg = msg;
	ldif_text = ldb_ldif_write_string(ldb, mem_ctx, &ldif);

	torture_assert_int_equal(torture,
				 strcmp(ldif_text, dda1d01d_ldif), 0,
				 "ldif form differs from binary form");

	torture_assert_int_equal(torture,
				 ldb_unpack_data_only_attr_list_flags(ldb, &data,
								      msg,
								      NULL, 0,
								      LDB_UNPACK_DATA_FLAG_NO_DATA_ALLOC|
								      LDB_UNPACK_DATA_FLAG_NO_VALUES_ALLOC,
								      &nb_elements_in_db),
				 0,
				 "ldb_unpack_data failed");

	ldif.changetype = LDB_CHANGETYPE_NONE;
	ldif.msg = msg;
	ldif_text = ldb_ldif_write_string(ldb, mem_ctx, &ldif);

	torture_assert_int_equal(torture,
				 strcmp(ldif_text, dda1d01d_ldif), 0,
				 "ldif form differs from binary form");

	torture_assert_int_equal(torture,
				 ldb_unpack_data_only_attr_list_flags(ldb, &data,
								      msg,
								      NULL, 0,
								      LDB_UNPACK_DATA_FLAG_NO_DN,
								      &nb_elements_in_db),
				 0,
				 "ldb_unpack_data failed");

	torture_assert(torture,
		       msg->dn == NULL,
		       "msg->dn should be NULL");

	return true;
}

static bool torture_ldb_parse_ldif(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	const char *ldif_text = dda1d01d_ldif;
	struct ldb_context *ldb;
	struct ldb_ldif *ldif;
	struct ldb_val binary;
	struct ldb_val data = data_blob_const(dda1d01d_bin, sizeof(dda1d01d_bin));
	struct ldb_message *msg = ldb_msg_new(mem_ctx);

	ldb = samba_ldb_init(mem_ctx, torture->ev, NULL,NULL,NULL);
	torture_assert(torture,
		       ldb != NULL,
		       "Failed to init ldb");

	ldif = ldb_ldif_read_string(ldb, &ldif_text);
	torture_assert(torture,
		       ldif != NULL,
		       "ldb_ldif_read_string failed");
	torture_assert_int_equal(torture, ldif->changetype, LDB_CHANGETYPE_NONE,
				 "changetype is incorrect");
	torture_assert_int_equal(torture,
				 ldb_pack_data(ldb, ldif->msg, &binary), 0,
				 "ldb_pack_data failed");

	torture_assert_int_equal(torture, ldb_unpack_data(ldb, &data, msg), 0,
				 "ldb_unpack_data failed");

	torture_assert(torture,
		       helper_ldb_message_compare(torture, ldif->msg, msg),
		       "Forms differ in memory");

	return true;
}

static bool torture_ldb_unpack_only_attr_list(struct torture_context *torture)
{
	TALLOC_CTX *mem_ctx = talloc_new(torture);
	struct ldb_context *ldb;
	struct ldb_val data = data_blob_const(dda1d01d_bin, sizeof(dda1d01d_bin));
	struct ldb_message *msg = ldb_msg_new(mem_ctx);
	const char *lookup_names[] = {"instanceType", "nonexistant", "whenChanged",
				      "objectClass", "uSNCreated",
				      "showInAdvancedViewOnly", "name", "cnNotHere"};
	unsigned int nb_elements_in_db;
	const char *ldif_text;
	struct ldb_ldif ldif;

	ldb = samba_ldb_init(mem_ctx, torture->ev, NULL, NULL, NULL);
	torture_assert(torture,
		       ldb != NULL,
		       "Failed to init samba");

	torture_assert_int_equal(torture,
				 ldb_unpack_data_only_attr_list(ldb, &data, msg,
							  lookup_names, ARRAY_SIZE(lookup_names),
							  &nb_elements_in_db), 0,
				 "ldb_unpack_data_only_attr_list failed");
	torture_assert_int_equal(torture, nb_elements_in_db, 13,
				 "Got wrong count of elements");

	/* Compare data in binary form */
	torture_assert_int_equal(torture, msg->num_elements, 6,
				 "Got wrong number of parsed elements");

	torture_assert_str_equal(torture, msg->elements[0].name, "objectClass",
				 "First element has wrong name");
	torture_assert_int_equal(torture, msg->elements[0].num_values, 2,
				 "First element has wrong count of values");
	torture_assert_int_equal(torture,
				 msg->elements[0].values[0].length, 3,
				 "First element's first value is of wrong length");
	torture_assert_mem_equal(torture,
				 msg->elements[0].values[0].data, "top", 3,
				 "First element's first value is incorrect");
	torture_assert_int_equal(torture,
				 msg->elements[0].values[1].length, strlen("container"),
				 "First element's second value is of wrong length");
	torture_assert_mem_equal(torture, msg->elements[0].values[1].data,
				 "container", strlen("container"),
				 "First element's second value is incorrect");

	torture_assert_str_equal(torture, msg->elements[1].name, "instanceType",
				 "Second element has wrong name");
	torture_assert_int_equal(torture, msg->elements[1].num_values, 1,
				 "Second element has too many values");
	torture_assert_int_equal(torture, msg->elements[1].values[0].length, 1,
				 "Second element's value is of wrong length");
	torture_assert_mem_equal(torture, msg->elements[1].values[0].data,
				 "4", 1,
				 "Second element's value is incorrect");

	torture_assert_str_equal(torture, msg->elements[2].name, "whenChanged",
				 "Third element has wrong name");
	torture_assert_int_equal(torture, msg->elements[2].num_values, 1,
				 "Third element has too many values");
	torture_assert_int_equal(torture, msg->elements[2].values[0].length,
				 strlen("20150708224310.0Z"),
				 "Third element's value is of wrong length");
	torture_assert_mem_equal(torture, msg->elements[2].values[0].data,
				 "20150708224310.0Z", strlen("20150708224310.0Z"),
				 "Third element's value is incorrect");

	torture_assert_str_equal(torture, msg->elements[3].name, "uSNCreated",
				 "Fourth element has wrong name");
	torture_assert_int_equal(torture, msg->elements[3].num_values, 1,
				 "Fourth element has too many values");
	torture_assert_int_equal(torture, msg->elements[3].values[0].length, 4,
				 "Fourth element's value is of wrong length");
	torture_assert_mem_equal(torture, msg->elements[3].values[0].data,
				 "3467", 4,
				 "Fourth element's value is incorrect");

	torture_assert_str_equal(torture, msg->elements[4].name, "showInAdvancedViewOnly",
				 "Fifth element has wrong name");
	torture_assert_int_equal(torture, msg->elements[4].num_values, 1,
				 "Fifth element has too many values");
	torture_assert_int_equal(torture, msg->elements[4].values[0].length, 4,
				 "Fifth element's value is of wrong length");
	torture_assert_mem_equal(torture, msg->elements[4].values[0].data,
				 "TRUE", 4,
				 "Fourth element's value is incorrect");

	torture_assert_str_equal(torture, msg->elements[5].name, "name",
				 "Sixth element has wrong name");
	torture_assert_int_equal(torture, msg->elements[5].num_values, 1,
				 "Sixth element has too many values");
	torture_assert_int_equal(torture, msg->elements[5].values[0].length,
				 strlen("dda1d01d-4bd7-4c49-a184-46f9241b560e"),
				 "Sixth element's value is of wrong length");
	torture_assert_mem_equal(torture, msg->elements[5].values[0].data,
				 "dda1d01d-4bd7-4c49-a184-46f9241b560e",
				 strlen("dda1d01d-4bd7-4c49-a184-46f9241b560e"),
				 "Sixth element's value is incorrect");

	/* Compare data in ldif form */
	ldif.changetype = LDB_CHANGETYPE_NONE;
	ldif.msg = msg;
	ldif_text = ldb_ldif_write_string(ldb, mem_ctx, &ldif);

	torture_assert_str_equal(torture, ldif_text, dda1d01d_ldif_reduced,
				 "Expected fields did not match");

	return true;
}

struct torture_suite *torture_ldb(TALLOC_CTX *mem_ctx)
{
	struct torture_suite *suite = torture_suite_create(mem_ctx, "ldb");

	if (suite == NULL) {
		return NULL;
	}

	torture_suite_add_simple_test(suite, "attrs", torture_ldb_attrs);
	torture_suite_add_simple_test(suite, "dn-attrs", torture_ldb_dn_attrs);
	torture_suite_add_simple_test(suite, "dn-extended",
				      torture_ldb_dn_extended);
	torture_suite_add_simple_test(suite, "dn-invalid-extended",
				      torture_ldb_dn_invalid_extended);
	torture_suite_add_simple_test(suite, "dn", torture_ldb_dn);
	torture_suite_add_simple_test(suite, "unpack-data",
				      torture_ldb_unpack);
	torture_suite_add_simple_test(suite, "unpack-data-flags",
				      torture_ldb_unpack_flags);
	torture_suite_add_simple_test(suite, "parse-ldif",
				      torture_ldb_parse_ldif);
	torture_suite_add_simple_test(suite, "unpack-data-only-attr-list",
				      torture_ldb_unpack_only_attr_list);

	suite->description = talloc_strdup(suite, "LDB (samba-specific behaviour) tests");

	return suite;
}