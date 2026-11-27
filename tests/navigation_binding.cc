#include <il2cpp-config.h>
#if _WIN32
#undef IL2CPP_IMPORT
#define IL2CPP_IMPORT
#endif
#include "prime/navigation_binding.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
Il2CppClass receiver_class, base_class, value_class;
Il2CppObject receiver{}, boxed{};
Il2CppType boolean_type{}, int64_type{}, wrong_type{};
const Il2CppType* parameters[] = {&int64_type};
MethodInfo method{};
bool answer = true, throws_managed = false, null_box = false, null_data = false, receiver_matches = true;
int invocations = 0;
int64_t observed_id = 0;
}
#if _WIN32
#define API(ret, name, params) extern "C" ret name params
#define END_API
#else
#define API(ret, name, params) name##_t name = +[] params->ret
#define END_API ;
#endif
API(const MethodInfo*, il2cpp_class_get_methods, (Il2CppClass* cls, void** iterator))
{
  if (cls != &base_class || *iterator) return nullptr;
  *iterator = &method;
  return &method;
}
END_API
API(Il2CppClass*, il2cpp_class_get_parent, (Il2CppClass* cls))
{ return cls == &receiver_class ? &base_class : nullptr; }
END_API
API(char*, il2cpp_type_get_name, (const Il2CppType* type))
{
  const char* name = type == &boolean_type ? "System.Boolean" : type == &int64_type ? "System.Int64" : "System.Int32";
  const auto size = std::strlen(name) + 1;
  auto* result = static_cast<char*>(std::malloc(size));
  std::memcpy(result, name, size);
  return result;
}
END_API
API(void, il2cpp_free, (void* value)) { std::free(value); }
END_API
API(const Il2CppType*, il2cpp_class_get_type, (Il2CppClass*)) { return &boolean_type; }
END_API
API(void*, il2cpp_object_unbox, (Il2CppObject*)) { return null_data ? nullptr : &answer; }
END_API
API(Il2CppObject*, il2cpp_runtime_invoke, (const MethodInfo*, void* object, void** args, Il2CppException** exception))
{
  ++invocations;
  receiver_matches = receiver_matches && object == &receiver;
  observed_id = args ? *static_cast<int64_t*>(args[0]) : 0;
  if (throws_managed) *exception = reinterpret_cast<Il2CppException*>(&boxed);
  return null_box ? nullptr : &boxed;
}
END_API

void Require(bool condition) { if (!condition) throw std::runtime_error("navigation binding regression"); }
int main()
{
  receiver.klass = &receiver_class;
  boxed.klass = &value_class;
  method.name = "SelectPoi";
  method.parameters = parameters;
  method.parameters_count = 1;
  method.return_type = &boolean_type;
  method.methodPointer = reinterpret_cast<Il2CppMethodPointer>(1);
  method.invoker_method = reinterpret_cast<InvokerMethod>(1);
  const int64_t id = 0x12345678abcdefLL;
  Require(navigation_binding::Select(&receiver, "SelectPoi", id));
  Require(observed_id == id && invocations == 1 && receiver_matches); // Windows must not truncate Int64
  answer = false;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id));
  answer = true;
  throws_managed = true;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id));
  throws_managed = false;
  null_box = true;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id));
  null_box = false;
  null_data = true;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id));
  null_data = false;
  const int previous = invocations;
  parameters[0] = &wrong_type;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id) && invocations == previous);
  parameters[0] = &int64_type;
  method.return_type = &wrong_type;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id) && invocations == previous);
  method.return_type = &boolean_type;
  method.flags = METHOD_ATTRIBUTE_STATIC;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id) && invocations == previous);
  method.flags = 0;
  method.invoker_method = nullptr;
  Require(!navigation_binding::Select(&receiver, "SelectPoi", id) && invocations == previous);
  Require(!navigation_binding::Select(nullptr, "SelectPoi", id));
  std::cout << "Navigation bindings reject signature drift, null results and managed exceptions; Int64 preserved\n";
}
