#pragma once

/**
 * This header implements functionality to build PyTorch with only a certain
 * set of operators (+ dependencies) included.
 *
 * - Build with -DTORCH_OPERATOR_WHITELIST="aten::add;aten::sub" and only these
 *   two ops will be included in your build.  The whitelist records operators
 *   only, no overloads; if you include aten::add, all overloads of aten::add
 *   will be included.
 *
 * Internally, this is done by removing the operator registration calls
 * using compile time programming, and the linker will then prune all
 * operator functions that weren't registered.
 * See Note [Selective build] for more details
 *
 * WARNING: The whitelist mechanism doesn't work for all ways you could go about
 * registering an operator.  If the dispatch key / operator name is not
 * sufficiently obvious at compile time, then the whitelisting mechanism
 * will fail (and the operator will be included in the binary anyway).
 */

#include <c10/util/string_view.h>
#include <c10/core/DispatchKey.h>
#include <c10/macros/Macros.h>

namespace c10 {

namespace impl {

// returns true iff whitelist contains item
// op_whitelist_contains("a;bc;d", "bc") == true
constexpr bool op_whitelist_contains(string_view whitelist, string_view item) {
    size_t next = -1;
    for (size_t cur = 0; cur <= whitelist.size(); cur = next) {
      next = whitelist.find(';', cur);
      if (next != string_view::npos) {
        if (whitelist.substr(cur, next - cur).compare(item) == 0) {
          return true;
        }
        next++;
      } else {
        if (whitelist.substr(cur).compare(item) == 0) {
          return true;
        }
        break;
      }
    }
    return false;
}

// Returns true iff the given op name is on the whitelist
// and should be registered
constexpr bool op_whitelist_check(c10::string_view op_name) {
  assert(op_name.find("::").compare(string_view::npos) != 0);
#if !defined(TORCH_OPERATOR_WHITELIST)
  // If the TORCH_OPERATOR_WHITELIST parameter is not defined,
  // all ops are to be registered
  return true;
#else
  return op_whitelist_contains(
    C10_STRINGIZE(TORCH_OPERATOR_WHITELIST),
    // Strip overload name (as whitelist doesn't contain overloads)
    OperatorNameView::parse(op_name).name
  );
#endif
}

// Returns true iff the given schema string is on the whitelist
// and should be registered
constexpr bool schema_whitelist_check(c10::string_view schema) {
#if defined(TORCH_FORCE_SCHEMA_REGISTRATION)
  return true;
#else
  return op_whitelist_check(schema.substr(0, schema.find("(")));
#endif
}

// Returns true iff the given dispatch key is on the whitelist
// and should be registered.  Right now, the list of valid
// mobile dispatch keys is hard coded.
constexpr bool dispatch_key_whitelist_check(DispatchKey k) {
#ifdef C10_MOBILE
  return k == DispatchKey::CPU || k == DispatchKey::Vulkan || k == DispatchKey::QuantizedCPU || k == DispatchKey::BackendSelect || k == DispatchKey::CatchAll;
#else
  return true;
#endif
}

} // namespace impl
} // namespace c10
