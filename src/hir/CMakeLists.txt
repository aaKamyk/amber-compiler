file(GLOB_RECURSE sources "*.cpp")

target_sources(kcp
        PRIVATE
        ${sources}
)