require 'ecdsa'
require 'securerandom'
require 'choripan/packer'
require 'choripan/messages'

module Choripan
  class Signer
    def initialize
      @group = ECDSA::Group::Secp256k1
      @private = rando
      @public = @group.generator.multiply_by_scalar @private

      # lol
      @temp = rando
    end

    def sign(uuid)
      digest = ECDSA.sign(@group, @private, uuid, @temp)

      p = Packer.new
      p.pack digest.r, digest.s
    end

    def verify(uuid, signature)
      p = Packer.new
      r, s = p.unpack signature

      sig = ECDSA::Signature.new r, s
      correct = ECDSA.valid_signature?(@public, uuid, sig)
      return correct
    end

    private
    def to_pubkey
      Messages::PubKey.new groupname: @group.name, x: @public.x.to_s, y: @public.y.to_s
    end

    def rando
      1 + SecureRandom.random_number(@group.order - 1)
    end
  end
end
