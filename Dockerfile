FROM gcc AS build
WORKDIR /app
COPY . .
ENV CC=gcc
RUN make clean
RUN make release

FROM debian
COPY --from=build /app/bin/web-server /usr/local/bin/web-server
EXPOSE 8080
CMD ["/usr/local/bin/web-server"]