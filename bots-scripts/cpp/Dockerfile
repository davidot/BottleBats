FROM docker.io/alpine:3.15.3 as build
LABEL stage=build

RUN apk update
RUN apk --no-cache add g++

COPY . /

RUN g++ -x c++ -std=c++20 -O2 -o program *.cpp -static

FROM scratch
COPY --from=build program /

ENTRYPOINT ["/program"]
