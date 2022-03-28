FROM docker.io/gcc:11.2

ADD vijfbot.cpp /

RUN echo "START USER PART" && g++ -x c++ -std=c++20 -O2 -o program vijfbot.cpp && echo "DONE USER PART"

ENTRYPOINT ["./program"]
