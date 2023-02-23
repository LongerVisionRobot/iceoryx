#ifndef IOX_HOOFS_ERROR_REPORTING_API_HPP
#define IOX_HOOFS_ERROR_REPORTING_API_HPP

#include "iceoryx_hoofs/error_reporting/error_forwarding.hpp"
#include "iceoryx_hoofs/error_reporting/platform/error_kind.hpp"

// The following macros are statements (not expressions).
// This is important, as it enforces correct use to some degree.
// For example thye cannot be used as function arguments and must be terminated with a ';'.

/// @brief calls panic handler and does not return
/// @param msg optional message string literal
/// @note could actually throw if desired without breaking control flow asssumptions
#define IOX_PANIC(...)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        iox::err::panic(CURRENT_SOURCE_LOCATION, ##__VA_ARGS__);                                                       \
    } while (false)

/// @brief report error of some kind
/// @param error error object (or code)
/// @param kind kind of error
#define IOX_REPORT(error, kind)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        iox::err::forwardError(CURRENT_SOURCE_LOCATION, iox::err::toError(error), kind);                               \
    } while (false)

/// @brief report fatal error
/// @param error error object (or code)
#define IOX_REPORT_FATAL(error) IOX_REPORT(error, FATAL)

/// @brief report error of some kind if expr evaluates to true
/// @param expr boolean expression
/// @param error error object (or code)
/// @param kind kind of error
#define IOX_REPORT_IF(expr, error, kind)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (expr)                                                                                                      \
        {                                                                                                              \
            iox::err::forwardError(CURRENT_SOURCE_LOCATION, iox::err::toError(error), kind);                           \
        }                                                                                                              \
    } while (false)

/// @brief report fatal error if expr evaluates to false
/// @note for conditions that may actually happen during correct use
/// @param expr boolean expression that must hold
/// @param error error object (or code)
#define IOX_REQUIRE(expr, error) IOX_REPORT_IF(!(expr), error, FATAL)

//*****************************
//* For safe mode and debugging
//*****************************

// Later on there can be variadic versions that do not need a default message but for now we leave
// make the more general version mandatory

// There are no error codes/errors required here on purpose, as it would make the use cumbersome.
// Instead a special internal error type is used.
// If required, a custom error option can be added but for now location should be sufficient.

/// @brief if enabled: report fatal error if expr evaluates to false
/// @param expr boolean expression that must hold upon entry of the function it appears in
/// @param message message to be logged in case of violation
#ifdef IOX_CHECK_PRECONDITIONS
#define IOX_PRECONDITION(expr, ...)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            iox::err::forwardError(CURRENT_SOURCE_LOCATION,                                                            \
                                   iox::err::Violation(iox::err::ErrorCode::PRECONDITION_VIOLATION),                   \
                                   iox::err::PRECONDITION_VIOLATION,                                                   \
                                   ##__VA_ARGS__);                                                                     \
        }                                                                                                              \
    } while (false)
#else
#define IOX_PRECONDITION(expr, ...)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        iox::err::discard([&]() { return expr; }, ##__VA_ARGS__);                                                      \
    } while (false)
#endif

/// @brief if enabled: report fatal error if expr evaluates to false
/// @note for conditions that should not happen with correct use
/// @param expr boolean expression that must hold
/// @param message message to be logged in case of violation
#ifdef IOX_CHECK_ASSUMPTIONS
#define IOX_ASSUME(expr, ...)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            iox::err::forwardError(CURRENT_SOURCE_LOCATION,                                                            \
                                   iox::err::Violation(iox::err::ErrorCode::DEBUG_ASSERT_VIOLATION),                   \
                                   iox::err::DEBUG_ASSERT_VIOLATION,                                                   \
                                   ##__VA_ARGS__);                                                                     \
        }                                                                                                              \
    } while (false)

#else
#define IOX_ASSUME(expr, ...)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        iox::err::discard([&]() { return expr; }, ##__VA_ARGS__);                                                      \
    } while (false)
#endif

/// @note We have to discard the expr otherwise we may get some unused variable warnings at call
/// site if variables of the expression are not used elsewhere (unusual but possible).

/// @note We can get the same effect of optimized out code by using compile time logic
/// such as defining requiresHandling<Kind>::value defining whether the error should be handled
/// This would make #ifdef switches largely redundant (optionally only at configuration time)

#endif
