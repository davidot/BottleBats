FROM docker.io/mono:6.12.0.122 as build
LABEL stage=build

ARG filename
COPY . /

RUN csc -optimize *.cs -out:Program.exe

RUN mkbundle -o program --static --simple Program.exe --deps --machine-config /etc/mono/4.5/machine.config --config /etc/mono/config

FROM docker.io/mono:6.12.0.122-slim
COPY --from=build program /

ENTRYPOINT ["/program"]
