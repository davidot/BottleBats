FROM docker.io/python:alpine3.15

ARG filename
ADD . /
ENV PYTHON_FILE=${filename}

ENTRYPOINT ["sh", "-c", "python ${PYTHON_FILE}"]
