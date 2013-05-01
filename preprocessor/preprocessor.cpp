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

#include <preprocessor/preprocessor.h>
#include <utils.h>

reaver::assembler::preprocessor::preprocessor(reaver::assembler::frontend & front) : _frontend{ front }, _parser{ _lexer }
{
}

std::vector<reaver::assembler::line> reaver::assembler::preprocessor::preprocess(const std::string & _buffer)
{
    if (_frontend.default_includes())
    {
        for (auto & file : _frontend.get_default_includes())
        {
            std::stringstream input{ _frontend.read_file(file, { { std::string("<command line>"), 0, false } }).first };

            _include_stream(input, { { std::string("<command line>"), 0, false }, { file, 0, false } });
        }
    }

    std::stringstream input(_buffer);
    include_chain include_chain{ { _frontend.absolute_name(), 0, false } };

    _include_stream(input, include_chain);

    for (auto & x : _lines)
    {
        std::cout << *x << std::endl;
    }

    return _lines;
}

void reaver::assembler::preprocessor::_include_stream(std::istream & input, include_chain include_chain)
{
    std::string buffer;
    uint64_t new_lines = 1;

    while (std::getline(input, buffer) && (include_chain.back().line += new_lines))
    {
        new_lines = 1;
        std::vector<std::string> v{ buffer };

        if (include_chain.size() > 128)
        {
            print_include_chain(include_chain);
            dlog(error) << "maximum include (or macro) depth (128) reached.";

            std::exit(-1);
        }

        while (buffer.size() && buffer.back() == '\\')
        {
            buffer.pop_back();

            std::string next;

            if (!std::getline(input, next))
            {
                print_include_chain(include_chain);
                dlog(error) << "invalid `\\` at the end of file.";

                std::exit(-1);
            }

            buffer.append(next);
            v.push_back(next);
            ++new_lines;
        }

        auto tokens = reaver::lexer::tokenize(buffer, _lexer.desc);
        auto begin = tokens.cbegin();

        {
            auto define = reaver::parser::parse(_parser.define, begin, tokens.cend(), _parser.skip);

            if (define)
            {
                if (_defines.find(define->name) != _defines.end())
                {
                    print_include_chain(include_chain);
                    dlog(error) << "macro name `" << style::style(colors::bgray, colors::def, styles::bold) << define->name
                        << style::style() << "` redefined.";
                    print_include_chain(_defines[define->name].source());
                    dlog() << "Note: first defined here.";
                }

                if (define->directive == "%define" && !define->args)
                {
                    std::string definition;

                    for (; begin != tokens.cend(); ++begin)
                    {
                        definition.append(begin->as<std::string>());
                    }

                    _defines.emplace(define->name, assembler::define{ define->name, definition, include_chain });
                }

                else if (define->directive == "%xdefine" && !define->args)
                {
                    _defines.emplace(define->name, assembler::define{ define->name, _apply_defines(begin, tokens.cend(),
                        include_chain), include_chain});
                }

                else if (define->directive == "%define")
                {
                    std::string definition;

                    while (begin != tokens.cend())
                    {
                        definition.append((begin++)->as<std::string>());
                    }

                    _defines.emplace(define->name, assembler::define{ define->name, *define->args, definition,
                        include_chain });
                }

                else
                {
                    _defines.emplace(define->name, assembler::define{ define->name, *define->args, _apply_defines(begin,
                        tokens.cend(), include_chain), include_chain });
                }

                continue;
            }

            _lines.emplace_back(_apply_defines(buffer, include_chain), include_chain, v);
        }
    }
}

std::string reaver::assembler::preprocessor::_apply_defines(std::vector<lexer::token>::const_iterator begin,
    std::vector<lexer::token>::const_iterator end, include_chain inc) const
{
    std::string ret;

    while (begin != end)
    {
        if (_defines.find(begin->as<std::string>()) != _defines.end())
        {
            auto & macro = _defines.at(begin->as<std::string>());

            if (macro.parameters().size())
            {
                auto t = begin;
                auto match = reaver::parser::parse(_parser.macro_call, t, end, _parser.skip);

                if (match)
                {

                }

                else
                {
                    print_include_chain(inc);
                    dlog(error) << "No parameters supplied for macro `" << style::style(colors::white, colors::def, styles::bold)
                        << macro.name() << style::style() << "` taking " << macro.parameters().size() << " arguments.";
                }
            }

            else
            {
                ret.append(_apply_defines(macro.definition(), inc));
            }
        }

        else
        {
            ret.append(begin->as<std::string>());
        }

        ++begin;
    }

    return ret;
}

std::string reaver::assembler::preprocessor::_apply_defines(std::string str, include_chain inc) const
{
    auto t = reaver::lexer::tokenize(str, _lexer.desc);
    return _apply_defines(t.cbegin(), t.cend(), inc);
}
