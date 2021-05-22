//Modified version of https://github.com/riscv/riscv-pk/blob/master/machine/encoding.h


// Copyright (c) 2013, The Regents of the University of California (Regents).
// All Rights Reserved.
// IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
// SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
// OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
// HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
// MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


// TODO: ADD AS REFERENCE TO FYP


#ifndef _RISCV_BITS_H
#define _RISCV_BITS_H

# define SLL32    sll
# define STORE    sw
# define LOAD     lw
# define LWU      lw
# define LOG_REGBYTES 2
#define REGBYTES (1 << LOG_REGBYTES)

#endif