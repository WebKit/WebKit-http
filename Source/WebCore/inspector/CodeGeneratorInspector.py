#!/usr/bin/env python
# Copyright (c) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os.path
import sys
import string
import optparse
from string import join
try:
    import json
except ImportError:
    import simplejson as json


DOMAIN_DEFINE_NAME_MAP = {
    "Database": "SQL_DATABASE",
    "Debugger": "JAVASCRIPT_DEBUGGER",
    "DOMDebugger": "JAVASCRIPT_DEBUGGER",
    "FileSystem": "FILE_SYSTEM",
    "IndexedDB": "INDEXED_DATABASE",
    "Profiler": "JAVASCRIPT_DEBUGGER",
    "Worker": "WORKERS",
}


# Manually-filled map of type name replacements.
TYPE_NAME_FIX_MAP = {
    "RGBA": "Rgba",  # RGBA is reported to be conflicting with a define name in Windows CE.
    "": "Empty",
}


TYPES_WITH_RUNTIME_CAST_SET = frozenset([])


cmdline_parser = optparse.OptionParser()
cmdline_parser.add_option("--output_h_dir")
cmdline_parser.add_option("--output_cpp_dir")

try:
    arg_options, arg_values = cmdline_parser.parse_args()
    if (len(arg_values) != 1):
        raise Exception("Exactly one plain argument expected (found %s)" % len(arg_values))
    input_json_filename = arg_values[0]
    output_header_dirname = arg_options.output_h_dir
    output_cpp_dirname = arg_options.output_cpp_dir
    if not output_header_dirname:
        raise Exception("Output .h directory must be specified")
    if not output_cpp_dirname:
        raise Exception("Output .cpp directory must be specified")
except Exception, e:
    sys.stderr.write("Failed to parse command-line arguments: %s\n\n" % e)
    sys.stderr.write("Usage: <script> Inspector.json --output_h_dir <output_header_dir> --output_cpp_dir <output_cpp_dir>\n")
    exit(1)


def dash_to_camelcase(word):
    return ''.join(x.capitalize() or '-' for x in word.split('-'))


class Capitalizer:
    @staticmethod
    def lower_camel_case_to_upper(str):
        if len(str) > 0 and str[0].islower():
            str = str[0].upper() + str[1:]
        return str

    @staticmethod
    def upper_camel_case_to_lower(str):
        pos = 0
        while pos < len(str) and str[pos].isupper():
            pos += 1
        if pos == 0:
            return str
        if pos == 1:
            return str[0].lower() + str[1:]
        if pos < len(str):
            pos -= 1
        possible_abbreviation = str[0:pos]
        if possible_abbreviation not in Capitalizer.ABBREVIATION:
            raise Exception("Unknown abbreviation %s" % possible_abbreviation)
        str = possible_abbreviation.lower() + str[pos:]
        return str

    @staticmethod
    def camel_case_to_capitalized_with_underscores(str):
        if len(str) == 0:
            return str
        output = Capitalizer.split_camel_case_(str)
        return "_".join(output).upper()

    @staticmethod
    def split_camel_case_(str):
        output = []
        pos_being = 0
        pos = 1
        has_oneletter = False
        while pos < len(str):
            if str[pos].isupper():
                output.append(str[pos_being:pos].upper())
                if pos - pos_being == 1:
                    has_oneletter = True
                pos_being = pos
            pos += 1
        output.append(str[pos_being:])
        if has_oneletter:
            array_pos = 0
            while array_pos < len(output) - 1:
                if len(output[array_pos]) == 1:
                    array_pos_end = array_pos + 1
                    while array_pos_end < len(output) and len(output[array_pos_end]) == 1:
                        array_pos_end += 1
                    if array_pos_end - array_pos > 1:
                        possible_abbreviation = "".join(output[array_pos:array_pos_end])
                        if possible_abbreviation.upper() in Capitalizer.ABBREVIATION:
                            output[array_pos:array_pos_end] = [possible_abbreviation]
                        else:
                            array_pos = array_pos_end - 1
                array_pos += 1
        return output

    ABBREVIATION = frozenset(["XHR", "DOM", "CSS"])

VALIDATOR_IFDEF_NAME = "!ASSERT_DISABLED"


class DomainNameFixes:
    @classmethod
    def get_fixed_data(cls, domain_name):
        field_name_res = Capitalizer.upper_camel_case_to_lower(domain_name) + "Agent"

        class Res(object):
            skip_js_bind = domain_name in cls.skip_js_bind_domains
            agent_field_name = field_name_res

            @staticmethod
            def get_guard():
                if domain_name in DOMAIN_DEFINE_NAME_MAP:
                    define_name = DOMAIN_DEFINE_NAME_MAP[domain_name]

                    class Guard:
                        @staticmethod
                        def generate_open(output):
                            output.append("#if ENABLE(%s)\n" % define_name)

                        @staticmethod
                        def generate_close(output):
                            output.append("#endif // ENABLE(%s)\n" % define_name)

                    return Guard

        return Res

    skip_js_bind_domains = set(["Runtime", "DOMDebugger"])


class CParamType(object):
    def __init__(self, type, setter_format="%s"):
        self.type = type
        self.setter_format = setter_format

    def get_text(self):
        return self.type

    def get_setter_format(self):
        return self.setter_format


class RawTypes(object):
    @staticmethod
    def get(json_type):
        if json_type == "boolean":
            return RawTypes.Bool
        elif json_type == "string":
            return RawTypes.String
        elif json_type == "array":
            return RawTypes.Array
        elif json_type == "object":
            return RawTypes.Object
        elif json_type == "integer":
            return RawTypes.Int
        elif json_type == "number":
            return RawTypes.Number
        elif json_type == "any":
            return RawTypes.Any
        else:
            raise Exception("Unknown type: %s" % json_type)

    # For output parameter all values are passed by pointer except RefPtr-based types.
    class OutputPassModel:
        class ByPointer:
            @staticmethod
            def get_argument_prefix():
                return "&"

            @staticmethod
            def get_parameter_type_suffix():
                return "*"

        class ByReference:
            @staticmethod
            def get_argument_prefix():
                return ""

            @staticmethod
            def get_parameter_type_suffix():
                return "&"

    class BaseType(object):
        need_internal_runtime_cast_ = False

        @classmethod
        def get_c_param_type(cls, param_type, optional):
            return cls.default_c_param_type

        @staticmethod
        def is_event_param_check_optional():
            return False

        @classmethod
        def request_raw_internal_runtime_cast(cls):
            if not cls.need_internal_runtime_cast_:
                RawTypes.types_to_generate_validator_.append(cls)
                cls.need_internal_runtime_cast_ = True

        @classmethod
        def generate_validate_method(cls, writer):
            params = cls.get_validate_method_params()
            writer.newline("static void assert%s(InspectorValue* value)\n" % params.name)
            writer.newline("{\n")
            writer.newline("    %s v;\n" % params.var_type)
            writer.newline("    bool castRes = value->as%s(&v);\n" % params.as_method_name)
            writer.newline("    ASSERT_UNUSED(castRes, castRes);\n")
            writer.newline("}\n\n\n")

        @classmethod
        def get_raw_validator_call_text(cls):
            return "assert%s" % cls.get_validate_method_params().name

    types_to_generate_validator_ = []

    @classmethod
    def generate_validate_methods(cls, writer):
        for t in cls.types_to_generate_validator_:
            if t.need_internal_runtime_cast_:
                t.generate_validate_method(writer)


    class String(BaseType):
        @classmethod
        def get_c_param_type(cls, param_type, optional):
            if param_type == ParamType.EVENT or param_type == ParamType.TYPE_BUILDER_OUTPUT:
                return cls._ref_c_type
            else:
                return cls._plain_c_type

        @staticmethod
        def get_getter_name():
            return "String"

        get_setter_name = get_getter_name

        @staticmethod
        def get_c_initializer():
            return "\"\""

        @staticmethod
        def get_js_bind_type():
            return "string"

        @staticmethod
        def get_validate_method_params():
            class ValidateMethodParams:
                name = "String"
                var_type = "String"
                as_method_name = "String"
            return ValidateMethodParams

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByPointer

        @staticmethod
        def is_heavy_value():
            return True

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "String"

        _plain_c_type = CParamType("String")
        _ref_c_type = CParamType("const String&")

    class Int(BaseType):
        @staticmethod
        def get_getter_name():
            return "Int"

        @staticmethod
        def get_setter_name():
            return "Number"

        @staticmethod
        def get_c_initializer():
            return "0"

        @staticmethod
        def get_js_bind_type():
            return "number"

        @staticmethod
        def get_validate_method_params():
            class ValidateMethodParams:
                name = "Int"
                var_type = "int"
                as_method_name = "Number"
            return ValidateMethodParams

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByPointer

        @staticmethod
        def is_heavy_value():
            return False

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "int"

        default_c_param_type = CParamType("int")

    class Number(BaseType):
        @staticmethod
        def get_getter_name():
            return "Object"

        @staticmethod
        def get_setter_name():
            return "Number"

        @staticmethod
        def get_c_initializer():
            raise Exception("Unsupported")

        @staticmethod
        def get_js_bind_type():
            raise Exception("Unsupported")

        @staticmethod
        def get_validate_method_params():
            raise Exception("TODO")

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByPointer

        @staticmethod
        def is_heavy_value():
            return False

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "double"

        default_c_param_type = CParamType("double")

    class Bool(BaseType):
        @classmethod
        def get_c_param_type(cls, param_type, optional):
            if param_type == ParamType.EVENT:
                if optional:
                    return cls._ref_c_type
                else:
                    return cls._plain_c_type
            else:
                return cls._plain_c_type

        @staticmethod
        def get_getter_name():
            return "Boolean"

        get_setter_name = get_getter_name

        @staticmethod
        def get_c_initializer():
            return "false"

        @staticmethod
        def get_js_bind_type():
            return "boolean"

        @staticmethod
        def is_event_param_check_optional():
            return True

        @staticmethod
        def get_validate_method_params():
            class ValidateMethodParams:
                name = "Boolean"
                var_type = "bool"
                as_method_name = "Boolean"
            return ValidateMethodParams

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByPointer

        @staticmethod
        def is_heavy_value():
            return False

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "bool"

        _plain_c_type = CParamType("bool")
        _ref_c_type = CParamType("const bool* const", "*%s")

    class Object(BaseType):
        @classmethod
        def get_c_param_type(cls, param_type, optional):
            if param_type == ParamType.EVENT or param_type == ParamType.TYPE_BUILDER_OUTPUT:
                return cls._ref_c_type
            else:
                return cls._plain_c_type

        @staticmethod
        def get_getter_name():
            return "Object"

        get_setter_name = get_getter_name

        @staticmethod
        def get_c_initializer():
            return "InspectorObject::create()"

        @staticmethod
        def get_js_bind_type():
            return "object"

        @staticmethod
        def is_event_param_check_optional():
            return True

        @staticmethod
        def get_output_argument_prefix():
            return ""

        @staticmethod
        def get_validate_method_params():
            raise Exception("TODO")

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByReference

        @staticmethod
        def is_heavy_value():
            return True

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "InspectorObject"

        _plain_c_type = CParamType("RefPtr<InspectorObject>")
        _ref_c_type = CParamType("PassRefPtr<InspectorObject>")

    class Any(BaseType):
        @classmethod
        def get_c_param_type(cls, param_type, optional):
            if param_type == ParamType.EVENT or param_type == ParamType.TYPE_BUILDER_OUTPUT:
                return cls._ref_c_type
            else:
                return cls._plain_c_type

        @staticmethod
        def get_getter_name():
            return "Value"

        get_setter_name = get_getter_name

        @staticmethod
        def get_c_initializer():
            raise Exception("Unsupported")

        @staticmethod
        def get_js_bind_type():
            raise Exception("Unsupported")

        @staticmethod
        def is_event_param_check_optional():
            return True

        @staticmethod
        def generate_validate_method(writer):
            writer.newline("static void assertAny(InspectorValue*)\n")
            writer.newline("{\n")
            writer.newline("    // No-op.\n")
            writer.newline("}\n\n\n")

        @staticmethod
        def get_raw_validator_call_text():
            return "assertAny"

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByReference

        @staticmethod
        def is_heavy_value():
            return True

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "InspectorValue"

        _plain_c_type = CParamType("RefPtr<InspectorValue>")
        _ref_c_type = CParamType("PassRefPtr<InspectorValue>")

    class Array(BaseType):
        @classmethod
        def get_c_param_type(cls, param_type, optional):
            if param_type == ParamType.OUTPUT:
                return cls._plain_c_type
            elif param_type == ParamType.INPUT:
                return cls._plain_c_type
            else:
                return cls._ref_c_type

        @staticmethod
        def get_getter_name():
            return "Array"

        get_setter_name = get_getter_name

        @staticmethod
        def get_c_initializer():
            return "InspectorArray::create()"

        @staticmethod
        def get_js_bind_type():
            return "object"

        @staticmethod
        def is_event_param_check_optional():
            return True

        @staticmethod
        def get_output_argument_prefix():
            return ""

        @staticmethod
        def get_validate_method_params():
            raise Exception("TODO")

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByReference

        @staticmethod
        def is_heavy_value():
            return True

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "InspectorArray"

        _plain_c_type = CParamType("RefPtr<InspectorArray>")
        _ref_c_type = CParamType("PassRefPtr<InspectorArray>")


class ParamType(object):
    INPUT = "input"
    OUTPUT = "output"
    EVENT = "event"
    TYPE_BUILDER_OUTPUT = "typeBuilderOutput"

# Collection of InspectorObject class methods that are likely to be overloaded in generated class.
# We must explicitly import all overloaded methods or they won't be available to user.
INSPECTOR_OBJECT_SETTER_NAMES = frozenset(["setValue", "setBoolean", "setNumber", "setString", "setValue", "setObject", "setArray"])


def fix_type_name(json_name):
    if json_name in TYPE_NAME_FIX_MAP:
        fixed = TYPE_NAME_FIX_MAP[json_name]

        class Result(object):
            class_name = fixed

            @staticmethod
            def output_comment(writer):
                writer.newline("// Type originally was named '%s'.\n" % json_name)
    else:

        class Result(object):
            class_name = json_name

            @staticmethod
            def output_comment(writer):
                pass

    return Result


class Writer:
    def __init__(self, output, indent):
        self.output = output
        self.indent = indent

    def newline(self, str):
        if (self.indent):
            self.output.append(self.indent)
        self.output.append(str)

    def append(self, str):
        self.output.append(str)

    def newline_multiline(self, str):
        parts = str.split('\n')
        self.newline(parts[0])
        for p in parts[1:]:
            self.output.append('\n')
            if p:
                self.newline(p)

    def append_multiline(self, str):
        parts = str.split('\n')
        self.append(parts[0])
        for p in parts[1:]:
            self.output.append('\n')
            if p:
                self.newline(p)

    def get_indented(self, additional_indent):
        return Writer(self.output, self.indent + additional_indent)

    def insert_writer(self, additional_indent):
        new_output = []
        self.output.append(new_output)
        return Writer(new_output, self.indent + additional_indent)


class EnumConstants:
    map_ = {}
    constants_ = []

    @classmethod
    def add_constant(cls, value):
        if value in cls.map_:
            return cls.map_[value]
        else:
            pos = len(cls.map_)
            cls.map_[value] = pos
            cls.constants_.append(value)
            return pos

    @classmethod
    def get_enum_constant_code(cls):
        output = []
        for item in cls.constants_:
            output.append("    \"" + item + "\"")
        return join(output, ",\n") + "\n"


# Typebuilder code is generated in several passes: first typedefs, then other classes.
# Manual pass management is needed because we cannot have forward declarations for typedefs.
class TypeBuilderPass:
    TYPEDEF = "typedef"
    MAIN = "main"


class TypeBindings:
    @staticmethod
    def create_named_type_declaration(json_typable, context_domain_name, type_data):
        json_type = type_data.get_json_type()

        class Helper:
            is_ad_hoc = False
            full_name_prefix_for_use = "TypeBuilder::" + context_domain_name + "::"
            full_name_prefix_for_impl = "TypeBuilder::" + context_domain_name + "::"

            @staticmethod
            def write_doc(writer):
                if "description" in json_type:
                    writer.newline("/* ")
                    writer.append(json_type["description"])
                    writer.append(" */\n")

            @staticmethod
            def add_to_forward_listener(forward_listener):
                forward_listener.add_type_data(type_data)


        fixed_type_name = fix_type_name(json_type["id"])
        return TypeBindings.create_type_declaration_(json_typable, context_domain_name, fixed_type_name, Helper)

    @staticmethod
    def create_ad_hoc_type_declaration(json_typable, context_domain_name, ad_hoc_type_context):
        class Helper:
            is_ad_hoc = True
            full_name_prefix_for_use = ""
            full_name_prefix_for_impl = ad_hoc_type_context.container_full_name_prefix

            @staticmethod
            def write_doc(writer):
                pass

            @staticmethod
            def add_to_forward_listener(forward_listener):
                pass
        fixed_type_name = ad_hoc_type_context.get_type_name_fix()
        return TypeBindings.create_type_declaration_(json_typable, context_domain_name, fixed_type_name, Helper)

    @staticmethod
    def create_type_declaration_(json_typable, context_domain_name, fixed_type_name, helper):
        if json_typable["type"] == "string":
            if "enum" in json_typable:

                class EnumBinding:
                    need_user_runtime_cast_ = False
                    need_internal_runtime_cast_ = False

                    @classmethod
                    def resolve_inner(cls, resolve_context):
                        pass

                    @classmethod
                    def request_user_runtime_cast(cls, request):
                        if request:
                            cls.need_user_runtime_cast_ = True
                            request.acknowledge()

                    @classmethod
                    def request_internal_runtime_cast(cls):
                        cls.need_internal_runtime_cast_ = True

                    @classmethod
                    def get_code_generator(enum_binding_cls):
                        #FIXME: generate ad-hoc enums too once we figure out how to better implement them in C++.
                        comment_out = helper.is_ad_hoc

                        class CodeGenerator:
                            @staticmethod
                            def generate_type_builder(writer, generate_context):
                                enum = json_typable["enum"]
                                helper.write_doc(writer)
                                enum_name = fixed_type_name.class_name
                                fixed_type_name.output_comment(writer)
                                writer.newline("struct ")
                                writer.append(enum_name)
                                writer.append(" {\n")
                                writer.newline("    enum Enum {\n")
                                for enum_item in enum:
                                    enum_pos = EnumConstants.add_constant(enum_item)

                                    item_c_name = enum_item.replace('-', '_')
                                    item_c_name = Capitalizer.lower_camel_case_to_upper(item_c_name)
                                    if item_c_name in TYPE_NAME_FIX_MAP:
                                        item_c_name = TYPE_NAME_FIX_MAP[item_c_name]
                                    writer.newline("        ")
                                    writer.append(item_c_name)
                                    writer.append(" = ")
                                    writer.append("%s" % enum_pos)
                                    writer.append(",\n")
                                writer.newline("    };\n")
                                if enum_binding_cls.need_user_runtime_cast_:
                                    raise Exception("Not yet implemented")

                                if enum_binding_cls.need_internal_runtime_cast_:
                                    writer.append("#if %s\n" % VALIDATOR_IFDEF_NAME)
                                    writer.newline("    static void assertCorrectValue(InspectorValue* value);\n")
                                    writer.append("#endif  // %s\n" % VALIDATOR_IFDEF_NAME)

                                    validator_writer = generate_context.validator_writer
                                    validator_writer.newline("void %s%s::assertCorrectValue(InspectorValue* value)\n" % (helper.full_name_prefix_for_impl, enum_name))
                                    validator_writer.newline("{\n")
                                    validator_writer.newline("    WTF::String s;\n")
                                    validator_writer.newline("    bool cast_res = value->asString(&s);\n")
                                    validator_writer.newline("    ASSERT(cast_res);\n")
                                    if len(enum) > 0:
                                        condition_list = []
                                        for enum_item in enum:
                                            enum_pos = EnumConstants.add_constant(enum_item)
                                            condition_list.append("s == \"%s\"" % enum_item)
                                        validator_writer.newline("    ASSERT(%s);\n" % join(condition_list, " || "))
                                    validator_writer.newline("}\n\n\n")


                                writer.newline("}; // struct ")
                                writer.append(enum_name)
                                writer.append("\n\n")

                            @staticmethod
                            def register_use(forward_listener):
                                pass

                            @staticmethod
                            def get_generate_pass_id():
                                return TypeBuilderPass.MAIN

                        return CodeGenerator

                    @classmethod
                    def get_validator_call_text(cls):
                        return helper.full_name_prefix_for_use + fixed_type_name.class_name + "::assertCorrectValue"

                    @classmethod
                    def get_in_c_type_text(cls, optional):
                        return helper.full_name_prefix_for_use + fixed_type_name.class_name + "::Enum"

                    @classmethod
                    def get_array_item_c_type_text(cls):
                        return cls.get_in_c_type_text(False)

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return "getEnumConstantValue(%s)"

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.String

                return EnumBinding
            else:
                if helper.is_ad_hoc:

                    class PlainString:
                        @classmethod
                        def resolve_inner(cls, resolve_context):
                            pass

                        @staticmethod
                        def request_user_runtime_cast(request):
                            raise Exception("Unsupported")

                        @staticmethod
                        def request_internal_runtime_cast():
                            pass

                        @staticmethod
                        def get_code_generator():
                            return None

                        @classmethod
                        def get_validator_call_text(cls):
                            return RawTypes.String.get_raw_validator_call_text()

                        @staticmethod
                        def reduce_to_raw_type():
                            return RawTypes.String

                        @staticmethod
                        def get_setter_value_expression_pattern():
                            return None

                        @classmethod
                        def get_in_c_type_text(cls, optional):
                            return cls.reduce_to_raw_type().get_c_param_type(ParamType.EVENT, optional).get_text()

                        @classmethod
                        def get_array_item_c_type_text(cls):
                            return cls.reduce_to_raw_type().get_array_item_raw_c_type_text()

                    return PlainString

                else:

                    class TypedefString:
                        @classmethod
                        def resolve_inner(cls, resolve_context):
                            pass

                        @staticmethod
                        def request_user_runtime_cast(request):
                            raise Exception("Unsupported")

                        @staticmethod
                        def request_internal_runtime_cast():
                            RawTypes.String.request_raw_internal_runtime_cast()

                        @staticmethod
                        def get_code_generator():
                            class CodeGenerator:
                                @staticmethod
                                def generate_type_builder(writer, generate_context):
                                    helper.write_doc(writer)
                                    fixed_type_name.output_comment(writer)
                                    writer.newline("typedef String ")
                                    writer.append(fixed_type_name.class_name)
                                    writer.append(";\n\n")

                                @staticmethod
                                def register_use(forward_listener):
                                    pass

                                @staticmethod
                                def get_generate_pass_id():
                                    return TypeBuilderPass.TYPEDEF

                            return CodeGenerator

                        @classmethod
                        def get_validator_call_text(cls):
                            return RawTypes.String.get_raw_validator_call_text()

                        @staticmethod
                        def reduce_to_raw_type():
                            return RawTypes.String

                        @staticmethod
                        def get_setter_value_expression_pattern():
                            return None

                        @classmethod
                        def get_in_c_type_text(cls, optional):
                            return "const %s%s&" % (helper.full_name_prefix_for_use, fixed_type_name.class_name)

                        @classmethod
                        def get_array_item_c_type_text(cls):
                            return cls.get_in_c_type_text(False)

                    return TypedefString

        elif json_typable["type"] == "object":
            if "properties" in json_typable:

                class ClassBinding:
                    resolve_data_ = None
                    need_user_runtime_cast_ = False
                    need_internal_runtime_cast_ = False

                    @classmethod
                    def resolve_inner(cls, resolve_context):
                        if cls.resolve_data_:
                            return

                        properties = json_typable["properties"]
                        main = []
                        optional = []

                        ad_hoc_type_list = []

                        for prop in properties:
                            prop_name = prop["name"]
                            ad_hoc_type_context = cls.AdHocTypeContextImpl(prop_name, fixed_type_name.class_name, resolve_context, ad_hoc_type_list, helper.full_name_prefix_for_impl)
                            binding = resolve_param_type(prop, context_domain_name, ad_hoc_type_context)

                            code_generator = binding.get_code_generator()
                            if code_generator:
                                code_generator.register_use(resolve_context.forward_listener)

                            class PropertyData:
                                param_type_binding = binding
                                p = prop

                            if prop.get("optional"):
                                optional.append(PropertyData)
                            else:
                                main.append(PropertyData)

                        class ResolveData:
                            main_properties = main
                            optional_properties = optional
                            ad_hoc_types = ad_hoc_type_context.ad_hoc_type_list

                        cls.resolve_data_ = ResolveData

                        for ad_hoc in ad_hoc_type_list:
                            ad_hoc.resolve_inner(resolve_context)

                    @classmethod
                    def request_user_runtime_cast(cls, request):
                        if not request:
                            return
                        cls.need_user_runtime_cast_ = True
                        request.acknowledge()
                        cls.request_internal_runtime_cast()

                    @classmethod
                    def request_internal_runtime_cast(cls):
                        cls.need_internal_runtime_cast_ = True
                        for p in cls.resolve_data_.main_properties:
                            p.param_type_binding.request_internal_runtime_cast()
                        for p in cls.resolve_data_.optional_properties:
                            p.param_type_binding.request_internal_runtime_cast()

                    @classmethod
                    def get_code_generator(class_binding_cls):
                        class CodeGenerator:
                            @classmethod
                            def generate_type_builder(cls, writer, generate_context):
                                resolve_data = class_binding_cls.resolve_data_
                                helper.write_doc(writer)
                                class_name = fixed_type_name.class_name
                                fixed_type_name.output_comment(writer)
                                writer.newline("class ")
                                writer.append(class_name)
                                writer.append(" : public InspectorObject {\n")
                                writer.newline("public:\n")
                                ad_hoc_type_writer = writer.insert_writer("    ")

                                for ad_hoc_type in resolve_data.ad_hoc_types:
                                    code_generator = ad_hoc_type.get_code_generator()
                                    if code_generator:
                                        code_generator.generate_type_builder(ad_hoc_type_writer, generate_context)

                                writer.newline_multiline(
"""    enum {
        NoFieldsSet = 0,
""")

                                state_enum_items = []
                                if len(resolve_data.main_properties) > 0:
                                    pos = 0
                                    for prop_data in resolve_data.main_properties:
                                        item_name = Capitalizer.lower_camel_case_to_upper(prop_data.p["name"]) + "Set"
                                        state_enum_items.append(item_name)
                                        writer.newline("        %s = 1 << %s,\n" % (item_name, pos))
                                        pos += 1
                                    all_fields_set_value = "(" + (" | ".join(state_enum_items)) + ")"
                                else:
                                    all_fields_set_value = "0"

                                writer.newline_multiline(
"""        AllFieldsSet = %s
    };

    template<int STATE>
    class Builder {
    private:
        RefPtr<InspectorObject> m_result;

        template<int STEP> Builder<STATE | STEP>& castState()
        {
            return *reinterpret_cast<Builder<STATE | STEP>*>(this);
        }

        Builder(PassRefPtr<%s> ptr)
        {
            COMPILE_ASSERT(STATE == NoFieldsSet, builder_created_in_non_init_state);
            m_result = ptr;
        }
        friend class %s;
    public:
""" % (all_fields_set_value, class_name, class_name))

                                pos = 0
                                for prop_data in resolve_data.main_properties:
                                    prop_name = prop_data.p["name"]

                                    param_type_binding = prop_data.param_type_binding
                                    param_raw_type = param_type_binding.reduce_to_raw_type()
                                    for param_type_opt in MethodGenerateModes.get_modes(param_type_binding):
                                        writer.newline_multiline("""
        Builder<STATE | %s>& set%s(%s value)
        {
            COMPILE_ASSERT(!(STATE & %s), property_%s_already_set);
            m_result->set%s("%s", %s);
            return castState<%s>();
        }
"""
                                        % (state_enum_items[pos],
                                           Capitalizer.lower_camel_case_to_upper(prop_name),
                                           param_type_opt.get_c_param_type_text(param_type_binding),
                                           state_enum_items[pos], prop_name,
                                           param_raw_type.get_setter_name(), prop_name,
                                           param_type_opt.get_setter_value_expression(param_type_binding, "value"),
                                           state_enum_items[pos]))

                                    pos += 1

                                writer.newline_multiline("""
        operator RefPtr<%s>& ()
        {
            COMPILE_ASSERT(STATE == AllFieldsSet, result_is_not_ready);
            return *reinterpret_cast<RefPtr<%s>*>(&m_result);
        }

        operator PassRefPtr<%s> ()
        {
            return RefPtr<%s>(*this);
        }
    };

"""
                                % (class_name, class_name, class_name, class_name))

                                writer.newline("    /*\n")
                                writer.newline("     * Synthetic constructor:\n")
                                writer.newline("     * RefPtr<%s> result = %s::create()" % (class_name, class_name))
                                for prop_data in resolve_data.main_properties:
                                    writer.append_multiline("\n     *     .set%s(...)" % Capitalizer.lower_camel_case_to_upper(prop_data.p["name"]))
                                writer.append_multiline(";\n     */\n")

                                writer.newline_multiline(
"""    static Builder<NoFieldsSet> create()
    {
        return Builder<NoFieldsSet>(adoptRef(new %s()));
    }
""" % class_name)

                                writer.newline("    typedef TypeBuilder::StructItemTraits ItemTraits;\n")

                                for prop_data in resolve_data.optional_properties:
                                    prop_name = prop_data.p["name"]
                                    param_type_binding = prop_data.param_type_binding
                                    setter_name = "set%s" % Capitalizer.lower_camel_case_to_upper(prop_name)

                                    for param_type_opt in MethodGenerateModes.get_modes(param_type_binding):
                                        writer.append_multiline("\n    void %s" % setter_name)
                                        writer.append("(%s value)\n" % param_type_opt.get_c_param_type_text(param_type_binding))
                                        writer.newline("    {\n")
                                        writer.newline("        this->set%s(\"%s\", %s);\n"
                                            % (param_type_binding.reduce_to_raw_type().get_setter_name(), prop_data.p["name"],
                                               param_type_opt.get_setter_value_expression(param_type_binding, "value")))
                                        writer.newline("    }\n")


                                    if setter_name in INSPECTOR_OBJECT_SETTER_NAMES:
                                        writer.newline("    using InspectorObject::%s;\n\n" % setter_name)

                                if class_binding_cls.need_user_runtime_cast_:
                                    writer.newline("    static PassRefPtr<%s> runtimeCast(PassRefPtr<InspectorValue> value)\n" % class_name)
                                    writer.newline("    {\n")
                                    writer.newline("        RefPtr<InspectorObject> object;\n")
                                    writer.newline("        bool castRes = value->asObject(&object);\n")
                                    writer.newline("        ASSERT_UNUSED(castRes, castRes);\n")
                                    writer.append("#if %s\n" % VALIDATOR_IFDEF_NAME)
                                    writer.newline("        assertCorrectValue(object.get());\n")
                                    writer.append("#endif  // %s\n" % VALIDATOR_IFDEF_NAME)
                                    writer.newline("        COMPILE_ASSERT(sizeof(%s) == sizeof(InspectorObject), type_cast_problem);\n" % class_name)
                                    writer.newline("        return static_cast<%s*>(object.get());\n" % class_name)
                                    writer.newline("    }\n")
                                    writer.append("\n")

                                if class_binding_cls.need_internal_runtime_cast_:
                                    writer.append("#if %s\n" % VALIDATOR_IFDEF_NAME)
                                    writer.newline("    static void assertCorrectValue(InspectorValue* value);\n")
                                    writer.append("#endif  // %s\n" % VALIDATOR_IFDEF_NAME)

                                    validator_writer = generate_context.validator_writer
                                    validator_writer.newline("void %s%s::assertCorrectValue(InspectorValue* value)\n" % (helper.full_name_prefix_for_impl, class_name))
                                    validator_writer.newline("{\n")
                                    validator_writer.newline("    RefPtr<InspectorObject> object;\n")
                                    validator_writer.newline("    bool castRes = value->asObject(&object);\n")
                                    validator_writer.newline("    ASSERT_UNUSED(castRes, castRes);\n")
                                    for prop_data in resolve_data.main_properties:
                                        validator_writer.newline("    {\n")
                                        it_name = "%sPos" % prop_data.p["name"]
                                        validator_writer.newline("        InspectorObject::iterator %s;\n" % it_name)
                                        validator_writer.newline("        %s = object->find(\"%s\");\n" % (it_name, prop_data.p["name"]))
                                        validator_writer.newline("        ASSERT(%s != object->end());\n" % it_name)
                                        validator_writer.newline("        %s(%s->second.get());\n" % (prop_data.param_type_binding.get_validator_call_text(), it_name))
                                        validator_writer.newline("    }\n")

                                    validator_writer.newline("    int foundPropertiesCount = %s;\n" % len(resolve_data.main_properties))

                                    for prop_data in resolve_data.optional_properties:
                                        validator_writer.newline("    {\n")
                                        it_name = "%sPos" % prop_data.p["name"]
                                        validator_writer.newline("        InspectorObject::iterator %s;\n" % it_name)
                                        validator_writer.newline("        %s = object->find(\"%s\");\n" % (it_name, prop_data.p["name"]))
                                        validator_writer.newline("        if (%s != object->end()) {\n" % it_name)
                                        validator_writer.newline("            %s(%s->second.get());\n" % (prop_data.param_type_binding.get_validator_call_text(), it_name))
                                        validator_writer.newline("            ++foundPropertiesCount;\n")
                                        validator_writer.newline("        }\n")
                                        validator_writer.newline("    }\n")

                                    validator_writer.newline("    ASSERT(foundPropertiesCount == object->size());\n")
                                    validator_writer.newline("}\n\n\n")

                                writer.newline("};\n\n")

                            @staticmethod
                            def generate_forward_declaration(writer):
                                class_name = fixed_type_name.class_name
                                writer.newline("class ")
                                writer.append(class_name)
                                writer.append(";\n")

                            @staticmethod
                            def register_use(forward_listener):
                                helper.add_to_forward_listener(forward_listener)

                            @staticmethod
                            def get_generate_pass_id():
                                return TypeBuilderPass.MAIN

                        return CodeGenerator

                    @staticmethod
                    def get_validator_call_text():
                        return helper.full_name_prefix_for_use + fixed_type_name.class_name + "::assertCorrectValue"

                    @classmethod
                    def get_in_c_type_text(cls, optional):
                        return "PassRefPtr<" + helper.full_name_prefix_for_use + fixed_type_name.class_name + ">"

                    @classmethod
                    def get_array_item_c_type_text(cls):
                        return helper.full_name_prefix_for_use + fixed_type_name.class_name

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return None

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.Object

                    class AdHocTypeContextImpl:
                        def __init__(self, property_name, class_name, resolve_context, ad_hoc_type_list, parent_full_name_prefix):
                            self.property_name = property_name
                            self.class_name = class_name
                            self.resolve_context = resolve_context
                            self.ad_hoc_type_list = ad_hoc_type_list
                            self.container_full_name_prefix = parent_full_name_prefix + class_name + "::"

                        def get_type_name_fix(self):
                            class NameFix:
                                class_name = Capitalizer.lower_camel_case_to_upper(self.property_name)

                                @staticmethod
                                def output_comment(writer):
                                    writer.newline("// Named after property name '%s' while generating %s.\n" % (self.property_name, self.class_name))

                            return NameFix

                        def add_type(self, binding):
                            self.ad_hoc_type_list.append(binding)

                return ClassBinding
            else:

                class PlainObjectBinding:
                    @classmethod
                    def resolve_inner(cls, resolve_context):
                        pass

                    @staticmethod
                    def request_user_runtime_cast(request):
                        pass

                    @staticmethod
                    def request_internal_runtime_cast():
                        pass

                    @staticmethod
                    def get_code_generator():
                        pass

                    @staticmethod
                    def get_validator_call_text():
                        raise Exception("Unsupported")

                    @classmethod
                    def get_in_c_type_text(cls, optional):
                        return cls.reduce_to_raw_type().get_c_param_type(ParamType.EVENT, optional).get_text()

                    @classmethod
                    def get_array_item_c_type_text(cls):
                        return cls.reduce_to_raw_type().get_array_item_raw_c_type_text()

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return None

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.Object

                return PlainObjectBinding
        elif json_typable["type"] == "array":
            if "items" in json_typable:

                ad_hoc_types = []

                class AdHocTypeContext:
                    container_full_name_prefix = "<not yet defined>"

                    @staticmethod
                    def get_type_name_fix():
                        return fixed_type_name

                    @staticmethod
                    def add_type(binding):
                        ad_hoc_types.append(binding)

                item_binding = resolve_param_type(json_typable["items"], context_domain_name, AdHocTypeContext)

                class ArrayBinding:
                    resolve_data_ = None
                    need_internal_runtime_cast_ = False

                    @classmethod
                    def resolve_inner(cls, resolve_context):
                        if cls.resolve_data_:
                            return

                        class ResolveData:
                            item_type_binding = item_binding
                            ad_hoc_type_list = ad_hoc_types

                        cls.resolve_data_ = ResolveData

                        for t in ad_hoc_types:
                            t.resolve_inner(resolve_context)

                    @classmethod
                    def request_user_runtime_cast(cls, request):
                        raise Exception("Not implemented yet")

                    @classmethod
                    def request_internal_runtime_cast(cls):
                        cls.need_internal_runtime_cast_ = True
                        cls.resolve_data_.item_type_binding.request_internal_runtime_cast()

                    @classmethod
                    def get_code_generator(array_binding_cls):

                        class CodeGenerator:
                            @staticmethod
                            def generate_type_builder(writer, generate_context):
                                ad_hoc_type_writer = writer

                                resolve_data = array_binding_cls.resolve_data_

                                for ad_hoc_type in resolve_data.ad_hoc_type_list:
                                    code_generator = ad_hoc_type.get_code_generator()
                                    if code_generator:
                                        code_generator.generate_type_builder(ad_hoc_type_writer, generate_context)

                            @staticmethod
                            def generate_forward_declaration(writer):
                                pass

                            @staticmethod
                            def register_use(forward_listener):
                                item_code_generator = item_binding.get_code_generator()
                                if item_code_generator:
                                    item_code_generator.register_use(forward_listener)

                            @staticmethod
                            def get_generate_pass_id():
                                return TypeBuilderPass.MAIN

                        return CodeGenerator

                    @classmethod
                    def get_validator_call_text(cls):
                        return cls.get_array_item_c_type_text() + "::assertCorrectValue"

                    @classmethod
                    def get_in_c_type_text(cls, optional):
                        return "PassRefPtr<TypeBuilder::Array<%s > >" % cls.resolve_data_.item_type_binding.get_array_item_c_type_text()

                    @classmethod
                    def get_array_item_c_type_text(cls):
                        return "TypeBuilder::Array<%s >" % cls.resolve_data_.item_type_binding.get_array_item_c_type_text()

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return None

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.Array

                return ArrayBinding
            else:
                # Fall-through to raw type.
                pass

        raw_type = RawTypes.get(json_typable["type"])

        return RawTypeBinding(raw_type)


class RawTypeBinding:
    def __init__(self, raw_type):
        self.raw_type_ = raw_type

    def resolve_inner(self, resolve_context):
        pass

    def request_user_runtime_cast(self, request):
        raise Exception("Unsupported")

    def request_internal_runtime_cast(self):
        self.raw_type_.request_raw_internal_runtime_cast()

    def get_code_generator(self):
        return None

    def get_validator_call_text(self):
        return self.raw_type_.get_raw_validator_call_text()

    def get_in_c_type_text(self, optional):
        return self.raw_type_.get_c_param_type(ParamType.EVENT, optional).get_text()

    def get_array_item_c_type_text(self):
        return self.raw_type_.get_array_item_raw_c_type_text()

    def get_setter_value_expression_pattern(self):
        return None

    def reduce_to_raw_type(self):
        return self.raw_type_


class TypeData(object):
    def __init__(self, json_type, json_domain, domain_data):
        self.json_type_ = json_type
        self.json_domain_ = json_domain
        self.domain_data_ = domain_data

        if "type" not in json_type:
            raise Exception("Unknown type")

        json_type_name = json_type["type"]
        raw_type = RawTypes.get(json_type_name)
        self.raw_type_ = raw_type
        self.binding_being_resolved_ = False
        self.binding_ = None

    def get_raw_type(self):
        return self.raw_type_

    def get_binding(self):
        if not self.binding_:
            if self.binding_being_resolved_:
                raise Error("Type %s is already being resolved" % self.json_type_["type"])
            # Resolve only lazily, because resolving one named type may require resolving some other named type.
            self.binding_being_resolved_ = True
            try:
                self.binding_ = TypeBindings.create_named_type_declaration(self.json_type_, self.json_domain_["domain"], self)
            finally:
                self.binding_being_resolved_ = False

        return self.binding_

    def get_json_type(self):
        return self.json_type_

    def get_name(self):
        return self.json_type_["id"]

    def get_domain_name(self):
        return self.json_domain_["domain"]


class DomainData:
    def __init__(self, json_domain):
        self.json_domain = json_domain
        self.types_ = []

    def add_type(self, type_data):
        self.types_.append(type_data)

    def name(self):
        return self.json_domain["domain"]

    def types(self):
        return self.types_


class TypeMap:
    def __init__(self, api):
        self.map_ = {}
        self.domains_ = []
        for json_domain in api["domains"]:
            domain_name = json_domain["domain"]

            domain_map = {}
            self.map_[domain_name] = domain_map

            domain_data = DomainData(json_domain)
            self.domains_.append(domain_data)

            if "types" in json_domain:
                for json_type in json_domain["types"]:
                    type_name = json_type["id"]
                    type_data = TypeData(json_type, json_domain, domain_data)
                    domain_map[type_name] = type_data
                    domain_data.add_type(type_data)

    def domains(self):
        return self.domains_

    def get(self, domain_name, type_name):
        return self.map_[domain_name][type_name]


def resolve_param_type(json_parameter, scope_domain_name, ad_hoc_type_context):
    if "$ref" in json_parameter:
        json_ref = json_parameter["$ref"]
        type_data = get_ref_data(json_ref, scope_domain_name)
        return type_data.get_binding()
    elif "type" in json_parameter:
        result = TypeBindings.create_ad_hoc_type_declaration(json_parameter, scope_domain_name, ad_hoc_type_context)
        ad_hoc_type_context.add_type(result)
        return result
    else:
        raise Exception("Unknown type")

def resolve_param_raw_type(json_parameter, scope_domain_name):
    if "$ref" in json_parameter:
        json_ref = json_parameter["$ref"]
        type_data = get_ref_data(json_ref, scope_domain_name)
        return type_data.get_raw_type()
    elif "type" in json_parameter:
        json_type = json_parameter["type"]
        return RawTypes.get(json_type)
    else:
        raise Exception("Unknown type")


def get_ref_data(json_ref, scope_domain_name):
    dot_pos = json_ref.find(".")
    if dot_pos == -1:
        domain_name = scope_domain_name
        type_name = json_ref
    else:
        domain_name = json_ref[:dot_pos]
        type_name = json_ref[dot_pos + 1:]

    return type_map.get(domain_name, type_name)


input_file = open(input_json_filename, "r")
json_string = input_file.read()
json_api = json.loads(json_string)


class Templates:
    def get_this_script_path_(absolute_path):
        absolute_path = os.path.abspath(absolute_path)
        components = []

        def fill_recursive(path_part, depth):
            if depth <= 0 or path_part == '/':
                return
            fill_recursive(os.path.dirname(path_part), depth - 1)
            components.append(os.path.basename(path_part))

        # Typical path is /Source/WebCore/inspector/CodeGeneratorInspector.py
        # Let's take 4 components from the real path then.
        fill_recursive(absolute_path, 4)

        return "/".join(components)

    file_header_ = ("// File is generated by %s\n\n" % get_this_script_path_(sys.argv[0]) +
"""// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
""")



    frontend_domain_class = string.Template(
"""    class $domainClassName {
    public:
        $domainClassName(InspectorFrontendChannel* inspectorFrontendChannel) : m_inspectorFrontendChannel(inspectorFrontendChannel) { }
${frontendDomainMethodDeclarations}        void setInspectorFrontendChannel(InspectorFrontendChannel* inspectorFrontendChannel) { m_inspectorFrontendChannel = inspectorFrontendChannel; }
        InspectorFrontendChannel* getInspectorFrontendChannel() { return m_inspectorFrontendChannel; }
    private:
        InspectorFrontendChannel* m_inspectorFrontendChannel;
    };

    $domainClassName* $domainFieldName() { return &m_$domainFieldName; }

""")

    backend_method = string.Template(
"""void InspectorBackendDispatcherImpl::${domainName}_$methodName(long callId, InspectorObject*$requestMessageObject)
{
    RefPtr<InspectorArray> protocolErrors = InspectorArray::create();

    if (!$agentField)
        protocolErrors->pushString("${domainName} handler is not available.");
$methodOutCode
    ErrorString error;
$methodInCode
    if (!protocolErrors->length())
        $agentField->$methodName(&error$agentCallParams);

    RefPtr<InspectorObject> result = InspectorObject::create();
${responseCook}    sendResponse(callId, result, String::format("Some arguments of method '%s' can't be processed", "$domainName.$methodName"), protocolErrors, error);
}
""")

    frontend_method = string.Template("""void InspectorFrontend::$domainName::$eventName($parameters)
{
    RefPtr<InspectorObject> ${eventName}Message = InspectorObject::create();
    ${eventName}Message->setString("method", "$domainName.$eventName");
$code    if (m_inspectorFrontendChannel)
        m_inspectorFrontendChannel->sendMessageToFrontend(${eventName}Message->toJSONString());
}
""")

    frontend_h = string.Template(file_header_ +
"""#ifndef InspectorFrontend_h
#define InspectorFrontend_h

#include "InspectorTypeBuilder.h"
#include "InspectorValues.h"
#include <PlatformString.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class InspectorFrontendChannel;

// Both InspectorObject and InspectorArray may or may not be declared at this point as defined by ENABLED_INSPECTOR.
// Double-check we have them at least as forward declaration.
class InspectorArray;
class InspectorObject;

typedef String ErrorString;

#if ENABLE(INSPECTOR)

class InspectorFrontend {
public:
    InspectorFrontend(InspectorFrontendChannel*);


$domainClassList
private:
    InspectorFrontendChannel* m_inspectorFrontendChannel;
${fieldDeclarations}};

#endif // ENABLE(INSPECTOR)

} // namespace WebCore
#endif // !defined(InspectorFrontend_h)
""")

    backend_h = string.Template(file_header_ +
"""#ifndef InspectorBackendDispatcher_h
#define InspectorBackendDispatcher_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>
#include "InspectorTypeBuilder.h"

namespace WebCore {

class InspectorAgent;
class InspectorObject;
class InspectorArray;
class InspectorFrontendChannel;

typedef String ErrorString;

class InspectorBackendDispatcher: public RefCounted<InspectorBackendDispatcher> {
public:
    static PassRefPtr<InspectorBackendDispatcher> create(InspectorFrontendChannel* inspectorFrontendChannel);
    virtual ~InspectorBackendDispatcher() { }

$agentInterfaces
$virtualSetters

    virtual void clearFrontend() = 0;

    enum CommonErrorCode {
        ParseError = 0,
        InvalidRequest,
        MethodNotFound,
        InvalidParams,
        InternalError,
        ServerError,
        LastEntry,
    };

    void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage) const;
    virtual void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage, PassRefPtr<InspectorArray> data) const = 0;
    virtual void dispatch(const String& message) = 0;
    static bool getCommandName(const String& message, String* result);

    enum MethodNames {
$methodNamesEnumContent
    };

    static const char* commandNames[];
};

} // namespace WebCore
#endif // !defined(InspectorBackendDispatcher_h)


""")

    backend_cpp = string.Template(file_header_ +
"""

#include "config.h"
#include "InspectorBackendDispatcher.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

#if ENABLE(INSPECTOR)

#include "InspectorAgent.h"
#include "InspectorValues.h"
#include "InspectorFrontendChannel.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

const char* InspectorBackendDispatcher::commandNames[] = {
$methodNameDeclarations
};


class InspectorBackendDispatcherImpl : public InspectorBackendDispatcher {
public:
    InspectorBackendDispatcherImpl(InspectorFrontendChannel* inspectorFrontendChannel)
        : m_inspectorFrontendChannel(inspectorFrontendChannel)
$constructorInit
    { }

    virtual void clearFrontend() { m_inspectorFrontendChannel = 0; }
    virtual void dispatch(const String& message);
    virtual void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage, PassRefPtr<InspectorArray> data) const;
    using InspectorBackendDispatcher::reportProtocolError;

$setters
private:
$methodDeclarations

    InspectorFrontendChannel* m_inspectorFrontendChannel;
$fieldDeclarations

    static int getInt(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static String getString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static bool getBoolean(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static PassRefPtr<InspectorObject> getObject(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static PassRefPtr<InspectorArray> getArray(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    void sendResponse(long callId, PassRefPtr<InspectorObject> result, const String& errorMessage, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError);

};

$methods

PassRefPtr<InspectorBackendDispatcher> InspectorBackendDispatcher::create(InspectorFrontendChannel* inspectorFrontendChannel)
{
    return adoptRef(new InspectorBackendDispatcherImpl(inspectorFrontendChannel));
}


void InspectorBackendDispatcherImpl::dispatch(const String& message)
{
    RefPtr<InspectorBackendDispatcher> protect = this;
    typedef void (InspectorBackendDispatcherImpl::*CallHandler)(long callId, InspectorObject* messageObject);
    typedef HashMap<String, CallHandler> DispatchMap;
    DEFINE_STATIC_LOCAL(DispatchMap, dispatchMap, );
    long callId = 0;

    if (dispatchMap.isEmpty()) {
        static CallHandler handlers[] = {
$messageHandlers
        };
        size_t length = sizeof(commandNames) / sizeof(commandNames[0]);
        for (size_t i = 0; i < length; ++i)
            dispatchMap.add(commandNames[i], handlers[i]);
    }

    RefPtr<InspectorValue> parsedMessage = InspectorValue::parseJSON(message);
    if (!parsedMessage) {
        reportProtocolError(0, ParseError, "Message must be in JSON format");
        return;
    }

    RefPtr<InspectorObject> messageObject = parsedMessage->asObject();
    if (!messageObject) {
        reportProtocolError(0, InvalidRequest, "Message must be a JSONified object");
        return;
    }

    RefPtr<InspectorValue> callIdValue = messageObject->get("id");
    if (!callIdValue) {
        reportProtocolError(0, InvalidRequest, "'id' property was not found");
        return;
    }

    if (!callIdValue->asNumber(&callId)) {
        reportProtocolError(0, InvalidRequest, "The type of 'id' property must be number");
        return;
    }

    RefPtr<InspectorValue> methodValue = messageObject->get("method");
    if (!methodValue) {
        reportProtocolError(&callId, InvalidRequest, "'method' property wasn't found");
        return;
    }

    String method;
    if (!methodValue->asString(&method)) {
        reportProtocolError(&callId, InvalidRequest, "The type of 'method' property must be string");
        return;
    }

    HashMap<String, CallHandler>::iterator it = dispatchMap.find(method);
    if (it == dispatchMap.end()) {
        reportProtocolError(&callId, MethodNotFound, "'" + method + "' wasn't found");
        return;
    }

    ((*this).*it->second)(callId, messageObject.get());
}

void InspectorBackendDispatcherImpl::sendResponse(long callId, PassRefPtr<InspectorObject> result, const String& errorMessage, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError)
{
    if (protocolErrors->length()) {
        reportProtocolError(&callId, InvalidParams, errorMessage, protocolErrors);
        return;
    }
    if (invocationError.length()) {
        reportProtocolError(&callId, ServerError, invocationError);
        return;
    }

    RefPtr<InspectorObject> responseMessage = InspectorObject::create();
    responseMessage->setObject("result", result);
    responseMessage->setNumber("id", callId);
    if (m_inspectorFrontendChannel)
        m_inspectorFrontendChannel->sendMessageToFrontend(responseMessage->toJSONString());
}

void InspectorBackendDispatcher::reportProtocolError(const long* const callId, CommonErrorCode code, const String& errorMessage) const
{
    reportProtocolError(callId, code, errorMessage, 0);
}

void InspectorBackendDispatcherImpl::reportProtocolError(const long* const callId, CommonErrorCode code, const String& errorMessage, PassRefPtr<InspectorArray> data) const
{
    DEFINE_STATIC_LOCAL(Vector<int>,s_commonErrors,);
    if (!s_commonErrors.size()) {
        s_commonErrors.insert(ParseError, -32700);
        s_commonErrors.insert(InvalidRequest, -32600);
        s_commonErrors.insert(MethodNotFound, -32601);
        s_commonErrors.insert(InvalidParams, -32602);
        s_commonErrors.insert(InternalError, -32603);
        s_commonErrors.insert(ServerError, -32000);
    }
    ASSERT(code >=0);
    ASSERT((unsigned)code < s_commonErrors.size());
    ASSERT(s_commonErrors[code]);
    RefPtr<InspectorObject> error = InspectorObject::create();
    error->setNumber("code", s_commonErrors[code]);
    error->setString("message", errorMessage);
    ASSERT(error);
    if (data)
        error->setArray("data", data);
    RefPtr<InspectorObject> message = InspectorObject::create();
    message->setObject("error", error);
    if (callId)
        message->setNumber("id", *callId);
    else
        message->setValue("id", InspectorValue::null());
    if (m_inspectorFrontendChannel)
        m_inspectorFrontendChannel->sendMessageToFrontend(message->toJSONString());
}

int InspectorBackendDispatcherImpl::getInt(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    int value = 0;

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '%s' with type 'Number'.", name.utf8().data()));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '%s' with type 'Number' was not found.", name.utf8().data()));
        return value;
    }

    if (!valueIterator->second->asNumber(&value))
        protocolErrors->pushString(String::format("Parameter '%s' has wrong type. It must be 'Number'.", name.utf8().data()));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}

String InspectorBackendDispatcherImpl::getString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    String value = "";

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '%s' with type 'String'.", name.utf8().data()));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '%s' with type 'String' was not found.", name.utf8().data()));
        return value;
    }

    if (!valueIterator->second->asString(&value))
        protocolErrors->pushString(String::format("Parameter '%s' has wrong type. It must be 'String'.", name.utf8().data()));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}

bool InspectorBackendDispatcherImpl::getBoolean(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    bool value = false;

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '%s' with type 'Boolean'.", name.utf8().data()));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '%s' with type 'Boolean' was not found.", name.utf8().data()));
        return value;
    }

    if (!valueIterator->second->asBoolean(&value))
        protocolErrors->pushString(String::format("Parameter '%s' has wrong type. It must be 'Boolean'.", name.utf8().data()));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}

PassRefPtr<InspectorObject> InspectorBackendDispatcherImpl::getObject(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    RefPtr<InspectorObject> value = InspectorObject::create();

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '%s' with type 'Object'.", name.utf8().data()));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '%s' with type 'Object' was not found.", name.utf8().data()));
        return value;
    }

    if (!valueIterator->second->asObject(&value))
        protocolErrors->pushString(String::format("Parameter '%s' has wrong type. It must be 'Object'.", name.utf8().data()));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}

PassRefPtr<InspectorArray> InspectorBackendDispatcherImpl::getArray(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    RefPtr<InspectorArray> value = InspectorArray::create();

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '%s' with type 'Array'.", name.utf8().data()));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '%s' with type 'Array' was not found.", name.utf8().data()));
        return value;
    }

    if (!valueIterator->second->asArray(&value))
        protocolErrors->pushString(String::format("Parameter '%s' has wrong type. It must be 'Array'.", name.utf8().data()));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}

bool InspectorBackendDispatcher::getCommandName(const String& message, String* result)
{
    RefPtr<InspectorValue> value = InspectorValue::parseJSON(message);
    if (!value)
        return false;

    RefPtr<InspectorObject> object = value->asObject();
    if (!object)
        return false;

    if (!object->getString("method", result))
        return false;

    return true;
}


} // namespace WebCore

#endif // ENABLE(INSPECTOR)
""")

    frontend_cpp = string.Template(file_header_ +
"""

#include "config.h"
#if ENABLE(INSPECTOR)

#include "InspectorFrontend.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

#include "InspectorFrontendChannel.h"
#include "InspectorValues.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

InspectorFrontend::InspectorFrontend(InspectorFrontendChannel* inspectorFrontendChannel)
    : m_inspectorFrontendChannel(inspectorFrontendChannel)
$constructorInit{
}

$methods

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
""")

    typebuilder_h = string.Template(file_header_ +
"""
#ifndef InspectorTypeBuilder_h
#define InspectorTypeBuilder_h

#if ENABLE(INSPECTOR)

#include "InspectorValues.h"
#include <PlatformString.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

namespace TypeBuilder {

// This class provides "Traits" type for the input type T. It is programmed using C++ template specialization
// technique. By default it simply takes "ItemTraits" type from T, but it doesn't work with the base types.
template<typename T>
struct ArrayItemHelper {
    typedef typename T::ItemTraits Traits;
};

template<typename T>
class Array : public InspectorArray {
private:
    Array() { }

public:
    void addItem(PassRefPtr<T> value)
    {
        ArrayItemHelper<T>::Traits::pushRefPtr(this, value);
    }

    void addItem(T value)
    {
        ArrayItemHelper<T>::Traits::pushRaw(this, value);
    }

    static PassRefPtr<Array<T> > create()
    {
        return adoptRef(new Array<T>());
    }

    static PassRefPtr<Array<T> > runtimeCast(PassRefPtr<InspectorValue> value)
    {
        RefPtr<InspectorArray> array;
        bool castRes = value->asArray(&array);
        ASSERT_UNUSED(castRes, castRes);
#if !ASSERT_DISABLED
        assertCorrectValue(array.get());
#endif  // !ASSERT_DISABLED
        COMPILE_ASSERT(sizeof(Array<T>) == sizeof(InspectorArray), type_cast_problem);
        return static_cast<Array<T>*>(array.get());
    }

#if """ + VALIDATOR_IFDEF_NAME + """
    static void assertCorrectValue(InspectorValue* value)
    {
        RefPtr<InspectorArray> array;
        bool castRes = value->asArray(&array);
        ASSERT_UNUSED(castRes, castRes);
        for (unsigned i = 0; i < array->length(); i++)
            ArrayItemHelper<T>::Traits::template assertCorrectValue<T>(array->get(i).get());
    }

#endif // """ + VALIDATOR_IFDEF_NAME + """
};

struct StructItemTraits {
    static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorObject> value)
    {
        array->pushObject(value);
    }

    template<typename T>
    static void assertCorrectValue(InspectorValue* value) {
        T::assertCorrectValue(value);
    }
};

template<>
struct ArrayItemHelper<String> {
    struct Traits {
        static void pushRaw(InspectorArray* array, const String& value)
        {
            array->pushString(value);
        }
    };
};

template<>
struct ArrayItemHelper<InspectorObject> {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorObject> value)
        {
            array->pushObject(value);
        }
    };
};

${forwards}

String getEnumConstantValue(int code);

${typeBuilders}
} // namespace TypeBuilder


} // namespace WebCore

#endif // ENABLE(INSPECTOR)

#endif // !defined(InspectorTypeBuilder_h)

""")

    typebuilder_cpp = string.Template(file_header_ +
"""

#include "config.h"
#if ENABLE(INSPECTOR)

#include "InspectorTypeBuilder.h"

namespace WebCore {

namespace TypeBuilder {

const char* const enum_constant_values[] = {
$enumConstantValues};

String getEnumConstantValue(int code) {
    return enum_constant_values[code];
}

} // namespace TypeBuilder

#if """ + VALIDATOR_IFDEF_NAME + """

$validatorCode

#endif // """ + VALIDATOR_IFDEF_NAME + """

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
""")

    backend_js = string.Template(file_header_ +
"""

$domainInitializers
""")

    param_container_access_code = """
    RefPtr<InspectorObject> paramsContainer = requestMessageObject->getObject("params");
    InspectorObject* paramsContainerPtr = paramsContainer.get();
    InspectorArray* protocolErrorsPtr = protocolErrors.get();
"""


type_map = TypeMap(json_api)


class NeedRuntimeCastRequest:
    def __init__(self):
        self.ack_ = None

    def acknowledge(self):
        self.ack_ = True

    def is_acknowledged(self):
        return self.ack_


def resolve_all_types():
    runtime_cast_generate_requests = {}
    for type_name in TYPES_WITH_RUNTIME_CAST_SET:
        runtime_cast_generate_requests[type_name] = NeedRuntimeCastRequest()

    class ForwardListener:
        type_data_set = set()
        already_declared_set = set()

        @classmethod
        def add_type_data(cls, type_data):
            if type_data not in cls.already_declared_set:
                cls.type_data_set.add(type_data)

    class ResolveContext:
        forward_listener = ForwardListener

    for domain_data in type_map.domains():
        for type_data in domain_data.types():
            # Do not generate forwards for this type any longer.
            ForwardListener.already_declared_set.add(type_data)

            binding = type_data.get_binding()
            binding.resolve_inner(ResolveContext)

    for domain_data in type_map.domains():
        for type_data in domain_data.types():
            full_type_name = "%s.%s" % (type_data.get_domain_name(), type_data.get_name())
            request = runtime_cast_generate_requests.pop(full_type_name, None)
            binding = type_data.get_binding()
            if request:
                binding.request_user_runtime_cast(request)

            if request and not request.is_acknowledged():
                raise Exception("Failed to generate runtimeCast in " + full_type_name)

    for full_type_name in runtime_cast_generate_requests:
        raise Exception("Failed to generate runtimeCast. Type " + full_type_name + " not found")

    return ForwardListener


global_forward_listener = resolve_all_types()



def get_annotated_type_text(raw_type, annotated_type):
    if annotated_type != raw_type:
        return "/*%s*/ %s" % (annotated_type, raw_type)
    else:
        return raw_type


# Chooses method generate modes for a particular type. A particular setter
# can be generated as one method with a simple parameter type,
# as one method with a raw type name and commented annotation about expected
# specialized type name or as overloaded method when raw and specialized
# parameter types require different behavior.
class MethodGenerateModes:
    @classmethod
    def get_modes(cls, type_binding):
        if type_binding.get_setter_value_expression_pattern():
            # In raw and strict modes method code is actually different.
            return [cls.StrictParameterMode, cls.RawParameterMode]
        else:
            # In raw and strict modes method code is the same.
            # Only put strict parameter type in comments.
            return [cls.CombinedMode]

    class StrictParameterMode:
        @staticmethod
        def get_c_param_type_text(type_binding):
            return type_binding.get_in_c_type_text(False)

        @staticmethod
        def get_setter_value_expression(param_type_binding, value_ref):
            pattern = param_type_binding.get_setter_value_expression_pattern()
            if pattern:
                return pattern % value_ref
            else:
                return value_ref

    class RawParameterMode:
        @staticmethod
        def get_c_param_type_text(type_binding):
            return type_binding.reduce_to_raw_type().get_c_param_type(ParamType.TYPE_BUILDER_OUTPUT, False).get_text()

        @staticmethod
        def get_setter_value_expression(param_type_binding, value_ref):
            return value_ref

    class CombinedMode:
        @staticmethod
        def get_c_param_type_text(type_binding):
            return get_annotated_type_text(type_binding.reduce_to_raw_type().get_c_param_type(ParamType.TYPE_BUILDER_OUTPUT, False).get_text(),
                type_binding.get_in_c_type_text(False))

        @staticmethod
        def get_setter_value_expression(param_type_binding, value_ref):
            pattern = param_type_binding.get_setter_value_expression_pattern()
            if pattern:
                return pattern % value_ref
            else:
                return value_ref

class Generator:
    frontend_class_field_lines = []
    frontend_domain_class_lines = []

    method_name_enum_list = []
    backend_method_declaration_list = []
    backend_method_implementation_list = []
    backend_method_name_declaration_list = []
    method_handler_list = []
    frontend_method_list = []
    backend_js_domain_initializer_list = []

    backend_virtual_setters_list = []
    backend_agent_interface_list = []
    backend_setters_list = []
    backend_constructor_init_list = []
    backend_field_list = []
    frontend_constructor_init_list = []
    type_builder_fragments = []
    type_builder_forwards = []
    validator_impl_list = []
    validator_impl_raw_types_list = []


    @staticmethod
    def go():
        Generator.validator_impl_list.append(Generator.validator_impl_raw_types_list)

        Generator.process_types(type_map)

        first_cycle_guardable_list_list = [
            Generator.backend_method_declaration_list,
            Generator.backend_method_implementation_list,
            Generator.backend_method_name_declaration_list,
            Generator.backend_agent_interface_list,
            Generator.frontend_class_field_lines,
            Generator.frontend_constructor_init_list,
            Generator.frontend_domain_class_lines,
            Generator.frontend_method_list,
            Generator.method_handler_list,
            Generator.method_name_enum_list]

        for json_domain in json_api["domains"]:
            domain_name = json_domain["domain"]
            domain_name_lower = domain_name.lower()

            domain_fixes = DomainNameFixes.get_fixed_data(domain_name)

            domain_guard = domain_fixes.get_guard()

            if domain_guard:
                for l in first_cycle_guardable_list_list:
                    domain_guard.generate_open(l)

            agent_field_name = domain_fixes.agent_field_name

            frontend_method_declaration_lines = []

            Generator.backend_js_domain_initializer_list.append("// %s.\n" % domain_name)

            if not domain_fixes.skip_js_bind:
                Generator.backend_js_domain_initializer_list.append("InspectorBackend.register%sDispatcher = InspectorBackend.registerDomainDispatcher.bind(InspectorBackend, \"%s\");\n" % (domain_name, domain_name))

            if "events" in json_domain:
                for json_event in json_domain["events"]:
                    Generator.process_event(json_event, domain_name, frontend_method_declaration_lines)

            Generator.frontend_class_field_lines.append("    %s m_%s;\n" % (domain_name, domain_name_lower))
            Generator.frontend_constructor_init_list.append("    , m_%s(inspectorFrontendChannel)\n" % domain_name_lower)
            Generator.frontend_domain_class_lines.append(Templates.frontend_domain_class.substitute(None,
                domainClassName=domain_name,
                domainFieldName=domain_name_lower,
                frontendDomainMethodDeclarations=join(flatten_list(frontend_method_declaration_lines), "")))

            agent_interface_name = Capitalizer.lower_camel_case_to_upper(domain_name) + "CommandHandler"
            Generator.backend_agent_interface_list.append("    class %s {\n" % agent_interface_name)
            Generator.backend_agent_interface_list.append("    public:\n")
            if "commands" in json_domain:
                for json_command in json_domain["commands"]:
                    Generator.process_command(json_command, domain_name, agent_field_name)
            Generator.backend_agent_interface_list.append("\n    protected:\n")
            Generator.backend_agent_interface_list.append("        virtual ~%s() { }\n" % agent_interface_name)
            Generator.backend_agent_interface_list.append("    };\n\n")

            if domain_guard:
                for l in reversed(first_cycle_guardable_list_list):
                    domain_guard.generate_close(l)
            Generator.backend_js_domain_initializer_list.append("\n")

        sorted_json_domains = list(json_api["domains"])
        sorted_json_domains.sort(key=lambda o: o["domain"])

        sorted_cycle_guardable_list_list = [
            Generator.backend_constructor_init_list,
            Generator.backend_virtual_setters_list,
            Generator.backend_setters_list,
            Generator.backend_field_list]

        for json_domain in sorted_json_domains:
            domain_name = json_domain["domain"]

            domain_fixes = DomainNameFixes.get_fixed_data(domain_name)
            domain_guard = domain_fixes.get_guard()

            if domain_guard:
                for l in sorted_cycle_guardable_list_list:
                    domain_guard.generate_open(l)

            agent_interface_name = Capitalizer.lower_camel_case_to_upper(domain_name) + "CommandHandler"

            agent_field_name = domain_fixes.agent_field_name
            Generator.backend_constructor_init_list.append("        , m_%s(0)" % agent_field_name)
            Generator.backend_virtual_setters_list.append("    virtual void registerAgent(%s* %s) = 0;" % (agent_interface_name, agent_field_name))
            Generator.backend_setters_list.append("    virtual void registerAgent(%s* %s) { ASSERT(!m_%s); m_%s = %s; }" % (agent_interface_name, agent_field_name, agent_field_name, agent_field_name, agent_field_name))
            Generator.backend_field_list.append("    %s* m_%s;" % (agent_interface_name, agent_field_name))

            if domain_guard:
                for l in reversed(sorted_cycle_guardable_list_list):
                    domain_guard.generate_close(l)

        RawTypes.generate_validate_methods(Writer(Generator.validator_impl_raw_types_list, ""))

    @staticmethod
    def process_event(json_event, domain_name, frontend_method_declaration_lines):
        event_name = json_event["name"]
        parameter_list = []
        method_line_list = []
        backend_js_event_param_list = []
        ad_hoc_type_output = []
        frontend_method_declaration_lines.append(ad_hoc_type_output)
        ad_hoc_type_writer = Writer(ad_hoc_type_output, "        ")
        if "parameters" in json_event:
            method_line_list.append("    RefPtr<InspectorObject> paramsObject = InspectorObject::create();\n")
            for json_parameter in json_event["parameters"]:
                parameter_name = json_parameter["name"]

                raw_type = resolve_param_raw_type(json_parameter, domain_name)

                json_optional = "optional" in json_parameter and json_parameter["optional"]

                optional_mask = raw_type.is_event_param_check_optional()
                c_type = raw_type.get_c_param_type(ParamType.EVENT, json_optional)

                setter_type = raw_type.get_setter_name()

                optional = optional_mask and json_optional

                param_type_binding = Generator.resolve_type_and_generate_ad_hoc(json_parameter, event_name, domain_name, ad_hoc_type_writer)

                annotated_type = get_annotated_type_text(c_type.get_text(), param_type_binding.get_in_c_type_text(json_optional))

                parameter_list.append("%s %s" % (annotated_type, parameter_name))

                setter_argument = c_type.get_setter_format() % parameter_name

                setter_code = "    paramsObject->set%s(\"%s\", %s);\n" % (setter_type, parameter_name, setter_argument)
                if optional:
                    setter_code = ("    if (%s)\n    " % parameter_name) + setter_code
                method_line_list.append(setter_code)

                backend_js_event_param_list.append("\"%s\"" % parameter_name)
            method_line_list.append("    %sMessage->setObject(\"params\", paramsObject);\n" % event_name)
        frontend_method_declaration_lines.append(
            "        void %s(%s);\n" % (event_name, join(parameter_list, ", ")))

        Generator.frontend_method_list.append(Templates.frontend_method.substitute(None,
            domainName=domain_name, eventName=event_name,
            parameters=join(parameter_list, ", "),
            code=join(method_line_list, "")))

        Generator.backend_js_domain_initializer_list.append("InspectorBackend.registerEvent(\"%s.%s\", [%s]);\n" % (
            domain_name, event_name, join(backend_js_event_param_list, ", ")))

    @staticmethod
    def process_command(json_command, domain_name, agent_field_name):
        json_command_name = json_command["name"]
        Generator.method_name_enum_list.append("        k%s_%sCmd," % (domain_name, json_command["name"]))
        Generator.method_handler_list.append("            &InspectorBackendDispatcherImpl::%s_%s," % (domain_name, json_command_name))
        Generator.backend_method_declaration_list.append("    void %s_%s(long callId, InspectorObject* requestMessageObject);" % (domain_name, json_command_name))

        ad_hoc_type_output = []
        Generator.backend_agent_interface_list.append(ad_hoc_type_output)
        ad_hoc_type_writer = Writer(ad_hoc_type_output, "        ")

        Generator.backend_agent_interface_list.append("        virtual void %s(ErrorString*" % json_command_name)

        method_in_code = ""
        method_out_code = ""
        agent_call_param_list = []
        response_cook_list = []
        backend_js_reply_param_list = []
        request_message_param = ""
        js_parameters_text = ""
        if "parameters" in json_command:
            json_params = json_command["parameters"]
            method_in_code += Templates.param_container_access_code
            request_message_param = " requestMessageObject"
            js_param_list = []

            for json_parameter in json_params:
                json_param_name = json_parameter["name"]
                param_raw_type = resolve_param_raw_type(json_parameter, domain_name)

                var_type = param_raw_type.get_c_param_type(ParamType.INPUT, None)
                formal_param_type = var_type
                getter_name = param_raw_type.get_getter_name()

                optional = json_parameter.get("optional")
                if optional:
                    code = ("    bool %s_valueFound = false;\n"
                            "    %s in_%s = get%s(paramsContainerPtr, \"%s\", &%s_valueFound, protocolErrorsPtr);\n" %
                           (json_param_name, var_type.get_text(), json_param_name, getter_name, json_param_name, json_param_name))
                    param = ", %s_valueFound ? &in_%s : 0" % (json_param_name, json_param_name)
                    formal_param_type_pattern = "const %s*"
                else:
                    code = ("    %s in_%s = get%s(paramsContainerPtr, \"%s\", 0, protocolErrorsPtr);\n" %
                            (var_type.get_text(), json_param_name, getter_name, json_param_name))
                    param = ", in_%s" % json_param_name
                    if param_raw_type.is_heavy_value():
                        formal_param_type_pattern = "const %s&"
                    else:
                        formal_param_type_pattern = "%s"

                method_in_code += code
                agent_call_param_list.append(param)
                Generator.backend_agent_interface_list.append(", %s in_%s" % (formal_param_type_pattern % formal_param_type.get_text(), json_param_name))

                js_bind_type = param_raw_type.get_js_bind_type()
                js_param_text = "{\"name\": \"%s\", \"type\": \"%s\", \"optional\": %s}" % (
                    json_param_name,
                    js_bind_type,
                    ("true" if ("optional" in json_parameter and json_parameter["optional"]) else "false"))

                js_param_list.append(js_param_text)

            js_parameters_text = join(js_param_list, ", ")

        response_cook_text = ""
        js_reply_list = "[]"
        if "returns" in json_command:
            method_out_code += "\n"
            for json_return in json_command["returns"]:

                json_return_name = json_return["name"]

                optional = "optional" in json_return and json_return["optional"]

                return_type_binding = Generator.resolve_type_and_generate_ad_hoc(json_return, json_command_name, domain_name, ad_hoc_type_writer)

                raw_type = return_type_binding.reduce_to_raw_type()
                setter_type = raw_type.get_setter_name()
                initializer = raw_type.get_c_initializer()
                var_type = raw_type.get_c_param_type(ParamType.OUTPUT, None)

                code = "    %s out_%s = %s;\n" % (var_type.get_text(), json_return_name, initializer)
                param = ", %sout_%s" % (raw_type.get_output_pass_model().get_argument_prefix(), json_return_name)
                cook = "        result->set%s(\"%s\", out_%s);\n" % (setter_type, json_return_name, json_return_name)
                if optional:
                    # FIXME: support optional properly. Probably an additional output parameter should be in each case.
                    # FIXME: refactor this condition; it's a hack now.
                    if var_type.get_text() == "bool" or var_type.get_text().startswith("RefPtr<"):
                        cook = ("        if (out_%s)\n    " % json_return_name) + cook
                    else:
                        cook = "        // FIXME: support optional here.\n" + cook

                method_out_code += code
                agent_call_param_list.append(param)

                annotated_type = get_annotated_type_text(var_type.get_text() + raw_type.get_output_pass_model().get_parameter_type_suffix(),
                                                         return_type_binding.get_in_c_type_text(optional))

                Generator.backend_agent_interface_list.append(", %s out_%s" % (annotated_type, json_return_name))
                response_cook_list.append(cook)

                backend_js_reply_param_list.append("\"%s\"" % json_return_name)

            js_reply_list = "[%s]" % join(backend_js_reply_param_list, ", ")

            response_cook_text = "    if (!protocolErrors->length() && !error.length()) {\n%s    }\n" % join(response_cook_list, "")

        Generator.backend_method_implementation_list.append(Templates.backend_method.substitute(None,
            domainName=domain_name, methodName=json_command_name,
            agentField="m_" + agent_field_name,
            methodInCode=method_in_code,
            methodOutCode=method_out_code,
            agentCallParams=join(agent_call_param_list, ""),
            requestMessageObject=request_message_param,
            responseCook=response_cook_text))
        Generator.backend_method_name_declaration_list.append("    \"%s.%s\"," % (domain_name, json_command_name))

        Generator.backend_js_domain_initializer_list.append("InspectorBackend.registerCommand(\"%s.%s\", [%s], %s);\n" % (domain_name, json_command_name, js_parameters_text, js_reply_list))
        Generator.backend_agent_interface_list.append(") = 0;\n")

    @staticmethod
    def resolve_type_and_generate_ad_hoc(json_param, method_name, domain_name, ad_hoc_type_writer):
        param_name = json_param["name"]
        ad_hoc_type_list = []

        class AdHocTypeContext:
            container_full_name_prefix = "<not yet defined>"

            @staticmethod
            def get_type_name_fix():
                class NameFix:
                    class_name = Capitalizer.lower_camel_case_to_upper(param_name)

                    @staticmethod
                    def output_comment(writer):
                        writer.newline("// Named after parameter '%s' while generating command/event %s.\n" % (param_name, method_name))

                return NameFix

            @staticmethod
            def add_type(binding):
                ad_hoc_type_list.append(binding)

        type_binding = resolve_param_type(json_param, domain_name, AdHocTypeContext)

        class InterfaceForwardListener:
            @staticmethod
            def add_type_data(type_data):
                pass

        class InterfaceResolveContext:
            forward_listener = InterfaceForwardListener

        for type in ad_hoc_type_list:
            type.resolve_inner(InterfaceResolveContext)

        class InterfaceGenerateContext:
            validator_writer = "not supported in InterfaceGenerateContext"

        for type in ad_hoc_type_list:
            generator = type.get_code_generator()
            if generator:
                generator.generate_type_builder(ad_hoc_type_writer, InterfaceGenerateContext)

        return type_binding

    @staticmethod
    def process_types(type_map):
        output = Generator.type_builder_fragments

        class GenerateContext:
            validator_writer = Writer(Generator.validator_impl_list, "")

        def generate_all_domains_code(out, type_data_callback):
            writer = Writer(out, "")
            for domain_data in type_map.domains():
                domain_fixes = DomainNameFixes.get_fixed_data(domain_data.name())
                domain_guard = domain_fixes.get_guard()

                namespace_declared = []

                def namespace_lazy_generator():
                    if not namespace_declared:
                        if domain_guard:
                            domain_guard.generate_open(out)
                        writer.newline("namespace ")
                        writer.append(domain_data.name())
                        writer.append(" {\n")
                        # What is a better way to change value from outer scope?
                        namespace_declared.append(True)
                    return writer

                for type_data in domain_data.types():
                    type_data_callback(type_data, namespace_lazy_generator)

                if namespace_declared:
                    writer.append("} // ")
                    writer.append(domain_data.name())
                    writer.append("\n\n")

                    if domain_guard:
                        domain_guard.generate_close(out)

        def create_type_builder_caller(generate_pass_id):
            def call_type_builder(type_data, writer_getter):
                code_generator = type_data.get_binding().get_code_generator()
                if code_generator and generate_pass_id == code_generator.get_generate_pass_id():
                    writer = writer_getter()

                    code_generator.generate_type_builder(writer, GenerateContext)
            return call_type_builder

        generate_all_domains_code(output, create_type_builder_caller(TypeBuilderPass.MAIN))

        Generator.type_builder_forwards.append("// Forward declarations.\n")

        def generate_forward_callback(type_data, writer_getter):
            if type_data in global_forward_listener.type_data_set:
                binding = type_data.get_binding()
                binding.get_code_generator().generate_forward_declaration(writer_getter())
        generate_all_domains_code(Generator.type_builder_forwards, generate_forward_callback)

        Generator.type_builder_forwards.append("// End of forward declarations.\n\n")

        Generator.type_builder_forwards.append("// Typedefs.\n")

        generate_all_domains_code(Generator.type_builder_forwards, create_type_builder_caller(TypeBuilderPass.TYPEDEF))

        Generator.type_builder_forwards.append("// End of typedefs.\n\n")


def flatten_list(input):
    res = []

    def fill_recursive(l):
        for item in l:
            if isinstance(item, list):
                fill_recursive(item)
            else:
                res.append(item)
    fill_recursive(input)
    return res


Generator.go()

backend_h_file = open(output_header_dirname + "/InspectorBackendDispatcher.h", "w")
backend_cpp_file = open(output_cpp_dirname + "/InspectorBackendDispatcher.cpp", "w")

frontend_h_file = open(output_header_dirname + "/InspectorFrontend.h", "w")
frontend_cpp_file = open(output_cpp_dirname + "/InspectorFrontend.cpp", "w")

typebuilder_h_file = open(output_header_dirname + "/InspectorTypeBuilder.h", "w")
typebuilder_cpp_file = open(output_cpp_dirname + "/InspectorTypeBuilder.cpp", "w")

backend_js_file = open(output_cpp_dirname + "/InspectorBackendStub.js", "w")


backend_h_file.write(Templates.backend_h.substitute(None,
    virtualSetters=join(Generator.backend_virtual_setters_list, "\n"),
    agentInterfaces=join(flatten_list(Generator.backend_agent_interface_list), ""),
    methodNamesEnumContent=join(Generator.method_name_enum_list, "\n")))

backend_cpp_file.write(Templates.backend_cpp.substitute(None,
    constructorInit=join(Generator.backend_constructor_init_list, "\n"),
    setters=join(Generator.backend_setters_list, "\n"),
    fieldDeclarations=join(Generator.backend_field_list, "\n"),
    methodNameDeclarations=join(Generator.backend_method_name_declaration_list, "\n"),
    methods=join(Generator.backend_method_implementation_list, "\n"),
    methodDeclarations=join(Generator.backend_method_declaration_list, "\n"),
    messageHandlers=join(Generator.method_handler_list, "\n")))

frontend_h_file.write(Templates.frontend_h.substitute(None,
    fieldDeclarations=join(Generator.frontend_class_field_lines, ""),
    domainClassList=join(Generator.frontend_domain_class_lines, "")))

frontend_cpp_file.write(Templates.frontend_cpp.substitute(None,
    constructorInit=join(Generator.frontend_constructor_init_list, ""),
    methods=join(Generator.frontend_method_list, "\n")))

typebuilder_h_file.write(Templates.typebuilder_h.substitute(None,
    typeBuilders=join(flatten_list(Generator.type_builder_fragments), ""),
    forwards=join(Generator.type_builder_forwards, "")))

typebuilder_cpp_file.write(Templates.typebuilder_cpp.substitute(None,
    enumConstantValues=EnumConstants.get_enum_constant_code(),
    validatorCode=join(flatten_list(Generator.validator_impl_list), "")))

backend_js_file.write(Templates.backend_js.substitute(None,
    domainInitializers=join(Generator.backend_js_domain_initializer_list, "")))

backend_h_file.close()
backend_cpp_file.close()

frontend_h_file.close()
frontend_cpp_file.close()

backend_js_file.close()
