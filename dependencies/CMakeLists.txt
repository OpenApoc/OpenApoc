# project name, and type
PROJECT(OpenApoc_Dependencies C CXX)

# hide dependency warnings
add_compile_options("-w")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)
