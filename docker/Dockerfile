FROM gxchain/builder
MAINTAINER GXS

RUN git lfs clone https://github.com/gxchain/gxb-core.git \
    && cd gxb-core \
    && git submodule update --init --recursive \
    && mkdir build && cd build \
    && cmake -DOPENSSL_ROOT_DIR=/usr/bin -DOPENSSL_INCLUDE_DIR=/usr/include/openssl -DOPENSSL_LIBRARIES=/usr/lib/openssh -DCMAKE_BUILD_TYPE=Release .. \ 
    && make witness_node cli_wallet \
    && cp -rf programs /usr/local/

# Home directory $HOME
WORKDIR /

ENV GXB_DATA /gxb
RUN rm -rf "$GXB_DATA" \
    && mkdir "$GXB_DATA" \
    && ln -sfn /usr/local/programs "$GXB_DATA"

VOLUME /gxb

# Copy default configuration
ADD docker/start.sh /start.sh
ADD genesis.json /genesis.json
RUN chmod a+x /start.sh

# rpc service port（cli_wallet or witness_node）
EXPOSE 8091

# P2P node to connect to on startup
EXPOSE 8092

# Endpoint for P2P node to listen on
EXPOSE 8093

# Entry point
ENTRYPOINT [ "/start.sh" ]
