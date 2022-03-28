FROM docker.io/openjdk:16-alpine3.13

ADD JaVijf.java /

RUN javac JaVijf.java -d .

ENTRYPOINT ["java","-Xms100m", "-Xmx100m", "JaVijf"]
