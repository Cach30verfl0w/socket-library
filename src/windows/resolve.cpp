#ifdef PLATFORM_WINDOWS
#include "sockslib/resolve.hpp"
#include "sockslib/socket.hpp"
#include <array>
#include <fmt/format.h>
#include <iostream>

struct QueryContext {
    ULONG RefCount;
    WCHAR QueryName[DNS_MAX_NAME_BUFFER_LENGTH];// NOLINT
    WORD QueryType;
    ULONG QueryOptions;
    DNS_QUERY_RESULT QueryResults;
    DNS_QUERY_CANCEL QueryCancelContext;
    HANDLE QueryCompletedEvent;
};

using PQUERY_CONTEXT = QueryContext*;// NOLINT

namespace sockslib {

    auto free_query_context(const PQUERY_CONTEXT query_context) noexcept -> void {
        if(InterlockedDecrement(&query_context->RefCount) == 0) {
            if(query_context->QueryCompletedEvent != nullptr) {
                CloseHandle(query_context->QueryCompletedEvent);
            }

            HeapFree(GetProcessHeap(), 0, query_context);
        }
    }

    auto allocate_query_context() -> kstd::Result<PQUERY_CONTEXT> {
        using namespace std::string_literals;
        auto* query_context =
                static_cast<PQUERY_CONTEXT>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(QueryContext)));
        if(query_context == nullptr) {
            return kstd::Error {fmt::format("Unable to allocate Query Context => {}", get_last_error())};
        }

        InterlockedIncrement(&query_context->RefCount);
        query_context->QueryResults.Version = DNS_QUERY_RESULTS_VERSION1;
        query_context->QueryCompletedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if(query_context->QueryCompletedEvent == nullptr) {
            free_query_context(query_context);
            return kstd::Error {fmt::format("Unable to allocate Query Context => {}", get_last_error())};
        }

        return query_context;
    }

    auto WINAPI query_complete_callback(_In_ PVOID context, _Inout_ PDNS_QUERY_RESULT query_results) -> void {
        auto* query_context = static_cast<PQUERY_CONTEXT>(context);
        if(SUCCEEDED(query_results->QueryStatus)) {
            // TODO: Return address with resolve_address / Function is unfinished
            std::cout << "It's working\n";
        }

        if(query_results->pQueryRecords != nullptr) {
            DnsRecordListFree(query_results->pQueryRecords, DnsFreeRecordList);
        }

        SetEvent(query_context->QueryCompletedEvent);
        free_query_context(query_context);
    }

    auto resolve_address(const std::string domain, const kstd::usize address_type, const kstd::usize timeout)// NOLINT
            -> kstd::Result<std::string> {
        using namespace std::string_literals;
        // Startup WSA and increment socket count
        if(Socket::_socket_count == 0) {
            WSADATA wsaData {};
            if(FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData))) {
                return kstd::Error {fmt::format("Unable to initialize WSA => {}", get_last_error())};
            }
        }
        Socket::_socket_count++;

        // Allocate context
        auto allocation_result = allocate_query_context();
        if(!allocation_result) {
            return kstd::Error {allocation_result.get_error()};
        }
        auto* query_context = allocation_result.get();

        // Convert to wide name
        auto wide_name = kstd::utils::to_wcs(domain);
        const auto wide_name_size = (wide_name.size() + 1) * sizeof(wchar_t);
        if(wide_name_size > DNS_MAX_NAME_BUFFER_LENGTH) {
            // Cleanup WSA and decrement socket count
            if(Socket::_socket_count == 1) {
                if(FAILED(WSACleanup())) {
                    return kstd::Error {fmt::format("Unable to cleanup WSA => {}", get_last_error())};
                }
            }
            Socket::_socket_count--;

            return kstd::Error {fmt::format("The name of the domain is too long ({} > {})", wide_name_size,
                                            DNS_MAX_NAME_BUFFER_LENGTH)};
        }

        kstd::libc::memset(reinterpret_cast<PWCHAR>(query_context->QueryName), 0, DNS_MAX_NAME_BUFFER_LENGTH);// NOLINT
        kstd::libc::memcpy(reinterpret_cast<PWCHAR>(query_context->QueryName), wide_name.c_str(),             // NOLINT
                           wide_name_size);
        query_context->QueryType = static_cast<WORD>(address_type);
        query_context->QueryOptions = DNS_QUERY_STANDARD | DNS_QUERY_BYPASS_CACHE;

        // Create request
        DNS_QUERY_REQUEST request {};
        request.Version = DNS_QUERY_REQUEST_VERSION1;
        request.QueryName = reinterpret_cast<PWCHAR>(query_context->QueryName);// NOLINT
        request.QueryType = query_context->QueryType;
        request.QueryOptions = query_context->QueryOptions;
        request.pQueryContext = query_context;
        request.pQueryCompletionCallback = query_complete_callback;
        request.pDnsServerList = nullptr;// Default to system's DNS server

        // Initiate query
        InterlockedIncrement(&query_context->RefCount);
        auto result = DnsQueryEx(&request, &query_context->QueryResults, &query_context->QueryCancelContext);
        if(result == DNS_REQUEST_PENDING) {
            // Wait for completion of task! If timeout exceeded, cancel the query and wait for completed even
            if(WaitForSingleObject(query_context->QueryCompletedEvent, timeout) == WAIT_TIMEOUT) {
                DnsCancelQuery(&query_context->QueryCancelContext);
                WaitForSingleObject(query_context->QueryCompletedEvent, INFINITE);

                // Free the Query Context
                free_query_context(query_context);

                // Cleanup WSA and decrement socket count
                if(Socket::_socket_count == 1) {
                    if(FAILED(WSACleanup())) {
                        return kstd::Error {fmt::format("Unable to cleanup WSA => {}", get_last_error())};
                    }
                }
                Socket::_socket_count--;
                return kstd::Error {fmt::format(
                        "Unable to resolve address over DNS => The query took more than {} seconds", timeout / 1000)};
            }

            return {""};// Return address
        }

        if(FAILED(result)) {
            // Free the Query Context
            free_query_context(query_context);

            // Cleanup WSA and decrement socket count
            if(Socket::_socket_count == 1) {
                if(FAILED(WSACleanup())) {
                    return kstd::Error {fmt::format("Unable to cleanup WSA => {}", get_last_error())};
                }
            }
            Socket::_socket_count--;
            return kstd::Error {fmt::format("Unable to resolve address over DNS => {}", get_last_error())};
        }

        // Call the query callback immediately
        query_complete_callback(query_context, &query_context->QueryResults);

        // Free the Query Context
        free_query_context(query_context);

        // Cleanup WSA and decrement socket count
        if(Socket::_socket_count == 1) {
            if(FAILED(WSACleanup())) {
                return kstd::Error {fmt::format("Unable to cleanup WSA => {}", get_last_error())};
            }
        }
        Socket::_socket_count--;

        return {""};// Return address
    }

    auto resolve_address(const std::string domain, const kstd::usize timeout = 10000) -> kstd::Result<std::string> {
        // TODO: Check if IPv6 is working with internet access
        auto ipv6_resolve_result = resolve_address(domain, DNS_TYPE_AAAA, timeout);
        if (ipv6_resolve_result) {
            return ipv6_resolve_result.get();
        }

        // TODO: Check if IPv4 is working with internet access
        const auto ipv4_resolve_result = resolve_address(domain, DNS_TYPE_A, timeout);
        if (ipv4_resolve_result) {
            return ipv4_resolve_result.get();
        }

        return kstd::Error {ipv6_resolve_result.get_error()};
    }
}// namespace sockslib
#endif