FROM docker.io/mono:6.12.0.122

ARG filename

COPY $filename /

RUN csc -optimize $filename -out:Program.exe

ENTRYPOINT ["mono", "/Program.exe"]
