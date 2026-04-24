# Sanitizer toggles. Driven by HARNESS_ENABLE_ASAN / HARNESS_ENABLE_TSAN.
function(harness_apply_sanitizers target)
    if(HARNESS_ENABLE_ASAN)
        if(MSVC)
            target_compile_options(${target} PRIVATE /fsanitize=address)
        else()
            target_compile_options(${target} PRIVATE
                -fsanitize=address -fsanitize=undefined
                -fno-omit-frame-pointer -fno-sanitize-recover=all)
            target_link_options(${target} PRIVATE
                -fsanitize=address -fsanitize=undefined)
        endif()
    endif()

    if(HARNESS_ENABLE_TSAN)
        if(MSVC)
            message(WARNING "TSan not supported on MSVC; ignoring HARNESS_ENABLE_TSAN.")
        else()
            target_compile_options(${target} PRIVATE
                -fsanitize=thread -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=thread)
        endif()
    endif()
endfunction()
