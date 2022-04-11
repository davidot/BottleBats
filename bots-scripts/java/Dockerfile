FROM docker.io/openjdk:16-alpine3.13

ARG filename


COPY $filename /

RUN javac $filename -d .

ARG programname
ENV JAVA_PROGRAM_NAME=${programname}

ENTRYPOINT ["sh", "-c", "java -Xms128m -Xmx128m ${JAVA_PROGRAM_NAME}"]
