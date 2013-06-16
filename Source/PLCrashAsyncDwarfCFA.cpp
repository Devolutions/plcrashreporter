/*
 * Copyright (c) 2013 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#include "PLCrashAsyncDwarfCFA.h"
#include "dwarf_opstream.h"
#include <inttypes.h>

/**
 * @internal
 * @ingroup plcrash_async_dwarf
 * @{
 */

/**
 * Evaluate a DWARF CFA program, as defined in the DWARF 4 Specification, Section 6.4.2. This
 * internal implementation is templated to support 32-bit and 64-bit evaluation.
 *
 * @param mobj The memory object from which the expression opcodes will be read.
 * @param cie_info The CIE info data for this opcode stream.
 * @param ptr_state GNU EH pointer state configuration; this defines the base addresses and other
 * information required to decode pointers in the CFA opcode stream. May be NULL if eh_frame
 * augmentation data is not available in @a cie_info.
 * @param byteoder The byte order of the data referenced by @a mobj.
 * @param address The task-relative address within @a mobj at which the opcodes will be fetched.
 * @param offset An offset to be applied to @a address.
 * @param length The total length of the opcodes readable at @a address + @a offset.
 *
 * @return Returns PLCRASH_ESUCCESS on success, or an appropriate plcrash_error_t values
 * on failure. If an invalid opcode is detected, PLCRASH_ENOTSUP will be returned.
 *
 * @todo Consider defining updated status codes or error handling to provide more structured
 * error data on failure.
 */
plcrash_error_t plcrash_async_dwarf_eval_cfa_program (plcrash_async_mobject_t *mobj,
                                                      plcrash_async_dwarf_cie_info_t *cie_info,
                                                      plcrash_async_dwarf_gnueh_ptr_state_t *ptr_state,
                                                      const plcrash_async_byteorder_t *byteorder,
                                                      pl_vm_address_t address,
                                                      pl_vm_off_t offset,
                                                      pl_vm_size_t length)
{
    plcrash::dwarf_opstream opstream;
    plcrash_error_t err;
    uint64_t ip = 0;

    /* Default to reading as a standard machine word */
    DW_EH_PE_t gnu_eh_ptr_encoding = DW_EH_PE_absptr;
    if (cie_info->has_eh_augmentation && cie_info->eh_augmentation.has_pointer_encoding && ptr_state != NULL) {
        gnu_eh_ptr_encoding = (DW_EH_PE_t) cie_info->eh_augmentation.pointer_encoding;
    }

    /* Configure the opstream */
    if ((err = opstream.init(mobj, byteorder, address, offset, length)) != PLCRASH_ESUCCESS)
        return err;

    uint8_t opcode;
    while (opstream.read_intU(&opcode)) {
        uint8_t const_operand = 0;

        /* Check for opcodes encoded in the top two bits, with an operand
         * in the bottom 6 bits. */
        if ((opcode & 0xC0) != 0) {
            opcode &= 0xC0;
            const_operand = opcode & 0x3F;
        }
        
        switch (opcode) {
            case DW_CFA_set_loc:
                if (cie_info->segment_size != 0) {
                    PLCF_DEBUG("Segment support has not been implemented");
                    return PLCRASH_ENOTSUP;
                }

                /* Try reading an eh_frame encoded pointer */
                if (!opstream.read_gnueh_ptr(ptr_state, gnu_eh_ptr_encoding, &ip)) {
                    PLCF_DEBUG("DW_CFA_set_loc failed to read the target pointer value");
                    return PLCRASH_EINVAL;
                }
                break;
                
            case DW_CFA_nop:
                break;
                
            default:
                PLCF_DEBUG("Unsupported opcode 0x%" PRIx8, opcode);
                return PLCRASH_ENOTSUP;
        }
    }

    return PLCRASH_ESUCCESS;
}

/**
 * @}
 */