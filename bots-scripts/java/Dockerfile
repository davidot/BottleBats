FROM docker.io/openjdk:16-alpine3.13

ARG filename


COPY . /

RUN javac $filename -d . -classpath .

ARG programname
ENV JAVA_PROGRAM_NAME=${programname}

ENTRYPOINT ["sh", "-c", "java -Xms128m -Xmx128m ${JAVA_PROGRAM_NAME}"]
