FROM docker.io/alpine:3.15.3

RUN apk add llvm13-libs --no-cache --repository=http://nl.alpinelinux.org/alpine/edge/main

RUN apk add zig --no-cache --repository=http://nl.alpinelinux.org/alpine/edge/testing

ADD main.zig /

RUN zig build-exe main.zig -O ReleaseSafe --strip --name program

ENTRYPOINT ["./program"]
