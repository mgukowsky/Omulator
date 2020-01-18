add_unit_test(smoke)

add_unit_test(exception_handler)
add_unit_test(ObjectPool)
add_unit_test(ResourceWrapper)
add_unit_test(Latch ${PROJECT_SOURCE_DIR}/src/Latch.cpp)
add_unit_test(Spinlock)

add_unit_test(Worker
  ${PROJECT_SOURCE_DIR}/src/scheduler/Worker.cpp
  ${PROJECT_SOURCE_DIR}/src/Latch.cpp 
)

add_unit_test(WorkerPool
  ${PROJECT_SOURCE_DIR}/src/scheduler/WorkerPool.cpp
  ${PROJECT_SOURCE_DIR}/src/scheduler/Worker.cpp
  ${PROJECT_SOURCE_DIR}/src/Latch.cpp 
)

