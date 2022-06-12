FROM docker.io/rust:1.61.0-slim as build
LABEL stage=build

ADD . /

RUN rustc --crate-type bin -O -C target-feature=+crt-static -o program main.rs

FROM scratch
COPY --from=build /program /


ENTRYPOINT ["/program"]
