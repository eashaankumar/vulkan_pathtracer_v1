CXX = g++
CXXFLAGS = -std=c++20 -Wall -pedantic-errors -g

APP = app1

SRCS =  ${APP}/src/*.cpp

LIBSDIR = -LC:\VulkanSDK\1.3.290.0\Lib

######
# SDL2main must come before SDL2
######
LIBS = -lvulkan-1 -lSDL2main -lSDL2
HEADERS = -IC:\VulkanSDK\1.3.290.0\Include -I${APP}/headers


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
