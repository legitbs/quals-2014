require 'fritas/codec'
require 'fritas/session'
require 'celluloid/io'
require 'logger'
require 'securerandom'

module Fritas
  class Server
    include Celluloid::IO
    finalizer :shutdown

    def initialize(host, port, options={})
      @host = host
      @port = port
      @logger = options[:logger] || ::Logger.new($stderr)
      start
    end

    def start
      @logger.info "fritas starting"
      @server = TCPServer.new @host, @port
      @logger.info "started"
      async.run
    end

    def shutdown
      @server.close if @server && !@server.closed?
    end

    def run
      loop do
        async.handle_connection @server.accept
      end
    end

    def handle_connection(socket)
      socket.setsockopt(Socket::IPPROTO_TCP, Socket::TCP_NODELAY, true)
      uuid = Celluloid::UUID.generate
      @logger.info "connected from #{socket.peeraddr(false).inspect} => #{uuid}" 
      codec = Codec.new socket
      session = Session.new codec, uuid, @logger
      session.run
    end
  end
end
