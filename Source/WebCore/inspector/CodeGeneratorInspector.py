#!/usr/bin/env python
# Copyright (c) 2011 Google Inc. All rights reserved.
# Copyright (c) 2012 Intel Corporation. All rights reserved.
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


TYPES_WITH_RUNTIME_CAST_SET = frozenset(["Runtime.RemoteObject", "Runtime.PropertyDescriptor",
                                         "Debugger.FunctionDetails", "Debugger.CallFrame", "Canvas.TraceLog",
                                         # This should be a temporary hack. TimelineEvent should be created via generated C++ API.
                                         "Timeline.TimelineEvent"])

TYPES_WITH_OPEN_FIELD_LIST_SET = frozenset(["Timeline.TimelineEvent",
                                            # InspectorStyleSheet not only creates this property but wants to read it and modify it.
                                            "CSS.CSSProperty",
                                            # InspectorResourceAgent needs to update mime-type.
                                            "Network.Response"])

EXACTLY_INT_SUPPORTED = False

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
except Exception:
    # Work with python 2 and 3 http://docs.python.org/py3k/howto/pyporting.html
    exc = sys.exc_info()[1]
    sys.stderr.write("Failed to parse command-line arguments: %s\n\n" % exc)
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

    skip_js_bind_domains = set(["DOMDebugger"])


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
        def request_raw_internal_runtime_cast(cls):
            if not cls.need_internal_runtime_cast_:
                cls.need_internal_runtime_cast_ = True

        @classmethod
        def get_raw_validator_call_text(cls):
            return "RuntimeCastHelper::assertType<InspectorValue::Type%s>" % cls.get_validate_method_params().template_type

    class String(BaseType):
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
                template_type = "String"
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

        @staticmethod
        def get_raw_type_model():
            return TypeModel.String

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

        @classmethod
        def get_raw_validator_call_text(cls):
            return "RuntimeCastHelper::assertInt"

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByPointer

        @staticmethod
        def is_heavy_value():
            return False

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "int"

        @staticmethod
        def get_raw_type_model():
            return TypeModel.Int

    class Number(BaseType):
        @staticmethod
        def get_getter_name():
            return "Double"

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
                template_type = "Number"
            return ValidateMethodParams

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByPointer

        @staticmethod
        def is_heavy_value():
            return False

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "double"

        @staticmethod
        def get_raw_type_model():
            return TypeModel.Number

    class Bool(BaseType):
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
        def get_validate_method_params():
            class ValidateMethodParams:
                template_type = "Boolean"
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

        @staticmethod
        def get_raw_type_model():
            return TypeModel.Bool

    class Object(BaseType):
        @staticmethod
        def get_getter_name():
            return "Object"

        @staticmethod
        def get_setter_name():
            return "Value"

        @staticmethod
        def get_c_initializer():
            return "InspectorObject::create()"

        @staticmethod
        def get_js_bind_type():
            return "object"

        @staticmethod
        def get_output_argument_prefix():
            return ""

        @staticmethod
        def get_validate_method_params():
            class ValidateMethodParams:
                template_type = "Object"
            return ValidateMethodParams

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByReference

        @staticmethod
        def is_heavy_value():
            return True

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "InspectorObject"

        @staticmethod
        def get_raw_type_model():
            return TypeModel.Object

    class Any(BaseType):
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
        def get_raw_validator_call_text():
            return "RuntimeCastHelper::assertAny"

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByReference

        @staticmethod
        def is_heavy_value():
            return True

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "InspectorValue"

        @staticmethod
        def get_raw_type_model():
            return TypeModel.Any

    class Array(BaseType):
        @staticmethod
        def get_getter_name():
            return "Array"

        @staticmethod
        def get_setter_name():
            return "Value"

        @staticmethod
        def get_c_initializer():
            return "InspectorArray::create()"

        @staticmethod
        def get_js_bind_type():
            return "object"

        @staticmethod
        def get_output_argument_prefix():
            return ""

        @staticmethod
        def get_validate_method_params():
            class ValidateMethodParams:
                template_type = "Array"
            return ValidateMethodParams

        @staticmethod
        def get_output_pass_model():
            return RawTypes.OutputPassModel.ByReference

        @staticmethod
        def is_heavy_value():
            return True

        @staticmethod
        def get_array_item_raw_c_type_text():
            return "InspectorArray"

        @staticmethod
        def get_raw_type_model():
            return TypeModel.Array


def replace_right_shift(input_str):
    return input_str.replace(">>", "> >")


class CommandReturnPassModel:
    class ByReference:
        def __init__(self, var_type, set_condition):
            self.var_type = var_type
            self.set_condition = set_condition

        def get_return_var_type(self):
            return self.var_type

        @staticmethod
        def get_output_argument_prefix():
            return ""

        @staticmethod
        def get_output_to_raw_expression():
            return "%s"

        def get_output_parameter_type(self):
            return self.var_type + "&"

        def get_set_return_condition(self):
            return self.set_condition

    class ByPointer:
        def __init__(self, var_type):
            self.var_type = var_type

        def get_return_var_type(self):
            return self.var_type

        @staticmethod
        def get_output_argument_prefix():
            return "&"

        @staticmethod
        def get_output_to_raw_expression():
            return "%s"

        def get_output_parameter_type(self):
            return self.var_type + "*"

        @staticmethod
        def get_set_return_condition():
            return None

    class OptOutput:
        def __init__(self, var_type):
            self.var_type = var_type

        def get_return_var_type(self):
            return "TypeBuilder::OptOutput<%s>" % self.var_type

        @staticmethod
        def get_output_argument_prefix():
            return "&"

        @staticmethod
        def get_output_to_raw_expression():
            return "%s.getValue()"

        def get_output_parameter_type(self):
            return "TypeBuilder::OptOutput<%s>*" % self.var_type

        @staticmethod
        def get_set_return_condition():
            return "%s.isAssigned()"


class TypeModel:
    class RefPtrBased(object):
        def __init__(self, class_name):
            self.class_name = class_name
            self.optional = False

        def get_optional(self):
            result = TypeModel.RefPtrBased(self.class_name)
            result.optional = True
            return result

        def get_command_return_pass_model(self):
            if self.optional:
                set_condition = "%s"
            else:
                set_condition = None
            return CommandReturnPassModel.ByReference(replace_right_shift("RefPtr<%s>" % self.class_name), set_condition)

        def get_input_param_type_text(self):
            return replace_right_shift("PassRefPtr<%s>" % self.class_name)

        @staticmethod
        def get_event_setter_expression_pattern():
            return "%s"

    class Enum(object):
        def __init__(self, base_type_name):
            self.type_name = base_type_name + "::Enum"

        def get_optional(base_self):
            class EnumOptional:
                @classmethod
                def get_optional(cls):
                    return cls

                @staticmethod
                def get_command_return_pass_model():
                    return CommandReturnPassModel.OptOutput(base_self.type_name)

                @staticmethod
                def get_input_param_type_text():
                    return base_self.type_name + "*"

                @staticmethod
                def get_event_setter_expression_pattern():
                    raise Exception("TODO")
            return EnumOptional

        def get_command_return_pass_model(self):
            return CommandReturnPassModel.ByPointer(self.type_name)

        def get_input_param_type_text(self):
            return self.type_name

        @staticmethod
        def get_event_setter_expression_pattern():
            return "%s"

    class ValueType(object):
        def __init__(self, type_name, is_heavy):
            self.type_name = type_name
            self.is_heavy = is_heavy

        def get_optional(self):
            return self.ValueOptional(self)

        def get_command_return_pass_model(self):
            return CommandReturnPassModel.ByPointer(self.type_name)

        def get_input_param_type_text(self):
            if self.is_heavy:
                return "const %s&" % self.type_name
            else:
                return self.type_name

        def get_opt_output_type_(self):
            return self.type_name

        @staticmethod
        def get_event_setter_expression_pattern():
            return "%s"

        class ValueOptional:
            def __init__(self, base):
                self.base = base

            def get_optional(self):
                return self

            def get_command_return_pass_model(self):
                return CommandReturnPassModel.OptOutput(self.base.get_opt_output_type_())

            def get_input_param_type_text(self):
                return "const %s* const" % self.base.type_name

            @staticmethod
            def get_event_setter_expression_pattern():
                return "*%s"

    class ExactlyInt(ValueType):
        def __init__(self):
            TypeModel.ValueType.__init__(self, "int", False)

        def get_input_param_type_text(self):
            return "TypeBuilder::ExactlyInt"

        def get_opt_output_type_(self):
            return "TypeBuilder::ExactlyInt"

    @classmethod
    def init_class(cls):
        cls.Bool = cls.ValueType("bool", False)
        if EXACTLY_INT_SUPPORTED:
            cls.Int = cls.ExactlyInt()
        else:
            cls.Int = cls.ValueType("int", False)
        cls.Number = cls.ValueType("double", False)
        cls.String = cls.ValueType("String", True,)
        cls.Object = cls.RefPtrBased("InspectorObject")
        cls.Array = cls.RefPtrBased("InspectorArray")
        cls.Any = cls.RefPtrBased("InspectorValue")

TypeModel.init_class()


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

    def get_indent(self):
        return self.indent

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
        return ",\n".join(output) + "\n"


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
            full_name_prefix_for_use = ad_hoc_type_context.container_relative_name_prefix
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

                                    domain_fixes = DomainNameFixes.get_fixed_data(context_domain_name)
                                    domain_guard = domain_fixes.get_guard()
                                    if domain_guard:
                                        domain_guard.generate_open(validator_writer)

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
                                        validator_writer.newline("    ASSERT(%s);\n" % " || ".join(condition_list))
                                    validator_writer.newline("}\n")

                                    if domain_guard:
                                        domain_guard.generate_close(validator_writer)

                                    validator_writer.newline("\n\n")

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
                    def get_array_item_c_type_text(cls):
                        return helper.full_name_prefix_for_use + fixed_type_name.class_name + "::Enum"

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return "TypeBuilder::getEnumConstantValue(%s)"

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.String

                    @staticmethod
                    def get_type_model():
                        return TypeModel.Enum(helper.full_name_prefix_for_use + fixed_type_name.class_name)

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
                        def get_type_model():
                            return TypeModel.String

                        @staticmethod
                        def get_setter_value_expression_pattern():
                            return None

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
                        def get_type_model():
                            return TypeModel.ValueType("%s%s" % (helper.full_name_prefix_for_use, fixed_type_name.class_name), True)

                        @staticmethod
                        def get_setter_value_expression_pattern():
                            return None

                        @classmethod
                        def get_array_item_c_type_text(cls):
                            return "const %s%s&" % (helper.full_name_prefix_for_use, fixed_type_name.class_name)

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
                            ad_hoc_types = ad_hoc_type_list

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
                        if cls.need_internal_runtime_cast_:
                            return
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

                                is_open_type = (context_domain_name + "." + class_name) in TYPES_WITH_OPEN_FIELD_LIST_SET

                                fixed_type_name.output_comment(writer)
                                writer.newline("class ")
                                writer.append(class_name)
                                writer.append(" : public ")
                                if is_open_type:
                                    writer.append("InspectorObject")
                                else:
                                    writer.append("InspectorObjectBase")
                                writer.append(" {\n")
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

        Builder(PassRefPtr</*%s*/InspectorObject> ptr)
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
                                       param_type_binding.get_type_model().get_input_param_type_text(),
                                       state_enum_items[pos], prop_name,
                                       param_raw_type.get_setter_name(), prop_name,
                                       format_setter_value_expression(param_type_binding, "value"),
                                       state_enum_items[pos]))

                                    pos += 1

                                writer.newline_multiline("""
        operator RefPtr<%s>& ()
        {
            COMPILE_ASSERT(STATE == AllFieldsSet, result_is_not_ready);
            COMPILE_ASSERT(sizeof(%s) == sizeof(InspectorObject), cannot_cast);
            return *reinterpret_cast<RefPtr<%s>*>(&m_result);
        }

        PassRefPtr<%s> release()
        {
            return RefPtr<%s>(*this).release();
        }
    };

"""
                                % (class_name, class_name, class_name, class_name, class_name))

                                writer.newline("    /*\n")
                                writer.newline("     * Synthetic constructor:\n")
                                writer.newline("     * RefPtr<%s> result = %s::create()" % (class_name, class_name))
                                for prop_data in resolve_data.main_properties:
                                    writer.append_multiline("\n     *     .set%s(...)" % Capitalizer.lower_camel_case_to_upper(prop_data.p["name"]))
                                writer.append_multiline(";\n     */\n")

                                writer.newline_multiline(
"""    static Builder<NoFieldsSet> create()
    {
        return Builder<NoFieldsSet>(InspectorObject::create());
    }
""")

                                writer.newline("    typedef TypeBuilder::StructItemTraits ItemTraits;\n")

                                for prop_data in resolve_data.optional_properties:
                                    prop_name = prop_data.p["name"]
                                    param_type_binding = prop_data.param_type_binding
                                    setter_name = "set%s" % Capitalizer.lower_camel_case_to_upper(prop_name)

                                    writer.append_multiline("\n    void %s" % setter_name)
                                    writer.append("(%s value)\n" % param_type_binding.get_type_model().get_input_param_type_text())
                                    writer.newline("    {\n")
                                    writer.newline("        this->set%s(\"%s\", %s);\n"
                                        % (param_type_binding.reduce_to_raw_type().get_setter_name(), prop_data.p["name"],
                                           format_setter_value_expression(param_type_binding, "value")))
                                    writer.newline("    }\n")


                                    if setter_name in INSPECTOR_OBJECT_SETTER_NAMES:
                                        writer.newline("    using InspectorObjectBase::%s;\n\n" % setter_name)

                                if class_binding_cls.need_user_runtime_cast_:
                                    writer.newline("    static PassRefPtr<%s> runtimeCast(PassRefPtr<InspectorValue> value)\n" % class_name)
                                    writer.newline("    {\n")
                                    writer.newline("        RefPtr<InspectorObject> object;\n")
                                    writer.newline("        bool castRes = value->asObject(&object);\n")
                                    writer.newline("        ASSERT_UNUSED(castRes, castRes);\n")
                                    writer.append("#if %s\n" % VALIDATOR_IFDEF_NAME)
                                    writer.newline("        assertCorrectValue(object.get());\n")
                                    writer.append("#endif  // %s\n" % VALIDATOR_IFDEF_NAME)
                                    writer.newline("        COMPILE_ASSERT(sizeof(%s) == sizeof(InspectorObjectBase), type_cast_problem);\n" % class_name)
                                    writer.newline("        return static_cast<%s*>(static_cast<InspectorObjectBase*>(object.get()));\n" % class_name)
                                    writer.newline("    }\n")
                                    writer.append("\n")

                                if class_binding_cls.need_internal_runtime_cast_:
                                    writer.append("#if %s\n" % VALIDATOR_IFDEF_NAME)
                                    writer.newline("    static void assertCorrectValue(InspectorValue* value);\n")
                                    writer.append("#endif  // %s\n" % VALIDATOR_IFDEF_NAME)

                                    closed_field_set = (context_domain_name + "." + class_name) not in TYPES_WITH_OPEN_FIELD_LIST_SET

                                    validator_writer = generate_context.validator_writer

                                    domain_fixes = DomainNameFixes.get_fixed_data(context_domain_name)
                                    domain_guard = domain_fixes.get_guard()
                                    if domain_guard:
                                        domain_guard.generate_open(validator_writer)

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

                                    if closed_field_set:
                                        validator_writer.newline("    int foundPropertiesCount = %s;\n" % len(resolve_data.main_properties))

                                    for prop_data in resolve_data.optional_properties:
                                        validator_writer.newline("    {\n")
                                        it_name = "%sPos" % prop_data.p["name"]
                                        validator_writer.newline("        InspectorObject::iterator %s;\n" % it_name)
                                        validator_writer.newline("        %s = object->find(\"%s\");\n" % (it_name, prop_data.p["name"]))
                                        validator_writer.newline("        if (%s != object->end()) {\n" % it_name)
                                        validator_writer.newline("            %s(%s->second.get());\n" % (prop_data.param_type_binding.get_validator_call_text(), it_name))
                                        if closed_field_set:
                                            validator_writer.newline("            ++foundPropertiesCount;\n")
                                        validator_writer.newline("        }\n")
                                        validator_writer.newline("    }\n")

                                    if closed_field_set:
                                        validator_writer.newline("    ASSERT(foundPropertiesCount == object->size());\n")
                                    validator_writer.newline("}\n")

                                    if domain_guard:
                                        domain_guard.generate_close(validator_writer)

                                    validator_writer.newline("\n\n")

                                if is_open_type:
                                    cpp_writer = generate_context.cpp_writer
                                    writer.append("\n")
                                    writer.newline("    // Property names for type generated as open.\n")
                                    for prop_data in resolve_data.main_properties + resolve_data.optional_properties:
                                        prop_name = prop_data.p["name"]
                                        prop_field_name = Capitalizer.lower_camel_case_to_upper(prop_name)
                                        writer.newline("    static const char* %s;\n" % (prop_field_name))
                                        cpp_writer.newline("const char* %s%s::%s = \"%s\";\n" % (helper.full_name_prefix_for_impl, class_name, prop_field_name, prop_name))


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
                    def get_array_item_c_type_text(cls):
                        return helper.full_name_prefix_for_use + fixed_type_name.class_name

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return None

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.Object

                    @staticmethod
                    def get_type_model():
                        return TypeModel.RefPtrBased(helper.full_name_prefix_for_use + fixed_type_name.class_name)

                    class AdHocTypeContextImpl:
                        def __init__(self, property_name, class_name, resolve_context, ad_hoc_type_list, parent_full_name_prefix):
                            self.property_name = property_name
                            self.class_name = class_name
                            self.resolve_context = resolve_context
                            self.ad_hoc_type_list = ad_hoc_type_list
                            self.container_full_name_prefix = parent_full_name_prefix + class_name + "::"
                            self.container_relative_name_prefix = ""

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
                        RawTypes.Object.request_raw_internal_runtime_cast()

                    @staticmethod
                    def get_code_generator():
                        pass

                    @staticmethod
                    def get_validator_call_text():
                        return "RuntimeCastHelper::assertType<InspectorValue::TypeObject>"

                    @classmethod
                    def get_array_item_c_type_text(cls):
                        return cls.reduce_to_raw_type().get_array_item_raw_c_type_text()

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return None

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.Object

                    @staticmethod
                    def get_type_model():
                        return TypeModel.Object

                return PlainObjectBinding
        elif json_typable["type"] == "array":
            if "items" in json_typable:

                ad_hoc_types = []

                class AdHocTypeContext:
                    container_full_name_prefix = "<not yet defined>"
                    container_relative_name_prefix = ""

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
                        if cls.need_internal_runtime_cast_:
                            return
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
                    def get_array_item_c_type_text(cls):
                        return replace_right_shift("TypeBuilder::Array<%s>" % cls.resolve_data_.item_type_binding.get_array_item_c_type_text())

                    @staticmethod
                    def get_setter_value_expression_pattern():
                        return None

                    @staticmethod
                    def reduce_to_raw_type():
                        return RawTypes.Array

                    @classmethod
                    def get_type_model(cls):
                        return TypeModel.RefPtrBased(cls.get_array_item_c_type_text())

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

    def get_array_item_c_type_text(self):
        return self.raw_type_.get_array_item_raw_c_type_text()

    def get_setter_value_expression_pattern(self):
        return None

    def reduce_to_raw_type(self):
        return self.raw_type_

    def get_type_model(self):
        return self.raw_type_.get_raw_type_model()


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
$methodInCode
    RefPtr<InspectorObject> result = InspectorObject::create();
    ErrorString error;
    if (!protocolErrors->length()) {
        $agentField->$methodName(&error$agentCallParams);

${responseCook}
    }
    sendResponse(callId, result, commandNames[$commandNameIndex], protocolErrors, error);
}
""")

    frontend_method = string.Template("""void InspectorFrontend::$domainName::$eventName($parameters)
{
    RefPtr<InspectorObject> jsonMessage = InspectorObject::create();
    jsonMessage->setString("method", "$domainName.$eventName");
$code    if (m_inspectorFrontendChannel)
        m_inspectorFrontendChannel->sendMessageToFrontend(jsonMessage->toJSONString());
}
""")

    callback_method = string.Template(
"""InspectorBackendDispatcher::$agentName::$callbackName::$callbackName(PassRefPtr<InspectorBackendDispatcherImpl> backendImpl, int id) : CallbackBase(backendImpl, id) {}

void InspectorBackendDispatcher::$agentName::$callbackName::sendSuccess($parameters)
{
    RefPtr<InspectorObject> jsonMessage = InspectorObject::create();
$code    sendIfActive(jsonMessage, ErrorString());
}
""")

    frontend_h = string.Template(file_header_ +
"""#ifndef InspectorFrontend_h
#define InspectorFrontend_h

#include "InspectorTypeBuilder.h"
#include "InspectorValues.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

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

class InspectorBackendDispatcherImpl;

class InspectorBackendDispatcher: public RefCounted<InspectorBackendDispatcher> {
public:
    static PassRefPtr<InspectorBackendDispatcher> create(InspectorFrontendChannel* inspectorFrontendChannel);
    virtual ~InspectorBackendDispatcher() { }

    class CallbackBase: public RefCounted<CallbackBase> {
    public:
        CallbackBase(PassRefPtr<InspectorBackendDispatcherImpl> backendImpl, int id);
        virtual ~CallbackBase();
        void sendFailure(const ErrorString&);
        bool isActive();

    protected:
        void sendIfActive(PassRefPtr<InspectorObject> partialMessage, const ErrorString& invocationError);

    private:
        void disable() { m_alreadySent = true; }

        RefPtr<InspectorBackendDispatcherImpl> m_backendImpl;
        int m_id;
        bool m_alreadySent;

        friend class InspectorBackendDispatcherImpl;
    };

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

        kMethodNamesEnumSize
    };

    static const char* commandNames[];
};

} // namespace WebCore
#endif // !defined(InspectorBackendDispatcher_h)


""")

    backend_cpp = string.Template(file_header_ +
"""

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorBackendDispatcher.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

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

    void sendResponse(long callId, PassRefPtr<InspectorObject> result, const ErrorString& invocationError);
    bool isActive() { return m_inspectorFrontendChannel; }

$setters
private:
$methodDeclarations

    InspectorFrontendChannel* m_inspectorFrontendChannel;
$fieldDeclarations

    template<typename R, typename V, typename V0>
    static R getPropertyValueImpl(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors, V0 initial_value, bool (*as_method)(InspectorValue*, V*), const char* type_name);

    static int getInt(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static double getDouble(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static String getString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static bool getBoolean(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static PassRefPtr<InspectorObject> getObject(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static PassRefPtr<InspectorArray> getArray(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);

    void sendResponse(long callId, PassRefPtr<InspectorObject> result, const char* commandName, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError);

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
        size_t length = WTF_ARRAY_LENGTH(commandNames);
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

void InspectorBackendDispatcherImpl::sendResponse(long callId, PassRefPtr<InspectorObject> result, const char* commandName, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError)
{
    if (protocolErrors->length()) {
        String errorMessage = String::format("Some arguments of method '%s' can't be processed", commandName);
        reportProtocolError(&callId, InvalidParams, errorMessage, protocolErrors);
        return;
    }
    sendResponse(callId, result, invocationError);
}

void InspectorBackendDispatcherImpl::sendResponse(long callId, PassRefPtr<InspectorObject> result, const ErrorString& invocationError)
{
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

template<typename R, typename V, typename V0>
R InspectorBackendDispatcherImpl::getPropertyValueImpl(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors, V0 initial_value, bool (*as_method)(InspectorValue*, V*), const char* type_name)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    V value = initial_value;

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '%s' with type '%s'.", name.utf8().data(), type_name));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '%s' with type '%s' was not found.", name.utf8().data(), type_name));
        return value;
    }

    if (!as_method(valueIterator->second.get(), &value))
        protocolErrors->pushString(String::format("Parameter '%s' has wrong type. It must be '%s'.", name.utf8().data(), type_name));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}

struct AsMethodBridges {
    static bool asInt(InspectorValue* value, int* output) { return value->asNumber(output); }
    static bool asDouble(InspectorValue* value, double* output) { return value->asNumber(output); }
    static bool asString(InspectorValue* value, String* output) { return value->asString(output); }
    static bool asBoolean(InspectorValue* value, bool* output) { return value->asBoolean(output); }
    static bool asObject(InspectorValue* value, RefPtr<InspectorObject>* output) { return value->asObject(output); }
    static bool asArray(InspectorValue* value, RefPtr<InspectorArray>* output) { return value->asArray(output); }
};

int InspectorBackendDispatcherImpl::getInt(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<int, int, int>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asInt, "Number");
}

double InspectorBackendDispatcherImpl::getDouble(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<double, double, double>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asDouble, "Number");
}

String InspectorBackendDispatcherImpl::getString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<String, String, String>(object, name, valueFound, protocolErrors, "", AsMethodBridges::asString, "String");
}

bool InspectorBackendDispatcherImpl::getBoolean(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<bool, bool, bool>(object, name, valueFound, protocolErrors, false, AsMethodBridges::asBoolean, "Boolean");
}

PassRefPtr<InspectorObject> InspectorBackendDispatcherImpl::getObject(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<PassRefPtr<InspectorObject>, RefPtr<InspectorObject>, InspectorObject*>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asObject, "Object");
}

PassRefPtr<InspectorArray> InspectorBackendDispatcherImpl::getArray(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<PassRefPtr<InspectorArray>, RefPtr<InspectorArray>, InspectorArray*>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asArray, "Array");
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

InspectorBackendDispatcher::CallbackBase::CallbackBase(PassRefPtr<InspectorBackendDispatcherImpl> backendImpl, int id)
    : m_backendImpl(backendImpl), m_id(id), m_alreadySent(false) {}

InspectorBackendDispatcher::CallbackBase::~CallbackBase() {}

void InspectorBackendDispatcher::CallbackBase::sendFailure(const ErrorString& error)
{
    ASSERT(error.length());
    sendIfActive(0, error);
}

bool InspectorBackendDispatcher::CallbackBase::isActive()
{
    return !m_alreadySent && m_backendImpl->isActive();
}

void InspectorBackendDispatcher::CallbackBase::sendIfActive(PassRefPtr<InspectorObject> partialMessage, const ErrorString& invocationError)
{
    if (m_alreadySent)
        return;
    m_backendImpl->sendResponse(m_id, partialMessage, invocationError);
    m_alreadySent = true;
}

COMPILE_ASSERT(static_cast<int>(InspectorBackendDispatcher::kMethodNamesEnumSize) == WTF_ARRAY_LENGTH(InspectorBackendDispatcher::commandNames), command_name_array_problem);

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
    : $constructorInit{
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
#include <wtf/Assertions.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

namespace TypeBuilder {

template<typename T>
class OptOutput {
public:
    OptOutput() : m_assigned(false) { }

    void operator=(T value)
    {
        m_value = value;
        m_assigned = true;
    }

    bool isAssigned() { return m_assigned; }

    T getValue()
    {
        ASSERT(isAssigned());
        return m_value;
    }

private:
    T m_value;
    bool m_assigned;

    WTF_MAKE_NONCOPYABLE(OptOutput);
};


// A small transient wrapper around int type, that can be used as a funciton parameter type
// cleverly disallowing C++ implicit casts from float or double.
class ExactlyInt {
public:
    template<typename T>
    ExactlyInt(T t) : m_value(cast_to_int<T>(t)) {}

    ExactlyInt() {}

    operator int() { return m_value; }
private:
    int m_value;

    template<typename T>
    static int cast_to_int(T) { return T::default_case_cast_is_not_supported(); }
};

template<>
inline int ExactlyInt::cast_to_int<int>(int i) { return i; }

template<>
inline int ExactlyInt::cast_to_int<unsigned int>(unsigned int i) { return i; }

class RuntimeCastHelper {
public:
#if !ASSERT_DISABLED
    template<InspectorValue::Type TYPE>
    static void assertType(InspectorValue* value)
    {
        ASSERT(value->type() == TYPE);
    }
    static void assertAny(InspectorValue*);
    static void assertInt(InspectorValue* value);
#endif
};


// This class provides "Traits" type for the input type T. It is programmed using C++ template specialization
// technique. By default it simply takes "ItemTraits" type from T, but it doesn't work with the base types.
template<typename T>
struct ArrayItemHelper {
    typedef typename T::ItemTraits Traits;
};

template<typename T>
class Array : public InspectorArrayBase {
private:
    Array() { }

    InspectorArray* openAccessors() {
        COMPILE_ASSERT(sizeof(InspectorArray) == sizeof(Array<T>), cannot_cast);
        return static_cast<InspectorArray*>(static_cast<InspectorArrayBase*>(this));
    }

public:
    void addItem(PassRefPtr<T> value)
    {
        ArrayItemHelper<T>::Traits::pushRefPtr(this->openAccessors(), value);
    }

    void addItem(T value)
    {
        ArrayItemHelper<T>::Traits::pushRaw(this->openAccessors(), value);
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
        return static_cast<Array<T>*>(static_cast<InspectorArrayBase*>(array.get()));
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
    static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorValue> value)
    {
        array->pushValue(value);
    }

#if """ + VALIDATOR_IFDEF_NAME + """
    template<typename T>
    static void assertCorrectValue(InspectorValue* value) {
        T::assertCorrectValue(value);
    }
#endif  // !ASSERT_DISABLED
};

template<>
struct ArrayItemHelper<String> {
    struct Traits {
        static void pushRaw(InspectorArray* array, const String& value)
        {
            array->pushString(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeString>(value);
        }
#endif  // !ASSERT_DISABLED
    };
};

template<>
struct ArrayItemHelper<int> {
    struct Traits {
        static void pushRaw(InspectorArray* array, int value)
        {
            array->pushInt(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertInt(value);
        }
#endif  // !ASSERT_DISABLED
    };
};

template<>
struct ArrayItemHelper<double> {
    struct Traits {
        static void pushRaw(InspectorArray* array, double value)
        {
            array->pushNumber(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeNumber>(value);
        }
#endif  // !ASSERT_DISABLED
    };
};

template<>
struct ArrayItemHelper<bool> {
    struct Traits {
        static void pushRaw(InspectorArray* array, bool value)
        {
            array->pushBoolean(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeBoolean>(value);
        }
#endif  // !ASSERT_DISABLED
    };
};

template<>
struct ArrayItemHelper<InspectorValue> {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorValue> value)
        {
            array->pushValue(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertAny(value);
        }
#endif  // !ASSERT_DISABLED
    };
};

template<>
struct ArrayItemHelper<InspectorObject> {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorValue> value)
        {
            array->pushValue(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeObject>(value);
        }
#endif  // !ASSERT_DISABLED
    };
};

template<>
struct ArrayItemHelper<InspectorArray> {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorArray> value)
        {
            array->pushArray(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeArray>(value);
        }
#endif  // !ASSERT_DISABLED
    };
};

template<typename T>
struct ArrayItemHelper<TypeBuilder::Array<T> > {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<TypeBuilder::Array<T> > value)
        {
            array->pushValue(value);
        }

#if """ + VALIDATOR_IFDEF_NAME + """
        template<typename S>
        static void assertCorrectValue(InspectorValue* value) {
            S::assertCorrectValue(value);
        }
#endif  // !ASSERT_DISABLED
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

$implCode

#if """ + VALIDATOR_IFDEF_NAME + """

void TypeBuilder::RuntimeCastHelper::assertAny(InspectorValue*)
{
    // No-op.
}


void TypeBuilder::RuntimeCastHelper::assertInt(InspectorValue* value)
{
    double v;
    bool castRes = value->asNumber(&v);
    ASSERT_UNUSED(castRes, castRes);
    ASSERT(static_cast<double>(static_cast<int>(v)) == v);
}

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


def format_setter_value_expression(param_type_binding, value_ref):
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
    type_builder_impl_list = []


    @staticmethod
    def go():
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
            Generator.method_name_enum_list,
            Generator.backend_constructor_init_list,
            Generator.backend_virtual_setters_list,
            Generator.backend_setters_list,
            Generator.backend_field_list]

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
            if Generator.frontend_constructor_init_list:
                Generator.frontend_constructor_init_list.append("    , ")
            Generator.frontend_constructor_init_list.append("m_%s(inspectorFrontendChannel)\n" % domain_name_lower)
            Generator.frontend_domain_class_lines.append(Templates.frontend_domain_class.substitute(None,
                domainClassName=domain_name,
                domainFieldName=domain_name_lower,
                frontendDomainMethodDeclarations="".join(flatten_list(frontend_method_declaration_lines))))

            agent_interface_name = Capitalizer.lower_camel_case_to_upper(domain_name) + "CommandHandler"
            Generator.backend_agent_interface_list.append("    class %s {\n" % agent_interface_name)
            Generator.backend_agent_interface_list.append("    public:\n")
            if "commands" in json_domain:
                for json_command in json_domain["commands"]:
                    Generator.process_command(json_command, domain_name, agent_field_name, agent_interface_name)
            Generator.backend_agent_interface_list.append("\n    protected:\n")
            Generator.backend_agent_interface_list.append("        virtual ~%s() { }\n" % agent_interface_name)
            Generator.backend_agent_interface_list.append("    };\n\n")

            Generator.backend_constructor_init_list.append("        , m_%s(0)" % agent_field_name)
            Generator.backend_virtual_setters_list.append("    virtual void registerAgent(%s* %s) = 0;" % (agent_interface_name, agent_field_name))
            Generator.backend_setters_list.append("    virtual void registerAgent(%s* %s) { ASSERT(!m_%s); m_%s = %s; }" % (agent_interface_name, agent_field_name, agent_field_name, agent_field_name, agent_field_name))
            Generator.backend_field_list.append("    %s* m_%s;" % (agent_interface_name, agent_field_name))

            if domain_guard:
                for l in reversed(first_cycle_guardable_list_list):
                    domain_guard.generate_close(l)
            Generator.backend_js_domain_initializer_list.append("\n")

    @staticmethod
    def process_event(json_event, domain_name, frontend_method_declaration_lines):
        event_name = json_event["name"]

        ad_hoc_type_output = []
        frontend_method_declaration_lines.append(ad_hoc_type_output)
        ad_hoc_type_writer = Writer(ad_hoc_type_output, "        ")

        decl_parameter_list = []

        json_parameters = json_event.get("parameters")
        Generator.generate_send_method(json_parameters, event_name, domain_name, ad_hoc_type_writer,
                                       decl_parameter_list,
                                       Generator.EventMethodStructTemplate,
                                       Generator.frontend_method_list, Templates.frontend_method, {"eventName": event_name})

        backend_js_event_param_list = []
        if json_parameters:
            for parameter in json_parameters:
                parameter_name = parameter["name"]
                backend_js_event_param_list.append("\"%s\"" % parameter_name)

        frontend_method_declaration_lines.append(
            "        void %s(%s);\n" % (event_name, ", ".join(decl_parameter_list)))

        Generator.backend_js_domain_initializer_list.append("InspectorBackend.registerEvent(\"%s.%s\", [%s]);\n" % (
            domain_name, event_name, ", ".join(backend_js_event_param_list)))

    class EventMethodStructTemplate:
        @staticmethod
        def append_prolog(line_list):
            line_list.append("    RefPtr<InspectorObject> paramsObject = InspectorObject::create();\n")

        @staticmethod
        def append_epilog(line_list):
            line_list.append("    jsonMessage->setObject(\"params\", paramsObject);\n")

        container_name = "paramsObject"

    @staticmethod
    def process_command(json_command, domain_name, agent_field_name, agent_interface_name):
        json_command_name = json_command["name"]

        cmd_enum_name = "k%s_%sCmd" % (domain_name, json_command["name"])

        Generator.method_name_enum_list.append("        %s," % cmd_enum_name)
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

                getter_name = param_raw_type.get_getter_name()

                optional = json_parameter.get("optional")

                non_optional_type_model = param_raw_type.get_raw_type_model()
                if optional:
                    type_model = non_optional_type_model.get_optional()
                else:
                    type_model = non_optional_type_model

                if optional:
                    code = ("    bool %s_valueFound = false;\n"
                            "    %s in_%s = get%s(paramsContainerPtr, \"%s\", &%s_valueFound, protocolErrorsPtr);\n" %
                           (json_param_name, non_optional_type_model.get_command_return_pass_model().get_return_var_type(), json_param_name, getter_name, json_param_name, json_param_name))
                    param = ", %s_valueFound ? &in_%s : 0" % (json_param_name, json_param_name)
                    # FIXME: pass optional refptr-values as PassRefPtr
                    formal_param_type_pattern = "const %s*"
                else:
                    code = ("    %s in_%s = get%s(paramsContainerPtr, \"%s\", 0, protocolErrorsPtr);\n" %
                            (non_optional_type_model.get_command_return_pass_model().get_return_var_type(), json_param_name, getter_name, json_param_name))
                    param = ", in_%s" % json_param_name
                    # FIXME: pass not-optional refptr-values as NonNullPassRefPtr
                    if param_raw_type.is_heavy_value():
                        formal_param_type_pattern = "const %s&"
                    else:
                        formal_param_type_pattern = "%s"

                method_in_code += code
                agent_call_param_list.append(param)
                Generator.backend_agent_interface_list.append(", %s in_%s" % (formal_param_type_pattern % non_optional_type_model.get_command_return_pass_model().get_return_var_type(), json_param_name))

                js_bind_type = param_raw_type.get_js_bind_type()
                js_param_text = "{\"name\": \"%s\", \"type\": \"%s\", \"optional\": %s}" % (
                    json_param_name,
                    js_bind_type,
                    ("true" if ("optional" in json_parameter and json_parameter["optional"]) else "false"))

                js_param_list.append(js_param_text)

            js_parameters_text = ", ".join(js_param_list)

        response_cook_text = ""
        if json_command.get("async") == True:
            callback_name = Capitalizer.lower_camel_case_to_upper(json_command_name) + "Callback"

            callback_output = []
            callback_writer = Writer(callback_output, ad_hoc_type_writer.get_indent())

            decl_parameter_list = []
            Generator.generate_send_method(json_command.get("returns"), json_command_name, domain_name, ad_hoc_type_writer,
                                           decl_parameter_list,
                                           Generator.CallbackMethodStructTemplate,
                                           Generator.backend_method_implementation_list, Templates.callback_method,
                                           {"callbackName": callback_name, "agentName": agent_interface_name})

            callback_writer.newline("class " + callback_name + " : public CallbackBase {\n")
            callback_writer.newline("public:\n")
            callback_writer.newline("    " + callback_name + "(PassRefPtr<InspectorBackendDispatcherImpl>, int id);\n")
            callback_writer.newline("    void sendSuccess(" + ", ".join(decl_parameter_list) + ");\n")
            callback_writer.newline("};\n")

            ad_hoc_type_output.append(callback_output)

            method_out_code += "    RefPtr<" + agent_interface_name + "::" + callback_name + "> callback = adoptRef(new " + agent_interface_name + "::" + callback_name + "(this, callId));\n"
            agent_call_param_list.append(", callback")
            response_cook_text += "        if (!error.length()) \n"
            response_cook_text += "            return;\n"
            response_cook_text += "        callback->disable();\n"
            Generator.backend_agent_interface_list.append(", PassRefPtr<%s> callback" % callback_name)
        else:
            if "returns" in json_command:
                method_out_code += "\n"
                for json_return in json_command["returns"]:

                    json_return_name = json_return["name"]

                    optional = bool(json_return.get("optional"))

                    return_type_binding = Generator.resolve_type_and_generate_ad_hoc(json_return, json_command_name, domain_name, ad_hoc_type_writer, agent_interface_name + "::")

                    raw_type = return_type_binding.reduce_to_raw_type()
                    setter_type = raw_type.get_setter_name()
                    initializer = raw_type.get_c_initializer()

                    type_model = return_type_binding.get_type_model()
                    if optional:
                        type_model = type_model.get_optional()

                    code = "    %s out_%s;\n" % (type_model.get_command_return_pass_model().get_return_var_type(), json_return_name)
                    param = ", %sout_%s" % (type_model.get_command_return_pass_model().get_output_argument_prefix(), json_return_name)
                    var_name = "out_%s" % json_return_name
                    setter_argument = type_model.get_command_return_pass_model().get_output_to_raw_expression() % var_name
                    if return_type_binding.get_setter_value_expression_pattern():
                        setter_argument = return_type_binding.get_setter_value_expression_pattern() % setter_argument

                    cook = "            result->set%s(\"%s\", %s);\n" % (setter_type, json_return_name,
                                                                         setter_argument)

                    set_condition_pattern = type_model.get_command_return_pass_model().get_set_return_condition()
                    if set_condition_pattern:
                        cook = ("            if (%s)\n    " % (set_condition_pattern % var_name)) + cook
                    annotated_type = type_model.get_command_return_pass_model().get_output_parameter_type()

                    param_name = "out_%s" % json_return_name
                    if optional:
                        param_name = "opt_" + param_name

                    Generator.backend_agent_interface_list.append(", %s %s" % (annotated_type, param_name))
                    response_cook_list.append(cook)

                    method_out_code += code
                    agent_call_param_list.append(param)

                response_cook_text = "".join(response_cook_list)

                if len(response_cook_text) != 0:
                    response_cook_text = "        if (!error.length()) {\n" + response_cook_text + "        }"

        backend_js_reply_param_list = []
        if "returns" in json_command:
            for json_return in json_command["returns"]:
                json_return_name = json_return["name"]
                backend_js_reply_param_list.append("\"%s\"" % json_return_name)

        js_reply_list = "[%s]" % ", ".join(backend_js_reply_param_list)

        Generator.backend_method_implementation_list.append(Templates.backend_method.substitute(None,
            domainName=domain_name, methodName=json_command_name,
            agentField="m_" + agent_field_name,
            methodInCode=method_in_code,
            methodOutCode=method_out_code,
            agentCallParams="".join(agent_call_param_list),
            requestMessageObject=request_message_param,
            responseCook=response_cook_text,
            commandNameIndex=cmd_enum_name))
        Generator.backend_method_name_declaration_list.append("    \"%s.%s\"," % (domain_name, json_command_name))

        Generator.backend_js_domain_initializer_list.append("InspectorBackend.registerCommand(\"%s.%s\", [%s], %s);\n" % (domain_name, json_command_name, js_parameters_text, js_reply_list))
        Generator.backend_agent_interface_list.append(") = 0;\n")

    class CallbackMethodStructTemplate:
        @staticmethod
        def append_prolog(line_list):
            pass

        @staticmethod
        def append_epilog(line_list):
            pass

        container_name = "jsonMessage"

    # Generates common code for event sending and callback response data sending.
    @staticmethod
    def generate_send_method(parameters, event_name, domain_name, ad_hoc_type_writer, decl_parameter_list,
                             method_struct_template,
                             generator_method_list, method_template, template_params):
        method_line_list = []
        if parameters:
            method_struct_template.append_prolog(method_line_list)
            for json_parameter in parameters:
                parameter_name = json_parameter["name"]

                param_type_binding = Generator.resolve_type_and_generate_ad_hoc(json_parameter, event_name, domain_name, ad_hoc_type_writer, "")

                raw_type = param_type_binding.reduce_to_raw_type()
                raw_type_binding = RawTypeBinding(raw_type)

                optional = bool(json_parameter.get("optional"))

                setter_type = raw_type.get_setter_name()

                type_model = param_type_binding.get_type_model()
                raw_type_model = raw_type_binding.get_type_model()
                if optional:
                    type_model = type_model.get_optional()
                    raw_type_model = raw_type_model.get_optional()

                annotated_type = type_model.get_input_param_type_text()
                mode_type_binding = param_type_binding

                decl_parameter_list.append("%s %s" % (annotated_type, parameter_name))

                setter_argument = raw_type_model.get_event_setter_expression_pattern() % parameter_name
                if mode_type_binding.get_setter_value_expression_pattern():
                    setter_argument = mode_type_binding.get_setter_value_expression_pattern() % setter_argument

                setter_code = "    %s->set%s(\"%s\", %s);\n" % (method_struct_template.container_name, setter_type, parameter_name, setter_argument)
                if optional:
                    setter_code = ("    if (%s)\n    " % parameter_name) + setter_code
                method_line_list.append(setter_code)

            method_struct_template.append_epilog(method_line_list)

        generator_method_list.append(method_template.substitute(None,
            domainName=domain_name,
            parameters=", ".join(decl_parameter_list),
            code="".join(method_line_list), **template_params))

    @staticmethod
    def resolve_type_and_generate_ad_hoc(json_param, method_name, domain_name, ad_hoc_type_writer, container_relative_name_prefix_param):
        param_name = json_param["name"]
        ad_hoc_type_list = []

        class AdHocTypeContext:
            container_full_name_prefix = "<not yet defined>"
            container_relative_name_prefix = container_relative_name_prefix_param

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
            cpp_writer = validator_writer

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
            cpp_writer = Writer(Generator.type_builder_impl_list, "")

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


# A writer that only updates file if it actually changed to better support incremental build.
class SmartOutput:
    def __init__(self, file_name):
        self.file_name_ = file_name
        self.output_ = ""

    def write(self, text):
        self.output_ += text

    def close(self):
        text_changed = True

        try:
            read_file = open(self.file_name_, "r")
            old_text = read_file.read()
            read_file.close()
            text_changed = old_text != self.output_
        except:
            # Ignore, just overwrite by default
            pass

        if text_changed:
            out_file = open(self.file_name_, "w")
            out_file.write(self.output_)
            out_file.close()


Generator.go()

backend_h_file = SmartOutput(output_header_dirname + "/InspectorBackendDispatcher.h")
backend_cpp_file = SmartOutput(output_cpp_dirname + "/InspectorBackendDispatcher.cpp")

frontend_h_file = SmartOutput(output_header_dirname + "/InspectorFrontend.h")
frontend_cpp_file = SmartOutput(output_cpp_dirname + "/InspectorFrontend.cpp")

typebuilder_h_file = SmartOutput(output_header_dirname + "/InspectorTypeBuilder.h")
typebuilder_cpp_file = SmartOutput(output_cpp_dirname + "/InspectorTypeBuilder.cpp")

backend_js_file = SmartOutput(output_cpp_dirname + "/InspectorBackendCommands.js")


backend_h_file.write(Templates.backend_h.substitute(None,
    virtualSetters="\n".join(Generator.backend_virtual_setters_list),
    agentInterfaces="".join(flatten_list(Generator.backend_agent_interface_list)),
    methodNamesEnumContent="\n".join(Generator.method_name_enum_list)))

backend_cpp_file.write(Templates.backend_cpp.substitute(None,
    constructorInit="\n".join(Generator.backend_constructor_init_list),
    setters="\n".join(Generator.backend_setters_list),
    fieldDeclarations="\n".join(Generator.backend_field_list),
    methodNameDeclarations="\n".join(Generator.backend_method_name_declaration_list),
    methods="\n".join(Generator.backend_method_implementation_list),
    methodDeclarations="\n".join(Generator.backend_method_declaration_list),
    messageHandlers="\n".join(Generator.method_handler_list)))

frontend_h_file.write(Templates.frontend_h.substitute(None,
    fieldDeclarations="".join(Generator.frontend_class_field_lines),
    domainClassList="".join(Generator.frontend_domain_class_lines)))

frontend_cpp_file.write(Templates.frontend_cpp.substitute(None,
    constructorInit="".join(Generator.frontend_constructor_init_list),
    methods="\n".join(Generator.frontend_method_list)))

typebuilder_h_file.write(Templates.typebuilder_h.substitute(None,
    typeBuilders="".join(flatten_list(Generator.type_builder_fragments)),
    forwards="".join(Generator.type_builder_forwards)))

typebuilder_cpp_file.write(Templates.typebuilder_cpp.substitute(None,
    enumConstantValues=EnumConstants.get_enum_constant_code(),
    implCode="".join(flatten_list(Generator.type_builder_impl_list)),
    validatorCode="".join(flatten_list(Generator.validator_impl_list))))

backend_js_file.write(Templates.backend_js.substitute(None,
    domainInitializers="".join(Generator.backend_js_domain_initializer_list)))

backend_h_file.close()
backend_cpp_file.close()

frontend_h_file.close()
frontend_cpp_file.close()

typebuilder_h_file.close()
typebuilder_cpp_file.close()

backend_js_file.close()
