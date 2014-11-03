require 'choripan/collection'
require 'choripan/signer'

module Choripan
  class Session
    attr_reader :codec, :uuid, :logger, :collection, :signer

    def initialize(codec, uuid, logger)
      @codec = codec
      @uuid = uuid
      @logger = logger
      @collection = Collection.new
      @signer = Signer.new
    end

    def run
      say_hello

      add_payload

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

    def add_payload
      @collection.posts.add Messages::Post.new(uuid: SecureRandom.uuid,
                                               title: 'key',
                                               body: "The flag is: #{$key}")

      examples = 2.times.map do |t|
        p = Messages::Post.new(uuid: SecureRandom.uuid,
                               title: "Example post #{t}",
                               body: "This is example post #{t}.")

        @collection.posts.add p

        p
      end
      examples.each do |e|
        s = @signer.sign e.uuid

        @collection.logs.add Messages::Log.new(
                                               uuid: e.uuid,
                                               timestamp: Time.now.to_i - rand(60),
                                               signature: s
                                               )
      end
    end

    def consume_message
      @logger.info "Waiting to consume #{@uuid}"
      message = @codec.receive
      @logger.info "Got #{message.inspect} from #{@uuid}"
      message.process(self)
    end
  end
end
