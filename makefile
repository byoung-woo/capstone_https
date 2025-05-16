# ---------- 공통 설정 ----------
CC       := gcc
CFLAGS   := -Wall -O2
CPPFLAGS := -I.

LDLIBS   := -lssl -lcrypto

# ---------- 서버 소스/오브젝트 ----------
SERVER_SRCS := server.c \
               ssl_init.c \
               path_response.c \
               static_file.c \
               fixed_response.c \
               header_time.c \
               form_input.c \
               logger.c

SERVER_OBJS := $(SERVER_SRCS:.c=.o)

# ---------- 클라이언트 소스/오브젝트 ----------
CLIENT_SRCS := client.c \
               ssl_init.c

CLIENT_OBJS := $(CLIENT_SRCS:.c=.o)

# ---------- 기본 타깃 ----------
.PHONY: all clean
all: server client

# ---------- 서버 빌드 ----------
server: $(SERVER_OBJS)
        $(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# ---------- 클라이언트 빌드 ----------
client: $(CLIENT_OBJS)
        $(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# ---------- 정리 ----------
clean:
        rm -f $(SERVER_OBJS) $(CLIENT_OBJS) server client
