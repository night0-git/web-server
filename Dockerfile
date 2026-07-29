FROM gcc AS build
WORKDIR /app
COPY src/ src/
COPY Makefile Makefile
ENV CC=gcc
RUN make clean
RUN make release

FROM debian
WORKDIR /usr/local/bin
COPY  files/ .
COPY --from=build /app/bin/web-server web-server
EXPOSE 8080
CMD ["/usr/local/bin/web-server"]