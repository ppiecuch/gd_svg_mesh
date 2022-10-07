/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#include "interface.h"

static const char tove_version[] = "TOVE_VERSION";

extern "C" {

EXPORT const char *GetVersion() {
	return tove_version;
}

}
