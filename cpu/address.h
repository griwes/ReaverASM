/**
 * Reaver Project Assembler License
 *
 * Copyright (C) 2013 Reaver Project Team:
 * 1. Michał "Griwes" Dominiak
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation is required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Michał "Griwes" Dominiak
 *
 **/

#pragma once

#include <boost/variant.hpp>

#include <cpu/overrides.h>
#include <cpu/register.h>

#include <parser/helpers.h>

#include <utils.h>

namespace reaver
{
    namespace assembler
    {
        class effective_address : public operand_base
        {
        public:
            effective_address(const boost::optional<cpu_register> seg, const std::vector<boost::variant<cpu_register,
                integer, size_overriden_identifier>> & v)
            {
                if (v.size() > 4)
                {
                    throw "too many address operands for effective address";
                }

                operands = v;
                segment = seg;
            }

            boost::optional<cpu_register> segment;
            std::vector<boost::variant<cpu_register, integer, size_overriden_identifier>> operands;

            virtual bool has_segment_override() const
            {
                return segment;
            }

            virtual const cpu_register & get_segment_override() const
            {
                return *segment;
            }
        };
    }
}
