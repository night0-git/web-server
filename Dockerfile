FROM debian AS build
RUN apt-get update && apt-get install -y --no-install-recommends clang make
WORKDIR /app
COPY src/ src/
COPY Makefile Makefile
ENV CC=clang
RUN make clean
RUN make release

FROM debian
WORKDIR /usr/local/bin
COPY  files/ files/
COPY --from=build /app/bin/web-server web-server
EXPOSE 8080
CMD ["/usr/local/bin/web-server"]