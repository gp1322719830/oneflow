set(BUILD_CUDA YES CACHE BOOL "")
set(BUILD_GIT_VERSION YES CACHE BOOL "")
set(BUILD_TESTING YES CACHE BOOL "")
set(BUILD_RDMA YES CACHE BOOL "")
set(TREAT_WARNINGS_AS_ERRORS YES CACHE BOOL "")
set(THIRD_PARTY_MIRROR aliyun CACHE STRING "")
set(PIP_INDEX_MIRROR "https://pypi.tuna.tsinghua.edu.cn/simple" CACHE STRING "")
set(CMAKE_BUILD_TYPE Release CACHE STRING "")
set(CMAKE_GENERATOR Ninja CACHE STRING "")
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda CACHE STRING "")
set(CUDNN_ROOT_DIR /usr/local/cudnn CACHE STRING "")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "")
set(CMAKE_CUDA_COMPILER "/usr/lib64/ccache/nvcc" CACHE STRING "")
set(CMAKE_CUDA_HOST_COMPILER "/usr/lib64/ccache/g++" CACHE STRING "")
set(CMAKE_CUDA_ARCHITECTURES "61;75" CACHE STRING "")
set(CUDNN_STATIC ON CACHE BOOL "")
