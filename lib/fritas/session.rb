require 'fritas/collection'

module Fritas
  class Session
    attr_reader :codec, :uuid, :logger, :collection

    def initialize(codec, uuid, logger)
      @codec = codec
      @uuid = uuid
      @logger = logger
      @collection = Collection.new
    end

    def run
      say_hello
      loop do
        consume_message
      end
      @codec.close
    rescue => e
      @logger.error e
      @codec.close
    end

    private
    def say_hello
      @logger.info "Hello #{@uuid}"
      @codec.write :Hello, uuid: @uuid
      resp = @codec.expect :Hello
      if resp.uuid != @uuid
        @codec.error "Failed to say Hello."
      end
      @logger.info "Ready #{@uuid}"
      @codec.write :Ready
    end

    def consume_message
      @logger.info "Waiting to consume #{@uuid}"
      message = @codec.receive
      @logger.info "Got #{message.inspect} from #{@uuid}"
      message.process(self)
    end
  end
end