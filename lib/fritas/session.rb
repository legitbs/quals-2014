module Fritas
  class Session
    def initialize(codec, uuid, logger)
      @codec = codec
      @uuid = uuid
      @logger = logger
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
      @codec.send :Hello, uuid: @uuid
      resp = @codec.expect :Hello
      if resp.uuid != @uuid
        @codec.error "Failed to say Hello."
      end
      @logger.info "Ready #{@uuid}"
      @codec.send :Ready
    end
  end
end