FROM ubuntu:14.04

MAINTAINER GXB

RUN set -ex \
	&& apt-get update -y \
	&& apt-get install -y wget

# install gxb binaries
RUN set -ex \
	&& cd /tmp \
	&& wget -qO gxb.tar.gz "http://gxb-package.oss-cn-hangzhou.aliyuncs.com/gxb-core/gxb_1.0.171222.tar.gz" \
	&& tar -xzvf gxb.tar.gz -C /usr/local \
	&& rm -rf /tmp/*

# Home directory $HOME
WORKDIR /

ENV GXB_DATA /gxb
RUN rm -rf "$GXB_DATA" \
    && mkdir "$GXB_DATA" \
    && ln -sfn /usr/local/programs "$GXB_DATA"

VOLUME /gxb

# Copy default configuration
ADD start.sh /start.sh
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