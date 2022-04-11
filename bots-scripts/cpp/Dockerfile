FROM docker.io/alpine:3.15.3 as build
LABEL stage=build

RUN apk update
RUN apk --no-cache add g++

ARG filename

COPY $filename /

RUN g++ -x c++ -std=c++20 -O2 -o program $filename -static

FROM scratch
COPY --from=build program /

ENTRYPOINT ["/program"]
