require 'csv'
require 'fritas/messages'
require 'fritas/messages_overlay'

module Fritas
  class Codec
    CODE_TO_MESSAGE = Hash.new
    MESSAGE_TO_CODE = Hash.new

    def initialize(sock)
      @sock = sock
      initialize_mapping_if_necessary
    end

    def close
      @sock.close unless @sock.nil? || @sock.closed?
    end

    def error(body)
      write :ErrorResp, reason: body
      close
      raise body
    end

    def write(message_name, options={})
      message_instance = Messages.const_get(message_name).new options
      message_encoded = message_instance.encode.to_s
      header = [MESSAGE_TO_CODE[message_name], message_encoded.length].pack 'CN'

      @sock.write header
      @sock.write message_encoded
      @sock.flush
    end

    def expect(message_name)
      message = receive
      unless message.is_a? materialize(message_name)
        err = "Expected #{message_name.to_s}, got #{message.class.name}"
        error err
      end

      return message
    end

    def receive
      header = @sock.read(5)
      raise "Failed to read header" if header.nil?
      code, length = header.unpack 'CN'
      body = @sock.read length if length > 0

      Messages.const_get(CODE_TO_MESSAGE[code]).decode(body || '')
    end

    private
    def materialize(symbol)
      Messages.const_get(symbol)
    end

    def initialize_mapping_if_necessary
      return unless CODE_TO_MESSAGE.empty?

      mapping_data = CSV.read(
        File.join(
          File.dirname(__FILE__),
          '..', '..', 'include', 'fritas.csv'))

      mapping_data.each do |md|
        next unless md[0] =~ /^\d+$/
        CODE_TO_MESSAGE[md[0].to_i] = md[1].to_sym
      end

      MESSAGE_TO_CODE.merge! CODE_TO_MESSAGE.invert
    end 
  end
end