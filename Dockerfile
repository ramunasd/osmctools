FROM gcc

COPY . /src/osmctools/
WORKDIR /src/osmctools/

RUN autoreconf --install
RUN ./configure
RUN make
RUN make install
