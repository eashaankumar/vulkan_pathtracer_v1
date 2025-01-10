CXX = g++
CXXFLAGS = -std=c++23 -Wall -pedantic-errors -g -Wno-unknown-pragmas 

APP = renderer

SRCS =  ${APP}/src/*.cpp

LIBSDIR = -LC:\VulkanSDK\1.3.290.0\Lib -LC:\Users\seana\dev\OpenGLLibs\Libs

######
# SDL2main must come before SDL2
######
VULKAN_SHADER_LIBS = -lshadercd -lshaderc_utild -lshaderc_combinedd
LIBS = -lvulkan-1 -lSDL2main -lSDL2 #${VULKAN_SHADER_LIBS} #-lglfw3
HEADERS = -IC:\VulkanSDK\1.3.290.0\Include -I${APP}/headers -IC:\Users\seana\dev\OpenGLLibs\Include


# OBJS = ${SRCS:.cpp=.o}

# MAIN = myprog

# all: ${MAIN}
# 	@echo   Simple compilter named myprog has been compiled

# ${MAIN}: ${OBJS}
# 	${CXX} ${CXXFLAGS} ${OBJS} -o ${MAIN}

# .cpp.o:
# 	${CXX} ${CXXFLAGS} -c $< -o $@

# clean:
# 	${RM} ${PROGS} ${OBJS} *.o *~.


####
# -l flags go after -L and at the END (https://stackoverflow.com/questions/30146283/undefined-reference-to-sdl-init)
####
app: ${APP}/src/*.cpp
	${CXX} ${CXXFLAGS} \
	${HEADERS} \
	${SRCS} \
	-L/usr/local/lib -Lstdlib ${LIBSDIR} ${LIBS} \
	-o ${APP}

# example make compile_shader_spv input=app1/src/shader.rgen output=compiled_shaders/shader.rgen.spv
compile_shader_spv: 
	echo Input: ${input}
	echo Output: ${output}
	C:\VulkanSDK\1.3.224.1\Bin\glslc.exe ${input} -o ${output} --target-spv=spv1.4

learning:
	echo argument is ${input}

help:
	echo app
	echo compile_shader_spv input=... output=...spv