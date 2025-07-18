# Copyright 2012 Emilie Gillet.
#
# Author: Emilie Gillet (emilie.o.gillet@gmail.com)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 
# See http://creativecommons.org/licenses/MIT/ for more information.

# System specifications
F_CRYSTAL      = 8000000L
F_CPU          = 72000000L
SYSCLOCK       = SYSCLK_FREQ_72MHz
FAMILY         = f10x
DENSITY        = md
MEMORY_MODE    = flash
# USB            = enabled

APPLICATION    = TRUE
BOOTLOADER     = braids_bootloader

# Preferred upload command
UPLOAD_COMMAND  = upload_combo_jtag

# Packages to build
TARGET         = braids
PACKAGES       = braids braids/drivers stmlib/utils stmlib/system braids/vocalist
RESOURCES      = braids/resources

TOOLCHAIN_PATH ?= /usr/local/arm-4.8.3/

SKIP_MINIMIZATION = build/braids/harmonics.o build/braids/stack.o build/braids/quantizer.o

include stmlib/makefile.inc

# don't use -Os for FM harmonics algorithm as it slows down computation resulting in dropouts
$(BUILD_DIR)%.o: ARCHFLAGS = -mcpu=cortex-m3 -mthumb -fno-unroll-loops -Os
$(SKIP_MINIMIZATION): ARCHFLAGS = -mcpu=cortex-m3 -mthumb -fno-unroll-loops -O3

# Rule for building the firmware update file
wav:  $(TARGET_BIN)
	PYTHONPATH=. python stm_audio_bootloader/qpsk/encoder.py \
		-s 48000 -b 12000 -c 6000 -p 256 \
		$(TARGET_BIN)
