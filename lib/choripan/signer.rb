require 'ecdsa'
require 'securerandom'
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

      s = Messages::Signature.new(r: digest.r.to_s, 
                                  s: digest.s.to_s, 
                                  k: to_pubkey)
    end

    def verify(uuid, signature)
      validate_signing_key signature.k
      sig = ECDSA::Signature.new signature.r.to_i, signature.s.to_i
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

    def validate_signing_key(k)
      return true if (k.groupname == @group.name) and
        (k.x == @public.x.to_s) and
        (k.y == @public.y.to_s)

      raise 'invalid signing key'
    end
  end
end
