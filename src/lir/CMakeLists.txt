file(GLOB_RECURSE sources "*.cpp")

target_sources(act
        PRIVATE
        ${sources}
)