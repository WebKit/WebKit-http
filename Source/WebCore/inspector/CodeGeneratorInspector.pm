# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

package CodeGeneratorInspector;

use strict;

use Class::Struct;
use File::stat;

my %typeTransform;
$typeTransform{"ApplicationCache"} = {
    "forward" => "InspectorApplicationCacheAgent",
    "header" => "InspectorApplicationCacheAgent.h",
    "domainAccessor" => "m_applicationCacheAgent",
};
$typeTransform{"CSS"} = {
    "forward" => "InspectorCSSAgent",
    "header" => "InspectorCSSAgent.h",
    "domainAccessor" => "m_cssAgent",
};
$typeTransform{"Console"} = {
    "forward" => "InspectorConsoleAgent",
    "header" => "InspectorConsoleAgent.h",
    "domainAccessor" => "m_consoleAgent",
};
$typeTransform{"Page"} = {
    "forward" => "InspectorPageAgent",
    "header" => "InspectorPageAgent.h",
    "domainAccessor" => "m_pageAgent",
};
$typeTransform{"Debugger"} = {
    "forward" => "InspectorDebuggerAgent",
    "header" => "InspectorDebuggerAgent.h",
    "domainAccessor" => "m_debuggerAgent",
};
$typeTransform{"DOMDebugger"} = {
    "forward" => "InspectorDOMDebuggerAgent",
    "header" => "InspectorDOMDebuggerAgent.h",
    "domainAccessor" => "m_domDebuggerAgent",
};
$typeTransform{"Database"} = {
    "forward" => "InspectorDatabaseAgent",
    "header" => "InspectorDatabaseAgent.h",
    "domainAccessor" => "m_databaseAgent",
};
$typeTransform{"DOM"} = {
    "forward" => "InspectorDOMAgent",
    "header" => "InspectorDOMAgent.h",
    "domainAccessor" => "m_domAgent",
};
$typeTransform{"DOMStorage"} = {
    "forward" => "InspectorDOMStorageAgent",
    "header" => "InspectorDOMStorageAgent.h",
    "domainAccessor" => "m_domStorageAgent",
};
$typeTransform{"FileSystem"} = {
    "forward" => "InspectorFileSystemAgent",
    "header" => "InspectorFileSystemAgent.h",
    "domainAccessor" => "m_fileSystemAgent",
};
$typeTransform{"Inspector"} = {
    "forward" => "InspectorAgent",
    "header" => "InspectorAgent.h",
    "domainAccessor" => "m_inspectorAgent",
};
$typeTransform{"Network"} = {
    "forward" => "InspectorResourceAgent",
    "header" => "InspectorResourceAgent.h",
    "domainAccessor" => "m_resourceAgent",
};
$typeTransform{"Profiler"} = {
    "forward" => "InspectorProfilerAgent",
    "header" => "InspectorProfilerAgent.h",
    "domainAccessor" => "m_profilerAgent",
};
$typeTransform{"Runtime"} = {
    "forward" => "InspectorRuntimeAgent",
    "header" => "InspectorRuntimeAgent.h",
    "domainAccessor" => "m_runtimeAgent",
};
$typeTransform{"Timeline"} = {
    "forward" => "InspectorTimelineAgent",
    "header" => "InspectorTimelineAgent.h",
    "domainAccessor" => "m_timelineAgent",
};
$typeTransform{"Worker"} = {
    "forward" => "InspectorWorkerAgent",
    "header" => "InspectorWorkerAgent.h",
    "domainAccessor" => "m_workerAgent",
};

$typeTransform{"Frontend"} = {
    "forward" => "InspectorFrontend",
    "header" => "InspectorFrontend.h",
};
$typeTransform{"PassRefPtr"} = {
    "forwardHeader" => "wtf/PassRefPtr.h",
};
$typeTransform{"RefCounted"} = {
    "forwardHeader" => "wtf/RefCounted.h",
};
$typeTransform{"InspectorFrontendChannel"} = {
    "forward" => "InspectorFrontendChannel",
    "header" => "InspectorFrontendChannel.h",
};
$typeTransform{"Object"} = {
    "param" => "PassRefPtr<InspectorObject>",
    "variable" => "RefPtr<InspectorObject>",
    "defaultValue" => "InspectorObject::create()",
    "forward" => "InspectorObject",
    "header" => "InspectorValues.h",
    "JSONType" => "Object",
    "JSType" => "object",
};
$typeTransform{"Array"} = {
    "param" => "PassRefPtr<InspectorArray>",
    "variable" => "RefPtr<InspectorArray>",
    "defaultValue" => "InspectorArray::create()",
    "forward" => "InspectorArray",
    "header" => "InspectorValues.h",
    "JSONType" => "Array",
    "JSType" => "object",
};
$typeTransform{"Value"} = {
    "param" => "PassRefPtr<InspectorValue>",
    "variable" => "RefPtr<InspectorValue>",
    "defaultValue" => "InspectorValue::null()",
    "forward" => "InspectorValue",
    "header" => "InspectorValues.h",
    "JSONType" => "Value",
    "JSType" => "",
};
$typeTransform{"String"} = {
    "param" => "const String&",
    "optional_param" => "const String* const",
    "optional_valueAccessor" => "*",
    "variable" => "String",
    "return" => "String",
    "defaultValue" => "\"\"",
    "forwardHeader" => "PlatformString.h",
    "header" => "PlatformString.h",
    "JSONType" => "String",
    "JSType" => "string"
};
$typeTransform{"long"} = {
    "param" => "long",
    "optional_param" => "const long* const",
    "optional_valueAccessor" => "*",
    "variable" => "long",
    "defaultValue" => "0",
    "forward" => "",
    "header" => "",
    "JSONType" => "Number",
    "JSType" => "number"
};
$typeTransform{"int"} = {
    "param" => "int",
    "optional_param" => "const int* const",
    "optional_valueAccessor" => "*",
    "variable" => "int",
    "defaultValue" => "0",
    "forward" => "",
    "header" => "",
    "JSONType" => "Number",
    "JSType" => "number"
};
$typeTransform{"unsigned long"} = {
    "param" => "unsigned long",
    "optional_param" => "const unsigned long* const",
    "optional_valueAccessor" => "*",
    "variable" => "unsigned long",
    "defaultValue" => "0u",
    "forward" => "",
    "header" => "",
    "JSONType" => "Number",
    "JSType" => "number"
};
$typeTransform{"unsigned int"} = {
    "param" => "unsigned int",
    "optional_param" => "const unsigned int* const",
    "optional_valueAccessor" => "*",
    "variable" => "unsigned int",
    "defaultValue" => "0u",
    "forward" => "",
    "header" => "",
    "JSONType" => "Number",
    "JSType" => "number"
};
$typeTransform{"double"} = {
    "param" => "double",
    "optional_param" => "const double* const",
    "optional_valueAccessor" => "*",
    "variable" => "double",
    "defaultValue" => "0.0",
    "forward" => "",
    "header" => "",
    "JSONType" => "Number",
    "JSType" => "number"
};
$typeTransform{"boolean"} = {
    "param" => "bool",
    "optional_param" => "const bool* const",
    "optional_valueAccessor" => "*",
    "variable"=> "bool",
    "defaultValue" => "false",
    "forward" => "",
    "header" => "",
    "JSONType" => "Boolean",
    "JSType" => "boolean"
};
$typeTransform{"void"} = {
    "forward" => "",
    "header" => ""
};
$typeTransform{"Vector"} = {
    "header" => "wtf/Vector.h"
};

# Default License Templates

my $licenseTemplate = << "EOF";
// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
EOF

my $codeGenerator;
my $outputDir;
my $outputHeadersDir;
my $writeDependencies;
my $verbose;

my $namespace;

my $backendClassName;
my $backendClassDeclaration;
my $backendJSStubName;
my %backendTypes;
my @backendMethods;
my @backendMethodsImpl;
my %backendMethodSignatures;
my $backendConstructor;
my @backendConstantDeclarations;
my @backendConstantDefinitions;
my @backendFooter;
my @backendJSStubs;
my @backendJSEvents;
my %backendDomains;

my $frontendClassName;
my %frontendTypes;
my @frontendMethods;
my @frontendAgentFields;
my @frontendMethodsImpl;
my %frontendMethodSignatures;
my $frontendConstructor;
my @frontendConstantDeclarations;
my @frontendConstantDefinitions;
my @frontendFooter;
my @frontendDomains;

# Default constructor
sub new
{
    my $object = shift;
    my $reference = { };

    $codeGenerator = shift;
    $outputDir = shift;
    $outputHeadersDir = shift;
    shift; # $useLayerOnTop
    shift; # $preprocessor
    $writeDependencies = shift;
    $verbose = shift;

    bless($reference, $object);
    return $reference;
}

# Params: 'idlDocument' struct
sub GenerateModule
{
    my $object = shift;
    my $dataNode = shift;

    $namespace = $dataNode->module;
    $namespace =~ s/core/WebCore/;

    $frontendClassName = "InspectorFrontend";
    $frontendConstructor = "    ${frontendClassName}(InspectorFrontendChannel*);";
    push(@frontendFooter, "private:");
    push(@frontendFooter, "    InspectorFrontendChannel* m_inspectorFrontendChannel;");
    $frontendTypes{"String"} = 1;
    $frontendTypes{"InspectorFrontendChannel"} = 1;
    $frontendTypes{"PassRefPtr"} = 1;

    $backendClassName = "InspectorBackendDispatcher";
    $backendClassDeclaration = "InspectorBackendDispatcher: public RefCounted<InspectorBackendDispatcher>";
    $backendJSStubName = "InspectorBackendStub";
    $backendTypes{"Inspector"} = 1;
    $backendTypes{"InspectorFrontendChannel"} = 1;
    $backendTypes{"PassRefPtr"} = 1;
    $backendTypes{"RefCounted"} = 1;
    $backendTypes{"Object"} = 1;
}

# Params: 'idlDocument' struct
sub GenerateInterface
{
    my $object = shift;
    my $interface = shift;
    my $defines = shift;

    my %agent = (
        methodDeclarations => [],
        methodSignatures => {}
    );
    generateFunctions($interface, \%agent);
    if (@{$agent{methodDeclarations}}) {
        push(@frontendDomains, $interface->name);
        generateAgentDeclaration($interface, \%agent);
    }
}

sub generateAgentDeclaration
{
    my $interface = shift;
    my $agent = shift;
    my $agentName = $interface->name;
    push(@frontendMethods, "    class ${agentName} {");
    push(@frontendMethods, "    public:");
    push(@frontendMethods, "        ${agentName}(InspectorFrontendChannel* inspectorFrontendChannel) : m_inspectorFrontendChannel(inspectorFrontendChannel) { }");
    push(@frontendMethods, @{$agent->{methodDeclarations}});
    push(@frontendMethods, "        void setInspectorFrontendChannel(InspectorFrontendChannel* inspectorFrontendChannel) { m_inspectorFrontendChannel = inspectorFrontendChannel; }");
    push(@frontendMethods, "        InspectorFrontendChannel* getInspectorFrontendChannel() { return m_inspectorFrontendChannel; }");
    push(@frontendMethods, "    private:");
    push(@frontendMethods, "        InspectorFrontendChannel* m_inspectorFrontendChannel;");
    push(@frontendMethods, "    };");
    push(@frontendMethods, "");

    my $getterName = lc($agentName);
    push(@frontendMethods, "    ${agentName}* ${getterName}() { return &m_${getterName}; }");
    push(@frontendMethods, "");

    push(@frontendFooter, "    ${agentName} m_${getterName};");

    push(@frontendAgentFields, "m_${getterName}");
}

sub generateFrontendConstructorImpl
{
    my @frontendConstructorImpl;
    push(@frontendConstructorImpl, "${frontendClassName}::${frontendClassName}(InspectorFrontendChannel* inspectorFrontendChannel)");
    push(@frontendConstructorImpl, "    : m_inspectorFrontendChannel(inspectorFrontendChannel)");
    foreach my $agentField (@frontendAgentFields) {
        push(@frontendConstructorImpl, "    , ${agentField}(inspectorFrontendChannel)");
    }
    push(@frontendConstructorImpl, "{");
    push(@frontendConstructorImpl, "}");
    return @frontendConstructorImpl;
}

sub generateFunctions
{
    my $interface = shift;
    my $agent = shift;

    foreach my $function (@{$interface->functions}) {
        if ($function->signature->extendedAttributes->{"event"}) {
            generateFrontendFunction($interface, $function, $agent);
        } else {
            generateBackendFunction($interface, $function);
        }
    }

    collectBackendJSStubFunctions($interface);
    collectBackendJSStubEvents($interface);
}

sub generateFrontendFunction
{
    my $interface = shift;
    my $function = shift;
    my $agent = shift;

    my $functionName = $function->signature->name;

    my $domain = $interface->name;
    my @argsFiltered = grep($_->direction eq "out", @{$function->parameters}); # just keep only out parameters for frontend interface.
    map($frontendTypes{$_->type} = 1, @argsFiltered); # register required types.
    my $arguments = join(", ", map(paramTypeTraits($_, "param") . " " . $_->name, @argsFiltered)); # prepare arguments for function signature.

    my $signature = "        void ${functionName}(${arguments});";
    !$agent->{methodSignatures}->{$signature} || die "Duplicate frontend function was detected for signature '$signature'.";
    $agent->{methodSignatures}->{$signature} = 1;
    push(@{$agent->{methodDeclarations}}, $signature);

    my @function;
    push(@function, "void ${frontendClassName}::${domain}::${functionName}(${arguments})");
    push(@function, "{");
    push(@function, "    RefPtr<InspectorObject> ${functionName}Message = InspectorObject::create();");
    push(@function, "    ${functionName}Message->setString(\"method\", \"$domain.$functionName\");");
    if (scalar(@argsFiltered)) {
        push(@function, "    RefPtr<InspectorObject> paramsObject = InspectorObject::create();");

        foreach my $parameter (@argsFiltered) {
            my $optional = $parameter->extendedAttributes->{"optional"} ? "if (" . $parameter->name . ")\n        " : "";
            push(@function, "    " . $optional . "paramsObject->set" . paramTypeTraits($parameter, "JSONType") . "(\"" . $parameter->name . "\", " . paramTypeTraits($parameter, "valueAccessor") . $parameter->name . ");");
        }
        push(@function, "    ${functionName}Message->setObject(\"params\", paramsObject);");
    }
    push(@function, "    if (m_inspectorFrontendChannel)");
    push(@function, "        m_inspectorFrontendChannel->sendMessageToFrontend(${functionName}Message->toJSONString());");
    push(@function, "}");
    push(@function, "");
    push(@frontendMethodsImpl, @function);
}

sub camelCase
{
    my $value = shift;
    $value =~ s/\b(\w)/\U$1/g; # make a camel-case name for type name
    $value =~ s/ //g;
    return $value;
}

sub generateBackendFunction
{
    my $interface = shift;
    my $function = shift;

    my $functionName = $function->signature->name;
    my $fullQualifiedFunctionName = $interface->name . "_" . $functionName;
    my $fullQualifiedFunctionNameDot = $interface->name . "." . $functionName;

    push(@backendConstantDeclarations, "        k${fullQualifiedFunctionName}Cmd,");
    push(@backendConstantDefinitions, "    \"${fullQualifiedFunctionNameDot}\",");

    map($backendTypes{$_->type} = 1, @{$function->parameters}); # register required types
    my @inArgs = grep($_->direction eq "in", @{$function->parameters});
    my @outArgs = grep($_->direction eq "out", @{$function->parameters});
    
    my $signature = "    void ${fullQualifiedFunctionName}(long callId, InspectorObject* requestMessageObject);";
    !$backendMethodSignatures{${signature}} || die "Duplicate function was detected for signature '$signature'.";
    $backendMethodSignatures{${signature}} = "$fullQualifiedFunctionName";
    push(@backendMethods, ${signature});

    my @function;
    my $requestMessageObject = scalar(@inArgs) ? " requestMessageObject" : "";
    push(@function, "void ${backendClassName}::${fullQualifiedFunctionName}(long callId, InspectorObject*$requestMessageObject)");
    push(@function, "{");
    push(@function, "    RefPtr<InspectorArray> protocolErrors = InspectorArray::create();");
    push(@function, "");

    my $domain = $interface->name;
    my $domainAccessor = typeTraits($domain, "domainAccessor");
    $backendTypes{$domain} = 1;
    $backendDomains{$domain} = 1;
    push(@function, "    if (!$domainAccessor)");
    push(@function, "        protocolErrors->pushString(\"$domain handler is not available.\");");
    push(@function, "");

    # declare local variables for out arguments.
    if (scalar(@outArgs)) {
        push(@function, map("    " . typeTraits($_->type, "variable") . " out_" . $_->name . " = " . typeTraits($_->type, "defaultValue") . ";", @outArgs));
        push(@function, "");
    }
    push(@function, "    ErrorString error;");
    push(@function, "");

    my $indent = "";
    if (scalar(@inArgs)) {
        push(@function, "    RefPtr<InspectorObject> paramsContainer = requestMessageObject->getObject(\"params\");");
        push(@function, "    InspectorObject* paramsContainerPtr = paramsContainer.get();");
        push(@function, "    InspectorArray* protocolErrorsPtr = protocolErrors.get();");

        foreach my $parameter (@inArgs) {
            my $name = $parameter->name;
            my $type = $parameter->type;
            my $typeString = camelCase($parameter->type);
            my $optionalFlagArgument = "0";
            if ($parameter->extendedAttributes->{"optional"}) {
                push(@function, "    bool ${name}_valueFound = false;");
                $optionalFlagArgument = "&${name}_valueFound";
            }
            push(@function, "    " . typeTraits($type, "variable") . " in_$name = get$typeString(paramsContainerPtr, \"$name\", $optionalFlagArgument, protocolErrorsPtr);");
        }
        push(@function, "");
        $indent = "    ";
    }

    my $args = join(", ",
                    ("&error",
                     map(($_->extendedAttributes->{"optional"} ?
                          $_->name . "_valueFound ? &in_" . $_->name . " : 0" :
                          "in_" . $_->name), @inArgs),
                     map("&out_" . $_->name, @outArgs)));

    push(@function, $indent . "if (!protocolErrors->length())");
    push(@function, $indent . "    $domainAccessor->$functionName($args);");
    push(@function, "");
    push(@function, "    RefPtr<InspectorObject> result = InspectorObject::create();");
    if (scalar(@outArgs)) {
        push(@function, "    if (!protocolErrors->length() && !error.length()) {");
        foreach my $parameter (@outArgs) {
            my $offset = "        ";
            # Don't add optional boolean parameter to the result unless it is "true"
            if ($parameter->extendedAttributes->{"optional"} && $parameter->type eq "boolean") {
                push(@function, $offset . "if (out_" . $parameter->name . ")");
                $offset .= "    ";
            }
            push(@function, $offset . "result->set" . typeTraits($parameter->type, "JSONType") . "(\"" . $parameter->name . "\", out_" . $parameter->name . ");");
        }
        push(@function, "    }");
    }
    push(@function, "    sendResponse(callId, result, String::format(\"Some arguments of method '%s' can't be processed\", \"$fullQualifiedFunctionNameDot\"), protocolErrors, error);");
    push(@function, "}");
    push(@function, "");
    push(@backendMethodsImpl, @function);
}

sub generateBackendSendResponse
{
    my $sendResponse = << "EOF";

void ${backendClassName}::sendResponse(long callId, PassRefPtr<InspectorObject> result, const String& errorMessage, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError)
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
EOF
    return split("\n", $sendResponse);
}

sub generateBackendReportProtocolError
{
    my $reportProtocolError = << "EOF";

void ${backendClassName}::reportProtocolError(const long* const callId, CommonErrorCode code, const String& errorMessage) const
{
    reportProtocolError(callId, code, errorMessage, 0);
}

void ${backendClassName}::reportProtocolError(const long* const callId, CommonErrorCode code, const String& errorMessage, PassRefPtr<InspectorArray> data) const
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
EOF
    return split("\n", $reportProtocolError);
}

sub generateArgumentGetters
{
    my $type = shift;
    my $json = typeTraits($type, "JSONType");
    my $variable = typeTraits($type, "variable");
    my $defaultValue = typeTraits($type, "defaultValue");
    my $return  = typeTraits($type, "return") ? typeTraits($type, "return") : typeTraits($type, "param");

    my $typeString = camelCase($type);
    push(@backendConstantDeclarations, "    static $return get$typeString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);");
    my $getterBody = << "EOF";

$return InspectorBackendDispatcher::get$typeString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    $variable value = $defaultValue;

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '\%s' with type '$json'.", name.utf8().data()));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '\%s' with type '$json' was not found.", name.utf8().data()));
        return value;
    }

    if (!valueIterator->second->as$json(&value))
        protocolErrors->pushString(String::format("Parameter '\%s' has wrong type. It must be '$json'.", name.utf8().data()));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}
EOF

    return split("\n", $getterBody);
}

sub generateBackendDispatcher
{
    my @body;
    my @mapEntries = map("        &${backendClassName}::$_,", map ($backendMethodSignatures{$_}, @backendMethods));
    my $mapEntries = join("\n", @mapEntries);

    my $backendDispatcherBody = << "EOF";
void ${backendClassName}::dispatch(const String& message)
{
    RefPtr<${backendClassName}> protect = this;
    typedef void (${backendClassName}::*CallHandler)(long callId, InspectorObject* messageObject);
    typedef HashMap<String, CallHandler> DispatchMap;
    DEFINE_STATIC_LOCAL(DispatchMap, dispatchMap, );
    long callId = 0;

    if (dispatchMap.isEmpty()) {
        static CallHandler handlers[] = {
$mapEntries
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
EOF
    return split("\n", $backendDispatcherBody);
}

sub generateBackendMessageParser
{
    my $messageParserBody = << "EOF";
bool ${backendClassName}::getCommandName(const String& message, String* result)
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
EOF
    return split("\n", $messageParserBody);
}

sub collectBackendJSStubFunctions
{
    my $interface = shift;
    my @functions = grep(!$_->signature->extendedAttributes->{"event"}, @{$interface->functions});
    my $domain = $interface->name;

    foreach my $function (@functions) {
        my $name = $function->signature->name;
        my @inArgs = grep($_->direction eq "in", @{$function->parameters});
        my $argumentNames = join(
            ",",
            map("\"" . $_->name . "\": {"
                . "\"optional\": " . ($_->extendedAttributes->{"optional"} ? "true " : "false") . ", "
                . "\"type\": \"" . typeTraits($_->type, "JSType") . "\""
                . "}",
                 @inArgs));
        push(@backendJSStubs, "    this._registerDelegate('{" .
            "\"method\": \"$domain.$name\", " .
            (scalar(@inArgs) ? "\"params\": {$argumentNames}, " : "") .
            "\"id\": 0" .
        "}');");
    }
}

sub collectBackendJSStubEvents
{
    my $interface = shift;
    my @functions = grep($_->signature->extendedAttributes->{"event"}, @{$interface->functions});
    my $domain = $interface->name;

    foreach my $function (@functions) {
        my $name = $domain . "." . $function->signature->name;
        my @outArgs = grep($_->direction eq "out", @{$function->parameters});
        my $argumentNames = join(",", map("\"" . $_->name . "\"" , @outArgs));
        push(@backendJSEvents, "    this._eventArgs[\"" . $name . "\"] = [" . $argumentNames ."];");
    }
}

sub generateBackendStubJS
{
    my $JSRegisterDomainDispatchers = join("\n", map("    this.register" . $_ . "Dispatcher = this._registerDomainDispatcher.bind(this, \"" . $_ ."\");", @frontendDomains));
    my $JSStubs = join("\n", @backendJSStubs);
    my $JSEvents = join("\n", @backendJSEvents);
    my $inspectorBackendStubJS = << "EOF";
$licenseTemplate

InspectorBackendStub = function()
{
    this._lastCallbackId = 1;
    this._pendingResponsesCount = 0;
    this._callbacks = {};
    this._domainDispatchers = {};
    this._eventArgs = {};
$JSStubs
$JSEvents
$JSRegisterDomainDispatchers
}

InspectorBackendStub.prototype = {
    dumpInspectorTimeStats: 0,
    dumpInspectorProtocolMessages: 0,

    _wrap: function(callback)
    {
        var callbackId = this._lastCallbackId++;
        this._callbacks[callbackId] = callback || function() {};
        return callbackId;
    },

    _registerDelegate: function(requestString)
    {
        var domainAndFunction = JSON.parse(requestString).method.split(".");
        var agentName = domainAndFunction[0] + "Agent";
        if (!window[agentName])
            window[agentName] = {};
        window[agentName][domainAndFunction[1]] = this._sendMessageToBackend.bind(this, requestString);
        window[agentName][domainAndFunction[1]]["invoke"] = this._invoke.bind(this, requestString)
    },

    _invoke: function(requestString, args, callback)
    {
        var request = JSON.parse(requestString);
        request.params = args;
        this._wrapCallbackAndSendMessageObject(request, callback);
    },

    _sendMessageToBackend: function()
    {
        var args = Array.prototype.slice.call(arguments);
        var request = JSON.parse(args.shift());
        var callback = (args.length && typeof args[args.length - 1] === "function") ? args.pop() : 0;
        var domainAndMethod = request.method.split(".");
        var agentMethod = domainAndMethod[0] + "Agent." + domainAndMethod[1];

        var hasParams = false;
        if (request.params) {
            for (var key in request.params) {
                var typeName = request.params[key].type;
                var optionalFlag = request.params[key].optional;

                if (args.length === 0 && !optionalFlag) {
                    console.error("Protocol Error: Invalid number of arguments for method '" + agentMethod + "' call. It must have the next arguments '" + JSON.stringify(request.params) + "'.");
                    return;
                }

                var value = args.shift();
                if (optionalFlag && typeof value === "undefined") {
                    delete request.params[key];
                    continue;
                }

                if (typeof value !== typeName) {
                    console.error("Protocol Error: Invalid type of argument '" + key + "' for method '" + agentMethod + "' call. It must be '" + typeName + "' but it is '" + typeof value + "'.");
                    return;
                }

                request.params[key] = value;
                hasParams = true;
            }
            if (!hasParams)
                delete request.params;
        }

        if (args.length === 1 && !callback) {
            if (typeof args[0] !== "undefined") {
                console.error("Protocol Error: Optional callback argument for method '" + agentMethod + "' call must be a function but its type is '" + typeof args[0] + "'.");
                return;
            }
        }

        this._wrapCallbackAndSendMessageObject(request, callback);
    },

    _wrapCallbackAndSendMessageObject: function(messageObject, callback)
    {
        messageObject.id = this._wrap(callback);

        if (this.dumpInspectorTimeStats) {
            var wrappedCallback = this._callbacks[messageObject.id];
            wrappedCallback.methodName = messageObject.method;
            wrappedCallback.sendRequestTime = Date.now();
        }

        if (this.dumpInspectorProtocolMessages)
            console.log("frontend: " + JSON.stringify(messageObject));

        ++this._pendingResponsesCount;
        this.sendMessageObjectToBackend(messageObject);
    },

    sendMessageObjectToBackend: function(messageObject)
    {
        console.timeStamp(messageObject.method);
        var message = JSON.stringify(messageObject);
        InspectorFrontendHost.sendMessageToBackend(message);
    },

    _registerDomainDispatcher: function(domain, dispatcher)
    {
        this._domainDispatchers[domain] = dispatcher;
    },

    dispatch: function(message)
    {
        if (this.dumpInspectorProtocolMessages)
            console.log("backend: " + ((typeof message === "string") ? message : JSON.stringify(message)));

        var messageObject = (typeof message === "string") ? JSON.parse(message) : message;

        if ("id" in messageObject) { // just a response for some request
            if (messageObject.error) {
                messageObject.error.__proto__ = {
                    getDescription: function()
                    {
                        switch(this.code) {
                            case -32700: return "Parse error";
                            case -32600: return "Invalid Request";
                            case -32601: return "Method not found";
                            case -32602: return "Invalid params";
                            case -32603: return "Internal error";;
                            case -32000: return "Server error";
                        }
                    },

                    toString: function()
                    {
                        var description ="Unknown error code";
                        return this.getDescription() + "(" + this.code + "): " + this.message + "." + (this.data ? " " + this.data.join(" ") : "");
                    },

                    getMessage: function()
                    {
                        return this.message;
                    }
                }

                if (messageObject.error.code !== -32000)
                    this.reportProtocolError(messageObject);
            }

            var arguments = [];
            if (messageObject.result) {
                for (var key in messageObject.result)
                    arguments.push(messageObject.result[key]);
            }

            var callback = this._callbacks[messageObject.id];
            if (callback) {
                var processingStartTime;
                if (this.dumpInspectorTimeStats && callback.methodName)
                    processingStartTime = Date.now();

                arguments.unshift(messageObject.error);
                callback.apply(null, arguments);
                --this._pendingResponsesCount;
                delete this._callbacks[messageObject.id];

                if (this.dumpInspectorTimeStats && callback.methodName)
                    console.log("time-stats: " + callback.methodName + " = " + (processingStartTime - callback.sendRequestTime) + " + " + (Date.now() - processingStartTime));
            }

            if (this._scripts && !this._pendingResponsesCount)
                this.runAfterPendingDispatches();

            return;
        } else {
            var method = messageObject.method.split(".");
            var domainName = method[0];
            var functionName = method[1];
            if (!(domainName in this._domainDispatchers)) {
                console.error("Protocol Error: the message is for non-existing domain '" + domainName + "'");
                return;
            }
            var dispatcher = this._domainDispatchers[domainName];
            if (!(functionName in dispatcher)) {
                console.error("Protocol Error: Attempted to dispatch an unimplemented method '" + messageObject.method + "'");
                return;
            }

            if (!this._eventArgs[messageObject.method]) {
                console.error("Protocol Error: Attempted to dispatch an unspecified method '" + messageObject.method + "'");
                return;
            }

            var params = [];
            if (messageObject.params) {
                var paramNames = this._eventArgs[messageObject.method];
                for (var i = 0; i < paramNames.length; ++i)
                    params.push(messageObject.params[paramNames[i]]);
            }

            var processingStartTime;
            if (this.dumpInspectorTimeStats)
                processingStartTime = Date.now();

            dispatcher[functionName].apply(dispatcher, params);

            if (this.dumpInspectorTimeStats)
                console.log("time-stats: " + messageObject.method + " = " + (Date.now() - processingStartTime));
        }
    },

    reportProtocolError: function(messageObject)
    {
        console.error("Request with id = " + messageObject.id + " failed. " + messageObject.error);
    },

    runAfterPendingDispatches: function(script)
    {
        if (!this._scripts)
            this._scripts = [];

        if (script)
            this._scripts.push(script);

        if (!this._pendingResponsesCount) {
            var scripts = this._scripts;
            this._scripts = []
            for (var id = 0; id < scripts.length; ++id)
                 scripts[id].call(this);
        }
    }
}

InspectorBackend = new InspectorBackendStub();

EOF
    return split("\n", $inspectorBackendStubJS);
}

sub generateHeader
{
    my $className = shift;
    my $classDeclaration = shift;
    my $types = shift;
    my $constructor = shift;
    my $constants = shift;
    my $methods = shift;
    my $footer = shift;

    my $forwardHeaders = join("\n", sort(map("#include <" . typeTraits($_, "forwardHeader") . ">", grep(typeTraits($_, "forwardHeader"), keys %{$types}))));
    my $forwardDeclarations = join("\n", sort(map("class " . typeTraits($_, "forward") . ";", grep(typeTraits($_, "forward"), keys %{$types}))));
    my $constantDeclarations = join("\n", @{$constants});
    my $methodsDeclarations = join("\n", @{$methods});

    my $headerBody = << "EOF";
// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef ${className}_h
#define ${className}_h

${forwardHeaders}

namespace $namespace {

$forwardDeclarations

typedef String ErrorString;

class $classDeclaration {
public:
$constructor

$constantDeclarations
$methodsDeclarations

$footer
};

} // namespace $namespace
#endif // !defined(${className}_h)

EOF
    return $headerBody;
}

sub generateSource
{
    my $className = shift;
    my $types = shift;
    my $constants = shift;
    my $methods = shift;

    my @sourceContent = split("\r", $licenseTemplate);
    push(@sourceContent, "\n#include \"config.h\"");
    push(@sourceContent, "#include \"$className.h\"");
    push(@sourceContent, "#include <wtf/text/WTFString.h>");
    push(@sourceContent, "#include <wtf/text/CString.h>");
    push(@sourceContent, "");
    push(@sourceContent, "#if ENABLE(INSPECTOR)");
    push(@sourceContent, "");

    my %headers;
    foreach my $type (keys %{$types}) {
        $headers{"#include \"" . typeTraits($type, "header") . "\""} = 1 if !typeTraits($type, "header") eq  "";
    }
    push(@sourceContent, sort keys %headers);
    push(@sourceContent, "");
    push(@sourceContent, "namespace $namespace {");
    push(@sourceContent, "");
    push(@sourceContent, join("\n", @{$constants}));
    push(@sourceContent, "");
    push(@sourceContent, @{$methods});
    push(@sourceContent, "");
    push(@sourceContent, "} // namespace $namespace");
    push(@sourceContent, "");
    push(@sourceContent, "#endif // ENABLE(INSPECTOR)");
    push(@sourceContent, "");
    return @sourceContent;
}

sub typeTraits
{
    my $type = shift;
    my $trait = shift;
    return $typeTransform{$type}->{$trait};
}

sub paramTypeTraits
{
    my $paramDescription = shift;
    my $trait = shift;
    if ($paramDescription->extendedAttributes->{"optional"}) {
        my $optionalResult = typeTraits($paramDescription->type, "optional_" . $trait);
        return $optionalResult if defined $optionalResult;
    }
    my $result = typeTraits($paramDescription->type, $trait);
    return defined $result ? $result : "";
}

sub generateBackendAgentFieldsAndConstructor
{
    my @arguments;
    my @fieldInitializers;

    push(@arguments, "InspectorFrontendChannel* inspectorFrontendChannel");
    push(@fieldInitializers, "        : m_inspectorFrontendChannel(inspectorFrontendChannel)");
    push(@backendFooter, "    InspectorFrontendChannel* m_inspectorFrontendChannel;");

    foreach my $domain (sort keys %backendDomains) {
        # Add agent field declaration to the footer.
        my $agentClassName = typeTraits($domain, "forward");
        my $field = typeTraits($domain, "domainAccessor");
        push(@backendFooter, "    ${agentClassName}* ${field};");

        # Add agent parameter and initializer.
        my $arg = substr($field, 2);
        push(@fieldInitializers, "        , ${field}(${arg})");
        push(@arguments, "${agentClassName}* ${arg}");
    }

    my $argumentString = join(", ", @arguments);

    my @backendHead;
    push(@backendHead, "    ${backendClassName}(${argumentString})");
    push(@backendHead, @fieldInitializers);
    push(@backendHead, "    { }");
    push(@backendHead, "");
    push(@backendHead, "    void clearFrontend() { m_inspectorFrontendChannel = 0; }");
    push(@backendHead, "");
    push(@backendHead, "    enum CommonErrorCode {");
    push(@backendHead, "        ParseError = 0,");
    push(@backendHead, "        InvalidRequest,");
    push(@backendHead, "        MethodNotFound,");
    push(@backendHead, "        InvalidParams,");
    push(@backendHead, "        InternalError,");
    push(@backendHead, "        ServerError,");
    push(@backendHead, "        LastEntry,");
    push(@backendHead, "    };");
    push(@backendHead, "");
    push(@backendHead, "    void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage) const;");
    push(@backendHead, "    void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage, PassRefPtr<InspectorArray> data) const;");
    push(@backendHead, "    void dispatch(const String& message);");
    push(@backendHead, "    static bool getCommandName(const String& message, String* result);");
    push(@backendHead, "");
    push(@backendHead, "    enum MethodNames {");
    $backendConstructor = join("\n", @backendHead);
}

sub finish
{
    my $object = shift;

    push(@backendMethodsImpl, generateBackendDispatcher());
    push(@backendMethodsImpl, generateBackendSendResponse());
    push(@backendMethodsImpl, generateBackendReportProtocolError());
    unshift(@frontendMethodsImpl, generateFrontendConstructorImpl(), "");

    open(my $SOURCE, ">$outputDir/$frontendClassName.cpp") || die "Couldn't open file $outputDir/$frontendClassName.cpp";
    print $SOURCE join("\n", generateSource($frontendClassName, \%frontendTypes, \@frontendConstantDefinitions, \@frontendMethodsImpl));
    close($SOURCE);
    undef($SOURCE);

    open(my $HEADER, ">$outputHeadersDir/$frontendClassName.h") || die "Couldn't open file $outputHeadersDir/$frontendClassName.h";
    print $HEADER generateHeader($frontendClassName, $frontendClassName, \%frontendTypes, $frontendConstructor, \@frontendConstantDeclarations, \@frontendMethods, join("\n", @frontendFooter));
    close($HEADER);
    undef($HEADER);

    unshift(@backendConstantDefinitions, "const char* InspectorBackendDispatcher::commandNames[] = {");
    push(@backendConstantDefinitions, "};");
    push(@backendConstantDefinitions, "");

    # Make dispatcher methods private on the backend.
    push(@backendConstantDeclarations, "};");
    push(@backendConstantDeclarations, "");
    push(@backendConstantDeclarations, "    static const char* commandNames[];");    
    push(@backendConstantDeclarations, "");
    push(@backendConstantDeclarations, "private:");

    foreach my $type (keys %backendTypes) {
        if (typeTraits($type, "JSONType")) {
            push(@backendMethodsImpl, generateArgumentGetters($type));
        }
    }

    push(@backendConstantDeclarations, "    void sendResponse(long callId, PassRefPtr<InspectorObject> result, const String& errorMessage, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError);");

    generateBackendAgentFieldsAndConstructor();

    push(@backendMethodsImpl, generateBackendMessageParser());
    push(@backendMethodsImpl, "");

    push(@backendConstantDeclarations, "");

    open($SOURCE, ">$outputDir/$backendClassName.cpp") || die "Couldn't open file $outputDir/$backendClassName.cpp";
    print $SOURCE join("\n", generateSource($backendClassName, \%backendTypes, \@backendConstantDefinitions, \@backendMethodsImpl));
    close($SOURCE);
    undef($SOURCE);

    open($HEADER, ">$outputHeadersDir/$backendClassName.h") || die "Couldn't open file $outputHeadersDir/$backendClassName.h";
    print $HEADER join("\n", generateHeader($backendClassName, $backendClassDeclaration, \%backendTypes, $backendConstructor, \@backendConstantDeclarations, \@backendMethods, join("\n", @backendFooter)));
    close($HEADER);
    undef($HEADER);

    open(my $JS_STUB, ">$outputDir/$backendJSStubName.js") || die "Couldn't open file $outputDir/$backendJSStubName.js";
    print $JS_STUB join("\n", generateBackendStubJS());
    close($JS_STUB);
    undef($JS_STUB);
}

1;
