
# ---------- 공통 설정 ----------
CC      := gcc
CFLAGS  := -Wall -O2               # 경고 + 최적화
CPPFLAGS:= -I.                     # 헤더 검색 경로(현재 디렉터리)

# OpenSSL 링크 라이브러리 추가 ★
LDLIBS  := -lssl -lcrypto

# ---------- 소스 목록 ----------
SERVER_SRCS  := main.c server.c \
                path_response.c static_file.c fixed_response.c \
                header_time.c form_input.c logger.c
CLIENT_SRCS  := client.c          # 추후 SSL 버전으로 교체 예정

SERVER_OBJS  := $(SERVER_SRCS:.c=.o)
CLIENT_OBJS  := $(CLIENT_SRCS:.c=.o)

# ---------- 기본 타깃 ----------
.PHONY: all
all: server client

# ---------- 서버 ----------
server: $(SERVER_OBJS)$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# ---------- 클라이언트 ----------
client: $(CLIENT_OBJS)$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# ---------- 청소 ----------
.PHONY: clean
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) server client

