FROM docker.io/mono:6.12.0.122

ADD Program.cs /

RUN echo "START USER PART" && csc -optimize Program.cs -out:Program.exe && echo "DONE USER PART"

ENTRYPOINT ["mono", "/Program.exe"]
