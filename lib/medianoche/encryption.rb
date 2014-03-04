require 'openssl'
require 'pbkdf2'

module Medianoche
  class Encryption
    def initialize(body, password)
      @body = body
      @password = password
    end

    def encrypt
      machine.key = derived_key
      machine.iv = "aaa goddamn bees"

      buf = machine.update @body
      buf << machine.final

      return buf
    end

    private
    def machine
      return @machine if defined? @machine
      @machine = OpenSSL::Cipher.new 'AES-256-OFB'
      @machine.encrypt
    end

    def derived_key
      pp @password
      @derived_key ||= PBKDF2.new do |p|
        p.password = @password

        # salt isn't significant, we're providing an encryption oracle
        p.salt = 'legitbs'
        p.iterations = 100
        p.hash_function = OpenSSL::Digest::SHA256
      end.bin_string
    end
  end
end