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
      password = random_tag
      related = random_tag

      @collection.posts.add Messages::Post.new(
                                               uuid: Celluloid::UUID.generate,
                                               title: 'key',
                                               body: "The flag is: #{$key}",
                                               tags: to_tags(["password=#{password}",
                                                      related,
                                                      random_tag
                                                     ])
                                               )
      
      @collection.posts.add Messages::Post.new(
                                               uuid: Celluloid::UUID.generate,
                                               title: 'hey kid',
                                               body: "i'm a computer",
                                               tags: to_tags([related, random_tag]))

      4.times do 
        @collection.posts.add Messages::Post.new(
                                                 uuid: Celluloid::UUID.generate,
                                                 title: random_tag,
                                                 body: random_tag,
                                                 tags: to_tags([random_tag, random_tag]))
      end
    end

    def consume_message
      @logger.info "Waiting to consume #{@uuid}"
      message = @codec.receive
      @logger.info "Got #{message.inspect} from #{@uuid}"
      message.process(self)
    end

    def random_tag
      SecureRandom.random_number(36**20).to_s(36)
    end

    def to_tags(arr)
      arr.map do |t|
        Messages::Tag.new tagname: t
      end
    end
  end
end
