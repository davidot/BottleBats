FROM docker.io/openjdk:16-alpine3.13

ADD JaVijf.java /

RUN javac JaVijf.java -d .

ENTRYPOINT ["java","-Xms128m", "-Xmx128m", "JaVijf"]
