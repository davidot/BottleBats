FROM docker.io/alpine:3.15.3 as build
LABEL stage=build

RUN apk update
RUN apk --no-cache add g++

ADD vijfbot.cpp /

RUN echo "START USER PART" && g++ -x c++ -std=c++20 -O2 -o program vijfbot.cpp -static && echo "DONE USER PART"

FROM scratch
COPY --from=build program /

ENTRYPOINT ["/program"]
