FROM docker.io/openjdk:16-alpine3.13

ADD . /

RUN javac -classpath . JaVijf.java -d .

ENTRYPOINT ["java","-Xms128m", "-Xmx128m", "JaVijf"]
