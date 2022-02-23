
aux_source_directory(. DIR_LIB_SRCS)
add_library (l1nkkkthread ${DIR_LIB_SRCS} mutex.cpp mutex.h fiber.cpp fiber.h test_fiber.cpp scheduler.cpp scheduler.h test_scheduler.cpp)