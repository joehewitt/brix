; C++, from tree-sitter-cpp, for the older node shapes in tree-sitter-objcpp

(call_expression
  function: (qualified_identifier
    name: (identifier) @function))

(template_function
  name: (identifier) @function)

(template_method
  name: (field_identifier) @function)

(function_declarator
  declarator: (qualified_identifier
    name: (identifier) @function))

(function_declarator
  declarator: (field_identifier) @function)

((namespace_identifier) @type
 (#match? @type "^[A-Z]"))

(auto) @type

(this) @variable.builtin
(nullptr) @constant

[
 "catch"
 "class"
 "co_await"
 "co_return"
 "co_yield"
 "constexpr"
 "constinit"
 "consteval"
 "delete"
 "explicit"
 "final"
 "friend"
 "mutable"
 "namespace"
 "noexcept"
 "new"
 "override"
 "private"
 "protected"
 "public"
 "template"
 "throw"
 "try"
 "typename"
 "using"
 "concept"
 "requires"
 "virtual"
] @keyword

(raw_string_literal) @string

; Objective-C

[
 "#import"
 "@import"
 "@interface"
 "@implementation"
 "@protocol"
 "@end"
 "@property"
 "@class"
 "@compatibility_alias"
 "@synthesize"
 "@dynamic"
 "@defs"
 "@autoreleasepool"
 "@synchronized"
 "@try"
 "@catch"
 "@finally"
 "@throw"
 "@selector"
 "@encode"
 "@available"
 "NS_ENUM"
 "NS_ERROR_ENUM"
 "NS_OPTIONS"
 "in"
 "out"
 "inout"
 "bycopy"
 "byref"
 "oneway"
 "typeof"
 "__typeof"
 "__typeof__"
 "__kindof"
 "__covariant"
 "__contravariant"
 "__block"
 "__weak"
 "__strong"
 "__autoreleasing"
 "__unsafe_unretained"
 "__bridge"
 "__bridge_retained"
 "__bridge_transfer"
 "_Nonnull"
 "_Nullable"
 "_Nullable_result"
 "_Null_unspecified"
 "__nonnull"
 "__nullable"
 (private)
 (public)
 (protected)
 (package)
 (optional)
 (required)
 (pragma)
] @keyword

[
 (getter)
 (setter)
 (nonnull)
 (nullable)
 (null_resettable)
 (null_unspecified)
 (unsafe_unretained)
 (direct)
 (readwrite)
 (readonly)
 (strong)
 (weak)
 (copy)
 (assign)
 (retain)
 (atomic)
 (nonatomic)
 (class)
 (NS_NONATOMIC_IOSONLY)
 (DISPATCH_QUEUE_REFERENCE_TYPE)
] @keyword

[
 "__attribute"
 "__attribute__"
 "__unused"
 "__builtin_available"
 "__deprecated_msg"
 "__deprecated_enum_msg"
 "__IOS_AVAILABLE"
 "API_AVAILABLE"
 "API_DEPRECATED"
 "API_UNAVAILABLE"
 "NS_AVAILABLE"
 "NS_AVAILABLE_IOS"
 "NS_CLASS_AVAILABLE_IOS"
 "NS_CLASS_DEPRECATED_IOS"
 "NS_DEPRECATED_IOS"
 "NS_ENUM_AVAILABLE_IOS"
 "NS_ENUM_DEPRECATED_IOS"
 "NS_EXTENSION_UNAVAILABLE_IOS"
 "NS_SWIFT_NAME"
 "NS_SWIFT_UNAVAILABLE"
 "NS_REFINED_FOR_SWIFT"
 "NS_UNAVAILABLE"
 "NS_NOESCAPE"
 "NS_FORMAT_FUNCTION"
 "NS_VALID_UNTIL_END_OF_SCOPE"
 "CF_FORMAT_FUNCTION"
 "DEPRECATED_ATTRIBUTE"
 "DEPRECATED_MSG_ATTRIBUTE"
 "UNAVAILABLE_ATTRIBUTE"
 "UI_APPEARANCE_SELECTOR"
 "IBOutlet"
 "IBInspectable"
] @attribute

[
 (self)
 (super)
] @variable.builtin

[
 (nil)
 (YES)
 (NO)
] @constant

(string_expression) @string
(module_string) @module
(number_expression) @number
(boolean_expression) @constant

[
 (id)
 (Class)
] @type

(class_interface
  name: (identifier) @type)
(category_interface
  name: (identifier) @type)
(category_interface
  category: (identifier) @type)
(class_implementation
  name: (identifier) @type)
(category_implementation
  name: (identifier) @type)
(category_implementation
  category: (identifier) @type)
(superclass_reference
  name: (identifier) @type)
(class_forward_declaration
  name: (identifier) @type)
(protocol_forward_declaration
  name: (identifier) @type)
(protocol_declaration
  name: (identifier) @type)
(protocol_qualifiers
  name: (identifier) @type)
(protocol_expression
  name: (identifier) @type)
(compatibility_alias_declaration
  alias_name: (identifier) @type)
(compatibility_alias_declaration
  class_name: (identifier) @type)
(parameterized_class_type_arguments
  type: (identifier) @type)
(generics_type_reference
  name: (identifier) @type)

(property_declaration
  declarator: (identifier) @property)
(property_declaration
  declarator: (_
    declarator: (identifier) @property))
(synthesize_property
  property: (identifier) @property)
(dynamic_definition
  property: (identifier) @property)

(method_declaration
  selector: (identifier) @function)
(method_definition
  selector: (identifier) @function)
(keyword_declarator
  keyword: (identifier) @function)
(keyword_declarator
  name: (identifier) @variable.parameter)
(message_expression
  selector: (identifier) @function)
(keyword_argument
  keyword: (identifier) @function)
(selector_expression
  name: (identifier) @function)
(getter
  name: (identifier) @function)
(setter
  name: (identifier) @function)
(swift_name_attribute_sepcifier
  method: (identifier) @function)

((message_expression
  receiver: (identifier) @type)
 (#match? @type "^[A-Z]"))
